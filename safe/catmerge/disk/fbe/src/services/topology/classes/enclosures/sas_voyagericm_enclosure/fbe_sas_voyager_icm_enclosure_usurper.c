/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_voyager_icm_enclosure_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains usurper functions for sas voyager_icm enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   7/12/2008:  Created file - Dipak Patel
 *
 ***************************************************************************/

#include "fbe_base_discovered.h"
#include "fbe_ses.h"
#include "sas_voyager_icm_enclosure_private.h"
#include "fbe_enclosure_data_access_private.h"
#include "fbe_transport_memory.h"


static fbe_status_t 
sas_voyager_icm_enclosure_getEnclosurePrvtInfo(fbe_sas_voyager_icm_enclosure_t *voyager_icmEnclosurePtr, 
                                     fbe_packet_t *packet);
/*!**************************************************************
 * @fn fbe_sas_voyager_icm_enclosure_control_entry(
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
 *   26-Feb-2010: Created from fbe_sas_viper_enclosure_control_entry. Dipak Patel
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_voyager_icm_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_voyager_icm_enclosure_t * sas_voyager_icm_enclosure = NULL;
    fbe_payload_control_operation_opcode_t control_code;

    sas_voyager_icm_enclosure = (fbe_sas_voyager_icm_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_voyager_icm_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_voyager_icm_enclosure),
                            "sas_voyager_icm_encl_ctrl_entry.\n");

    control_code = fbe_transport_get_control_code(packet);
    fbe_base_object_customizable_trace((fbe_base_object_t *)sas_voyager_icm_enclosure,
                                       FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_voyager_icm_enclosure),
                                       "sas_voyager_icm_encl_ctrl_entry:cntrlCode %s\n", 
                                       fbe_base_enclosure_getControlCodeString(control_code));
    switch(control_code) {

    case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_PRVT_INFO:
            status = sas_voyager_icm_enclosure_getEnclosurePrvtInfo(sas_voyager_icm_enclosure, packet);
        break;

        default:
            status = fbe_eses_enclosure_control_entry(object_handle, packet);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t *)sas_voyager_icm_enclosure,
                                      FBE_TRACE_LEVEL_WARNING,
                                      fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_voyager_icm_enclosure),
                                      "VoyagerICMEncl:sas_voyager_icm_encl_ctrl_entry, unsupported ctrlCode 0x%x, stat %d\n", 
                                        control_code,
                                        status);
            }
            break;
    }

    return status;
}

/*!**************************************************************
 * @fn sas_voyager_icm_enclosure_getEnclosurePrvtInfo(
 *                         fbe_sas_voyager_icm_enclosure_t *voyager_icmEnclosurePtr, 
 *                         fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function copy fbe_sas_voyager_icm_enclosure_t structure
 *  into buffer.
 *
 * @param voyager_icmEnclosurePtr - pointer to the fbe_sas_voyager_icm_enclosure_t.
 * @param packet - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   26-Feb-2010:  Created Dipak Patel
 *
 ****************************************************************/
static fbe_status_t 
sas_voyager_icm_enclosure_getEnclosurePrvtInfo(fbe_sas_voyager_icm_enclosure_t *voyager_icmEnclosurePtr, 
                                     fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t        bufferLength = 0;
    fbe_base_object_mgmt_get_enclosure_prv_info_t   *bufferPtr;
    fbe_enclosure_number_t      enclosure_number = 0;   
    fbe_status_t status;

    fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)voyager_icmEnclosurePtr, &enclosure_number);

    fbe_base_object_customizable_trace((fbe_base_object_t *)voyager_icmEnclosurePtr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)voyager_icmEnclosurePtr),
                          "VoyagerICMEncl:%s entry\n", __FUNCTION__);    

    bufferLength = sizeof(fbe_base_object_mgmt_get_enclosure_prv_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,(fbe_base_enclosure_t*)voyager_icmEnclosurePtr, TRUE, (fbe_payload_control_buffer_t *)&bufferPtr, bufferLength);
    if(status == FBE_STATUS_OK)
    {
	    /* Copy fbe_sas_voyager_icm_enclosure_t structute into buffer */
		memcpy(bufferPtr, voyager_icmEnclosurePtr, sizeof(fbe_sas_voyager_icm_enclosure_t));
	}
     
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return (status);

}   // end of sas_voyager_icm_enclosure_getEnclosurePrvtInfo

//End of file fbe_sas_voyager_icm_enclosure_usurper.c
