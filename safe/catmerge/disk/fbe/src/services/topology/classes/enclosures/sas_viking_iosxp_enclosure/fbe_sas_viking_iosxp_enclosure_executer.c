/**********************************************************************
 *  Copyright (C)  EMC Corporation 2013
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/
/*!*************************************************************************
 * @file fbe_sas_viking_iosxp_enclosure_executer.c
 ***************************************************************************
 *
 * @brief
 *  This file contains entry functions for the viking enclosure class.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"
#include "fbe_sas_port.h"
#include "sas_viking_iosxp_enclosure_private.h"
#include "edal_eses_enclosure_data.h"


static fbe_status_t 
fbe_sas_viking_iosxp_enclosure_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);

/*!*************************************************************************
* @fn fbe_sas_viking_iosxp_enclosure_io_entry(                              
*                    fbe_object_handle_t object_handle,
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function handles incoming io requests.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      object_handle - object handle.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   22-Nov-2013:  Greg Bailey - Created.
*
***************************************************************************/
fbe_status_t 
fbe_sas_viking_iosxp_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_sas_viking_iosxp_enclosure_t * pSasVikingEnclosure = NULL;
    fbe_transport_id_t          transport_id;
    fbe_status_t                status;

    pSasVikingEnclosure = (fbe_sas_viking_iosxp_enclosure_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_customizable_trace((fbe_base_object_t*)pSasVikingEnclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pSasVikingEnclosure),
                            "%s entry\n", __FUNCTION__);

    /* Fisrt we need to figure out to what transport this packet belong */
    fbe_transport_get_transport_id(packet, &transport_id);
    switch(transport_id) {
        case FBE_TRANSPORT_ID_DISCOVERY:
            /* The server part of fbe_discovery transport is a member of discovering class.
                Even more than that, we do not expect to receive discovery protocol packets
                for "non discovering" objects 
            */
            status = fbe_base_discovering_discovery_bouncer_entry((fbe_base_discovering_t *) pSasVikingEnclosure,
                                                                  (fbe_transport_entry_t)fbe_sas_viking_iosxp_enclosure_discovery_transport_entry,
                                                                  packet);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*************************************************************************
* @fn fbe_sas_viking_iosxp_enclosure_discovery_transport_entry(                              
*                    fbe_base_object_t * base_object,
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function handles incoming requests on our discovery edge.
 *  This is used to answer queries from the drive and other enclosure
 *  objects.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      base_object - base object.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   22-Nov-2013:  Greg Bailey - Created.
*
***************************************************************************/
static fbe_status_t 
fbe_sas_viking_iosxp_enclosure_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_sas_viking_iosxp_enclosure_t    * pSasVikingEnclosure = NULL;
    fbe_payload_ex_t                    * pPayload = NULL;
    fbe_payload_discovery_operation_t   * pPayloadDiscoveryOperation = NULL;
    fbe_payload_discovery_opcode_t      discovery_opcode;
    fbe_status_t                        status;

    pSasVikingEnclosure = (fbe_sas_viking_iosxp_enclosure_t *)base_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pSasVikingEnclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pSasVikingEnclosure),
                          "%s entry\n", __FUNCTION__);

    pPayload = fbe_transport_get_payload_ex(packet);
    pPayloadDiscoveryOperation = fbe_payload_ex_get_discovery_operation(pPayload);
    fbe_payload_discovery_get_opcode(pPayloadDiscoveryOperation, &discovery_opcode);
    
    switch(discovery_opcode)
    {
        default:
            status = fbe_eses_enclosure_discovery_transport_entry((fbe_base_object_t *)pSasVikingEnclosure, packet);
            break;
    }

    return status;
}

/*!*************************************************************************
* @fn fbe_sas_viking_iosxp_enclosure_event_entry(                              
*                    fbe_object_handle_t object_handle,
*                    fbe_event_type_t event_type,
*                    fbe_event_context_t event_context)                  
***************************************************************************
*
* @brief
*       This function handles incoming requests on our discovery edge.
*  This is used to answer queries from the drive and other enclosure
*  objects.
*
*
* @param      object_handle - base handle.
* @param      event_type - Type of Event.
* @param      event_context - Context of Event.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   22-Nov-2013:  Greg Bailey - Created.
*
***************************************************************************/
fbe_status_t 
fbe_sas_viking_iosxp_enclosure_event_entry(fbe_object_handle_t object_handle, 
                                    fbe_event_type_t event_type, 
                                    fbe_event_context_t event_context)
{
    fbe_sas_viking_iosxp_enclosure_t * pSasVikingEnclosure = NULL;
    fbe_status_t status;

    pSasVikingEnclosure = (fbe_sas_viking_iosxp_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)pSasVikingEnclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pSasVikingEnclosure),
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);
    /*
     * This class did not handle the event, just go to the superclass
     * to handle it.
     */        
    status = fbe_eses_enclosure_event_entry(object_handle, event_type, event_context);

    return status;

}

//End of file fbe_sas_viking_iosxp_enclosure_executer.c
