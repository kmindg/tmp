/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_enclosure_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains general functions for sas enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   11/20/2008:  Created file header. NC
 *
 ***************************************************************************/
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "sas_enclosure_private.h"

/* Class methods forward declaration */
fbe_status_t fbe_sas_enclosure_load(void);
fbe_status_t fbe_sas_enclosure_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_sas_enclosure_class_methods = {FBE_CLASS_ID_SAS_ENCLOSURE,
							        			  fbe_sas_enclosure_load,
												  fbe_sas_enclosure_unload,
												  fbe_sas_enclosure_create_object,
												  fbe_sas_enclosure_destroy_object,
												  fbe_sas_enclosure_control_entry,
												  fbe_sas_enclosure_event_entry,
                                                  NULL,
												  fbe_sas_enclosure_monitor_entry};


/* Forward declaration */
static fbe_status_t fbe_sas_enclosure_get_parent_address(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
static fbe_status_t sas_enclosure_get_enclosure_driver_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);



fbe_status_t fbe_sas_enclosure_load(void)
{
	fbe_topology_class_trace(FBE_CLASS_ID_SAS_ENCLOSURE,
	                         FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                         "%s entry\n", __FUNCTION__);
	return fbe_sas_enclosure_monitor_load_verify();
}

fbe_status_t fbe_sas_enclosure_unload(void)
{
	fbe_topology_class_trace(FBE_CLASS_ID_SAS_ENCLOSURE,
	                         FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                         "%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_sas_enclosure_create_object(                  
*                    fbe_packet_t * packet, 
*                    fbe_object_handle_t object_handle)                  
***************************************************************************
*
* @brief
*       This is the constructor for sas enclosure class.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      object_handle - object handle.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   20-Nov-2008 NC - Added header.
*
***************************************************************************/
fbe_status_t 
fbe_sas_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_sas_enclosure_t * sas_enclosure;
	fbe_status_t status;
	//NTSTATUS nt_status;

	fbe_topology_class_trace(FBE_CLASS_ID_SAS_ENCLOSURE,
	                         FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                         "%s entry\n", __FUNCTION__);

	/* Call parent constructor */
	status = fbe_base_enclosure_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	sas_enclosure = (fbe_sas_enclosure_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) sas_enclosure, FBE_CLASS_ID_SAS_ENCLOSURE);	

//	sas_enclosure->enclosures_number = 0;
    sas_enclosure->base_enclosure.enclosure_number = 0;

    sas_enclosure->sasEnclosureType = FBE_SAS_ENCLOSURE_TYPE_INVALID;
	
    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_sas_enclosure_destroy_object(                  
*                    fbe_object_handle_t object_handle)                  
***************************************************************************
*
* @brief
*       This is the destructor for sas enclosure class.
*
* @param      object_handle - object handle.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   20-Nov-2008 NC - Added header.
*
***************************************************************************/
fbe_status_t 
fbe_sas_enclosure_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;

	fbe_topology_class_trace(FBE_CLASS_ID_SAS_ENCLOSURE,
	                         FBE_TRACE_LEVEL_DEBUG_HIGH, 
	                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                         "%s entry\n", __FUNCTION__);
	
	/* Check parent edges */
	/* Cleanup */
	/* Cancel all packets from terminator queue */
	
	/* Call parent destructor */
	status = fbe_base_enclosure_destroy_object(object_handle);
	return status;
}


/*!***************************************************************
 * @fn fbe_sas_enclosure_send_smp_functional_packet()
 *                    fbe_sas_enclosure_t * sas_enclosure, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This function sends functional packet down to smp edge.
 *
 * @param  sas_enclosure - enclosure object
 * @param  packet - io packet pointer
 *
 * @return fbe_status_t - status from transport
 *
 * HISTORY:
 *  9/9/08 - created, NCHIU
 *
 ****************************************************************/
fbe_status_t fbe_sas_enclosure_send_smp_functional_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_smp_transport_send_functional_packet(&sas_enclosure->smp_edge, packet);

    return status;
}

/*!***************************************************************
 * @fn fbe_sas_enclosure_send_ssp_functional_packet()
 *                    fbe_sas_enclosure_t * sas_enclosure, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This function sends functional packet down to ssp edge.
 *
 * @param  sas_enclosure - enclosure object
 * @param  packet - io packet pointer
 *
 * @return fbe_status_t - status from transport
 *
 * HISTORY:
 *  9/9/08 - created, NCHIU
 *
 ****************************************************************/
fbe_status_t fbe_sas_enclosure_send_ssp_functional_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_ssp_transport_send_functional_packet(&sas_enclosure->ssp_edge, packet);

    return status;
}

/****************************************************************
 * fbe_sas_enclosure_send_ssp_control_packet()
 *                    fbe_sas_enclosure_t * sas_enclosure, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This function sends control packet down to ssp edge.
 *
 * @param  sas_enclosure - enclosure object
 * @param  packet - io packet pointer
 *
 * @return fbe_status_t - status from transport
 *
 * HISTORY:
 *  9/23/08 - created, Peter Puhov
 *
 ****************************************************************/
fbe_status_t fbe_sas_enclosure_send_ssp_control_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_ssp_transport_send_control_packet(&sas_enclosure->ssp_edge, packet);

    return status;
}


/****************************************************************
 * fbe_sas_enclosure_send_smp_control_packet()
 *                    fbe_sas_enclosure_t * sas_enclosure, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This function sends control packet down to smp edge.
 *
 * @param  sas_enclosure - enclosure object
 * @param  packet - io packet pointer
 *
 * @return fbe_status_t - status from transport
 *
 * HISTORY:
 *  11/20/08 - created.  NC
 *
 ****************************************************************/
fbe_status_t fbe_sas_enclosure_send_smp_control_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_smp_transport_send_control_packet(&sas_enclosure->smp_edge, packet);

    return status;
}

/*!***************************************************************
 * @fn fbe_sas_enclosure_get_smp_path_attributes(
 *                            fbe_sas_enclosure_t * sas_enclosure, 
 *                            fbe_path_attr_t * path_attr)            
 ****************************************************************
 * @brief
 *  This function gets the path attribute of the SMP edge. 
 *  It is only valid when the path state of the SMP edge is FBE_PATH_STATE_ENABLED.
 *
 * @param  sas_enclosure - enclosure object
 * @param  path_attr - the pointer to the path attribute.
 *
 * @return fbe_status_t - status from transport
 *
 * HISTORY:
 *  08-Jan-2009 PHE - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_sas_enclosure_get_smp_path_attributes(fbe_sas_enclosure_t * sas_enclosure, fbe_path_attr_t * path_attr)
{
	fbe_path_state_t path_state;
	fbe_status_t status;

    status = fbe_smp_transport_get_path_state(&sas_enclosure->smp_edge, &path_state);
	if(path_state != FBE_PATH_STATE_ENABLED){
		return FBE_STATUS_EDGE_NOT_ENABLED;
	}

    status = fbe_smp_transport_get_path_attributes(&sas_enclosure->smp_edge, path_attr);
	return status;
}

/*!***************************************************************
 * @fn fbe_sas_enclosure_get_ssp_path_attributes(
 *                            fbe_sas_enclosure_t * sas_enclosure, 
 *                            fbe_path_attr_t * path_attr)            
 ****************************************************************
 * @brief
 *  This function gets the path attribute of the SSP edge. 
 *  It is only valid when the path state of the SSP edge is FBE_PATH_STATE_ENABLED.
 *
 * @param  sas_enclosure - enclosure object
 * @param  path_attr - the pointer to the path attribute.
 *
 * @return fbe_status_t - status from transport
 *
 * HISTORY:
 *  08-Jan-2009 PHE - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_sas_enclosure_get_ssp_path_attributes(fbe_sas_enclosure_t * sas_enclosure, fbe_path_attr_t * path_attr)
{
	fbe_path_state_t path_state;
	fbe_status_t status;

    status = fbe_ssp_transport_get_path_state(&sas_enclosure->ssp_edge, &path_state);
	if(path_state != FBE_PATH_STATE_ENABLED){
		return FBE_STATUS_EDGE_NOT_ENABLED;
	}

    status = fbe_ssp_transport_get_path_attributes(&sas_enclosure->ssp_edge, path_attr);
	return status;
}

