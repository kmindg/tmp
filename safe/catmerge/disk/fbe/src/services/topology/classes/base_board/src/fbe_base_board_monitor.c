#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_board.h"
#include "fbe_scheduler.h"

#include "base_discovering_private.h"
#include "base_board_private.h"
#include "base_board_pe_private.h"
#include "fbe_private_space_layout.h"

extern fbe_base_board_pe_init_data_t   pe_init_data[FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES];

fbe_status_t 
fbe_base_board_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_base_board_t * base_board = NULL;

    base_board = (fbe_base_board_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)base_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(base_board),
                                      (fbe_base_object_t*)base_board, packet);
}

fbe_status_t fbe_base_board_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(base_board));
}

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t base_board_get_port_object_id_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_board_invalid_type_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_board_discovery_poll_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_board_pe_status_unknown_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_board_discovery_update_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_board_init_private_space_layout_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

/*--- lifecycle callback functions -----------------------------------------------------*/


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(base_board);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(base_board);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(base_board) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        base_board,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t base_board_invalid_type_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_INVALID_TYPE,
        base_board_invalid_type_cond_function)
};

static fbe_lifecycle_const_cond_t base_board_get_port_object_id_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_GET_PORT_OBJECT_ID,
        base_board_get_port_object_id_cond_function)
};

static fbe_lifecycle_const_cond_t base_board_discovery_poll_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
        base_board_discovery_poll_cond_function)
};

static fbe_lifecycle_const_cond_t base_board_discovery_update_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE,
        base_board_discovery_update_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

/* FBE_BASE_BOARD_LIFECYCLE_COND_INIT_BOARD condition */
static fbe_lifecycle_const_base_cond_t base_board_init_board_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_board_init_board_cond",
        FBE_BASE_BOARD_LIFECYCLE_COND_INIT_BOARD,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

/* FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION condition */
static fbe_lifecycle_const_base_cond_t base_board_update_hardware_configuration_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_board_update_hardware_configuration_cond",
        FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

/* FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION_POLL condition */
static fbe_lifecycle_const_base_timer_cond_t base_board_update_hardware_configuration_poll_cond = {
    {
    FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
        "base_board_update_hardware_configuration_poll_cond",
        FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION_POLL,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    300 /* fires every 3 seconds */
};


/* FBE_BASE_BOARD_LIFECYCLE_COND_PE_STATUS_UNKNOWN condition */
static fbe_lifecycle_const_base_cond_t base_board_pe_status_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_board_pe_status_unknown_cond",
        FBE_BASE_BOARD_LIFECYCLE_COND_PE_STATUS_UNKNOWN,
        base_board_pe_status_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,        /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,          /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,           /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,              /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)           /* DESTROY */
};

/* FBE_BASE_BOARD_LIFECYCLE_COND_INITIALIZE_PRIVATE_SPACE_LAYOUT condition */
static fbe_lifecycle_const_base_cond_t base_board_init_private_space_layout_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_board_init_private_space_layout_cond",
        FBE_BASE_BOARD_LIFECYCLE_COND_INITIALIZE_PRIVATE_SPACE_LAYOUT,
        base_board_init_private_space_layout_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,        /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,          /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,           /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,              /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)           /* DESTROY */
};



static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(base_board)[] = {
    &base_board_init_board_cond,
    &base_board_update_hardware_configuration_cond,
    (fbe_lifecycle_const_base_cond_t*)&base_board_update_hardware_configuration_poll_cond,
    &base_board_pe_status_unknown_cond, 
    &base_board_init_private_space_layout_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(base_board);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t base_board_specialize_rotary[] = { 
    /* Base discovered derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_get_port_object_id_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_invalid_type_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_init_private_space_layout_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_init_board_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t base_board_activate_rotary[] = {   
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_pe_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t base_board_ready_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_update_hardware_configuration_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_update_hardware_configuration_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_board_pe_status_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(base_board)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, base_board_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, base_board_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, base_board_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(base_board);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_board) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        base_board,
        FBE_CLASS_ID_BASE_BOARD,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_discovering))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/

static fbe_lifecycle_status_t
base_board_init_private_space_layout_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_board_t * base_board = (fbe_base_board_t*)base_object;
    fbe_status_t status;

    status = fbe_private_space_layout_initialize_library(base_board->platformInfo.platformType);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't init private space layout library: 0x%X",
                                __FUNCTION__, status);
    }
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t
base_board_get_port_object_id_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;

    /* The board object does not have a downstream port, so just clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_board_invalid_type_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_board_t * base_board = (fbe_base_board_t*)base_object;
    fbe_board_type_t board_type;
    fbe_class_id_t class_id;
    fbe_status_t status;

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);    

    status = fbe_base_board_get_platform_info_and_board_type(base_board, &board_type);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_board,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't get board type, status: 0x%x.\n",
                                __FUNCTION__, status);
        
        fbe_lifecycle_set_cond(&fbe_base_board_lifecycle_const,
                               (fbe_base_object_t*)base_board,
                               FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_base_board_init(base_board);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_base_board_init failed, status: 0x%x.\n",
                                __FUNCTION__, status);
        
        fbe_lifecycle_set_cond(&fbe_base_board_lifecycle_const,
                               (fbe_base_object_t*)base_board,
                               FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    class_id = FBE_CLASS_ID_INVALID;    
    switch(board_type) {
        case FBE_BOARD_TYPE_HAMMERHEAD:
            class_id = FBE_CLASS_ID_HAMMERHEAD_BOARD;
            break;
        case FBE_BOARD_TYPE_SLEDGEHAMMER:
            class_id = FBE_CLASS_ID_SLEDGEHAMMER_BOARD;
            break;
        case FBE_BOARD_TYPE_JACKHAMMER:
            class_id = FBE_CLASS_ID_JACKHAMMER_BOARD;
            break;
        case FBE_BOARD_TYPE_BOOMSLANG:
            class_id = FBE_CLASS_ID_BOOMSLANG_BOARD;
            break;
        case FBE_BOARD_TYPE_DELL:
            class_id = FBE_CLASS_ID_DELL_BOARD;
            break;
        case FBE_BOARD_TYPE_FLEET:
            class_id = FBE_CLASS_ID_FLEET_BOARD;
            break;
        case FBE_BOARD_TYPE_MAGNUM:
            class_id = FBE_CLASS_ID_MAGNUM_BOARD;
            break;
        case FBE_BOARD_TYPE_ARMADA:
            class_id = FBE_CLASS_ID_ARMADA_BOARD;
            break;
        default:
            break;
    }

    if (class_id != FBE_CLASS_ID_INVALID) {
        fbe_base_object_lock((fbe_base_object_t*)base_board);
        fbe_base_object_change_class_id((fbe_base_object_t*)base_board, class_id);  
        fbe_base_object_unlock((fbe_base_object_t*)base_board);
        fbe_base_object_trace((fbe_base_object_t *)base_board, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Set board class-id, board-type: 0x%X , class-id: 0x%X\n", board_type, class_id);
        /* Tell the leaf class to initialize itself. */
        status = fbe_lifecycle_set_cond(&fbe_base_board_lifecycle_const,
                                        (fbe_base_object_t*)base_board,
                                        FBE_BASE_BOARD_LIFECYCLE_COND_INIT_BOARD);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't set initialize condition, status: 0x%X, cond id: 0x%X",
                                    __FUNCTION__, status, FBE_BASE_BOARD_LIFECYCLE_COND_INIT_BOARD);
        }
    }
    else {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_BOARD_TYPE_UKNOWN,
                              "Unknown board type: 0x%X", board_type);

        /* We do not know this board type, go to the DESTROY state. */
        /* Setting this condition causes a state change to DESTROY. */
        status = fbe_lifecycle_set_cond(&fbe_base_board_lifecycle_const,
                                        (fbe_base_object_t*)base_board,
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);

    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_board);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!*************************************************************************
* @fn base_board_discovery_poll_cond_function(                  
*                    fbe_base_object_t * base_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition is scheduled to run every 3 second.  It triggers
*       other conditions:
*           -query SPS serial port for SPS status
*           -refresh the processor enclosure hardware status
*
*
* @param      base_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   05-Jan-2010     Joe Perry - Created.
*
***************************************************************************/
static fbe_lifecycle_status_t 
base_board_discovery_poll_cond_function(fbe_base_object_t * base_object, fbe_packet_t * p_packet)
{
    fbe_status_t        status;

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);
    
    /* We need to get the latest status from time to time.
     * We should have set FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE here
     * to allow recreate child objects. But since completion routine for get_status
     * sets it, we skip it here.
     */
    status = fbe_lifecycle_set_cond(&fbe_base_board_lifecycle_const, 
                base_object, 
                FBE_BASE_BOARD_LIFECYCLE_COND_PE_STATUS_UNKNOWN);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, can't set PE_STATUS_UNKNOWN condition, status: 0x%x.\n",
                                __FUNCTION__, status);

        /* Do not clear the condition so we will run this condition function 
         * to try to set the PE_STATUS_UNKNOWN condition again.
         */
        return FBE_LIFECYCLE_STATUS_DONE;

    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s can't clear DISCOVERY_POLL condition, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}   // end of base_board_discovery_poll_cond_function


/*!*************************************************************************
* @fn base_board_pe_status_unknown_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function runs when it comes to the PE_STATUS_UNKNOWN condition.
*       It reads the config from the lower level driver and then clears the condition.
*
* @param      base_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   19-Feb-2010 PHE - Created.
***************************************************************************/
static fbe_lifecycle_status_t 
base_board_pe_status_unknown_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_board_t * base_board = (fbe_base_board_t *)base_object;

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n", __FUNCTION__);
    
    /* Read Misc and LED status to populate EDAL Misc info.
     * It has to be done before other components for spid. 
     */
    status = fbe_base_board_read_misc_and_led_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_misc_and_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }
    
    /*Read IO status to populate EDAL IO module, IO port and IO annex info. */
    status = fbe_base_board_read_io_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_io_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    /* Read Mezzanine status to populate EDAL mezzanine and IO port info. */
    status = fbe_base_board_read_mezzanine_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_mezzanine_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    /* Read Temperature status to populate EDAL temperature. */
    status = fbe_base_board_read_temperature_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_temperature_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

#if 0
    /* Read Multiplexer status to populate EDAL IO port LED info. */
    status = fbe_base_board_read_mplxr_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_mplxr_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }
#endif

    /* Read Power Supply status to populate EDAL power supply info. */
    status = fbe_base_board_read_power_supply_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_power_supply_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    /* Read SPS status to populate EDAL SPS info. */
    status = fbe_base_board_read_sps_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_sps_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    /* Read Battery status to populate EDAL Battery info. */
    status = fbe_base_board_read_battery_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_battery_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    /* Read Cooling status to populate EDAL cooling info. */
    status = fbe_base_board_read_cooling_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_cooling_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }    

    /* Read Management module status to populate EDAL management module info. */
    status = fbe_base_board_read_mgmt_module_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_mgmt_module_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    /*Read Fault Register status to populate EDAL fault register info. */
    status = fbe_base_board_read_flt_reg_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_flt_reg_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    /*Read Slave Port status to populate EDAL slave port info. */
    status = fbe_base_board_read_slave_port_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_slave_port_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }    

    /* Read Suitcase status to populate EDAL suitcase info. */
    status = fbe_base_board_read_suitcase_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_suitcase_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    /* Read BMC status to populate EDAL BMC info. */
    status = fbe_base_board_read_bmc_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_bmc_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);
    }

    /* Read Cache Card status to populate EDAL Cache Card info. */
    status = fbe_base_board_read_cache_card_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_cache_card_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);
    }

    /* Read DIMM status to populate EDAL dimm info. */
    status = fbe_base_board_read_dimm_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_dimm_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);
    }

    /* Read Resume Prom status to populate EDAL resume prom info. */
    status = fbe_base_board_read_resume_prom_status(base_board);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s read_resume_prom_status failed, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }

    /* The SSD status doesn't need to be checked as often as SPECL data, check this every 5 minutes */
    if((base_board->ssdPollCounter == 0) ||
        (base_board->logSsdTemperature != FBE_BASE_BOARD_SSD_TEMP_LOG_NONE))
    {
        //query the ssd drive(s) status(es)
        status = fbe_base_board_read_ssd_status(base_board);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s read_ssd_status failed, status: 0x%x.\n",
                            __FUNCTION__, status);
        }
        else
        {
            base_board->ssdPollCounter = 100; // wait another 5 minutes before checking again
        }


    }
    else
    {
        fbe_base_object_trace(base_object,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s not time yet to read SSD status count:%d.\n",
                            __FUNCTION__, base_board->ssdPollCounter);
        base_board->ssdPollCounter--;
    }

    /* set DISCOVERY_UPDATE condition so that it will be check whether there are data change. */
    status = fbe_lifecycle_set_cond(&fbe_base_board_lifecycle_const, 
            base_object, 
            FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, can't set DISCOVERY_UPDATE condition, status: 0x%x.\n",
                                __FUNCTION__, status);

        /* This is coding error. 
         * Do not clear the condition so that it will be stuck at this this condition.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s can't clear PE_STATUS_UNKNOWN condition, status: 0x%x.\n",
                __FUNCTION__, status);       
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}// End of function base_board_pe_status_unknown_cond_function

/*!*************************************************************************
* @fn base_board_discovery_update_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition sends the state change notification.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   04-May-2010 PHE - Created.
*
***************************************************************************/
static fbe_lifecycle_status_t 
base_board_discovery_update_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_board_t * base_board = (fbe_base_board_t *)base_object;
    fbe_edal_block_handle_t edal_block_handle = NULL;


    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n", __FUNCTION__);
   
    /*
     * Check to see if there were any changes in Enclosure Data
     */
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);

    status = base_board_process_state_change(base_board, &pe_init_data[0]);      

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s base_board_process_state_change failed.\n", __FUNCTION__);      

        /* This is coding errro. 
         * Do not clear the condition so that it will be stuck at this this condition.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*
     * Check to see if there were any changes in firmware upgrade info.
     */
    status = fbe_base_board_process_fup_status_change(base_board);
    if(status != FBE_STATUS_OK)
    {
         fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, process_fup_status_change failed, status: 0x%x.\n",  
                         __FUNCTION__, status);

    }

    /* Check the outstanding command queue to verify that the writes have completed successfully */
    fbe_base_board_check_command_queue(base_board);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s can't clear DISCOVERY_UPDATE condition, status: 0x%x.\n",
                        __FUNCTION__, status);        
    }


    return FBE_LIFECYCLE_STATUS_DONE;
}


