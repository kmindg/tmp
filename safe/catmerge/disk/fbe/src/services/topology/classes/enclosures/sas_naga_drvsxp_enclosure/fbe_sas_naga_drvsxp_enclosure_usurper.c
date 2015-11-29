/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_naga_drvsxp_enclosure_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains usurper functions for sas naga_drvsxp enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ***************************************************************************/

#include "fbe_base_discovered.h"
#include "fbe_ses.h"
#include "sas_naga_drvsxp_enclosure_private.h"
#include "fbe_enclosure_data_access_private.h"
#include "fbe_transport_memory.h"


static fbe_status_t 
sas_naga_drvsxp_enclosure_getEnclosurePrvtInfo(fbe_sas_naga_drvsxp_enclosure_t *naga_drvsxpEnclosurePtr, 
                                     fbe_packet_t *packet);
/*!**************************************************************
 * @fn fbe_sas_naga_drvsxp_enclosure_control_entry(
 *                         fbe_object_handle_t object_handle, 
 *                         fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function is the entry point for control
 *  operations to the holster enclosure object.
 *
 * @param object_handle - Handler to the enclosure object.
 * @param packet - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_naga_drvsxp_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t                        status;
    fbe_sas_naga_drvsxp_enclosure_t   *pSasNagaDrvsxpEnclosure = NULL;
    fbe_payload_control_operation_opcode_t control_code;

    pSasNagaDrvsxpEnclosure = (fbe_sas_naga_drvsxp_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)pSasNagaDrvsxpEnclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pSasNagaDrvsxpEnclosure),
                            "%s entry .\n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);
    fbe_base_object_customizable_trace((fbe_base_object_t *)pSasNagaDrvsxpEnclosure,
                                       FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pSasNagaDrvsxpEnclosure),
                                       "%s: cntrlCode %s\n", 
                                       __FUNCTION__, 
                                       fbe_base_enclosure_getControlCodeString(control_code));
    switch(control_code)
    {

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_PRVT_INFO:
            status = sas_naga_drvsxp_enclosure_getEnclosurePrvtInfo(pSasNagaDrvsxpEnclosure, packet);
            break;

        default:
            status = fbe_eses_enclosure_control_entry(object_handle, packet);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t *)pSasNagaDrvsxpEnclosure,
                                      FBE_TRACE_LEVEL_WARNING,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pSasNagaDrvsxpEnclosure),
                                      "sas_naga_drvsxp_ctrl_entry, unsupported ctrlCode 0x%X, status 0x%X.\n", 
                                      control_code,
                                      status);
            }
            break;
    }

    return status;
} //fbe_sas_naga_drvsxp_enclosure_control_entry

/*!**************************************************************
 * @fn sas_naga_drvsxp_enclosure_getEnclosurePrvtInfo(
 *                         fbe_sas_naga_drvsxp_enclosure_t *naga_drvsxpEnclosurePtr, 
 *                         fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function copy fbe_sas_naga_drvsxp_enclosure_t structure
 *  into buffer.
 *
 * @param naga_drvsxpEnclosurePtr - pointer to the fbe_sas_naga_drvsxp_enclosure_t.
 * @param packet - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ****************************************************************/
static fbe_status_t 
sas_naga_drvsxp_enclosure_getEnclosurePrvtInfo(fbe_sas_naga_drvsxp_enclosure_t *pNagaDrvSxpEnclosure,
                                     fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t             bufferLength = 0;
    fbe_base_object_mgmt_get_enclosure_prv_info_t   *bufferPtr;
    fbe_enclosure_number_t                          enclosure_number = 0;   
    fbe_status_t                                    status;

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)pNagaDrvSxpEnclosure, &enclosure_number);

    fbe_base_object_customizable_trace((fbe_base_object_t *)pNagaDrvSxpEnclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pNagaDrvSxpEnclosure),
                          "%s: entry\n", __FUNCTION__);    

    bufferLength = sizeof(fbe_base_object_mgmt_get_enclosure_prv_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,(fbe_base_enclosure_t*)pNagaDrvSxpEnclosure, TRUE, (fbe_payload_control_buffer_t *)&bufferPtr, bufferLength);
    if(status == FBE_STATUS_OK)
    {
	    /* Copy fbe_sas_naga_drvsxp_enclosure_t structute into buffer */
		memcpy(bufferPtr, pNagaDrvSxpEnclosure, sizeof(fbe_sas_naga_drvsxp_enclosure_t));
	}
     
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return (status);

}   // end of sas_naga_drvsxp_enclosure_getEnclosurePrvtInfo

//End of file fbe_sas_naga_drvsxp_enclosure_usurper.c
