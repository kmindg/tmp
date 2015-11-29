/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_enclosure_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Enclosure related APIs.
 *      
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_enclosure_interface
 * 
 * @version
 *      10/14/08    sharel - Created
 *    
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "EmcPAL_Memory.h"

#ifdef C4_INTEGRATED
#include "fbe/fbe_physical_package.h"
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/
static fbe_status_t fbe_api_enclosure_resume_read_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);
                                                                                                            
/*!***************************************************************
 * @fn fbe_api_enclosure_firmware_download(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_download_op_t* download_firmware) 
 *****************************************************************
 * @brief
 *  This function downloads the firmware code. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param download_firmware - download data.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
/*!***************************************************************
 * @fn fbe_api_enclosure_firmware_download(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_download_op_t* download_firmware) 
 *****************************************************************
 * @brief
 *  This function downloads the firmware code. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param download_firmware - download data.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_firmware_download (fbe_object_id_t object_id, 
                                                               fbe_enclosure_mgmt_download_op_t* download_firmware)
{
    fbe_u8_t            num_sg_elements = 0;
    fbe_sg_element_t    *sgp;
    fbe_sg_element_t    sg_element[2];
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_payload_control_operation_opcode_t      ctrlCode; 
    fbe_topology_object_type_t                  objectType;


    
    EmcpalZeroVariable(sg_element);
    if (download_firmware == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "enc_frmw_dwnld:Firmware buf NULL.\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Determine the type of object this is directed to
    status = fbe_api_get_object_type(object_id, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        ctrlCode = FBE_BASE_BOARD_CONTROL_CODE_FIRMWARE_OP;
        break;

    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
    case FBE_TOPOLOGY_OBJECT_TYPE_LCC:
        ctrlCode = FBE_BASE_ENCLOSURE_CONTROL_CODE_FIRMWARE_OP;
        break;

    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (unsigned int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    if (download_firmware->op_code == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD)
    {
        if (download_firmware->image_p == NULL)
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "enc_frmw_dwnld:download buffer NULL.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // Set the element size and address for the data buffer
        sgp = sg_element;
        fbe_sg_element_init(sgp, 
                        download_firmware->size,        // image size
                        download_firmware->image_p);    // image pointer

        // go to next element and terminate it
        fbe_sg_element_increment(&sgp);
        fbe_sg_element_terminate(sgp);
        // 1 buffer element + 1 termination element = 2
        num_sg_elements = 2;

        status = fbe_api_common_send_control_packet_with_sg_list(ctrlCode,
                                                             download_firmware,
                                                             sizeof (fbe_enclosure_mgmt_download_op_t),                                                             
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_element,
                                                             num_sg_elements,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);
    }
    else
    {
        // else this is an activate or status or abort operation
        status = fbe_api_common_send_control_packet(ctrlCode,
                                                 download_firmware,
                                                 sizeof (fbe_enclosure_mgmt_download_op_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    }

   /* To compensate for possible corruption when sending from user to kernel, 
    * we restore the original pointers.
    * In the case of user space to kernel space, 
    * the data_buf_p would be overwritten by the kernel space address 
    * in fbe_eses_enclosure_process_eses_ctrl_op,
    * we need to copy back the user space address here in case the call needs to retrieve it. 
    */
    download_firmware->image_p = sg_element[0].address;

     if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "enc_frmw_dwnld:packet error: 0x%X.\n",status);        
        return status;

    }
 
    if ((status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "enc_frmw_dwnld:pkt: err %d,qualifier %d; payload err:%d, qualifier %d\n",
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_api_enclosure_firmware_download_status(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_download_op_t* download_status) 
 *****************************************************************
 * @brief
 *  This function checks the status of the download operation. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param download_status - pointer to the download data.
 *
 * @return fbe_status_t - FBE_STATUS_OK
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_firmware_download_status(fbe_object_id_t object_id, 
                                                                     fbe_enclosure_mgmt_download_op_t* download_status)
{
    /*for now, the implementation is exactly the same, the objkect itself is checking the status and the client is 
    setting the flags in the structure to get it*/
    fbe_api_enclosure_firmware_download (object_id, download_status);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_api_enclosure_firmware_update(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_download_op_t* firmware_download_status) 
 *****************************************************************
 * @brief
 *  This function moves to the new code. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param firmware_download_status - firmware download status.
 *
 * @return fbe_status_t - FBE_STATUS_OK
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_firmware_update(fbe_object_id_t object_id,
                                                            fbe_enclosure_mgmt_download_op_t* firmware_download_status)
{
    /* for now, the implementation is exactly the same, the object itself is checking the status and 
    * the client is setting the flags in the structure to get it
    */
    fbe_api_enclosure_firmware_download (object_id, firmware_download_status);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_firmware_update_status(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_download_op_t* firmware_download_status) 
 *****************************************************************
 * @brief
 *  This function checks the move to the new code. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param firmware_download_status - download data status.
 *
 * @return fbe_status_t - FBE_STATUS_OK
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_firmware_update_status(fbe_object_id_t object_id,
                                                                   fbe_enclosure_mgmt_download_op_t* firmware_download_status)
{
        /*for now, the implementation is exactly the same, the objkect itself is checking the status and the client is 
    setting the flags in the structure to get it*/
    fbe_api_enclosure_firmware_download (object_id, firmware_download_status);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_basic_info(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_get_enclosure_basic_info_t *enclosure_info) 
 *****************************************************************
 * @brief
 *  This function gets Basic information about the enclosure. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param enclosure_info - information passed from the enclosure.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/15/09: Joe Perry - created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_get_basic_info(fbe_object_id_t object_id, 
                                 fbe_base_object_mgmt_get_enclosure_basic_info_t *enclosure_info)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;;

    if (enclosure_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:enclose info buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_BASIC_INFO,
                                                 enclosure_info,
                                                 sizeof (fbe_base_object_mgmt_get_enclosure_basic_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

   return status;

}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_info(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_get_enclosure_info_t *enclosure_info) 
 *****************************************************************
 * @brief
 *  This function gets an information about the enclosure. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param enclosure_info - information passed from the enclosure.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/14/08: sharel  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_info(fbe_object_id_t object_id, 
                                                     fbe_base_object_mgmt_get_enclosure_info_t *enclosure_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;;

    if (enclosure_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Enclosure info buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_INFO,
                                                 enclosure_info,
                                                 sizeof (fbe_base_object_mgmt_get_enclosure_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!********************************************************************
 * @fn fbe_api_enclosure_get_prvt_info(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_get_enclosure_prv_info_t *enclosure_prvt_info) 
 **********************************************************************
 * @brief
 *  This function gets an information about the enclosure. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param enclosure_prvt_info - information passed from the enclosure.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    04/02/09: Dipak Patel  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_prvt_info(fbe_object_id_t object_id, 
                                                          fbe_base_object_mgmt_get_enclosure_prv_info_t *enclosure_prvt_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;;

    if (enclosure_prvt_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Enclosure info buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_PRVT_INFO,
                                                 enclosure_prvt_info,
                                                 sizeof (fbe_base_object_mgmt_get_enclosure_prv_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!********************************************************************
 * @fn fbe_api_enclosure_bypass_drive(
 *     fbe_object_id_t object_id, 
 *     fbe_base_enclosure_drive_bypass_command_t *command) 
 **********************************************************************
 * @brief
 *  This function bypass a drive in the enclosure. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param  command - bypass command.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/14/08: sharel  created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_bypass_drive(fbe_object_id_t object_id, 
                                                         fbe_base_enclosure_drive_bypass_command_t *command)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (command == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_BYPASS,
                                                 command,
                                                 sizeof (fbe_base_enclosure_drive_bypass_command_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;


}

/*!****************************************************************************
 * @fn      fbe_api_enclosure_setup_info_memory()
 ******************************************************************************
 *
 * @brief
 * This function allocates a block of memory to store the enclosure
 * info control structure.
 *
 * @param fbe_enclosure_info - used to return the pointer to the allocated block.
 *
 * @return
 *  FBE_STATUS_OK if the memory was allocated and zeroed.
 *
 * HISTORY
 *  05/24/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t fbe_api_enclosure_setup_info_memory(fbe_base_object_mgmt_get_enclosure_info_t **fbe_enclosure_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;

    /* Allocate memory for enclosure info block.
     */
    *fbe_enclosure_info = (fbe_base_object_mgmt_get_enclosure_info_t *)fbe_api_allocate_memory(sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
    if(NULL == *fbe_enclosure_info)
    {
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
        fbe_KvTrace("%s: Memory allocation for Enclosure Info block failed. status:%d\n",
                __FUNCTION__, status);
        return status;
    }

    fbe_zero_memory(*fbe_enclosure_info, sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
    return FBE_STATUS_OK;
}



/*!****************************************************************************
 * @fn      void fbe_api_enclosure_release_info_memory()
 ******************************************************************************
 *
 * @brief
 *  This function frees an enclosure info block.
 *
 * @param fbe_enclosure_info - pointer to block to free
 *
 * HISTORY
 *  05/24/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
void fbe_api_enclosure_release_info_memory(fbe_base_object_mgmt_get_enclosure_info_t *fbe_enclosure_info)
{
    fbe_api_free_memory(fbe_enclosure_info);
}

/*!********************************************************************
 * @fn fbe_api_enclosure_unbypass_drive(
 *     fbe_object_id_t object_id, 
 *     fbe_base_enclosure_drive_unbypass_command_t *command) 
 **********************************************************************
 * @brief
 *  This function unbypass a drive in the enclosure. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param  command - bypass command.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/14/08: sharel  created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_unbypass_drive(fbe_object_id_t object_id, 
                                                           fbe_base_enclosure_drive_unbypass_command_t *command)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (command == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_UNBYPASS,
                                                 command,
                                                 sizeof (fbe_base_enclosure_drive_unbypass_command_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!********************************************************************
 * @fn fbe_api_enclosure_getSetupInfo(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_get_enclosure_setup_info_t *enclosureSetupInfo)
 **********************************************************************
 * @brief
 *  This function gets enclosure setup info. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param enclosureSetupInfo - pointer to the enclosure setup info structure.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/8/08: Joe Perry    Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_getSetupInfo(fbe_object_id_t object_id, 
                                                         fbe_base_object_mgmt_get_enclosure_setup_info_t *enclosureSetupInfo)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;

    if (enclosureSetupInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_SETUP_INFO,
                                                enclosureSetupInfo,
                                                sizeof (fbe_base_object_mgmt_get_enclosure_setup_info_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!********************************************************************
 * @fn fbe_api_enclosure_elem_group_info(
 *     fbe_object_id_t object_id, 
 *     fbe_eses_elem_group_t *elem_group_info,
 *     fbe_u8_t max)
 **********************************************************************
 * @brief
 *  This function gets enclosure setup info. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param elem_group_info - pointer to the element group info
 * @param max - max no. of element groups
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/8/08: Dipak Patel  Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_elem_group_info(fbe_object_id_t object_id, 
                                                            fbe_eses_elem_group_t *elem_group_info, 
                                                            fbe_u8_t max)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    //fbe_u32_t                                       qualifier = 0;

    if (elem_group_info == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ELEM_GROUP_INFO,
                                                elem_group_info,
                                                sizeof (fbe_eses_elem_group_t) * max,
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!********************************************************************
 * @fn fbe_api_enclosure_get_max_elem_group(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_max_elem_group *elem_group_no)
 **********************************************************************
 * @brief
 *  This function gets max no. of enclosure elem groups. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param elem_group_no - pointer to the element group info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/8/08: Dipak Patel  Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_max_elem_group(fbe_object_id_t object_id, fbe_enclosure_mgmt_max_elem_group *elem_group_no)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;    
    //fbe_u32_t                                       qualifier = 0;

    if (elem_group_no == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_MAX_ELEM_GROUPS,
                                                elem_group_no,
                                                sizeof (fbe_enclosure_mgmt_max_elem_group),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!********************************************************************
 * @fn fbe_api_enclosure_setControl(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_set_enclosure_control_t *enclosureControlInfo)
 **********************************************************************
 * @brief
 *  This function set enclosure control. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param enclosureControlInfo - pointer to the fbe_enclosure eses status page struct
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/8/08: Joe Perry    Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_setControl(fbe_object_id_t object_id, 
                                                       fbe_base_object_mgmt_set_enclosure_control_t *enclosureControlInfo)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;;

    if (enclosureControlInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_CONTROL,
                                                enclosureControlInfo,
                                                sizeof (fbe_base_object_mgmt_set_enclosure_control_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if(status == FBE_STATUS_BUSY)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "fbe_api_enclSetCntrl: Returned Status Busy\n");
        return status;
    }
   
   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "fbe_api_enclSetCntrl:pkt err:%d, pkt qualifier:%d, payload err:%d, payload qualifier:%d\n",
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        status =  FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!********************************************************************
 * @fn fbe_api_enclosure_setLeds(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_set_enclosure_led_t *enclosureLedInfo)
 **********************************************************************
 * @brief
 *  This function set enclosure LED's. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param enclosureLedInfo - pointer to the fbe_enclosure eses status page struct
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/8/08: Joe Perry    Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_setLeds(fbe_object_id_t object_id, 
                                                    fbe_base_object_mgmt_set_enclosure_led_t *enclosureLedInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (enclosureLedInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_LEDS,
                                                enclosureLedInfo,
                                                sizeof (fbe_base_object_mgmt_set_enclosure_led_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!********************************************************************
 * @fn fbe_api_enclosure_resetShutdownTimer(
 *     fbe_object_id_t object_id)
 **********************************************************************
 * @brief
 *  This function set enclosure Shutdown Reason. 
 *
 * @param object_id - The logical drive object id to send request to.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11/July/2012: Zhou Eric   Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_resetShutdownTimer(fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_RESET_ENCLOSURE_SHUTDOWN_TIMER,
                                                0,
                                                0,
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!********************************************************************
 * @fn fbe_api_enclosure_send_encl_debug_control(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_encl_dbg_ctrl_t *enclosureDebugInfo)
 **********************************************************************
 * @brief
 *  This function send ENCLOSURE_DEBUG_CONTROL to enclosure.
 *
 * @param object_id - The object id to send request to.
 * @param enclosureDebugInfo - Pointer to the EnclosureDebugInfo info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  8/12/12: Lin Lou    Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_encl_debug_control(fbe_object_id_t object_id,
                                          fbe_base_object_mgmt_encl_dbg_ctrl_t *enclosureDebugInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (enclosureDebugInfo == NULL)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_ENCLOSURE_DEBUG_CONTROL,
                                                enclosureDebugInfo,
                                                sizeof (fbe_base_object_mgmt_encl_dbg_ctrl_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) ||
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_send_encl_debug_control

/*!********************************************************************
 * @fn fbe_api_enclosure_send_drive_debug_control(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_drv_dbg_ctrl_t *enclosureDriveDebugInfo)
 **********************************************************************
 * @brief
 *  This function send DRIVE_DEBUG_CONTROL to enclosure. 
 *
 * @param object_id - The object id to send request to (the parent object that controls the disk/slot)
 * @param enclosureDriveDebugInfo - pointer to the DriveDebugControl info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/8/08: Joe Perry    Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_drive_debug_control(fbe_object_id_t object_id, 
                                           fbe_base_object_mgmt_drv_dbg_ctrl_t *enclosureDriveDebugInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (enclosureDriveDebugInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_DEBUG_CONTROL,
                                                enclosureDriveDebugInfo,
                                                sizeof (fbe_base_object_mgmt_drv_dbg_ctrl_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_send_drive_debug_control

/*!********************************************************************
 * @fn fbe_api_enclosure_send_drive_power_control(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_drv_power_ctrl_t *drivPowerInfo)
 **********************************************************************
 * @brief
 *  This function send Drive Power Control. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param drivPowerInfo - pointer to the fbe_enclosure eses status page struct
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/8/08: Joe Perry    Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_drive_power_control(fbe_object_id_t object_id, 
                                           fbe_base_object_mgmt_drv_power_ctrl_t *drivPowerInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (drivPowerInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_POWER_CONTROL,
                                                drivPowerInfo,
                                                sizeof (fbe_base_object_mgmt_drv_power_ctrl_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_send_drive_power_control

/*!********************************************************************
 * @fn fbe_api_enclosure_send_expander_control(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_exp_ctrl_t *expanderInfo)
 **********************************************************************
 * @brief
 *  This function send expander control. 
 *
 * @param object_id - The logical drive object id to send request to.
 * @param expanderInfo - pointer to the expander data structure
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11/3/08: Joe Perry    Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_expander_control(fbe_object_id_t object_id, 
                                        fbe_base_object_mgmt_exp_ctrl_t *expanderInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (expanderInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_CONTROL,
                                                expanderInfo,
                                                sizeof (fbe_base_object_mgmt_exp_ctrl_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_send_expander_control

/*!********************************************************************
 * @fn fbe_api_enclosure_send_mgmt_ctrl_op_no_rtn_data(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr,
 *     fbe_base_enclosure_control_code_t control_code)
 **********************************************************************
 * @brief
 *      This function sends generic mgmt eses page.
 *  It does not expect return data.
 *
 * @param object_id - The logical drive object id to send request to.
 * @param esesPageOpPtr - pointer to eses Page Op 
 * @param control_code - control code
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/21/09: Joe Perry    Created
 *    03/05/09: Naizhong Chiu Changed to generic function.
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_mgmt_ctrl_op_no_rtn_data(fbe_object_id_t object_id, 
                                                     fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr,
                                                     fbe_base_enclosure_control_code_t control_code)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;

    if (esesPageOpPtr == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(control_code,
                                                esesPageOpPtr,
                                                sizeof (fbe_enclosure_mgmt_ctrl_op_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        if((status_info.control_operation_status == FBE_PAYLOAD_CONTROL_STATUS_OK) &&
           (status_info.control_operation_qualifier == (fbe_payload_control_status_qualifier_t)FBE_ENCLOSURE_STATUS_BUSY))
        {
            return FBE_STATUS_BUSY;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;

}   // end of fbe_api_enclosure_send_mgmt_ctrl_op_no_rtn_data

/*!********************************************************************
 * @fn fbe_api_enclosure_send_exp_test_mode_control(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr)
 **********************************************************************
 * @brief
 *  This function sends exp test mode control.
 *
 * @param object_id - The logical drive object id to send request to.
 * @param esesPageOpPtr - pointer to eses Page Op 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/21/09: Joe Perry    Created
 *    03/06/09: Naizhong Chiu  Use the new interface function.
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_exp_test_mode_control(fbe_object_id_t object_id, 
                                             fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr)

{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_no_rtn_data(object_id, 
                                    esesPageOpPtr,
                                    FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_TEST_MODE_CONTROL);

    return status;

}   // end of fbe_api_enclosure_send_exp_test_mode_control

/*!********************************************************************
 * @fn fbe_api_enclosure_send_exp_string_out_control(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr)
 **********************************************************************
 * @brief
 *  This function sends exp string out control.
 *
 * @param object_id - The logical drive object id to send request to.
 * @param esesPageOpPtr - pointer to eses Page Op 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/21/09: Joe Perry    Created
 *    03/06/09: Naizhong Chiu  Use the new interface function.
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_exp_string_out_control(fbe_object_id_t object_id, 
                                              fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_no_rtn_data(object_id, 
                        esesPageOpPtr,
                        FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_STRING_OUT_CONTROL);

    return status;

}   // end of fbe_api_enclosure_send_exp_string_out_control

/*!***************************************************************
 * @fn fbe_api_enclosure_update_drive_fault_led(fbe_base_object_t *base_object,
 *                                       fbe_object_id_t parent_object_id,
 *                                       fbe_u32_t slot,
 *                                       fbe_base_enclosure_led_behavior_t  ledBehavior)
 ****************************************************************
 * @brief
 *  This function will send control code to drive slot LED for an enclosure.
 *
 * @param 
 *   base_object - pointer to the enclosure setup info structure.
 *   parent_object_id - Drive Parent object ID
 *   slot - slot Number of Drive
 *   ledBehavior - How to set LED for Drive
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 * 
 * @version
 *  03-Aug-2012     Randall Porteus - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_update_drive_fault_led(fbe_u32_t bus, 
                                         fbe_u32_t enclosure, 
                                         fbe_u32_t slot,
                                         fbe_base_enclosure_led_behavior_t  ledBehavior)
{
    fbe_base_enclosure_led_status_control_t ledCommandInfo;
    fbe_u32_t status;
    fbe_object_id_t           parent_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_enclosure_get_disk_parent_object_id(bus, 
                                                         enclosure, 
                                                         slot,
                                                         &parent_object_id);
    if (status != FBE_STATUS_OK || parent_object_id == FBE_OBJECT_ID_INVALID) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:Fail to get valid object ID :%d, bus:%d, enclosure:%d, slot:%d\n", __FUNCTION__,
                        status, bus, enclosure, slot);
        return status;
    }

    fbe_zero_memory(&ledCommandInfo, sizeof(fbe_base_enclosure_led_status_control_t));
    ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
    ledCommandInfo.driveCount = 1;
    ledCommandInfo.driveSlot = slot;
    ledCommandInfo.ledInfo.enclDriveFaultLeds[slot] = ledBehavior;

    status = fbe_api_enclosure_send_set_led(parent_object_id, &ledCommandInfo);
    
    return status;

}
/*!********************************************************************
 * @fn fbe_api_enclosure_send_set_led(
 *     fbe_object_id_t object_id, 
 *     fbe_base_enclosure_led_status_control_t *ledStatusControlInfo)
 **********************************************************************
 * @brief
 *  This function sends set LED.
 *
 * @param object_id - The logical drive object id to send request to.
 * @param ledStatusControlInfo - pointer to LED Status control info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/29/09: Joe Perry    Created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_set_led(fbe_object_id_t object_id, 
                               fbe_base_enclosure_led_status_control_t *ledStatusControlInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (ledStatusControlInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_LED_STATUS_CONTROL,
                                                ledStatusControlInfo,
                                                sizeof (fbe_base_enclosure_led_status_control_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_send_set_led

/*!********************************************************************
 * @fn fbe_api_enclosure_get_trace_buffer_info(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,
 *     fbe_enclosure_status_t *enclosure_status)
 **********************************************************************
 * @brief
 *  This function gets an information about the enclosure.
 *
 * @param object_id - The logical drive object id to send request to.
 * @param eses_page_op - pointer to eses page op
 * @param enclosure_status - pointer to enclosure status
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/02/09: Arunkumar Selvapillai  created
 *    03/06/09: Naizhong Chiu  Use the new interface function.
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_trace_buffer_info(fbe_object_id_t object_id, 
                                                     fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,
                                                     fbe_enclosure_status_t *enclosure_status)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(object_id, 
                        eses_page_op,
                        FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_TRACE_BUF_INFO_STATUS,
                        enclosure_status);

    return status;
}

/*!********************************************************************
 * @fn fbe_api_enclosure_send_trace_buffer_control(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *eses_page_op)
 **********************************************************************
 * @brief
 *  This function sends trace buffer control.
 *
 * @param object_id - The logical drive object id to send request to.
 * @param eses_page_op - pointer to eses page op
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/02/09: Arunkumar Selvapillai    Created
 *    03/06/09: Naizhong Chiu  Use the new interface function.
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_trace_buffer_control(fbe_object_id_t object_id, 
                                            fbe_enclosure_mgmt_ctrl_op_t *eses_page_op)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_no_rtn_data(object_id, 
                        eses_page_op, 
                        FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_TRACE_BUF_INFO_CTRL);

    return status;
}   // end of fbe_api_enclosure_send_trace_buffer_control


/*!********************************************************************
 * @fn fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(    
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *operation,
 *     fbe_base_enclosure_control_code_t control_code,
 *     fbe_enclosure_status_t *encl_status)
 *********************************************************************
 * @brief:
 *  This function is generic mgmt eses page operation.
 *  It expects return data.
 *
 * @param object_id - The object id to send request to
 * @param operation - pointer to the information passed from the the client
 * @param control_code - specifies the type of read buffer operation
 * @param encl_status - pointer to the enclosure status returned from the operation (out)
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/20/09: GB  created
 *    03/05/09: NC  make it more generic
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(fbe_object_id_t object_id, 
                                           fbe_enclosure_mgmt_ctrl_op_t *operation,
                                           fbe_base_enclosure_control_code_t control_code,
                                           fbe_enclosure_status_t *encl_status)
{
    fbe_sg_element_t                            sg_element[2];
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t                                    sg_item = 0;

#ifdef C4_INTEGRATED
    if (call_from_fbe_cli_user)
    {
        fbe_api_fbe_cli_rdevice_read_buffer_t  rdev_in_buff;
        fbe_eses_read_buf_desc_t               *read_buf_desc_p;
        unsigned char                          *rdev_out_buff_ptr;
        csx_status_e                           csx_status;
        csx_size_t                             bytesReturned = 0;
        rdev_in_buff.object_id = object_id;
        csx_p_memcpy (&(rdev_in_buff.operation), operation,
                         sizeof(fbe_enclosure_mgmt_ctrl_op_t));
        rdev_in_buff.control_code = control_code;

        rdev_out_buff_ptr = csx_p_util_mem_alloc_maybe(sizeof(fbe_api_fbe_cli_rdevice_read_buffer_t) +
                                                    operation->response_buf_size);

        if (rdev_out_buff_ptr == NULL) {
            csx_p_display_std_note("%s: failed to allocate rdevice buffer of size %llu bytes\n",
                                    __FUNCTION__,
				    (unsigned long long)(sizeof(fbe_api_fbe_cli_rdevice_read_buffer_t) +
                                                  operation->response_buf_size));
            return FBE_STATUS_GENERIC_FAILURE;
        }

        csx_status = csx_p_rdevice_ioctl(ppDeviceRef,
                                    FBE_CLI_SEND_PKT_SGL_IOCTL,
                                    &rdev_in_buff,
                                    sizeof(fbe_api_fbe_cli_rdevice_read_buffer_t),
                                    rdev_out_buff_ptr,
                                    sizeof(fbe_api_fbe_cli_rdevice_read_buffer_t) +
                                    operation->response_buf_size,
                                    &bytesReturned);

        if (CSX_FAILURE(csx_status)) {
            csx_p_display_std_note("%s: failed rdevice ioctl to PP status = 0x%x (%s)\n",
                                   __FUNCTION__, csx_status,csx_p_cvt_csx_status_to_string(csx_status));
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            if(0) csx_p_display_std_note("%s: rdevice ioctl to PP succeeded, bytesReturned = %llu...\n",
                                         __FUNCTION__,
					 (unsigned long long)bytesReturned);
            read_buf_desc_p = (fbe_eses_read_buf_desc_t *) (rdev_out_buff_ptr +
                              sizeof(fbe_api_fbe_cli_rdevice_read_buffer_t));
            csx_p_memcpy (operation->response_buf_p, read_buf_desc_p, operation->response_buf_size);
            operation->required_response_buf_size =
                ((fbe_api_fbe_cli_rdevice_read_buffer_t *) rdev_out_buff_ptr)->operation.required_response_buf_size;

            *encl_status = ((fbe_api_fbe_cli_rdevice_read_buffer_t *) rdev_out_buff_ptr)->enclosure_status;
            status = FBE_STATUS_OK;
        }

        csx_p_util_mem_free(rdev_out_buff_ptr);
        return status;
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity - C4BUG - IOCTL model mixing */

    EmcpalZeroVariable(sg_element);
    if (operation == NULL)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ( operation->response_buf_p == NULL )
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Response buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // set next sg element size and address for the response buffer
    fbe_sg_element_init(&sg_element[sg_item], 
                        operation->response_buf_size, 
                        operation->response_buf_p);

    sg_item++;

    // no more elements
    fbe_sg_element_terminate(&sg_element[sg_item]);
    sg_item++;

    status = fbe_api_common_send_control_packet_with_sg_list(control_code,
                                                             operation,
                                                             sizeof (fbe_enclosure_mgmt_ctrl_op_t), //in
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_element,        // pointer to sg list
                                                             sg_item,           // number of items on the list
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);
   
   /* To compensate for possible corruption when sending from user to kernel, 
    * we restore the original pointers.
    * In the case of user space to kernel space, 
    * the response_buf_p would be overwritten by the kernel space address 
    * in fbe_eses_enclosure_repack_ctrl_op,
    * we need to copy back the user space address here. 
    */
    operation->response_buf_p = sg_element[0].address;

    // update the enclosure status, can be used by the client
    *encl_status = status_info.control_operation_qualifier;

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
       fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                        "%s: status %d, pkt qual %d op status %d, op qual %d\n", 
                        __FUNCTION__,
                        status, 
                        status_info.packet_qualifier, 
                        status_info.control_operation_status, 
                        status_info.control_operation_qualifier);

       if((status_info.control_operation_status == FBE_PAYLOAD_CONTROL_STATUS_OK) &&
          (status_info.control_operation_qualifier == (fbe_payload_control_status_qualifier_t)FBE_ENCLOSURE_STATUS_BUSY))
       {
           return FBE_STATUS_BUSY;
       }
       else
       {
           return FBE_STATUS_GENERIC_FAILURE;
       }
    }

    return status;
} // end fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data

/*!********************************************************************
 * @fn fbe_api_enclosure_send_raw_rcv_diag_pg(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *operation,
 *     fbe_enclosure_status_t *enclosure_status)
 *********************************************************************
 * @brief:
 *  This function sends pass-through receive diag page directly to the object.
 *
 * @param object_id - The object id to send request to
 * @param operation - pointer to the information passed from the the client
 * @param enclosure_status - pointer to the enclosure status
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    03/09/09: NC  created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_send_raw_rcv_diag_pg(fbe_object_id_t object_id, 
                                                    fbe_enclosure_mgmt_ctrl_op_t *operation,
                                                    fbe_enclosure_status_t *enclosure_status)
{

    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(object_id, 
                                                   operation,
                                                   FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_RCV_DIAG,
                                                   enclosure_status);

    return status;
} // end fbe_api_enclosure_send_raw_rcv_diag_pg

/*!********************************************************************
 * @fn fbe_api_enclosure_send_raw_inquiry_pg(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *operation,
 *     fbe_enclosure_status_t *enclosure_status)
 *********************************************************************
 * @brief:
 *  This function sends Inquiry page directly to the object.
 *
 * @param object_id - The object id to send request to
 * @param operation - pointer to the information passed from the the client
 * @param enclosure_status - pointer to the enclosure status
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    25/03/09: AS  created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_send_raw_inquiry_pg(fbe_object_id_t object_id, 
                                                    fbe_enclosure_mgmt_ctrl_op_t *operation,
                                                    fbe_enclosure_status_t *enclosure_status)
{

    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(object_id, 
                                                   operation,
                                                   FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_INQUIRY,
                                                   enclosure_status);

    return status;
} // end fbe_api_enclosure_send_raw_inquiry_pg

/*!*******************************************************************
 * @fn fbe_api_enclosure_resume_read(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *operation,
 *     fbe_enclosure_status_t *enclosure_status)
 *********************************************************************
 * @brief:
 *  This function resumes prom read for read buffer command.
 *
 * @param object_id - The object id to send request to
 * @param operation - pointer to the information passed from the the client
 * @param enclosure_status - pointer to the enclosure status
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    01/12/09: GB  created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_resume_read(fbe_object_id_t object_id, 
                                           fbe_enclosure_mgmt_ctrl_op_t *operation,
                                           fbe_enclosure_status_t *enclosure_status)
{

    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(object_id, 
                                                   operation,
                                                   FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_READ,
                                                   enclosure_status);

    return status;
} // end fbe_api_enclosure_resume_read

/*********************************************************************
 * @fn fbe_api_enclosure_resume_read_synch()
 *********************************************************************
 * @brief:
 *      Sync Resume prom read.
 *
 * @param object_id - The object id to send request to
 * @param pGetResumeProm - pointer to fbe_resume_prom_cmd_t
 *
 * @return fbe_status_t, success or failure
 *
 * @version:
 *    19-Jul-2011: PHE - created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_resume_read_synch(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_cmd_t *pReadResumeProm)                                           
{
    fbe_enclosure_mgmt_ctrl_op_t    operation = {0};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;  
    fbe_sg_element_t                sg_element[2];  
    fbe_u32_t                       sg_item = 0;
    fbe_api_control_operation_status_info_t     status_info = {0};

    if (pReadResumeProm == NULL || object_id >= FBE_OBJECT_ID_INVALID) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pGetResumeProm buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }  
 
    operation.response_buf_p = pReadResumeProm->pBuffer;
    operation.response_buf_size = pReadResumeProm->bufferSize;
    operation.cmd_buf.read_buf.buf_offset = pReadResumeProm->offset;
    operation.cmd_buf.resume_read.deviceType = pReadResumeProm->deviceType;
    operation.cmd_buf.resume_read.deviceLocation = pReadResumeProm->deviceLocation;


    // set next sg element size and address for the response buffer
    fbe_sg_element_init(&sg_element[sg_item], 
                        operation.response_buf_size, 
                        operation.response_buf_p);
    sg_item++;

    // no more elements
    fbe_sg_element_terminate(&sg_element[sg_item]);
    sg_item++;

    status = fbe_api_common_send_control_packet_with_sg_list(FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_READ,
                                                                   &operation,
                                                                   sizeof(fbe_enclosure_mgmt_ctrl_op_t),                                           
                                                                   object_id, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   sg_element, 
                                                                   sg_item,
                                                                   &status_info,
                                                                   FBE_PACKAGE_ID_PHYSICAL);


    /* Copy the status */
    pReadResumeProm->readOpStatus = fbe_api_enclosure_get_resume_status(status,
                                                                        status_info.control_operation_status,
                                                                        status_info.control_operation_qualifier);

    if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
    
} // end fbe_api_enclosure_resume_read_synch

/*********************************************************************
 * @fn fbe_api_enclosure_resume_write_synch()
 *********************************************************************
 * @brief:
 *      Sync Resume prom write.
 *
 * @param object_id - The object id to send request to
 * @param pWriteResumeProm - pointer to fbe_resume_prom_cmd_t
 *
 * @return fbe_status_t, success or failure
 *
 * @version:
 *    19-Jul-2011: PHE - created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_resume_write_synch(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_cmd_t * pWriteResumeProm)                                           
{
    fbe_enclosure_mgmt_ctrl_op_t    operation = {0};
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;  
    fbe_sg_element_t                sg_element[2];  
    fbe_u32_t                       sg_item = 0;
    fbe_api_control_operation_status_info_t     status_info = {0};

    if (pWriteResumeProm == NULL || object_id >= FBE_OBJECT_ID_INVALID) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pGetResumeProm buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }  
 
    operation.cmd_buf.resume_write.data_buf_p = pWriteResumeProm->pBuffer;
    operation.cmd_buf.resume_write.data_buf_size = pWriteResumeProm->bufferSize;
    operation.cmd_buf.resume_write.data_offset = pWriteResumeProm->offset;
    operation.cmd_buf.resume_write.deviceType = pWriteResumeProm->deviceType;
    operation.cmd_buf.resume_write.deviceLocation = pWriteResumeProm->deviceLocation;
    operation.response_buf_p = NULL;
    operation.response_buf_size = 0;

    // set next sg element size and address for the response buffer
    fbe_sg_element_init(&sg_element[sg_item], 
                        operation.cmd_buf.resume_write.data_buf_size, 
                        operation.cmd_buf.resume_write.data_buf_p);
    sg_item++;

    // no more elements
    fbe_sg_element_terminate(&sg_element[sg_item]);
    sg_item++;

    status = fbe_api_common_send_control_packet_with_sg_list(FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_WRITE,
                                                                   &operation,
                                                                   sizeof(fbe_enclosure_mgmt_ctrl_op_t),                                           
                                                                   object_id, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   sg_element, 
                                                                   sg_item,
                                                                   &status_info,
                                                                   FBE_PACKAGE_ID_PHYSICAL);


    if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        if(status_info.control_operation_qualifier == (fbe_payload_control_status_qualifier_t)FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND)
        {
            return FBE_STATUS_COMPONENT_NOT_FOUND;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
    
} // end fbe_api_enclosure_resume_write_synch

/*********************************************************************
 * @fn fbe_api_enclosure_resume_read_asynch()
 *********************************************************************
 * @brief:
 *      Async Resume prom read for read buffer command.
 *
 *  @param object_id - The object id to send request to
 *  @param read_resume_op - pointer to the information passed from the the client
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *    8-Feb-2010: Dipak Patel  created
 *    23-Dec-2010: Arun S - Modified for ESP resume read.
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_resume_read_asynch(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_get_resume_async_t *pGetResumeProm)                                           
{
    fbe_enclosure_mgmt_resume_read_asynch_t *read_resume_op;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;    
    fbe_u32_t           sg_item = 0;
	fbe_packet_t *		packet = NULL;

    if (pGetResumeProm == NULL || object_id >= FBE_OBJECT_ID_INVALID) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pGetResumeProm buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    read_resume_op = (fbe_enclosure_mgmt_resume_read_asynch_t *) fbe_api_allocate_memory (sizeof (fbe_enclosure_mgmt_resume_read_asynch_t));

    if (read_resume_op == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: read_resume_op buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    read_resume_op->operation.response_buf_p = pGetResumeProm->resumeReadCmd.pBuffer;
    read_resume_op->operation.response_buf_size = pGetResumeProm->resumeReadCmd.bufferSize;
    read_resume_op->operation.cmd_buf.read_buf.buf_offset = pGetResumeProm->resumeReadCmd.offset;
    read_resume_op->operation.cmd_buf.resume_read.deviceType = pGetResumeProm->resumeReadCmd.deviceType;
    read_resume_op->operation.cmd_buf.resume_read.deviceLocation = pGetResumeProm->resumeReadCmd.deviceLocation;

    read_resume_op->pGetResumeProm = pGetResumeProm;

    // set next sg element size and address for the response buffer
    fbe_sg_element_init(&read_resume_op->sg_element[sg_item], 
                        read_resume_op->operation.response_buf_size, 
                        read_resume_op->operation.response_buf_p);
    sg_item++;

    // no more elements
    fbe_sg_element_terminate(&read_resume_op->sg_element[sg_item]);
    sg_item++;

	packet = fbe_api_get_contiguous_packet();/*no need to touch or initialize, it's taken from a pool and taken care of*/
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        fbe_api_free_memory(read_resume_op);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet_with_sg_list_async(packet,
																   FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_READ,
                                                                   &read_resume_op->operation,
                                                                   sizeof(fbe_enclosure_mgmt_ctrl_op_t),                                           
                                                                   object_id, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   read_resume_op->sg_element, 
                                                                   sg_item,
                                                                   fbe_api_enclosure_resume_read_asynch_callback, 
                                                                   read_resume_op,
                                                                   FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY && status != FBE_STATUS_NO_OBJECT){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
    }
    
    return status;
} // end fbe_api_enclosure_resume_read_asynch

/*********************************************************************
 * @fn fbe_api_enclosure_resume_read_asynch_callback()
 *********************************************************************
 * @brief:
 *      callback function for async Resume prom read for read buffer command.
 *
 *  @param packet - pointer to fbe_packet_t.
 *  @param context - packet completion context
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *    8-Feb-2010: Dipak Patel  created
 *********************************************************************/
static fbe_status_t fbe_api_enclosure_resume_read_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_enclosure_mgmt_resume_read_asynch_t *read_resume_op = (fbe_enclosure_mgmt_resume_read_asynch_t *)context;
    fbe_resume_prom_get_resume_async_t *pGetResumeProm = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;

    pGetResumeProm = read_resume_op->pGetResumeProm;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    /* Copy the status */
    pGetResumeProm->resumeReadCmd.readOpStatus = fbe_api_enclosure_get_resume_status(status,
                                                                                     control_status,
                                                                                     control_status_qualifier);

    /* Copy back the data from the PP to response buffer */
    read_resume_op->operation.response_buf_p = read_resume_op->sg_element[0].address;
    
    /* Since this is a async call, we cannot assume that the status will be always OK. 
     * It could also be BUSY. If this is something other than OK, print out the ktrace.
     */    
    if ((status != FBE_STATUS_OK) ||
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
        (control_status_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:status:%d, payload status:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, control_status, control_status_qualifier);

        status = FBE_STATUS_GENERIC_FAILURE;
    }   
    /* read_resume_op was allocated by fbe_api_allocate_memory in function 
     * fbe_api_enclosure_resume_read_asynch 
     * It needs to be freed here.
     */       
    fbe_api_free_memory(read_resume_op);

    fbe_api_return_contiguous_packet(packet);/*no need to destroy of free, it's part of a queue and taken care off*/

    /* call callback function */
    (pGetResumeProm->completion_function) (pGetResumeProm->completion_context, pGetResumeProm);

    return status;
}

/*********************************************************************
 * @fn fbe_api_enclosure_get_statistics()
 *********************************************************************
 * @brief:
 *  Get enclosure statistics.
 *
 *  @param object_id - The object id to send request (in)
 *  @param operation - pointer to the information passed from the the client(in)
 *  @param enclosure_status - pointer to the enclosure status (out)
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *    04/28/09: GB  created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_statistics(fbe_object_id_t object_id, 
                                           fbe_enclosure_mgmt_ctrl_op_t *operation,
                                           fbe_enclosure_status_t *enclosure_status)
{

    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(object_id, 
                                                   operation,
                                                   FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_STATISTICS,
                                                   enclosure_status);

    return status;
} // end fbe_api_enclosure_get_statistics

/*!*******************************************************************
 * @fn fbe_api_enclosure_get_threshold_in(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *operation,
 *     fbe_enclosure_status_t *enclosure_status)
 *********************************************************************
 * @brief:
 *  This function gets enclosure threshold 
 *
 *  @param object_id - The object id to send request (in)
 *  @param operation - pointer to the information passed from the the client(in)
 *  @param enclosure_status - pointer to the enclosure status (out)
 *
 *  @return fbe_status_t, success or failure
 *
 * @version
 *    17- Sep-2009: Prasenjeet Ghaywat  created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_threshold_in(fbe_object_id_t object_id, 
                                           fbe_enclosure_mgmt_ctrl_op_t *operation,
                                           fbe_enclosure_status_t *enclosure_status)
{

    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(object_id, 
                                                   operation,
                                                   FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_THRESHOLD_IN_CONTROL,
                                                   enclosure_status);
    return status;
} // end fbe_api_enclosure_get_threshold_in

/*!***************************************************************
 * @fn fbe_api_enclosure_build_statistics_cmd(
 *     fbe_enclosure_mgmt_ctrl_op_t * op_p, 
 *     fbe_enclosure_element_type_t element_type,  
 *     fbe_enclosure_slot_number_t start_slot,
 *     fbe_enclosure_slot_number_t end_slot,
 *                                  fbe_u8_t * response_buf_p, 
 *                                  fbe_u32_t response_buf_size)
 ****************************************************************
 * @brief
 *  Fill in details for the get statistics command (mgmt_ctrl and eses_ctrl).
 *
 * @param  op_p - The pointer to the management control operation.
 * @param  element_type - The element type to fill in.
 * @param  start_slot - The first slot number to fill in.
 * @param  end_slot - The last slot number to fill in.
 * @param  response_bufp - The pointer to the response buffer.
 * @param  response_buf_size - The size of the responser buffer.
 *
 * @return  fbe_status_t - Always return FBE_STATUS_OK
 * 
 * @version
 *  28-Apr-2009 GB - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_build_statistics_cmd(fbe_enclosure_mgmt_ctrl_op_t * op_p,
                                                    fbe_enclosure_element_type_t element_type,
                                                    fbe_enclosure_slot_number_t start_slot,
                                                    fbe_enclosure_slot_number_t end_slot,
                                                    fbe_u8_t * response_bufp,
                                                    fbe_u32_t response_buf_size)
{
    fbe_enclosure_mgmt_statistics_cmd_t *stats_cmdp = NULL;

 
    
    fbe_zero_memory(op_p, sizeof(fbe_enclosure_mgmt_ctrl_op_t));

    if(response_bufp != NULL)
    {
        fbe_zero_memory(response_bufp, response_buf_size);
    }

    op_p->response_buf_p = response_bufp;
    op_p->response_buf_size = response_buf_size;

    stats_cmdp = &(op_p->cmd_buf.element_statistics);
    stats_cmdp->type = element_type;
    stats_cmdp->first = start_slot;
    stats_cmdp->last = end_slot;  

    return FBE_STATUS_OK;
} // end fbe_api_enclosure_build_statistics_cmd

/*!***************************************************************
 * @fn fbe_api_enclosure_build_threshold_in_cmd(
 *     fbe_enclosure_mgmt_ctrl_op_t * op_p,
 *     fbe_enclosure_component_types_t componentType,
 *     fbe_u8_t * response_bufp, 
 *                                  fbe_u32_t response_buf_size)
 ****************************************************************
 * @brief
 *  Fill in details for the get threshold command (mgmt_ctrl and eses_ctrl).
 *
 * @param  op_p - The pointer to the management control operation.
 * @param  componentType - The component type to fill in.
 * @param  response_bufp - The pointer to the response buffer.
 * @param  response_buf_size - The size of the responser buffer.
 *
 * @return  fbe_status_t - Always return FBE_STATUS_OK
 *
 * @version
 *  17-Sep-2009 Prasenjeet - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_build_threshold_in_cmd(fbe_enclosure_mgmt_ctrl_op_t * op_p,
                                                    fbe_enclosure_component_types_t componentType,
                                                    fbe_u8_t * response_bufp,
                                                    fbe_u32_t response_buf_size)
{
    fbe_enclosure_mgmt_threshold_in_cmd_t *thresh_cmdp = NULL;
    fbe_zero_memory(op_p, sizeof(fbe_enclosure_mgmt_ctrl_op_t));

    if (response_bufp == NULL)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   else
    {
        fbe_zero_memory(response_bufp, response_buf_size);
    }
    op_p->response_buf_p = response_bufp;
    op_p->response_buf_size = response_buf_size;
    thresh_cmdp = &(op_p->cmd_buf.threshold_in);
    //thresh_cmdp->elem_type = element_type;
    thresh_cmdp->componentType = componentType;

    if ( op_p->response_buf_p == NULL )
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Response buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
} // end fbe_api_enclosure_build_threshold_in_cmd

/*!********************************************************************
 * @fn fbe_api_enclosure_read_buffer(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *operation,
 *     fbe_enclosure_status_t *enclosure_status)
 *********************************************************************
 * @brief:
 *  This function directs read buffer command.
 *
 *  @param object_id - The object id to send request (in)
 *  @param operation - pointer to the information passed from the the client(in)
 *  @param enclosure_status - pointer to the enclosure status (out)
 *
 *  @return fbe_status_t, success or failure
 *
 * @version
 *    01/20/09: GB  created
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_read_buffer(fbe_object_id_t object_id, 
                                           fbe_enclosure_mgmt_ctrl_op_t *operation,
                                           fbe_enclosure_status_t *enclosure_status)
{

    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(object_id, 
                                                   operation,
                                                   FBE_BASE_ENCLOSURE_CONTROL_CODE_READ_BUF,
                                                   enclosure_status);

    return status;
} // end fbe_api_enclosure_read_buffer

/*!***************************************************************
 * @fn fbe_api_enclosure_build_read_buf_cmd(
 *     fbe_enclosure_mgmt_ctrl_op_t * eses_page_op_p, 
 *                          fbe_u8_t buf_id, 
 *                          fbe_eses_read_buf_mode_t mode,
 *                          fbe_u32_t buf_offset,
 *                          fbe_u8_t * response_buf_p, 
 *                          fbe_u32_t response_buf_size)
 ****************************************************************
 * @brief
 *  Function to build the read buffer command.
 *
 * @param  eses_page_op_p - The pointer to the eses_page_op.
 * @param  buf_id - The buffer id.
 * @param  mode - read buffer mode.
 * @param  buf_offset - The read starting point. 
 * @param  response_buf_p - The pointer to the response buffer.
 * @param  response_buf_size - The size of the responser buffer.
 *
 * @return  fbe_status_t - Always returns FBE_STATUS_OK.
 *
 * @version
 *  03-Feb-2009 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_build_read_buf_cmd(fbe_enclosure_mgmt_ctrl_op_t * eses_page_op_p, 
                           fbe_u8_t buf_id, 
                           fbe_eses_read_buf_mode_t mode,
                           fbe_u32_t buf_offset,
                           fbe_u8_t * response_buf_p, 
                           fbe_u32_t response_buf_size)
{
    fbe_enclosure_mgmt_read_buf_cmd_t * read_buf_cmd_p = NULL;

    fbe_zero_memory(response_buf_p, response_buf_size);
    fbe_zero_memory(eses_page_op_p, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
    eses_page_op_p->response_buf_p = response_buf_p;
    eses_page_op_p->response_buf_size = response_buf_size;

    read_buf_cmd_p = &(eses_page_op_p->cmd_buf.read_buf);
    read_buf_cmd_p->mode = mode;
    read_buf_cmd_p->buf_id = buf_id;
    read_buf_cmd_p->buf_offset = buf_offset;   

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_build_trace_buf_cmd(
 *    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op_p, 
 *                                  fbe_u8_t buf_id,  
 *                                  fbe_enclosure_mgmt_trace_buf_op_t buf_op,
 *                                  fbe_u8_t * response_buf_p, 
 *                                  fbe_u32_t response_buf_size)
 ****************************************************************
 * @brief
 *  Function to build the trace  buffer command.
 *
 * @param  eses_page_op_p - The pointer to the eses_page_op.
 * @param  buf_id - The buffer id.
 * @param  buf_op - The trace buffer op.
 * @param  response_buf_p - The pointer to the response buffer.
 * @param  response_buf_size - The size of the responser buffer.
 *
 * @return  fbe_status_t - Always return FBE_STATUS_OK
 *
 * @version
 *  03-Feb-2009 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_build_trace_buf_cmd(fbe_enclosure_mgmt_ctrl_op_t * eses_page_op_p, 
                                    fbe_u8_t buf_id,  
                                    fbe_enclosure_mgmt_trace_buf_op_t buf_op,
                                    fbe_u8_t * response_buf_p, 
                                    fbe_u32_t response_buf_size)
{
    fbe_enclosure_mgmt_trace_buf_cmd_t * trace_buf_cmd_p = NULL;

    fbe_zero_memory(eses_page_op_p, sizeof(fbe_enclosure_mgmt_ctrl_op_t));
    if(response_buf_p != NULL)
    {
        fbe_zero_memory(response_buf_p, response_buf_size);
    }

    eses_page_op_p->response_buf_p = response_buf_p;
    eses_page_op_p->response_buf_size = response_buf_size;

    trace_buf_cmd_p = &(eses_page_op_p->cmd_buf.trace_buf_cmd);
    trace_buf_cmd_p->buf_id = buf_id;
    trace_buf_cmd_p->buf_op = buf_op;  

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_build_slot_to_phy_mapping_cmd(
 *     fbe_enclosure_mgmt_ctrl_op_t * ctrl_op_p, 
 *     fbe_enclosure_slot_number_t slot_num_start,  
 *     fbe_enclosure_slot_number_t slot_num_end,
 *                                  fbe_u8_t * response_buf_p, 
 *                                  fbe_u32_t response_buf_size)
 ****************************************************************
 * @brief
 *  Function to build the slot to phy mapping command.
 *
 * @param  ctrl_op_p - The pointer to the fbe_enclosure_mgmt_ctrl_op_t.
 * @param  slot_num_start - The first drive slot number to get the phy mapping for.
 * @param  slot_num_end - The last drive slot number to get the phy mapping for.
 * @param  response_buf_p - The pointer to the response buffer.
 * @param  response_buf_size - The size of the responser buffer.
 *
 * @return  fbe_status_t - Always return FBE_STATUS_OK
 *
 * @version
 *  18-Mar-2009 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_build_slot_to_phy_mapping_cmd(fbe_enclosure_mgmt_ctrl_op_t * ctrl_op_p, 
                                    fbe_enclosure_slot_number_t slot_num_start, 
                                    fbe_enclosure_slot_number_t slot_num_end,
                                    fbe_u8_t * response_buf_p, 
                                    fbe_u32_t response_buf_size)
{
    fbe_enclosure_mgmt_slot_to_phy_mapping_cmd_t * slot_to_phy_mapping_cmd_p = NULL;

    fbe_zero_memory(ctrl_op_p, sizeof(fbe_enclosure_mgmt_ctrl_op_t));

    if(response_buf_p != NULL)
    {
        fbe_zero_memory(response_buf_p, response_buf_size);
    }

    ctrl_op_p->response_buf_p = response_buf_p;
    ctrl_op_p->response_buf_size = response_buf_size;

    slot_to_phy_mapping_cmd_p = &(ctrl_op_p->cmd_buf.slot_to_phy_mapping_cmd);
    slot_to_phy_mapping_cmd_p->slot_num_start = slot_num_start;
    slot_to_phy_mapping_cmd_p->slot_num_end = slot_num_end;  

    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @fn fbe_api_enclosure_send_resume_write(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *eses_page_op)
 *********************************************************************
 * @brief:
 *  This function sends resume prom write command.
 *
 * @param object_id - The object id to send request to
 * @param eses_page_op - pointer to the eses page operation
 *
 * @return fbe_status_t, success or failure
 *
 * @version:
 *    02/02/09: Arunkumar Selvapillai    Created
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_send_resume_write(fbe_object_id_t object_id, 
                                    fbe_enclosure_mgmt_ctrl_op_t *eses_page_op)
{
    fbe_u8_t                                    sg_count = 0;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sg_element_t                            sg_element[2];

    EmcpalZeroVariable(sg_element);
    if (eses_page_op == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ( eses_page_op->cmd_buf.resume_write.data_buf_p == NULL )
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Response buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the element size and address for the data buffer */
    fbe_sg_element_init(&sg_element[sg_count], 
                        eses_page_op->cmd_buf.resume_write.data_buf_size, 
                        eses_page_op->cmd_buf.resume_write.data_buf_p);
    sg_count++;

    /* No more elements. Terminate it. */
    fbe_sg_element_terminate(&sg_element[sg_count]);
    sg_count++;

    status = fbe_api_common_send_control_packet_with_sg_list(FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_WRITE,
                                                             eses_page_op,
                                                             sizeof (fbe_enclosure_mgmt_ctrl_op_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_element,
                                                             sg_count,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

   /* To compensate for possible corruption when sending from user to kernel, 
    * we restore the original pointers.
    * In the case of user space to kernel space, 
    * the data_buf_p would be overwritten by the kernel space address 
    * in fbe_eses_enclosure_repack_ctrl_op,
    * we need to copy back the user space address here in case the call needs to retrieve it. 
    */
    eses_page_op->cmd_buf.resume_write.data_buf_p = sg_element[0].address;

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

   return status;
}   // end of fbe_api_enclosure_send_resume_write

/*!*******************************************************************
 * @fn fbe_api_enclosure_get_dl_info(
 *     fbe_object_id_t object_id, 
 *     fbe_eses_encl_fup_info_t *dl_info, 
 *     fbe_u32_t num_bytes) 
 *********************************************************************
 * @brief:
 *  This function gets enclosure elem group info.
 *
 * @param object_id - The object id to send request to
 * @param dl_info - pointer to the eses enclosure FUP info
 * @param num_bytes - max no of element groups
 *  
 * @return fbe_status_t, success or failure
 *
 * @version
 *    10/8/08: Dipak Patel  Created
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_dl_info(fbe_object_id_t object_id, 
                                                        fbe_eses_encl_fup_info_t *dl_info, 
                                                        fbe_u32_t num_bytes)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (dl_info == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DL_INFO,
                                                dl_info,
                                                num_bytes,
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_enclosure_get_eir_info(
 *     fbe_object_id_t object_id, 
 *     fbe_eses_encl_fup_info_t *dl_info, 
 *     fbe_u32_t num_bytes) 
 *********************************************************************
 * @brief:
 *  This function gets enclosure elem group info.
 *
 * @param object_id - The object id to send request to
 * @param dl_info - pointer to the eses enclosure FUP info
 * @param num_bytes - max no of element groups
 *  
 * @return fbe_status_t, success or failure
 *
 * @version
 *    10/8/08: Dipak Patel  Created
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_eir_info(fbe_object_id_t object_id, 
                                                         fbe_eses_encl_eir_info_t *eir_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (eir_info == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_EIR_INFO,
                                                eir_info,
                                                sizeof(fbe_eses_encl_eir_info_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}   // end of fbe_api_enclosure_get_eir_info

/*!********************************************************************
 * @fn fbe_api_enclosure_get_slot_to_phy_mapping(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_mgmt_ctrl_op_t *ctrl_op,
 *     fbe_enclosure_status_t *enclosure_status)
 *********************************************************************
 * @brief
 *  This function gets the slot to phy mapping for a group of drive slots.
 *
 * @param  object_id - The enclosure object id.
 * @param  ctrl_op - The pointer to the slot to phy mapping control operation.
 * @param  enclosure_status - pointer to the enclosure status
 *
 * @return fbe_status_t - FBE_STATUS_OK - no error.
 *      otherwise - error found.
 *
 * @version
 *    18-Mar-2009: PHE - Created.
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_slot_to_phy_mapping(fbe_object_id_t object_id, 
                                                     fbe_enclosure_mgmt_ctrl_op_t *ctrl_op,
                                                     fbe_enclosure_status_t *enclosure_status)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(object_id, 
                        ctrl_op,
                        FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_TO_PHY_MAPPING,
                        enclosure_status);

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_enclosure_get_fault_info(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_fault_t *enclosure_fault_info)
 *********************************************************************
 * @brief
 *  This function gets the slot to phy mapping for a group of drive slots.
 *
 * @param  object_id - The enclosure object id to send request to
 * @param  enclosure_fault_info    - Information passed from the enclosure
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    04/27/09: Nayana Chaudhari  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_fault_info(fbe_object_id_t object_id, 
                                                           fbe_enclosure_fault_t *enclosure_fault_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;;

    if (enclosure_fault_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Enclosure fault info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_FAULT_INFO,
                                                 enclosure_fault_info,
                                                 sizeof (fbe_enclosure_fault_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!*******************************************************************
 * @fn fbe_api_enclosure_get_scsi_error_info(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_scsi_error_info_t *scsi_error_info)
 *********************************************************************
 * @brief
 *  This function gets scsi error information about the enclosure.
 *
 * @param  object_id - The enclosure object id to send request to
 * @param  scsi_error_info    - Information passed from the enclosure
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/14/09: Nayana Chaudhari  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_scsi_error_info(fbe_object_id_t object_id, 
                                                   fbe_enclosure_scsi_error_info_t *scsi_error_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;;

    if (scsi_error_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: scsi error info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SCSI_ERROR_INFO,
                                                 scsi_error_info,
                                                 sizeof (fbe_enclosure_scsi_error_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}
/*********************************************************************
 *            fbe_api_enclosure_send_exp_threshold_out_control()
 *********************************************************************
 *  Description: 
 *
 *  Inputs: object_handle_p
 *             pointer to fbe_enclosure_mgmt_ctrl_op_t struct
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/09/09: Dipak Patel    Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_api_enclosure_send_exp_threshold_out_control(fbe_object_id_t object_id, 
                                              fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr)
                                              
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_enclosure_send_mgmt_ctrl_op_no_rtn_data(object_id, 
                        esesPageOpPtr,
                        FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_THRESHOLD_OUT_CONTROL);

    return status;    

}   // end of fbe_api_enclosure_send_exp_threshold_out_control()

/*!*******************************************************************
 * @fn fbe_api_get_enclosure_object_id_by_location(
 *     fbe_u32_t port, 
 *     fbe_u32_t enclosure, 
 *     fbe_object_id_t *object_id )
 *********************************************************************
 * @brief
 *  This function returns the object id of the enclosure based on it's 
 *  topology location.
 *
 * @param  port - port number
 * @param  enclosure - Information passed from the enclosure
 * @param  object_id - pointer to the object ID
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    07/21/09: Nayana Chaudhari  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_enclosure_object_id_by_location(fbe_u32_t port, 
                                                                      fbe_u32_t enclosure, 
                                                                      fbe_object_id_t *object_id)
{
    fbe_topology_control_get_enclosure_by_location_t    topology_control_get_enclosure_by_location;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t             status_info;

    if (object_id == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_control_get_enclosure_by_location.port_number = port;
    topology_control_get_enclosure_by_location.enclosure_number = enclosure;
    topology_control_get_enclosure_by_location.enclosure_object_id = FBE_OBJECT_ID_INVALID;

     status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_ENCLOSURE_BY_LOCATION,
                                                             &topology_control_get_enclosure_by_location,
                                                             sizeof(fbe_topology_control_get_enclosure_by_location_t),
                                                             FBE_SERVICE_ID_TOPOLOGY,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

      if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
         if ((status_info.control_operation_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) && 
             (status_info.control_operation_qualifier == FBE_OBJECT_ID_INVALID)) {
             /* For Voyager, control_operation_status FBE_PAYLOAD_CONTROL_STATUS_FAILURE could be caused by not being able to 
              * get the edge expander object id. We can not update the ICM object id to FBE_OBJECT_ID_INVALID. 
              * Just get what it returns.
              */
             *object_id = topology_control_get_enclosure_by_location.enclosure_object_id;
             return FBE_STATUS_OK;
         }

        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
     }

     *object_id = topology_control_get_enclosure_by_location.enclosure_object_id;
     return status;

}

/*!*******************************************************************
 * @fn fbe_api_get_enclosure_object_ids_array_by_location(
 *     fbe_u32_t port, 
 *     fbe_u32_t enclosure, 
 *     fbe_topology_control_get_enclosure_by_location_t *enclosure_by_location)
 *********************************************************************
 * @brief
 *  This function returns the object id of the enclosure based on it's 
 *  topology location.
 *
 * @param  port - port number
 * @param  enclosure - Information passed from the enclosure
 * @param  enclosure_by_location - pointer to the array of object ID
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    07/21/11: Rashmi Sawale  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_enclosure_object_ids_array_by_location(fbe_u32_t port, 
                                                                      fbe_u32_t enclosure, 
                                                                      fbe_topology_control_get_enclosure_by_location_t *enclosure_by_location)
{
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t             status_info;

    if (enclosure_by_location == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    enclosure_by_location->port_number = port;
    enclosure_by_location->enclosure_number = enclosure;
    enclosure_by_location->enclosure_object_id = FBE_OBJECT_ID_INVALID;

     status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_ENCLOSURE_BY_LOCATION,
                                                             enclosure_by_location,
                                                             sizeof(fbe_topology_control_get_enclosure_by_location_t),
                                                             FBE_SERVICE_ID_TOPOLOGY,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

      if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
         if ((status_info.control_operation_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) && 
             (status_info.control_operation_qualifier == FBE_OBJECT_ID_INVALID)) {
             return FBE_STATUS_OK;
         }

        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
     }
	  
     return status;
}


/*********************************************************************
 *            fbe_api_enclosure_send_ps_margin_test_control()
 *********************************************************************
 *  Description: 
 *
 *  Inputs: object_handle_p
 *          fbe_enclosure eses status page struct
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/2/09: Joe Perry    Created
 *    
 *********************************************************************/
fbe_status_t 
fbe_api_enclosure_send_ps_margin_test_control(fbe_object_id_t object_id, 
                                              fbe_base_enclosure_ps_margin_test_ctrl_t *psMarginTestInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (psMarginTestInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_PS_MARGIN_TEST_CONTROL,
                                                psMarginTestInfo,
                                                sizeof (fbe_base_enclosure_ps_margin_test_ctrl_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_send_ps_margin_test_control

/*!********************************************************************
 * @fn fbe_api_enclosure_getPsInfo(
 *     fbe_object_id_t object_id, 
 *     fbe_base_object_mgmt_set_enclosure_control_t *enclosureControlInfo)
 **********************************************************************
 * @brief
 *  This function gets enclosure Power Supply info. 
 *
 * @param object_id - The enclosure object id to send request to.
 * @param enclosureControlInfo - pointer to the fbe_enclosure PS Info struct
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  29/04/10: Joe Perry    Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_getPsInfo(fbe_object_id_t object_id, 
                                                      fbe_power_supply_info_t *enclosurePsInfo)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;

    if (enclosurePsInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_INFO,
                                                enclosurePsInfo,
                                                sizeof (fbe_power_supply_info_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!********************************************************************
 * @fn fbe_api_enclosure_parse_image_header(fbe_object_id_t object_id, 
 *                                          fbe_enclosure_mgmt_parse_image_header_t * parseImageHeader)
 **********************************************************************
 * @brief
 *  This function parses the image header info. 
 *
 * @param object_id - The enclosure object id to send request to.
 * @param pParseImageHeader - pointer to fbe_enclosure_mgmt_parse_image_header_t
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  21-June-2010 PHE - Created
 *  11-Oct-2012 PHE - Add the check for board or enclosure.
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_parse_image_header(fbe_object_id_t object_id, 
                                                      fbe_enclosure_mgmt_parse_image_header_t * pParseImageHeader)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_payload_control_operation_opcode_t       ctrlCode; 
    fbe_topology_object_type_t                   objectType;

    if (pParseImageHeader == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    // Determine the type of object this is directed to
    status = fbe_api_get_object_type(object_id, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        ctrlCode = FBE_BASE_BOARD_CONTROL_CODE_PARSE_IMAGE_HEADER;
        break;

    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
    case FBE_TOPOLOGY_OBJECT_TYPE_LCC:
        ctrlCode = FBE_BASE_ENCLOSURE_CONTROL_CODE_PARSE_IMAGE_HEADER;
        break;

    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (unsigned int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    status = fbe_api_common_send_control_packet(ctrlCode,
                                                pParseImageHeader,
                                                sizeof (fbe_enclosure_mgmt_parse_image_header_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_api_enclosure_get_lcc_info(fbe_device_physical_location_t * pLocation,
 *                                     fbe_lcc_info_t * pLccInfo)
 *****************************************************************
 * @brief
 *  This function gets the info for the specified LCC. 
 *
 * @param pLocation - The physical location of the LCC.
 * @param pLccInfo(OUTPUT) - The pointer to the LCC info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10-Aug-2010: PHE - created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_lcc_info(fbe_device_physical_location_t * pLocation,
                                                         fbe_lcc_info_t * pLccInfo)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo;
    fbe_enclosure_mgmt_get_lcc_info_t               getLccInfo = {0};
    fbe_topology_control_get_enclosure_by_location_t enclByLocation = {0};
    
    
    status = fbe_api_get_enclosure_object_ids_array_by_location(pLocation->bus, 
                                                                pLocation->enclosure, 
                                                                &enclByLocation);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Failed to get object Id for Encl(%d_%d), status 0x%x.\n",
                      __FUNCTION__,
                      pLocation->bus,
                      pLocation->enclosure,
                      status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if(pLocation->componentId == 0) 
    {
        objectId = enclByLocation.enclosure_object_id;
    }
    else
    {
        objectId = enclByLocation.comp_object_id[pLocation->componentId];
    }
    
    if(FBE_OBJECT_ID_INVALID == objectId)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Encl(%d_%d.%d) LCC%d, object Id not valid.\n",
                      __FUNCTION__,
                      pLocation->bus,
                      pLocation->enclosure,
                      pLocation->componentId,
                      pLocation->slot);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (pLccInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: LCC info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getLccInfo.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_INFO,
                                                 &getLccInfo,
                                                 sizeof (fbe_enclosure_mgmt_get_lcc_info_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(pLccInfo, &getLccInfo.lccInfo, sizeof(fbe_lcc_info_t));

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_connector_info(fbe_device_physical_location_t * pLocation,
 *                                          fbe_connector_info_t * pConnectorInfo)
 *****************************************************************
 * @brief
 *  This function gets the info for the specified Connector. 
 *
 * @param pLocation - The physical location of the Connector.
 * @param pConnectorInfo(OUTPUT) - The pointer to the Connector info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  23-Aug-2011: PHE - created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_connector_info(fbe_device_physical_location_t * pLocation,
                                                         fbe_connector_info_t * pConnectorInfo)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo;
    fbe_enclosure_mgmt_get_connector_info_t         getConnectorInfo = {0};
    
    status = fbe_api_get_enclosure_object_id_by_location(pLocation->bus, pLocation->enclosure, &objectId);
    if ((status != FBE_STATUS_OK) || (FBE_OBJECT_ID_INVALID == objectId))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Connector(%d_%d_%d) Failed to get encl object Id or object Id not valid, status 0x%x.\n",
                      __FUNCTION__,
                      pLocation->bus,
                      pLocation->enclosure,
                      pLocation->slot,
                      status);
        return FBE_STATUS_GENERIC_FAILURE;
    }   

    if (pConnectorInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pConnectorInfo is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getConnectorInfo.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CONNECTOR_INFO,
                                                 &getConnectorInfo,
                                                 sizeof (fbe_enclosure_mgmt_get_connector_info_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(pConnectorInfo, &getConnectorInfo.connectorInfo, sizeof(fbe_connector_info_t));

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_ps_count(fbe_object_id_t objectId,
 *                                    fbe_u32_t * pPsCount)
 *****************************************************************
 * @brief
 *  This function gets the ps count for the specified enclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pPsCount(OUTPUT) - The pointer to the PS count.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10-Aug-2010: PHE - created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_ps_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pPsCount)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_payload_control_operation_opcode_t          ctrlCode;
    fbe_topology_object_type_t                      objectType;
    
    if (pPsCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: PS count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        ctrlCode = FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_COUNT;
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        ctrlCode = FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_COUNT;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    *pPsCount = 0;
    status = fbe_api_common_send_control_packet (ctrlCode,
                                                 pPsCount,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_slot_count(fbe_object_id_t objectId,
 *                                      fbe_u32_t * pSlotCount)
 *****************************************************************
 * @brief
 *  This function gets the drive slot count for the specified enclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pSlotCount(OUTPUT) - The pointer to the slot count.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  17-Feb-2011: Joe Perry - created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_slot_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pSlotCount)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_topology_object_type_t                      objectType;
    
    if (pSlotCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Slot count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    if(objectType == FBE_TOPOLOGY_OBJECT_TYPE_BOARD)
    {
        *pSlotCount = 0;
        return FBE_STATUS_OK;
    }

    *pSlotCount = 0;
    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_COUNT,
                                                 pSlotCount,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_slot_count_per_bank(fbe_object_id_t objectId,
 *                                      fbe_u32_t * pSlotCountPerBank)
 *****************************************************************
 * @brief
 *  This function gets the drive slot count for the specified enclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pSlotCountPerBank(OUTPUT) - The pointer to the slot count per bank.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  2-May-2012: Eric Zhou - created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_slot_count_per_bank(fbe_object_id_t objectId,
                                                         fbe_u32_t * pSlotCountPerBank)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_topology_object_type_t                      objectType;
    
    if (pSlotCountPerBank == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Slot count per bank buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    if(objectType == FBE_TOPOLOGY_OBJECT_TYPE_BOARD)
    {
        *pSlotCountPerBank = 0;
        return FBE_STATUS_OK;
    }

    *pSlotCountPerBank = 0;
    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_COUNT_PER_BANK,
                                                 pSlotCountPerBank,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_stand_alone_lcc_count(fbe_object_id_t objectId,
 *                                     fbe_u32_t * pLccCount)
 *****************************************************************
 * @brief
 *  This function gets the stand alone lcc count for the specified enclosure object id. 
 *  DPE like Jetfire also reports LCC info from eses, but we do not deal with that LCC
 *  here, we deal with it as part of Base Module. 
 *
 * @param objectId - The enclosure object id.
 * @param pPsCount(OUTPUT) - The pointer to the LCC count.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  30-Oct-2012: Rui Chang - created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_stand_alone_lcc_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pLccCount)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_topology_object_type_t                      objectType;
    
    if (pLccCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: LCC count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    if (objectType != FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (unsigned int)objectType);
        *pLccCount = 0;
        return FBE_STATUS_OK;
    }

    *pLccCount = 0;

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_STAND_ALONE_LCC_COUNT,
                                                 pLccCount,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_api_enclosure_get_lcc_count(fbe_object_id_t objectId,
 *                                     fbe_u32_t * pLccCount)
 *****************************************************************
 * @brief
 *  This function gets the lcc count for the specified enclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pPsCount(OUTPUT) - The pointer to the PS count.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  30-Aug-2010: PHE - created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_lcc_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pLccCount)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_payload_control_operation_opcode_t          ctrlCode; 
    fbe_topology_object_type_t                      objectType;
    
    if (pLccCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: LCC count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        ctrlCode = FBE_BASE_BOARD_CONTROL_CODE_GET_LCC_COUNT;
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        ctrlCode = FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_COUNT;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    *pLccCount = 0;

    status = fbe_api_common_send_control_packet (ctrlCode,
                                                 pLccCount,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_subencl_lcc_startSlot(fbe_object_id_t objectId,
 *                                     fbe_u32_t * pSlot)
 *****************************************************************
 * @brief
 *  This function sends a packet to get the lcc slot for the
 *  specified subenclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pSlot(OUTPUT) - pointer to the slot number
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11-Nov-2011: PHE - created.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_subencl_lcc_startSlot(fbe_object_id_t objectId, 
                                                                      fbe_u32_t  *pSlot)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_topology_object_type_t                      objectType;

    if (pSlot == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Slot buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if(objectType != FBE_TOPOLOGY_OBJECT_TYPE_LCC)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pSlot = 0;
    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_START_SLOT,
                                                 pSlot,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload status:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, 
                       statusInfo.packet_qualifier, 
                       statusInfo.control_operation_status, 
                       statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
} // fbe_api_enclosure_get_subencl_lcc_startSlot


/*!***************************************************************
 * @fn fbe_api_enclosure_get_connector_count(fbe_object_id_t objectId,
 *                                     fbe_u32_t * pConnectorCount)
 *****************************************************************
 * @brief
 *  This function gets the lcc count for the specified enclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pPsCount(OUTPUT) - The pointer to the PS count.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  30-Aug-2010: PHE - created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_connector_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pConnectorCount)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_payload_control_operation_opcode_t          ctrlCode; 
    fbe_topology_object_type_t                      objectType;
    
    if (pConnectorCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pConnectorCount is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        *pConnectorCount = 0;
        return FBE_STATUS_OK;
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        ctrlCode = FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CONNECTOR_COUNT;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    *pConnectorCount = 0;

    status = fbe_api_common_send_control_packet (ctrlCode,
                                                 pConnectorCount,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d\n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!********************************************************************
 * @fn fbe_api_enclosure_get_fan_info(
 *     fbe_object_id_t object_id, 
 *     fbe_cooling_info_t * pFanInfo)
 **********************************************************************
 * @brief
 *  This function gets enclosure Fan(Cooling) info.
 *
 * @param object_id - The enclosure object id to send request to.
 * @param enclosureControlInfo - pointer to the fbe_enclosure Fan Info struct
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  21-Dec-2010: Rajesh V.   Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_fan_info(fbe_object_id_t object_id, 
                                                        fbe_cooling_info_t *pFanInfo)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_payload_control_operation_opcode_t       ctrlCode; 
    fbe_topology_object_type_t                   objectType;

    if (pFanInfo == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(object_id, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        ctrlCode = FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_INFO;
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        ctrlCode = FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_INFO;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    status = fbe_api_common_send_control_packet(ctrlCode,
                                                pFanInfo,
                                                sizeof (fbe_cooling_info_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!********************************************************************
 * @fn fbe_api_enclosure_get_chassis_cooling_status(
 *     fbe_object_id_t object_id, 
 *     fbe_u8_t *pCoolingStatus)
 **********************************************************************
 * @brief
 *  This function gets enclosure Fan(Cooling) info.
 *
 * @param object_id - The enclosure object id to send request to.
 * @param pCoolingFault  - pointer to the fbe_enclosure chassis cooling status
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  25-Aug-2012: Rui Chang   Created
 * 
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_chassis_cooling_status(fbe_object_id_t object_id, 
                                                        fbe_bool_t *pCoolingFault)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t      status_info;

    if (pCoolingFault == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CHASSIS_COOLING_FAULT_STATUS,
                                                pCoolingFault,
                                                sizeof (fbe_bool_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}


/*!***************************************************************
 * @fn fbe_api_enclosure_get_fan_count(fbe_object_id_t objectId,
 *                                    fbe_u32_t * pFanCount)
 *****************************************************************
 * @brief
 *  This function gets the Fan count for the specified enclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pFanCount(OUTPUT) - The pointer to the Fan count.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  21-Dec-2010: Rajesh V.   Created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_fan_count(fbe_object_id_t objectId,
                                                          fbe_u32_t * pFanCount)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_payload_control_operation_opcode_t          ctrlCode; 
    fbe_topology_object_type_t                      objectType;
    
    if (pFanCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Fan count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        ctrlCode = FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_COUNT;
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        ctrlCode = FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_COUNT;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    *pFanCount = 0;
    status = fbe_api_common_send_control_packet (ctrlCode,
                                                 pFanCount,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_api_enclosure_get_encl_type(fbe_object_id_t objectId,
 *                                     fbe_enclosure_types_t * pEnclType)
 *****************************************************************
 * @brief
 *  This function gets the Enclosure type. 
 *
 * @param objectId - The enclosure object id.
 * @param pEnclType(OUTPUT) - The pointer to the Encl type.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  17-Jan-2011: PHE - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_encl_type(fbe_object_id_t objectId,
                                                          fbe_enclosure_types_t * pEnclType)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    
    if (pEnclType == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Encl type buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pEnclType = FBE_ENCL_TYPE_INVALID;
    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCL_TYPE,
                                                 pEnclType,
                                                 sizeof (fbe_enclosure_types_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_enclosure_get_disk_battery_backed_status(
 *     fbe_object_id_t object_id, 
 *     fbe_bool_t * diskBatteryBackedSet) 
 *********************************************************************
 * @brief:
 *  This function check whether batterry back bit  of drives is set correctly.
 *
 * @param object_id - The object id to send request to
 * @param diskBatteryBackedSet - pointer to return whether the battery backed bit 
 *                               is set correctly
 *  
 * @return fbe_status_t, success or failure
 *
 * @version
 *  8-Nov-2012: Rui Chang - Created.
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_disk_battery_backed_status(fbe_object_id_t object_id, 
                                                         fbe_bool_t * diskBatteryBackedSet)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (diskBatteryBackedSet == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_BATTERY_BACKED_INFO,
                                                diskBatteryBackedSet,
                                                sizeof(fbe_bool_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}   // end of fbe_api_enclosure_get_eir_info

/*!*******************************************************************
 * @fn fbe_api_enclosure_set_enclosure_failed(
 *     fbe_object_id_t object_id, 
 *     fbe_u32_t reason) 
 *********************************************************************
 * @brief:
 *  This function forces an enclosure to transition to the failed state.
 *
 * @param object_id - The object id to send request to
 * @param reason - enclosure fault LED reason code
 *  
 * @return fbe_status_t, success or failure
 *
 * @version
 *  14-Feb-2013: bphilbin - Created.
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_set_enclosure_failed(fbe_object_id_t object_id, 
                                                         fbe_base_enclosure_fail_encl_cmd_t *fail_cmd)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (fail_cmd == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_FAILED,
                                                fail_cmd,
                                                sizeof(fbe_base_enclosure_fail_encl_cmd_t),
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}   // end of fbe_api_enclosure_get_eir_info

/*!***************************************************************
 * @fn fbe_api_enclosure_get_drive_slot_info(fbe_object_id_t objectId,
 *                                     fbe_enclosure_mgmt_get_drive_slot_info_t * getDriveSlotInfo)
 *****************************************************************
 * @brief
 *  This function gets the Fan count for the specified enclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pEnclType(OUTPUT) - The pointer to the Encl type.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  17-Jan-2011: PHE - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_drive_slot_info(fbe_object_id_t objectId,
                                           fbe_enclosure_mgmt_get_drive_slot_info_t * getDriveSlotInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    
    if (getDriveSlotInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: getDriveSlotInfo is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DRIVE_SLOT_INFO,
                                                 getDriveSlotInfo,
                                                 sizeof (fbe_enclosure_mgmt_get_drive_slot_info_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_shutdown_info(fbe_device_physical_location_t * pLocation,
 *                                         fbe_enclosure_mgmt_get_shutdown_info_t * getShutdownInfo)
 *****************************************************************
 * @brief
 *  This function gets the Shutdown info the specified enclosure object id. 
 *
 * @param pLocation - The physical location of the LCC.
 * @param getShutdownInfo(OUTPUT) - The pointer to the Encl Shutdown info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  24-Mar-2011:    Joe Perry - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_shutdown_info(fbe_device_physical_location_t * pLocation,
                                                              fbe_enclosure_mgmt_get_shutdown_info_t * getShutdownInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_object_id_t                                 objectId;
    
    status = fbe_api_get_enclosure_object_id_by_location(pLocation->bus, pLocation->enclosure, &objectId);
    if ((status != FBE_STATUS_OK) || (FBE_OBJECT_ID_INVALID == objectId))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: Encl(%d_%d) Failed to get object Id or object Id not valid, status 0x%x.\n",
                      __FUNCTION__,
                      pLocation->bus,
                      pLocation->enclosure,
                      status);
        return FBE_STATUS_GENERIC_FAILURE;
    }   
        
    if (getShutdownInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: getShutdownInfo is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SHUTDOWN_INFO,
                                                 getShutdownInfo,
                                                 sizeof (fbe_enclosure_mgmt_get_shutdown_info_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_get_shutdown_info

/*!***************************************************************
 * @fn fbe_api_enclosure_get_overtemp_info(fbe_device_physical_location_t * pLocation,
 *                                         fbe_enclosure_mgmt_get_over_temp_info_t * getOverTempInfo)
 *****************************************************************
 * @brief
 *  This function gets the OverTemp info the specified enclosure object id. 
 *
 * @param pLocation - The physical location of the LCC.
 * @param getOverTempInfo(OUTPUT) - The pointer to the Encl OverTemp info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  31-Jan-2012:    Rui Chang - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_overtemp_info(fbe_device_physical_location_t * pLocation,
                                                              fbe_enclosure_mgmt_get_overtemp_info_t * getOverTempInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_object_id_t                                 objectId;
    
    status = fbe_api_get_enclosure_object_id_by_location(pLocation->bus, pLocation->enclosure, &objectId);
    if ((status != FBE_STATUS_OK) || (FBE_OBJECT_ID_INVALID == objectId))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: Encl(%d_%d) Failed to get object Id or object Id not valid, status 0x%x.\n",
                      __FUNCTION__,
                      pLocation->bus,
                      pLocation->enclosure,
                      status);
        return FBE_STATUS_GENERIC_FAILURE;
    }   
        
    if (getOverTempInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: getOverTempInfo is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_OVERTEMP_INFO,
                                                 getOverTempInfo,
                                                 sizeof (fbe_enclosure_mgmt_get_overtemp_info_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_get_overtemp_info

/*!***************************************************************
 * @fn fbe_api_enclosure_get_sps_info(fbe_device_physical_location_t * pLocation,
 *                                    fbe_enclosure_mgmt_get_sps_info_t * getSpsInfo)
 *****************************************************************
 * @brief
 *  This function gets the SPS info the specified enclosure object id. 
 *
 * @param pLocation - The physical location of the LCC.
 * @param getSpsInfo(OUTPUT) - The pointer to the Encl SPS info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  06-Jun-2012:    Joe Perry - Created.
 * 
 ****************************************************************/
fbe_status_t 
FBE_API_CALL fbe_api_enclosure_get_sps_info(fbe_object_id_t objectId,
                                            fbe_sps_get_sps_status_t * getSpsInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    
    if (getSpsInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: getSpsInfo is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SPS_INFO,
                                                 getSpsInfo,
                                                 sizeof (fbe_sps_get_sps_status_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_get_sps_info

/*!***************************************************************
 * @fn fbe_api_enclosure_get_sps_manuf_info(fbe_device_physical_location_t * pLocation,
 *                                    fbe_base_board_mgmt_get_sps_manuf_info_t *getSpsManufInfo)
 *****************************************************************
 * @brief
 *  This function gets the SPS Manuf info the specified enclosure object id. 
 *
 * @param pLocation - The physical location of the LCC.
 * @param getSpsManufInfo(OUTPUT) - The pointer to the Encl SPS Manuf info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  06-Aug-2012:    Joe Perry - Created.
 * 
 ****************************************************************/
fbe_status_t 
FBE_API_CALL fbe_api_enclosure_get_sps_manuf_info(fbe_object_id_t objectId,
                                                  fbe_sps_get_sps_manuf_info_t *getSpsManufInfo)
{
    fbe_enclosure_mgmt_ctrl_op_t                    operation = {0};
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_sg_element_t                                sg_element[2];  
    fbe_u32_t                                       sg_item = 0;
    
    if (getSpsManufInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: getSpsInfo is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    operation.response_buf_p = (fbe_u8_t *)&getSpsManufInfo->spsManufInfo;
    operation.response_buf_size = sizeof(fbe_sps_manuf_info_t);

    // set next sg element size and address for the response buffer
    fbe_sg_element_init(&sg_element[sg_item], 
                        operation.response_buf_size, 
                        operation.response_buf_p);
    sg_item++;

    // no more elements
    fbe_sg_element_terminate(&sg_element[sg_item]);
    sg_item++;

    status = fbe_api_common_send_control_packet_with_sg_list(FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SPS_MANUF_INFO,
                                                                   &operation,
                                                                   sizeof(fbe_enclosure_mgmt_ctrl_op_t),                                           
                                                                   objectId, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   sg_element, 
                                                                   sg_item,
                                                                   &statusInfo,
                                                                   FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_get_sps_info

/*!***************************************************************
 * @fn fbe_api_enclosure_send_sps_command(fbe_object_id_t objectId,
 *                                        fbe_sps_action_type_t spsCommand)
 *****************************************************************
 * @brief
 *  This function sends the SPS Command to the specified enclosure object id. 
 *
 * @param pLocation - The physical location of the LCC.
 * @param spsCommand(OUTPUT) - The pointer to the Encl SPS Command.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  08-Aug-2012:    Joe Perry - Created.
 * 
 ****************************************************************/
fbe_status_t 
FBE_API_CALL fbe_api_enclosure_send_sps_command(fbe_object_id_t objectId,
                                                fbe_sps_action_type_t spsCommand)
{
    fbe_enclosure_mgmt_ctrl_op_t                    operation = {0};
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    
    if (objectId >= FBE_OBJECT_ID_INVALID) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: objectId is Invalid\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    operation.response_buf_p = NULL;
    operation.response_buf_size = 0;
    operation.cmd_buf.spsInBufferCmd.spsAction = spsCommand;

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_SPS_COMMAND,
                                                 &operation,
                                                 sizeof (fbe_enclosure_mgmt_ctrl_op_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_enclosure_send_sps_command

/*!*************************************************************************
 * @fn fbe_api_enclosure_get_resume_status()
 *
 ***************************************************************************
 * @brief
 *       This routine translate control_status qualifier into resume prom status
 *
 * @param   control_status - control status.
 *
 * @return  fbe_resume_prom_status_t.
 *
 * HISTORY
 *   04-Jan-2011 Arun S - Created.
 ****************************************************************************/
fbe_resume_prom_status_t 
fbe_api_enclosure_get_resume_status(fbe_status_t status,
                                    fbe_payload_control_status_t control_status,
                                    fbe_payload_control_status_qualifier_t control_status_qualifier)
{
    fbe_resume_prom_status_t resume_status;

    switch (status)
    {
        case FBE_STATUS_BUSY:
            resume_status = FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
            break;

        case FBE_STATUS_OK:
            if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) 
            {
                switch (control_status_qualifier)
                {
                case FBE_ENCLOSURE_STATUS_OK:
                    resume_status = FBE_RESUME_PROM_STATUS_READ_SUCCESS;
                    break;
            
                case FBE_ENCLOSURE_STATUS_CMD_FAILED:
                case FBE_ENCLOSURE_STATUS_BUSY:
                    resume_status = FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS;
                    break;
            
                case FBE_ENCLOSURE_STATUS_CHECKSUM_ERROR:
                    resume_status = FBE_RESUME_PROM_STATUS_CHECKSUM_ERROR;
                    break;
            
                case FBE_ENCLOSURE_STATUS_TARGET_NOT_FOUND:
                case FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND:
                    resume_status = FBE_RESUME_PROM_STATUS_DEVICE_NOT_PRESENT;
                    break;
            
                case FBE_ENCLOSURE_STATUS_COMP_TYPE_UNSUPPORTED:
                    resume_status = FBE_RESUME_PROM_STATUS_DEVICE_NOT_VALID_FOR_PLATFORM;
                    break;
            
                case FBE_ENCLOSURE_STATUS_INSUFFICIENT_RESOURCE:
                    resume_status = FBE_RESUME_PROM_STATUS_INSUFFICIENT_RESOURCES;
                    break;
            
                case FBE_ENCLOSURE_STATUS_MEM_ALLOC_FAILED:
                case FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT:
                    resume_status = FBE_RESUME_PROM_STATUS_BUFFER_SMALL;
                    break;
            
                case FBE_ENCLOSURE_STATUS_HARDWARE_ERROR:
                    /* Also applicable for cases FBE_RESUME_PROM_STATUS_DEVICE_DEAD,
                     * FBE_RESUME_PROM_STATUS_SMBUS_ERROR, FBE_RESUME_PROM_STATUS_ARB_ERROR?
                     */
                    resume_status = FBE_RESUME_PROM_STATUS_DEVICE_ERROR; 
                    break;
            
                default:
                    /* For all other enclosure status cases */
                     resume_status = FBE_RESUME_PROM_STATUS_INVALID;
                    break;
                }
            }
            else
            {
                resume_status = FBE_RESUME_PROM_STATUS_INVALID;
            }
            break;

        case FBE_STATUS_GENERIC_FAILURE:
        default:
            resume_status = FBE_RESUME_PROM_STATUS_INVALID;
            break;
    } // End of - switch (status)

    return (resume_status);
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_sps_count(fbe_object_id_t objectId,
 *                                    fbe_u32_t * pSpsCount)
 *****************************************************************
 * @brief
 *  This function gets the SPS count for the specified enclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pSpsCount(OUTPUT) - The pointer to the SPS count.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  07-Dec-2011: PHE - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_sps_count(fbe_object_id_t objectId,
                                                          fbe_u32_t * pSpsCount)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_payload_control_operation_opcode_t          ctrlCode; 
    fbe_topology_object_type_t                      objectType;
    
    if (pSpsCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: SPS count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        ctrlCode = FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_COUNT;
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        *pSpsCount = 0;
        return FBE_STATUS_OK;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    *pSpsCount = 0;
    status = fbe_api_common_send_control_packet (ctrlCode,
                                                 pSpsCount,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_battery_count(fbe_object_id_t objectId,
 *                                         fbe_u32_t * pBatteryCount)
 *****************************************************************
 * @brief
 *  This function gets the Battery count for the specified enclosure object id. 
 *
 * @param objectId - The enclosure object id.
 * @param pBatteryCount(OUTPUT) - The pointer to the Battery count.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  18-Apr-2012: Joe Perry - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_battery_count(fbe_object_id_t objectId,
                                                          fbe_u32_t * pBatteryCount)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_payload_control_operation_opcode_t          ctrlCode; 
    fbe_topology_object_type_t                      objectType;
    
    if (pBatteryCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Battery count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        ctrlCode = FBE_BASE_BOARD_CONTROL_CODE_GET_BATTERY_COUNT;
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        *pBatteryCount = 0;
        return FBE_STATUS_OK;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (unsigned int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    *pBatteryCount = 0;
    status = fbe_api_common_send_control_packet (ctrlCode,
                                                 pBatteryCount,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_encl_fault_led_info(fbe_object_id_t objectId,
 *                        fbe_encl_fault_led_info_t * pEnclFaultLedInfo)
 *****************************************************************
 * @brief
 *  This function gets the enclosure fault LED info. 
 *
 * @param objectId - The enclosure object id. For XPE/DPE, this is board object id.
 * @param pEnclFaultLedInfo(OUTPUT) - The pointer to the enclosure fault led info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  21-Dec-2010: Rajesh V.   Created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_encl_fault_led_info(fbe_object_id_t objectId,
                                                          fbe_encl_fault_led_info_t * pEnclFaultLedInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_topology_object_type_t                      objectType;
    fbe_board_mgmt_misc_info_t                      miscInfo = {0};  // For process enclosure led.
    fbe_base_enclosure_led_status_control_t         ledStatusControl = {0}; // For DAE enclosure led.

    
    if (pEnclFaultLedInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pEnclFaultLedInfo buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_MISC_INFO,
                                                 &miscInfo,
                                                 sizeof (fbe_board_mgmt_misc_info_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        ledStatusControl.ledAction = FBE_ENCLOSURE_LED_STATUS;
        status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_LED_STATUS_CONTROL,
                                                 &ledStatusControl,
                                                 sizeof (fbe_base_enclosure_led_status_control_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
        case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
            if(miscInfo.EnclosureFaultLED == LED_BLINK_OFF) 
            {
                pEnclFaultLedInfo->enclFaultLedStatus = FBE_LED_STATUS_OFF;
            }
            else if(miscInfo.EnclosureFaultLED == LED_BLINK_ON) 
            {
                pEnclFaultLedInfo->enclFaultLedStatus = FBE_LED_STATUS_ON;
            }
            else if(miscInfo.EnclosureFaultLED == LED_BLINK_INVALID) 
            {
                pEnclFaultLedInfo->enclFaultLedStatus = FBE_LED_STATUS_INVALID;
            }
            else
            {
                pEnclFaultLedInfo->enclFaultLedStatus = FBE_LED_STATUS_MARKED;
            }

            pEnclFaultLedInfo->enclFaultLedReason = miscInfo.enclFaultLedReason;
            break;
            
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
            if(ledStatusControl.ledInfo.enclFaultLedMarked) 
            {
                pEnclFaultLedInfo->enclFaultLedStatus = FBE_LED_STATUS_MARKED;
            }
            else if(ledStatusControl.ledInfo.enclFaultLedStatus == FBE_FALSE) 
            {
                pEnclFaultLedInfo->enclFaultLedStatus = FBE_LED_STATUS_OFF;
            }
            else if(ledStatusControl.ledInfo.enclFaultLedStatus == FBE_TRUE) 
            {
                pEnclFaultLedInfo->enclFaultLedStatus = FBE_LED_STATUS_ON;
            }
            pEnclFaultLedInfo->enclFaultLedReason = ledStatusControl.ledInfo.enclFaultReason;
            break;
    
        default:
            break;
    }

    return status;
}

/*!***************************************************************
 * @static fn fbe_api_enclosure_get_send_led_info_Packet(fbe_object_id_t objectId,
 *                        fbe_base_enclosure_led_status_control_t *pLedStatusControl)
 *****************************************************************
 * @brief
 *  This function sends request for enclosure's drive slot LED info. 
 *
 * @param objectId - The enclosure object id. 
 * @param pLedStatusControl(OUTPUT) - The pointer to Led Status/control.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  16-Oct-2012: R Porteus   Created
 * 
 ****************************************************************/
static fbe_status_t FBE_API_CALL fbe_api_enclosure_get_send_led_info_Packet(fbe_object_id_t objectId,
                                                      fbe_base_enclosure_led_status_control_t *pLedStatusControl)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    
    if (pLedStatusControl == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pLedStatusControl is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pLedStatusControl->ledAction = FBE_ENCLOSURE_LED_STATUS;
    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_LED_STATUS_CONTROL,
                                                 pLedStatusControl,
                                                 sizeof (fbe_base_enclosure_led_status_control_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);
        

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_encl_drive_slot_led_info(fbe_object_id_t objectId,
 *                        fbe_base_enclosure_led_behavior_t *pEnclDriveFaultLeds)
 *****************************************************************
 * @brief
 *  This function gets the enclosure's drive slot LED info. 
 *
 * @param objectId - The enclosure object id. 
 * @param pEnclDriveFaultLeds(OUTPUT) - The pointer to the drive slot led info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  6-Feb-2012: bphilbin   Created
 *  16-Oct-2012: R Porteus   Update to support sub enclosure.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_encl_drive_slot_led_info(fbe_object_id_t objectId,
                                                                         fbe_led_status_t *pEnclDriveFaultLeds)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_enclosure_led_status_control_t         ledStatusControl = {0}; // For DAE enclosure led.
    fbe_port_number_t           port_num;
    fbe_enclosure_number_t      enclosure_num;
    fbe_u32_t                   comp_object_handle;
    fbe_u32_t                   i;
    fbe_u32_t                   NumberOfDrives;
    fbe_u32_t                   SlotCountPerBank;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids = {0};

    if (pEnclDriveFaultLeds == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pEnclDriveFaultLeds buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_enclosure_get_slot_count(objectId, &NumberOfDrives);
    status = fbe_api_enclosure_get_slot_count_per_bank(objectId, &SlotCountPerBank);

    fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:Number of drives:%d, \n", __FUNCTION__, NumberOfDrives);   
    
    // If slots per Bank is set and there are more drives than contained in a bank
    // then the enclosure contains Edge Expanders, so process LED status for each one.     
    if(SlotCountPerBank && (NumberOfDrives >  SlotCountPerBank))
    {
        status = fbe_api_get_object_port_number ((fbe_object_id_t)objectId, &port_num);
        status = fbe_api_get_object_enclosure_number ((fbe_object_id_t)objectId, &enclosure_num);
        status = fbe_api_get_enclosure_object_ids_array_by_location(port_num,enclosure_num, &enclosure_object_ids);
        for (i = 0; i < FBE_API_MAX_ENCL_COMPONENTS && enclosure_object_ids.component_count >0; i++)
        {
            comp_object_handle = enclosure_object_ids.comp_object_id[i];
            if (comp_object_handle == FBE_OBJECT_ID_INVALID)
            {
                continue;
            }

            status = fbe_api_enclosure_get_send_led_info_Packet(comp_object_handle ,&ledStatusControl);
            if(status != FBE_STATUS_OK)
            {
                return status;
            }
        }
    }
    else
    {
        fbe_api_enclosure_get_send_led_info_Packet(objectId ,&ledStatusControl);
        if (status != FBE_STATUS_OK)
        {
                return status;
        }
    }

    fbe_copy_memory(pEnclDriveFaultLeds, 
                    ledStatusControl.ledInfo.enclDriveFaultLedStatus, 
                    (sizeof(fbe_led_status_t) * FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS));
    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_encl_side(fbe_object_id_t objectId,
 *                                     fbe_u32_t * pEnclSide)
 *****************************************************************
 * @brief
 *  This function gets the enclosure side. 
 *
 * @param objectId - The enclosure object id.
 * @param pEnclType(OUTPUT) - The pointer to the Encl type.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  17-Jan-2011: PHE - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_encl_side(fbe_object_id_t objectId,
                                                          fbe_u32_t * pEnclSide)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    
    if (pEnclSide == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Encl side buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pEnclSide = 0;
    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCL_SIDE,
                                                 pEnclSide,
                                                 sizeof (fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_number_of_drive_midplane(fbe_object_id_t objectId,
 *                                     fbe_u8_t * pNumDriveMidplane)
 *****************************************************************
 * @brief
 *  This function gets the number of drive midplane.
 *
 * @param objectId - The enclosure object id.
 * @param pEnpNoDriveMidplane(OUTPUT) - The pointer to the Number of drive midplane.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  11-Oct-2011: Dipak - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_number_of_drive_midplane(fbe_object_id_t objectId,
                                                          fbe_u8_t * pNumDriveMidplane)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_payload_control_operation_opcode_t          ctrlCode; 
    fbe_topology_object_type_t                      objectType;
    
    if (pNumDriveMidplane == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: NumDriveMidplane buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

        /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        *pNumDriveMidplane = 0;
        return FBE_STATUS_OK;
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        ctrlCode = FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_NUMBER_OF_DRIVE_MIDPLANE;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (unsigned int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    *pNumDriveMidplane = 0;
    status = fbe_api_common_send_control_packet (ctrlCode,
                                                 pNumDriveMidplane,
                                                 sizeof (fbe_u8_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*!***************************************************************
 * @fn fbe_api_enclosure_get_display_info(fbe_object_id_t objectId,
 *                        fbe_encl_display_info_t * pEnclDisplayInfo)
 *****************************************************************
 * @brief
 *  This function gets the enclosure fault LED info. 
 *
 * @param objectId - The enclosure object id. For XPE/DPE, this is board object id.
 * @param pEnclDisplayInfo(OUTPUT) - The pointer to the enclosure display info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  29-Jan-2012: bphilbin   Created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_display_info(fbe_object_id_t objectId,
                                                          fbe_encl_display_info_t * pEnclDisplayInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    
    if (pEnclDisplayInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pEnclDisplayInfo buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DISPLAY_INFO,
                                                 pEnclDisplayInfo,
                                                 sizeof (fbe_encl_display_info_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_enclosure_get_disk_parent_object_id(fbe_u32_t bus,
 *                                                 fbe_u32_t enclosure, 
 *                                                 fbe_u32_t slot,
 *                                                 fbe_base_object_mgmt_get_enclosure_info_t *pEnclInfo;
 *                                                 fbe_object_id_t *pParentObjId)
 *****************************************************************
 * @brief
 * Get the disk parent obj id. Useful for voyager enclosures,
 * where the disk slots are split between multiple expanders.
 * Get a list of edge expander object ids and test them to determine
 * which one the target disk slot belongs to.
 *
 * @param 
 * @param 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_disk_parent_object_id(fbe_u32_t bus, 
                                                                      fbe_u32_t enclosure, 
                                                                      fbe_u32_t slot,
                                                                      fbe_object_id_t *pParentObjId)
{
    fbe_u8_t                                            i;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t             statusInfo = {0};
    fbe_topology_control_get_enclosure_by_location_t    enclByLocation = {0};
    fbe_enclosure_mgmt_disk_slot_present_cmd_t          slotPresent = {0};

    if (pParentObjId == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Encl side buffer is NULL\n", __FUNCTION__);
    }

    status = fbe_api_get_enclosure_object_ids_array_by_location(bus, enclosure, &enclByLocation);
    if (status != FBE_STATUS_OK)
    {
        // Just return the above function displayed an error
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get physical drive object handle for bus:%d encl:%d slot:%d \n", __FUNCTION__,bus, enclosure, slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // check the enclosure obj
    fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:Check ParentObjId:0x%x, \n", __FUNCTION__, enclByLocation.enclosure_object_id);
    slotPresent.targetSlot = slot; // init the input
    slotPresent.slotFound = FALSE; // init the output
    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_PRESENT,
                                                 &slotPresent,
                                                 sizeof (fbe_enclosure_mgmt_disk_slot_present_cmd_t),
                                                 enclByLocation.enclosure_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return status;
    }
    
    if(slotPresent.slotFound)
    {
        *pParentObjId = enclByLocation.enclosure_object_id;
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:Found ParentObjId:0x%x, \n", __FUNCTION__, enclByLocation.enclosure_object_id);
    }
    else
    {
        // loop to process the obj id list
        for(i=0;i<FBE_API_MAX_ENCL_COMPONENTS;i++)
        {
            if(enclByLocation.comp_object_id[i] == FBE_OBJECT_ID_INVALID)
            {
                fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:Loc index %d FBE_OBJECT_ID_INVALID\n", __FUNCTION__, i);
            }
            if(enclByLocation.comp_object_id[i] != FBE_OBJECT_ID_INVALID)
            {
                fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:Check ParentObjId:0x%x, \n", __FUNCTION__, enclByLocation.comp_object_id[i]);
                slotPresent.targetSlot = slot; // init the input
                slotPresent.slotFound = FALSE; // init the output
                status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_PRESENT,
                                                             &slotPresent,
                                                             sizeof (fbe_enclosure_mgmt_disk_slot_present_cmd_t),
                                                             enclByLocation.comp_object_id[i],
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &statusInfo,
                                                             FBE_PACKAGE_ID_PHYSICAL);
                if (status != FBE_STATUS_OK)
                {
                    fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
                    return status;
                }
                if(slotPresent.slotFound)
                {
                    // save the parent obj id
                    *pParentObjId = enclByLocation.comp_object_id[i];
                    fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:Found ParentObjId:0x%x, \n", __FUNCTION__, *pParentObjId);
                    break;
                }
            }
        }
    }

    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK) ||
       (!slotPresent.slotFound))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
} // fbe_api_enclosure_get_disk_parent_object_id

/*!***************************************************************
 * @fn fbe_api_enclosure_get_ssc_count(fbe_object_id_t objectId,
 *                                    fbe_u32_t * pSscCount)
 *****************************************************************
 * @brief
 *  Send the packet to get theSSC count
 *
 * @param objectId - The enclosure object id.
 * @param pSscCount(OUTPUT) - The pointer to the SSC count.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  17-Dec-2013: GB - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_ssc_count(fbe_object_id_t objectId,
                                                          fbe_u8_t * pSscCount)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_topology_object_type_t                      objectType;
    
    if (pSscCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: SSC count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    if(objectType == FBE_TOPOLOGY_OBJECT_TYPE_BOARD)
    {
        *pSscCount = 0;
        return FBE_STATUS_OK;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SSC_COUNT,
                                                 pSscCount,
                                                 sizeof (fbe_u8_t),         // size of pSscCount (the return buffer)
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
} //fbe_api_enclosure_get_ssc_count

/*!***************************************************************
 * @fn fbe_api_enclosure_get_ssc_info(fbe_object_id_t objectId,
 *                                    fbe_u32_t * pSscInfo)
 *****************************************************************
 * @brief
 *  Send the packet to get the SSC info data.
 *
 * @param objectId - The enclosure object id.
 * @param pSscInfo(OUTPUT) - The pointer to the SSC info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  18-Dec-2013: GB - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_ssc_info(fbe_object_id_t objectId,
                                                         fbe_ssc_info_t *pSscInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    
    if (pSscInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: SSC info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SSC_INFO,
                                                 pSscInfo,
                                                 sizeof (fbe_ssc_info_t),   // size of the return buffer
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, \n", __FUNCTION__, status);        
        return status;

    }
       
    if((statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (statusInfo.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
} //fbe_api_enclosure_get_ssc_info

// end fbe_api_enclosure_interface.c

