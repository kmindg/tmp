#include "fbe/fbe_board.h"
#include "armada_board_private.h"

fbe_status_t 
fbe_armada_board_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_armada_board_t * armada_board = NULL;

    armada_board = (fbe_armada_board_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)armada_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(armada_board),
                                      (fbe_base_object_t*)armada_board, packet);
}

fbe_status_t fbe_armada_board_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(armada_board));
}

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t armada_board_init_board_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_lifecycle_status_t armada_board_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_lifecycle_status_t armada_board_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_status_t armada_board_discovery_poll_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t armada_board_update_hardware_configuration_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t armada_board_update_hardware_configuration_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t armada_board_update_hardware_configuration_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(armada_board);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_NULL_COND(armada_board);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(armada_board) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        armada_board,
        FBE_LIFECYCLE_NULL_FUNC,       /* no online function */
        FBE_LIFECYCLE_NULL_FUNC)       /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t armada_board_init_board_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_BOARD_LIFECYCLE_COND_INIT_BOARD,
        armada_board_init_board_cond_function)
};

static fbe_lifecycle_const_cond_t armada_board_update_hardware_configuration_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION,
        armada_board_update_hardware_configuration_cond_function)
};

static fbe_lifecycle_const_cond_t armada_board_update_hardware_configuration_poll_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_TIMER_COND(
        FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION_POLL,
        armada_board_update_hardware_configuration_poll_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(armada_board);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t armada_board_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(armada_board_init_board_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t armada_board_ready_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(armada_board_update_hardware_configuration_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(armada_board_update_hardware_configuration_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(armada_board)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, armada_board_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, armada_board_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(armada_board);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(armada_board) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        armada_board,
        FBE_CLASS_ID_ARMADA_BOARD,                /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_board))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/

static fbe_lifecycle_status_t 
armada_board_init_board_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_armada_board_t * armada_board = (fbe_armada_board_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)armada_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Initialize the board. */
    status = fbe_armada_board_init(armada_board);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)armada_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't initialize the armada board, status: 0x%X",
                              __FUNCTION__, status);

        /* We can't initialize the board, go to the DESTROY state. */
        (void)fbe_lifecycle_set_cond(&fbe_armada_board_lifecycle_const,
                                     (fbe_base_object_t*)armada_board,
                                     FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)armada_board);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)armada_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
armada_board_update_hardware_configuration_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_armada_board_t * armada_board = (fbe_armada_board_t*)p_object;    
    fbe_u32_t ii;
    fbe_bool_t rescan_required = FBE_TRUE;    

    fbe_base_object_trace((fbe_base_object_t*)armada_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    
    if(armada_board->rescan_countdown == 0)
    {
        /* Populate the board's table of ports. */
        status = fbe_armada_board_update_hardware_ports(armada_board,&rescan_required);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)armada_board, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't get hardware port info, status: 0x%X",
                                  __FUNCTION__, status);
    
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    
        /* Are there any ports to be created? */
        for (ii = 0; ii < FBE_ARMADA_BOARD_NUMBER_OF_IO_PORTS; ii++) {
            if (armada_board->io_port_array[ii].io_port_number == FBE_BASE_BOARD_IO_PORT_INVALID) {
                /* There are no more ports. */
                break;
            }
            else {
                if (!(armada_board->io_port_array[ii].client_instantiated)){
                    fbe_base_object_trace_at_startup((fbe_base_object_t*)armada_board, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "Armada board found backend port, BE: 0x%X, io_port: 0x%X, io_portal: 0x%X connect_class:%d\n",                                    
                                            armada_board->io_port_array[ii].be_port_number,
                                            armada_board->io_port_array[ii].io_port_number,
                                            armada_board->io_port_array[ii].io_portal_number,
                                            armada_board->io_port_array[ii].connect_class);

                    status = fbe_base_discovering_instantiate_client((fbe_base_discovering_t*)armada_board, FBE_CLASS_ID_BASE_PORT, ii);             
                    armada_board->io_port_array[ii].client_instantiated = FBE_TRUE;
                }
            }
        }
        armada_board->rescan_required = rescan_required;
        armada_board->rescan_countdown = ARMADA_BOARD_PORT_RESCAN_DELAY_COUNT;
    }
    else
    {
        armada_board->rescan_countdown--;
    }

    

    if (armada_board->rescan_required == FBE_TRUE){
        fbe_base_object_trace((fbe_base_object_t*)armada_board, 
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s rescan hardware required. \n",
                            __FUNCTION__);
    }

    /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)armada_board);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)armada_board, 
                            FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
        }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn armada_board_update_hardware_configuration_poll_cond_function(         
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function is triggered every 3 seconds, 
* and initiates the hardware configuration update.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/

static fbe_lifecycle_status_t 
armada_board_update_hardware_configuration_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_armada_board_t * armada_board = (fbe_armada_board_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)armada_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);


    if (armada_board->rescan_required == FBE_TRUE){
        status = fbe_lifecycle_set_cond(&fbe_armada_board_lifecycle_const, 
                                        (fbe_base_object_t*)armada_board, 
                                        FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)armada_board, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't set harware update condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)armada_board);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)armada_board, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
