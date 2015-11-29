/***************************************************************************
 *  terminator_encl_eses_plugin_read_buffer.c
 ***************************************************************************
 *
 *  Description
 *     This contains the funtions related to processing
 *     the "read buffer" ESES/SCSI command.
 *      
 *
 *  History:
 *      Jan-7-08    Rajesh V Created
 *    
 ***************************************************************************/
#include <math.h>
#include "fbe/fbe_types.h"

#include "terminator_sas_io_api.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "fbe_terminator.h"
#include "terminator_encl_eses_plugin.h"
#include "resume_prom.h"


/**************************************************************************
 *  process_payload_read_buffer()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes the "read buffer" cdb command.
 *
 *  PARAMETERS:
 *      Payload pointer, virtual phy object handle 
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jan-05-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t process_payload_read_buffer(fbe_payload_ex_t * payload, 
                                         fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sas_enclosure_type_t  encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_eses_read_buf_cdb_t *read_buf_cdb;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u8_t *sense_buffer, *buf_id;
    terminator_eses_buf_info_t *buf_info; // pointer to buffer info of correponding buffer
    fbe_u8_t buf_byte_boundary;
    fbe_u32_t requested_buf_offset, requested_buf_alloc_len, data_in_buf_len;
    fbe_u8_t *data_in_buf; // pointer to the Data in buffer/sg_list.
    fbe_u8_t *buf; // corresponding buffer in terminator
    fbe_u32_t buf_len;   // coresponding buffer length in terminator

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_virtual_phy_get_enclosure_type failed\n", __FUNCTION__);
        return status;
    }

    status = terminator_check_sas_enclosure_type(encl_type);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_check_sas_enclosure_type failed\n", __FUNCTION__);
        return status;
    }

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    if(payload_cdb_operation == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_ex_get_cdb_operation failed\n", __FUNCTION__);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // Get sense buffer from cdb
    status = fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_buffer);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_cdb_operation_get_sense_buffer failed\n", __FUNCTION__);
        return status;
    }

    read_buf_cdb = (fbe_eses_read_buf_cdb_t *)(payload_cdb_operation->cdb);

#if 0
    if(read_buf_cdb->mode != FBE_ESES_READ_BUF_MODE_DATA)
    {
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_PARAMETER_NOT_SUPPORTED,
                                FBE_SCSI_ASCQ_PARAMETER_NOT_SUPPORTED);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }
#endif
    buf_id = &read_buf_cdb->buf_id;
    // Get the pointer to the corresponding buffer info, assuming the buffer exists.
    status = eses_get_buf_info_by_buf_id(virtual_phy_handle, *buf_id, &buf_info);
    if(status != FBE_STATUS_OK)
    {
        // Most probably the buffer with the specified buffer id does
        // not exist.
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_CDB,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }
    
    // Get the client request buffer offset from the cdb.
    requested_buf_offset = 0x10000 * (read_buf_cdb->buf_offset_msbyte) +
                           0x100 * (read_buf_cdb->buf_offset_midbyte) +
                           0x1 * (read_buf_cdb->buf_offset_lsbyte);

    // Get the buffer byte boundary of the corresponding buffer.
    buf_byte_boundary =(fbe_u8_t) pow(buf_info->buf_offset_boundary, 2); 

    // The buffer offset should be a multiple of the "Buffer Byte Boundary"
    // of the corresponding buffer
    if((requested_buf_offset % buf_byte_boundary) != 0)
    {
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_CDB,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    if(sg_list->count == 0)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sg list count is 0, can't return the requested page\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    // Get the Data in buffer parameters from the SGlist.
    data_in_buf = (fbe_u8_t *)fbe_sg_element_address(sg_list);
    data_in_buf_len = (fbe_u32_t)fbe_sg_element_count(sg_list);

    // Get the requested allocation length from the cdb.
    requested_buf_alloc_len =  0x10000 * (read_buf_cdb->alloc_length_msbyte) +
                               0x100 * (read_buf_cdb->alloc_length_midbyte) +
                               0x1 * (read_buf_cdb->alloc_length_lsbyte);

    // Get the respective buffer and its length
    buf_len = buf_info->buf_len;
    buf = buf_info->buf;

    if(read_buf_cdb->mode == FBE_ESES_READ_BUF_MODE_DESC) 
    {
        data_in_buf[0] = (buf_len & 0xFF000000) >> 24;
        data_in_buf[1] = (buf_len & 0x00FF0000) >> 16;
        data_in_buf[2] = (buf_len & 0x0000FF00) >> 8;
        data_in_buf[3] = buf_len & 0x000000FF;
        
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS); 
    }
    else if(buf_len <= requested_buf_offset)
    { 
        /* Temp fix: return good status until the terminator is fixed to get the proper buf_len. */
        // We dont have to copy anything as the requested data length is zero
        // Just return good status
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);             
#if 0
        // The buffer length is less than the requested offset into the buffer
        // Just return saying EMA cannot accept the given offset.
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_CDB,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
#endif
    }
    else if(requested_buf_alloc_len == 0)
    { 
        // We dont have to copy anything as the requested data length is zero
        // Just return good status
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);             
    }
    else  
    { 
        // We are all set to copy the data from terminator/EMA buffer into the
        // Data-In buffer provided by the client

        fbe_u32_t buf_copy_size;

        if(requested_buf_alloc_len < (buf_len - requested_buf_offset))
        {
            buf_copy_size = requested_buf_alloc_len;
        }
        else
        {
            buf_copy_size = buf_len - requested_buf_offset;
        }

        memcpy(data_in_buf, (buf + requested_buf_offset), (buf_copy_size)*sizeof(fbe_u8_t));
        fbe_payload_cdb_set_transferred_count(payload_cdb_operation, (buf_copy_size)*sizeof(fbe_u8_t));
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS); 
    } 

    return(FBE_STATUS_OK);
 }


/**************************************************************************
 *  process_payload_write_buffer()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function processes the "write buffer" scsi cdb command.
 *
 *  PARAMETERS:
 *      Payload pointer, virtual phy object handle 
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jan-23-2008: Rajesh V created.
 **************************************************************************/

fbe_status_t process_payload_write_buffer(fbe_payload_ex_t * payload, 
                                         fbe_terminator_device_ptr_t virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sas_enclosure_type_t  encl_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_eses_write_buf_cdb_t *write_buf_cdb;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u8_t *sense_buffer, *buf_id;
    terminator_eses_buf_info_t *buf_info; // pointer to buffer info of correponding buffer
    fbe_u8_t buf_byte_boundary;
    fbe_u32_t requested_buf_offset, requested_buf_param_list_len, data_out_buf_len;
    fbe_u8_t *data_out_buf; // pointer to the Data out buffer/sg_list.
    fbe_u8_t *buf; // corresponding buffer in terminator
    fbe_u32_t buf_len;   // coresponding buffer length in terminator
    fbe_bool_t need_update_checksum;

    /* get the enclosure type thru the virtual_phy_handle */
    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_virtual_phy_get_enclosure_type failed\n", __FUNCTION__);
        return status;
    }

    status = terminator_check_sas_enclosure_type(encl_type);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_check_sas_enclosure_type failed\n", __FUNCTION__);
        return status;
    }

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    if(payload_cdb_operation == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_ex_get_cdb_operation failed\n", __FUNCTION__);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // Get sense buffer from cdb
    status = fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_buffer);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_cdb_operation_get_sense_buffer failed\n", __FUNCTION__);
        return status;
    }

    write_buf_cdb = (fbe_eses_write_buf_cdb_t *)(payload_cdb_operation->cdb);

    if(write_buf_cdb->mode != FBE_ESES_WRITE_BUF_MODE_DATA)
    {
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_PARAMETER_NOT_SUPPORTED,
                                FBE_SCSI_ASCQ_PARAMETER_NOT_SUPPORTED);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }

    buf_id = &write_buf_cdb->buf_id;
    // Get the pointer to the corresponding buffer info, assuming the buffer exists.
    status = eses_get_buf_info_by_buf_id(virtual_phy_handle, *buf_id, &buf_info);
    if(status != FBE_STATUS_OK)
    {
        // Most probably the buffer with the specified buffer id does
        // not exist.
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_CDB,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }

    // Make sure the buffer is a "writable" type buffer.
    if(!(buf_info->writable))
    {
        // Buffer is not writable
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_PARAMETER_NOT_SUPPORTED,
                                FBE_SCSI_ASCQ_PARAMETER_NOT_SUPPORTED);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }
    
    // Get the client requested buffer offset from the cdb.
    requested_buf_offset = 0x10000 * (write_buf_cdb->buf_offset_msbyte) +
                           0x100 * (write_buf_cdb->buf_offset_midbyte) +
                           0x1 * (write_buf_cdb->buf_offset_lsbyte);

    // Get the buffer byte boundary of the corresponding buffer.
    buf_byte_boundary =(fbe_u8_t) pow(buf_info->buf_offset_boundary, 2); 

    // The buffer offset should be a multiple of the "Buffer Byte Boundary"
    // of the corresponding buffer
    if((requested_buf_offset % buf_byte_boundary) != 0)
    {
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_CDB,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
        return(FBE_STATUS_OK);
    }

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    if(sg_list->count == 0)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sg list count is 0, can't return the requested page\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    // Get the Data Out buffer parameters from the SGlist.
    data_out_buf = (fbe_u8_t *)fbe_sg_element_address(sg_list);
    data_out_buf_len = (fbe_u32_t)fbe_sg_element_count(sg_list);

    // Get the length of the data to be written to the buffer.
    requested_buf_param_list_len =  0x10000 * (write_buf_cdb->param_list_length_msbyte) +
                                    0x100 * (write_buf_cdb->param_list_length_midbyte) +
                                    0x1 * (write_buf_cdb->param_list_length_lsbyte);

    // Get the respective buffer and its length
    buf_len = buf_info->buf_len;
    buf = buf_info->buf;

    if((buf_len - requested_buf_offset) < requested_buf_param_list_len)
    { 
        // The remaining buffer length, starting from the "requested buffer offset" is less
        // than the length of the data to be written to the buffer. This is not allowed and
        // we return an error and buffer is untouched.
        build_sense_info_buffer(sense_buffer, 
                                FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST, 
                                FBE_SCSI_ASC_INVALID_FIELD_IN_CDB,
                                FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ERROR);
    }
    else if(requested_buf_param_list_len == 0)
    { 
        // We dont have to copy anything as the length of data to be written, is zero
        // Just return good status
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS);             
    }
    else
    { 
        memcpy((buf+requested_buf_offset), data_out_buf, (requested_buf_param_list_len)*sizeof(fbe_u8_t));
        if(buf_info->buf_type == SES_BUF_TYPE_EEPROM)
        {
            terminator_get_need_update_enclosure_resume_prom_checksum(&need_update_checksum);
            if(need_update_checksum)
            {
                calculate_buffer_checksum(buf, buf_len);  // we need not check if buf_len is 2K again as we check that in TAPI
            }
        }
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS); 
    }

    return(FBE_STATUS_OK);
}


/**************************************************************************
 *  calculate_buffer_checksum()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function calculates checksum of specified buffer.
 *
 *  PARAMETERS:
 *      buf_ptr    - Pointer to buffer.
 *      buf_length - Length of buffer.
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Apr-09-2009: Prahlad Purohit - Adapted from calculateResumeChecksum
 **************************************************************************/
void
calculate_buffer_checksum(fbe_u8_t *buf_ptr, fbe_u32_t buf_length)
{
    fbe_u32_t checkSum    = 0;
    fbe_u32_t word_count  = 0;
    fbe_u32_t bufSize     = 0;
    fbe_u32_t *data_word  = (fbe_u32_t *) buf_ptr;
    fbe_u32_t index;

    bufSize = (buf_length - RESUME_PROM_CHECKSUM_SIZE);
    word_count  = bufSize / sizeof(fbe_u32_t);

    /* calculate the new checksum */
    for (index = 0; index < word_count; index++, data_word++)
    {
        checkSum ^= *data_word;
    }
    checkSum ^= RESUME_PROM_CHECKSUM_SEED;

    *((fbe_u32_t *) (buf_ptr + buf_length - RESUME_PROM_CHECKSUM_SIZE)) = checkSum;
}

