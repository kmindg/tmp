/***************************************************************************
 *  fbe_terminator_sim_main.c
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
#include "fbe_terminator.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_miniport_interface.h"

/**********************************/
/*        local variables         */
/**********************************/

//extern fbe_terminator_service_t 	terminator_service;

/******************************/
/*     local function         */
/*****************************/


/*********************************************************************
 *            fbe_terminator_init ()
 *********************************************************************
 *
 *  Description: initialize the terminator service
 *
 *	Inputs: 
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/11/08: sharel	created
 *    
 *********************************************************************/
fbe_status_t fbe_terminator_init(fbe_packet_t * packet)
{    
	
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

#if 0
	fbe_base_service_init((fbe_base_service_t *) &terminator_service);
	fbe_base_service_set_service_id((fbe_base_service_t *) &terminator_service, FBE_SERVICE_ID_TERMINATOR);
	fbe_base_service_set_trace_level((fbe_base_service_t *) &terminator_service, fbe_trace_get_default_trace_level());
#endif
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}    

/*********************************************************************
 *            fbe_terminator_destroy ()
 *********************************************************************
 *
 *  Description: destroy the terminator service
 *
 *	Inputs: 
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/11/08: sharel	created
 *    
 *********************************************************************/
fbe_status_t fbe_terminator_destroy(fbe_packet_t * packet)
{
    
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

#if 0
	fbe_base_service_destroy((fbe_base_service_t *) &terminator_service);
#endif

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

