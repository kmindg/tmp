#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_enclosure.h"
#include "fbe_scheduler.h"

#include "base_discovering_private.h"
#include "base_enclosure_private.h"

fbe_status_t 
fbe_base_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = NULL;

    base_enclosure = (fbe_base_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_base_enclosure_lifecycle_const, (fbe_base_object_t*)base_enclosure, packet);

    return status;  
}

fbe_status_t fbe_base_enclosure_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(base_enclosure));
}

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t base_enclosure_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t base_enclosure_monitor_online(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t base_enclosure_invalid_type_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_enclosure_get_port_object_id_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_enclosure_server_info_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_enclosure_ride_through_tick_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_enclosure_firmware_download_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_status_t base_enclosure_common_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t 
base_enclosure_drive_spinup_ctrl_needed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

/*--- lifecycle callback functions -----------------------------------------------------*/


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(base_enclosure);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(base_enclosure);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(base_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        base_enclosure,
        base_enclosure_monitor_online,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t base_enclosure_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        base_enclosure_self_init_cond_function)
};

static fbe_lifecycle_const_cond_t base_enclosure_invalid_type_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_INVALID_TYPE,
        base_enclosure_invalid_type_cond_function)
};

static fbe_lifecycle_const_cond_t base_enclosure_get_port_object_id_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_GET_PORT_OBJECT_ID,
        base_enclosure_get_port_object_id_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/
/* condition to extract server information, which includes address and port number
 * we could include SP id here in the future.
 */
static fbe_lifecycle_const_base_cond_t base_enclosure_server_info_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_enclosure_server_info_unknown_cond",
        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_SERVER_INFO_UNKNOWN,
        base_enclosure_server_info_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,       /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_enclosure_init_ride_through_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_enclosure_init_glitch_ride_through_cond",
        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_INIT_RIDE_THROUGH,
        base_enclosure_init_ride_through_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};

static fbe_lifecycle_const_base_timer_cond_t base_enclosure_ride_through_tick_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_enclosure_glitch_ride_through_tick_cond",
            FBE_BASE_ENCLOSURE_LIFECYCLE_COND_RIDE_THROUGH_TICK,
            base_enclosure_ride_through_tick_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY),         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    100 /* fires every second */
};

static fbe_lifecycle_const_base_cond_t base_enclosure_firmware_download_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_enclosure_firmware_download_cond",
        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_FIRMWARE_DOWNLOAD,
        base_enclosure_firmware_download_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,        /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,          /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_enclosure_drive_spinup_ctrl_needed_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_enclosure_drive_spinup_ctrl_needed_cond",
        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_DRIVE_SPINUP_CTRL_NEEDED,
        base_enclosure_drive_spinup_ctrl_needed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,          /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};

/* This has to be the same order as in fbe_base_enclosure_lifecycle_cond_id_t */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(base_enclosure)[] = {
    &base_enclosure_server_info_unknown_cond,
    &base_enclosure_init_ride_through_cond,
    (fbe_lifecycle_const_base_cond_t*)&base_enclosure_ride_through_tick_cond,
    &base_enclosure_firmware_download_cond,
    &base_enclosure_drive_spinup_ctrl_needed_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(base_enclosure);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t base_enclosure_specialize_rotary[] = {
    /* derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_enclosure_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_enclosure_get_port_object_id_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_enclosure_invalid_type_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_enclosure_server_info_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t base_enclosure_activate_rotary[] = {   
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_enclosure_init_ride_through_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_enclosure_ride_through_tick_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_enclosure_drive_spinup_ctrl_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t base_enclosure_ready_rotary[] = {  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_enclosure_firmware_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_enclosure_drive_spinup_ctrl_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(base_enclosure)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, base_enclosure_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, base_enclosure_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, base_enclosure_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(base_enclosure);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        base_enclosure,
        FBE_CLASS_ID_BASE_ENCLOSURE,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_discovering))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/
/****************************************************************
 * base_enclosure_self_init_cond_function()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function calls fbe_base_enclosure_init() to
 * initialize base enclosure related data structure.  
 *
 *  If base_enclosure or its child class object is created directly,
 * the constructor is called by the framework, which in turn calls
 * fbe_base_enclosure_init().  But if an object morph into
 * base_enclosure, by setting class id to base_enclosure, this 
 * condition will be set by the framework.  This will force
 * the framework to call fbe_base_enclosure_init().
 *
 * PARAMETERS:
 *  p_object - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_LIFECYCLE_STATUS_DONE - no error.
 *
 * HISTORY:
 *  08/04/08 - Created. NCHIU
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
base_enclosure_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t*)p_object;


    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                         "%s entry\n", __FUNCTION__);
     
    fbe_base_enclosure_init(base_enclosure);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                             "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
       
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t
base_enclosure_monitor_online(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry\n", __FUNCTION__);

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_enclosure_invalid_type_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t*)base_object;
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_enclosure);
    if (status != FBE_STATUS_OK) {
    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s can't clear current condition, status: 0x%X",
                        __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_enclosure_get_port_object_id_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_lifecycle_func_cond_t pp_cond_func;
    fbe_lifecycle_state_t lifecycle_state;

    /* We should have the ability to call super class condition function.
        Right now we can not do that easily, so we decided to do it hard way
    */
    status = fbe_lifecycle_get_state(&fbe_base_enclosure_lifecycle_const,
                                    (fbe_base_object_t *) base_object,
                                    &lifecycle_state);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace(base_object,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)base_object),
                            "%s fbe_lifecycle_get_state failed, status: 0x%X",
                            __FUNCTION__, status);
        

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_lifecycle_get_super_cond_func(&fbe_base_enclosure_lifecycle_const,
                                                (fbe_base_object_t *) base_object,
                                                lifecycle_state,
                                                FBE_BASE_DISCOVERED_LIFECYCLE_COND_GET_PORT_OBJECT_ID,
                                                &pp_cond_func);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace(base_object,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)base_object),
                            "%s fbe_lifecycle_get_super_cond_func failed, status: 0x%X",
                            __FUNCTION__, status);
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    return pp_cond_func(base_object, packet);

}

/****************************************************************
 * base_enclosure_init_ride_through_cond_function()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function initialize the glitch ride through logic.
 *
 * PARAMETERS:
 *  p_object - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/31/08 - Created. NCHIU
 *
 ****************************************************************/
fbe_lifecycle_status_t 
base_enclosure_init_ride_through_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t*)p_object;


    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry \n ", __FUNCTION__ );

    fbe_base_enclosure_increment_ride_through_count(base_enclosure);

    /* record the time in seconds we start the ride through */
    base_enclosure->time_ride_through_start = fbe_get_time();  

    /* take the default value if not set.  It could be changed upon other condition */
    if (base_enclosure->expected_ride_through_period == 0)
    {
        base_enclosure->expected_ride_through_period = base_enclosure->default_ride_through_period;
    }
    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "glitch ride through started at %lld second, expected %d second\n", 
                        base_enclosure->time_ride_through_start/FBE_TICK_PER_SECOND, 
                        base_enclosure->expected_ride_through_period);

    /* clear condition to start the timer */
    status = fbe_lifecycle_force_clear_cond(&fbe_base_enclosure_lifecycle_const, 
                                            (fbe_base_object_t*)base_enclosure, 
                                            FBE_BASE_ENCLOSURE_LIFECYCLE_COND_RIDE_THROUGH_TICK);

    if (status != FBE_STATUS_OK) {
    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s can't clear timer condition, status: 0x%X\n",
                        __FUNCTION__, status);
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);

    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/****************************************************************
 * base_enclosure_ride_through_tick_cond_function()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function checks how long the glitch ride through logic
 *  has been started. 
 *  If discovery edge is still not_present and timeout value expeires,
 *  the enclosure will be destroyed.
 *
 * PARAMETERS:
 *  p_object - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_LIFECYCLE_STATUS_DONE 
 *
 * HISTORY:
 *  7/31/08 - Created. NCHIU
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
base_enclosure_ride_through_tick_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t*)p_object;
    fbe_u32_t now_interval;
    fbe_path_attr_t path_attr;

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry\n", __FUNCTION__);

    /* There are cases where the glitch timeout was triggered before the timer was actually seeded 
     * with the start value. It is safe to verify the start time is not 0 before checking if the
     * timer has expired.
     */
    if ( base_enclosure->time_ride_through_start != 0 )
    {
        /* check the time interval in secs since we start the ride through.
         */
        now_interval = fbe_get_elapsed_seconds(base_enclosure->time_ride_through_start); 

        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "ride through, new interval %d seconds, expected %d seconds.\n", 
                        now_interval, base_enclosure->expected_ride_through_period);

        /* If ride through is properly set up, and timeout period expired*/
        if ((base_enclosure->expected_ride_through_period > 0) &&
            (now_interval > base_enclosure->expected_ride_through_period) )
        {
            /* We are in ACTIVATE state right now, we won't get here if path is not ENABLED,
             * this is guaranteed by FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_NOT_READY
             */
            /* check discovery edge attribute */
            status = fbe_base_discovered_get_path_attributes(
                                    (fbe_base_discovered_t*)base_enclosure, 
                                    &path_attr);

            fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "ride through, status %d, path_attr 0x%x.\n", 
                        status, path_attr);

            /* We need to be destroyed */
            if ((status == FBE_STATUS_OK) &&
                (path_attr & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader(base_enclosure),
                                    "Glitch ride through timed out after %d second, expected %d second\n", 
                                    now_interval, base_enclosure->expected_ride_through_period);

                status = fbe_lifecycle_set_cond(&fbe_base_enclosure_lifecycle_const, 
                                                (fbe_base_object_t*)base_enclosure, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);

                if (status != FBE_STATUS_OK) {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader(base_enclosure),
                                        "%s can't set BO:DESTROY condition, status: 0x%X\n",
                                        __FUNCTION__, status);
                }
            }
        }
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/****************************************************************
 * base_enclosure_firmware_download_cond_function()
 ****************************************************************
 * DESCRIPTION:
 *  This condition function handle firmware download.
 *  Place holder for now.
 *
 * PARAMETERS:
 *  p_object - our object context
 *  packet - request packet
 *
 * RETURNS:
 *  fbe_status_t - FBE_LIFECYCLE_STATUS_DONE.
 *
 * HISTORY:
 *  7/31/08 - Created a place holder. NCHIU
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
base_enclosure_firmware_download_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry\n", __FUNCTION__);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/**************************************************************************
* base_enclosure_server_info_unknown_cond_function()                  
***************************************************************************
*
* DESCRIPTION
*       This function runs when it comes to the EMC specific status unknown condition.
*       It sets the completion routine and sends down the get EMC specific status command.
*
* PARAMETERS
*       p_object - The pointer to the fbe_base_object_t.
*       packet - The pointer to the fbe_packet_t.
*
* RETURN VALUES
*       fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   15-Aug-2008 PHE - Created.
*   15-Sept-2008 nchiu - moved from viper enclosure to base enclosure.
***************************************************************************/
static fbe_lifecycle_status_t 
base_enclosure_server_info_unknown_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry \n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, base_enclosure_common_cond_completion, base_enclosure);

    /* Call the executer function to send the command. */
    status = fbe_base_enclosure_send_get_server_info_command(base_enclosure, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/**************************************************************************
* base_enclosure_common_cond_completion()                  
***************************************************************************
*
* DESCRIPTION
*       This routine can be used as the completion funciton for a condition
*       if it only needs to clear the current condition if it completes successfully.
*
* PARAMETERS
*       packet - The pointer to the fbe_packet_t.
*       context - The completion context.
*
* RETURN VALUES
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   15-Sept-2008 NCHIU - Created.
***************************************************************************/
static fbe_status_t 
base_enclosure_common_cond_completion(fbe_packet_t * packet, 
                                      fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t*)context;

    status = fbe_transport_get_status_code(packet);

    /* Check the packet status */
    if(status == FBE_STATUS_OK)
    {
        /* 
         * only clear the current condition when the status is OK. 
         * Don't clear the current condition here
         * so that the condition will be executed again.
         */      
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_enclosure);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
            
        }
         
    }   

    return FBE_STATUS_OK;
}

/*!***************************************************************
 *  @fn base_enclosure_drive_spinup_ctrl_needed_cond_function()
 ****************************************************************
 * @brief:
 *      This function runs when it comes to the drive spinup control needed condition.
 *      It gives the drives the permission to spin up if they are allowed.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return fbe_status_t 
 *   FBE_STATUS_OK - no error.
 *   Other values imply there was an error.
 *
 * HISTORY:
 *  03-Dec-2009 PHE - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
base_enclosure_drive_spinup_ctrl_needed_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_base_enclosure_t * base_enclosure = (fbe_base_enclosure_t*)p_object;
    fbe_u32_t slot_num = 0;
    fbe_discovery_edge_t  *     discovery_edge = NULL;
    fbe_u32_t curr_discovery_path_attrs = 0;


    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry \n ", __FUNCTION__ );

    // Loop through all the slots.
    for(slot_num = 0 ; slot_num < fbe_base_enclosure_get_number_of_slots(base_enclosure); slot_num++)
    { 
        discovery_edge = fbe_base_discovering_get_client_edge_by_server_index((fbe_base_discovering_t *) base_enclosure, 
                                                                              slot_num);

        if(discovery_edge == NULL)
        {
            // The drive is not present.
            continue;
        }

        status = fbe_base_discovering_get_path_attr((fbe_base_discovering_t *) base_enclosure, 
                                                slot_num, 
                                                &curr_discovery_path_attrs);

        if(status != FBE_STATUS_OK)
        {
            // It should not happen.
            fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "drive_spinup_ctrl_needed_cond, get_path_attr failed, slot: %d, status: 0x%x.\n",
                            slot_num, status); 
            
        }
        else
        {
            if((curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING) &&
                (base_enclosure->number_of_drives_spinningup < base_enclosure->max_number_of_drives_spinningup))
            {
                fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) base_enclosure, 
                                                slot_num, 
                                                FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED);

                fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) base_enclosure, 
                                                slot_num, 
                                                FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING);

                base_enclosure->number_of_drives_spinningup ++;
            }
        }
    }// End of - for(slot_num = 0 ; slot_num < fbe_base_enclosure_get_number_of_slots(base_enclosure); slot_num++)

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "drive_spinup_ctrl_needed_cond, can't clear current condition, status: 0x%x.\n",
                            status);

    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


/* End of file fbe_base_enclosure_monitor.c */
