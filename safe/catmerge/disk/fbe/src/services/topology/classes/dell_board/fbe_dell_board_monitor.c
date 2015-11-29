#include "fbe/fbe_board.h"
#include "dell_board_private.h"

fbe_status_t 
fbe_dell_board_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_dell_board_t * dell_board = NULL;

	dell_board = (fbe_dell_board_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)dell_board,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(dell_board),
	                                  (fbe_base_object_t*)dell_board, packet);
}

fbe_status_t fbe_dell_board_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(dell_board));
}

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t dell_board_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t dell_board_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_status_t dell_board_discovery_poll_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t dell_board_update_hardware_configuration_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t dell_board_update_hardware_configuration_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(dell_board);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_NULL_COND(dell_board);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(dell_board) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		dell_board,
		FBE_LIFECYCLE_NULL_FUNC,       /* online function */
		FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t dell_board_update_hardware_configuration_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION,
		dell_board_update_hardware_configuration_cond_function)
};

static fbe_lifecycle_const_cond_t dell_board_discovery_poll_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_TIMER_COND(
		FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
		dell_board_discovery_poll_cond_function)
};

static fbe_lifecycle_const_cond_t dell_board_discovery_update_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE,
		dell_board_discovery_update_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(dell_board);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t dell_board_ready_rotary[] = {
	/* Derived conditions */
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(dell_board_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(dell_board_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(dell_board_update_hardware_configuration_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(dell_board)[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, dell_board_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(dell_board);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(dell_board) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		dell_board,
		FBE_CLASS_ID_DELL_BOARD,                  /* This class */
		FBE_LIFECYCLE_CONST_DATA(base_board))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/
static fbe_lifecycle_status_t 
dell_board_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_dell_board_t * dell_board = (fbe_dell_board_t*)p_object;
	fbe_u32_t number_of_clients = 0;

	fbe_base_object_trace((fbe_base_object_t*)dell_board,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* We just push additional context on the monitor packet stack */
	status = fbe_transport_set_completion_function(packet, dell_board_discovery_poll_cond_completion, dell_board);

	/* This will be called by actuall poll function */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

	return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
dell_board_discovery_poll_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_status_t status;
	fbe_dell_board_t * dell_board = NULL;
	
	dell_board = (fbe_dell_board_t *)context;

	/* Check the packet status */

	/* I would assume that it is OK to clear CURRENT condition here.
		After all we are in condition completion context */

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)dell_board);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
dell_board_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
	fbe_status_t status;
	fbe_dell_board_t * dell_board = (fbe_dell_board_t*)p_object;
	fbe_u32_t number_of_clients = 0;

	fbe_base_object_trace((fbe_base_object_t*)dell_board,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	status = fbe_base_discovering_get_number_of_clients((fbe_base_discovering_t *)dell_board, &number_of_clients);
	if(number_of_clients == 0) {
		if(dell_board->io_port_type[0] == FBE_PORT_TYPE_SAS_LSI){
			status = fbe_base_discovering_instantiate_client((fbe_base_discovering_t *) dell_board, 
															  FBE_CLASS_ID_SAS_CPD_PORT,
															  0 /* We should have only one client */);
		}

		if(dell_board->io_port_type[0] == FBE_PORT_TYPE_SAS_PMC){
			status = fbe_base_discovering_instantiate_client((fbe_base_discovering_t *) dell_board, 
															FBE_CLASS_ID_SAS_PMC_PORT,
															0 /* We should have only one client */);
		}

	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)dell_board);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
dell_board_update_hardware_configuration_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_dell_board_t * dell_board = (fbe_dell_board_t*)p_object;
	fbe_u32_t number_of_clients = 0;

	fbe_base_object_trace((fbe_base_object_t*)dell_board,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* We just push additional context on the monitor packet stack */
	status = fbe_transport_set_completion_function(packet, dell_board_update_hardware_configuration_cond_completion, dell_board);

	fbe_dell_board_update_hardware_ports(dell_board, packet);
	return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
dell_board_update_hardware_configuration_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_status_t status;
	fbe_dell_board_t * dell_board = NULL;
	
	dell_board = (fbe_dell_board_t *)context;

	/* Check the packet status */

	/* I would assume that it is OK to clear CURRENT condition here.
		After all we are in condition completion context */

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)dell_board);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)dell_board, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_STATUS_OK;
}
