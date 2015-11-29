/**********************************************************************
 *  Copyright (C)  EMC Corporation 2014
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/**********************************************************************
 * fbe_sas_miranda_enclosure_executer.c
 **********************************************************************
 * DESCRIPTION: 
 *  This file contains entry functions for the miranda enclosure class.
 *
 * HISTORY:
 *   03/28/2014: Joe Perry - Created from fbe_sas_citadel_enclosure_executer.c
 *             
 ***********************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"
#include "fbe_sas_port.h"
#include "sas_miranda_enclosure_private.h"
#include "edal_eses_enclosure_data.h"


static fbe_status_t 
sas_miranda_enclosure_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);


/****************************************************************
 * fbe_sas_miranda_enclosure_io_entry()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles incoming io requests.
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
 *   03/28/2014: Joe Perry - Created from fbe_sas_citadel_enclosure_executer.c
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_miranda_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_sas_miranda_enclosure_t * sas_miranda_enclosure = NULL;
    fbe_transport_id_t transport_id;
    fbe_status_t status;

    sas_miranda_enclosure = (fbe_sas_miranda_enclosure_t *)fbe_base_handle_to_pointer(object_handle);	
    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_miranda_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_miranda_enclosure),
                            "%s entry\n", __FUNCTION__);

    /* Fisrt we need to figure out to what transport this packet belong */
    fbe_transport_get_transport_id(packet, &transport_id);
    switch(transport_id) {
        case FBE_TRANSPORT_ID_DISCOVERY:
            /* The server part of fbe_discovery transport is a member of discovering class.
                Even more than that, we do not expect to receive discovery protocol packets
                for "non discovering" objects 
            */
            status = fbe_base_discovering_discovery_bouncer_entry((fbe_base_discovering_t *) sas_miranda_enclosure,
                                                                        (fbe_transport_entry_t)sas_miranda_enclosure_discovery_transport_entry,
                                                                        packet);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/****************************************************************
 * sas_miranda_enclosure_discovery_transport_entry()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles incoming requests on our discovery edge.
 *  This is used to answer queries from the drive and other enclosure
 *  objects.
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
 *   03/28/2014: Joe Perry - Created from fbe_sas_citadel_enclosure_executer.c
 *
 ****************************************************************/
static fbe_status_t 
sas_miranda_enclosure_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_sas_miranda_enclosure_t * sas_miranda_enclosure = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_opcode_t discovery_opcode;
    fbe_status_t status;

    sas_miranda_enclosure = (fbe_sas_miranda_enclosure_t *)base_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_miranda_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_miranda_enclosure),
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);
    
    switch(discovery_opcode){


        default:
            status = fbe_eses_enclosure_discovery_transport_entry((fbe_base_object_t *)sas_miranda_enclosure, packet);
            break;
    }

    return status;
}

/****************************************************************
 * fbe_sas_miranda_enclosure_event_entry()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles events to the ESES enclosure object.
 *
 * PARAMETERS:
 *  object_handle - The object receiving the event
 *  event_type - The type of event.
 *  event_context - The context for the event.
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *   03/28/2014: Joe Perry - Created from fbe_sas_citadel_enclosure_executer.c
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_miranda_enclosure_event_entry(fbe_object_handle_t object_handle, 
                                    fbe_event_type_t event_type, 
                                    fbe_event_context_t event_context)
{
    fbe_sas_miranda_enclosure_t * sas_miranda_enclosure = NULL;
    fbe_status_t status;

    sas_miranda_enclosure = (fbe_sas_miranda_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_miranda_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_miranda_enclosure),
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);
    /*
     * This class did not handle the event, just go to the superclass
     * to handle it.
     */        
    status = fbe_eses_enclosure_event_entry(object_handle, event_type, event_context);

    return status;

}

//End of file fbe_sas_miranda_enclosure_executer.c
