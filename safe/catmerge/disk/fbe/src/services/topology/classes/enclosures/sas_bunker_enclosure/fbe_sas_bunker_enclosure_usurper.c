/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_bunker_enclosure_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains usurper functions for sas bunker enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *    08-Jun-2009 : Dipak Patel - Added header.
 *    08-Jun-2009 : Dipak Patel - Created from fbe_sas_viper_enclosure_usurper.c
 *
 ***************************************************************************/

#include "fbe_base_discovered.h"
#include "fbe_ses.h"
#include "sas_bunker_enclosure_private.h"
#include "fbe_enclosure_data_access_private.h"
#include "fbe_transport_memory.h"


static fbe_status_t 
sas_bunker_enclosure_getEnclosurePrvtInfo(fbe_sas_bunker_enclosure_t *bunkerEnclosurePtr, 
                                     fbe_packet_t *packetPtr);
/*!**************************************************************
 * @fn fbe_sas_bunker_enclosure_control_entry(
 *                         fbe_object_handle_t object_handle, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for control
 *  operations to the bunker enclosure object.
 *
 * @param object_handle - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *    08-Jun-2009 : Dipak Patel - Added header.
 *    08-Jun-2009 : Dipak Patel - Created from fbe_sas_viper_enclosure_control_entry
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_bunker_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_bunker_enclosure_t * sas_bunker_enclosure = NULL;
     fbe_payload_control_operation_opcode_t control_code;

    sas_bunker_enclosure = (fbe_sas_bunker_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bunker_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bunker_enclosure),
                            "%s entry .\n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);
    fbe_base_object_customizable_trace((fbe_base_object_t *)sas_bunker_enclosure,
                                       FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bunker_enclosure),
                                       "%s: cntrlCode %s\n", 
                                       __FUNCTION__, 
                                       fbe_base_enclosure_getControlCodeString(control_code));
    switch(control_code) {

		case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_PRVT_INFO:
            status = sas_bunker_enclosure_getEnclosurePrvtInfo(sas_bunker_enclosure, packet);
        break;

        default:
            status = fbe_eses_enclosure_control_entry(object_handle, packet);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t *)sas_bunker_enclosure,
                                      FBE_TRACE_LEVEL_ERROR,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bunker_enclosure),
                                      "%s: failed to process cntrlCode 0x%x, status %d\n",
                                        __FUNCTION__, 
                                        control_code,
                                        status);
            }
            break;
    }

    return status;
}

/*!**************************************************************
 * @fn sas_bunker_enclosure_getEnclosurePrvtInfo(
 *                         fbe_sas_bunker_enclosure_t *bunkerEnclosurePtr, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function copy fbe_sas_bunker_enclosure_t structure
 *  into buffer.
 *
 * @param bunkerEnclosurePtr - pointer to the fbe_sas_bunker_enclosure_t.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *    08-Jun-2009:  Created Dipak Patel
 *
 ****************************************************************/
static fbe_status_t 
sas_bunker_enclosure_getEnclosurePrvtInfo(fbe_sas_bunker_enclosure_t *bunkerEnclosurePtr, 
                                     fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t        bufferLength = 0;
    fbe_base_object_mgmt_get_enclosure_prv_info_t   *bufferPtr;
    fbe_enclosure_number_t      enclosure_number = 0;   
    fbe_status_t status;

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)bunkerEnclosurePtr, &enclosure_number);

    fbe_base_object_customizable_trace((fbe_base_object_t *)bunkerEnclosurePtr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)bunkerEnclosurePtr),
                          "bunkerEncl:%s entry\n", __FUNCTION__);    

    bufferLength = sizeof(fbe_base_object_mgmt_get_enclosure_prv_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)bunkerEnclosurePtr, TRUE, (fbe_payload_control_buffer_t *)&bufferPtr, bufferLength);
    if(status == FBE_STATUS_OK)
    {
	    /* Copy fbe_sas_bunker_enclosure_t structute into buffer */
		memcpy(bufferPtr, bunkerEnclosurePtr, sizeof(fbe_sas_bunker_enclosure_t));
	}
     
    fbe_transport_set_status(packetPtr, status, 0);
    fbe_transport_complete_packet(packetPtr);

    return (status);

}   // end of sas_bunker_enclosure_getEnclosurePrvtInfo

//End of file fbe_sas_bunker_enclosure_usurper.c
