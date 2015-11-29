/***************************************************************************
 *  fbe_terminator_main.c
 ***************************************************************************
 *
 *  Description
 *      main entry point to the terminator service
 *      
 *
 *  History:
 *      06/11/08	sharel	Created
 *    
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe_base_service.h"
#include "fbe/fbe_transport.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_terminator_service.h"
#include "fbe/fbe_terminator_api.h"

/**********************************/
/*        local variables         */
/**********************************/

#if 0
/* Declare our service methods */
fbe_status_t fbe_terminator_control_entry(fbe_packet_t * packet); 

/*the lines below will route all traffic from the service manager to our terminator*/
fbe_service_methods_t 				fbe_terminator_service_methods = {FBE_SERVICE_ID_TERMINATOR, fbe_terminator_control_entry};
#endif
//fbe_terminator_service_t 			terminator_service;


/******************************/
/*     local function         */
/*****************************/

#if 0
/*********************************************************************
 *            fbe_terminator_control_entry ()
 *********************************************************************
 *
 *  Description: ioctl entry point for the service
 *
 *	Inputs: 
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/11/08: sharel	created
 *    
 *********************************************************************/
fbe_status_t fbe_terminator_control_entry(fbe_packet_t * packet)
{    
	fbe_status_t 				status = FBE_STATUS_GENERIC_FAILURE;
	fbe_payload_control_operation_opcode_t control_code;

	control_code = fbe_transport_get_control_code(packet);

	if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
		status = fbe_terminator_init(packet);
		return status;
	}

	if(!fbe_base_service_is_initialized((fbe_base_service_t *) &terminator_service)){
		fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_NOT_INITIALIZED;
	}

	switch(control_code) {
		case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
			status = fbe_terminator_destroy( packet);
			break;
	case FBE_TERMINATOR_CONTROL_CODE_INSERT_BOARD:
			status = fbe_terminator_service_insert_board (packet);
			break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_FC_PORT:
        status = fbe_terminator_service_insert_fc_port(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_ISCSI_PORT:
        status = fbe_terminator_service_insert_iscsi_port(packet);
        break;
	case FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_PORT:
			status = fbe_terminator_service_insert_sas_port (packet);
			break;
	case FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_ENCLOSURE:
			status = fbe_terminator_service_insert_sas_enclosure (packet);
			break;
		default:
			status  = fbe_service_manager_send_control_packet (packet);/*this is our last resort*/
			break;
	}

	return status;

}
#endif

