/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*********************************************************************
 * @file fbe_eses_main.c
 **********************************************************************
 * @brief 
 *  The routines in this file handles the read/write of ESES pages. 
 *  
 * HISTORY:
 *  24-Jul-2008 PHE - Created.                  
 ***********************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"

/**************************
 * GLOBALS 
 **************************/



/*!*************************************************************************
*  @fn fbe_eses_build_receive_diagnostic_results_cdb(fbe_u8_t *cdb, 
*                                                    fbe_u16_t cdb_buffer_size, 
*                                                    fbe_u16_t response_buffer_size,
*                                                    ses_pg_code_enum page_code)               
***************************************************************************
* @brief
*       This function verifies the size of the CDB and builds the CDB to 
*       send "RECEIVE_DIAGNOSTIC_RESULTS" command. The page code specifies 
*       the diagnostic page to be returned:
*       (1) SES_PG_CODE_CONFIG (configuratoin diagnostic page);
*       (2) SES_PG_CODE_ENCL_STAT (status diagnostic page);
*       (3) SES_PG_CODE_ADDL_ELEM_STAT (additional status diagnostic page);
*       (4) SES_PG_CODE_EMC_ENCL_STAT (EMC enclosure status diagnostic page);
*       ...add other supported pages below if needed.
*
* @param    cdb - The pointer to the cdb.
* @param    cdb_buffer_size - The buffer size of the cdb.
* @param    response_buffer_size - The response buffer size.
* @param    page_code - The page code that is requested.
*
* @return  fbe_status_t 
*        FBE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 PHE - Created.
***************************************************************************/
fbe_status_t fbe_eses_build_receive_diagnostic_results_cdb(fbe_u8_t *cdb, 
                                                          fbe_u16_t cdb_buffer_size, 
                                                          fbe_u16_t response_buffer_size,
                                                          ses_pg_code_enum page_code)
{
    fbe_eses_receive_diagnostic_results_cdb_t *cdb_page;

    if(cdb_buffer_size < FBE_SCSI_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE) {
        return (FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_zero_memory(cdb, FBE_SCSI_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE);
    cdb_page = (fbe_eses_receive_diagnostic_results_cdb_t *)cdb;

    cdb_page->operation_code = FBE_SCSI_RECEIVE_DIAGNOSTIC;
    cdb_page->page_code_valid = 1;
    cdb_page->rsvd = 0;
    cdb_page->page_code = page_code;
    cdb_page->alloc_length_msbyte = ((response_buffer_size & 0xFF00) >> 8);
    cdb_page->alloc_length_lsbyte = (response_buffer_size & 0xFF);
    cdb_page->control = 0;

   return FBE_STATUS_OK;
}


/*!*************************************************************************
* @fn fbe_eses_build_send_diagnostic_cdb(fbe_u8_t *cdb, 
*                                        fbe_u16_t cdb_buffer_size, 
*                                        fbe_u16_t cmd_buffer_size)               
***************************************************************************
* @brief
*       This function verifies the size of the CDB and builds the CDB to 
*       send "SEND_DIAGNOSTIC" command which sends the control page to
*       the EMA in the LCC.
*
* @param    cdb - The pointer to the cdb.
* @param    cdb_buffer_size - The buffer size of the cdb.
* @param    cmd_buffer_size - The number of bytes to be written to the EMA.
*       
* @return  fbe_status_t 
*        FBE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   05-Aug-2008 PHE - Created.
***************************************************************************/
fbe_status_t fbe_eses_build_send_diagnostic_cdb(fbe_u8_t *cdb, 
                                                       fbe_u16_t cdb_buffer_size, 
                                                       fbe_u16_t cmd_buffer_size)
{
    fbe_eses_send_diagnostic_cdb_t *cdb_page;

    if(cdb_buffer_size < FBE_SCSI_SEND_DIAGNOSTIC_CDB_SIZE) {
        return (FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_zero_memory(cdb, FBE_SCSI_SEND_DIAGNOSTIC_CDB_SIZE);
    cdb_page = (fbe_eses_send_diagnostic_cdb_t *)cdb;

    cdb_page->operation_code = FBE_SCSI_SEND_DIAGNOSTIC;
    cdb_page->page_format = 1;
    cdb_page->unit_offline = 0;
    cdb_page->device_offline = 0;
    cdb_page->self_test = 0;
    cdb_page->self_test_code = 0;
    cdb_page->rsvd = 0;
    cdb_page->rsvd1 = 0;
    cdb_page->param_list_length_msbyte = ((cmd_buffer_size & 0xFF00) >> 8);
    cdb_page->param_list_length_lsbyte = (cmd_buffer_size & 0xFF);
    cdb_page->control = 0;

   return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_status_t fbe_eses_build_read_buf_cdb(fbe_u8_t *cdb, 
*                                                fbe_u8_t cdb_buffer_size, 
*                                                fbe_eses_read_buf_mode_t mode,
*                                                fbe_u8_t buf_id,
*                                                fbe_u32_t buf_offset,
*                                                fbe_u32_t response_buffer_size)        
***************************************************************************
* @brief
*       This function verifies the size of the CDB and builds the CDB to 
*       send "READ BUFFER" command to the EMA in the LCC to read a buffer 
*       in the enclosure. 
*
* @param   cdb - The pointer to the cdb.
* @param   cdb_buffer_size - The buffer size of the cdb.
* @param   mode - The mode of the read buffer command.
* @param   buf_id  - The identity of the buffer to be read. 
* @param   buf_offset - The byte offset into a buffer described by buffer id. Not used in all modes.  
* @param   response_buffer_size - The number of bytes the clients has allocated for the response.
*
* @return  fbe_status_t 
*        FBE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   20-Nov-2008 PHE - Created.
***************************************************************************/
fbe_status_t fbe_eses_build_read_buf_cdb(fbe_u8_t *cdb, 
                                                fbe_u8_t cdb_buffer_size, 
                                                fbe_eses_read_buf_mode_t mode,
                                                fbe_u8_t buf_id,
                                                fbe_u32_t buf_offset,
                                                fbe_u32_t response_buffer_size)
{
    fbe_eses_read_buf_cdb_t *cdb_page;

    if(cdb_buffer_size < FBE_SCSI_READ_BUFFER_CDB_SIZE) {
        return (FBE_STATUS_GENERIC_FAILURE);
    }
    fbe_zero_memory(cdb, FBE_SCSI_READ_BUFFER_CDB_SIZE);
    cdb_page = (fbe_eses_read_buf_cdb_t *)cdb;
  
    cdb_page->operation_code = FBE_SCSI_READ_BUFFER;
    cdb_page->mode = mode;
    cdb_page->buf_id = buf_id;
    
    cdb_page->alloc_length_msbyte =  ((response_buffer_size & 0xFF0000) >> 16);
    cdb_page->alloc_length_midbyte = ((response_buffer_size & 0x00FF00) >> 8);; 
    cdb_page->alloc_length_lsbyte = (response_buffer_size & 0xFF);

    switch(mode)
    {
        case FBE_ESES_READ_BUF_MODE_DATA:
            cdb_page->buf_offset_msbyte =  ((buf_offset & 0xFF0000) >> 16);
            cdb_page->buf_offset_midbyte = ((buf_offset & 0x00FF00) >> 8);; 
            cdb_page->buf_offset_lsbyte = (buf_offset & 0xFF);
            break;

        case FBE_ESES_READ_BUF_MODE_DESC:
            // In FBE_ESES_READ_BUF_MODE_DESC mode, EMA returns 4 bytes of a read buffer
            // descriptorfor the specified buffer.
            if(response_buffer_size < FBE_ESES_READ_BUF_DESC_SIZE)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        default:
            break;
    }
 
    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_build_mode_sense_10_cdb(fbe_u8_t *cdb, 
*                                        fbe_u16_t cdb_buffer_size, 
*                                        fbe_u16_t cmd_buffer_size)               
***************************************************************************
* @brief
*       This function verifies the size of the CDB and builds the CDB for the 
*       "mode sense" command. This is the 10 bytes cdb.
*
* @param    cdb - The pointer to the cdb.
* @param    cdb_buffer_size - The buffer size of the cdb.
* @param    response_buffer_size - The response buffer size.
* @param    page_code - The page code that is requested.
*       
* @return  fbe_status_t 
*        FBE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
fbe_status_t fbe_eses_build_mode_sense_10_cdb(fbe_u8_t *cdb, 
                                                          fbe_u16_t cdb_buffer_size, 
                                                          fbe_u16_t response_buffer_size,
                                                          ses_pg_code_enum page_code)
{
    fbe_eses_mode_sense_10_cdb_t *cdb_page;

    if(cdb_buffer_size < FBE_SCSI_MODE_SENSE_10_CDB_SIZE) {
        return (FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_zero_memory(cdb, FBE_SCSI_MODE_SENSE_10_CDB_SIZE);
    cdb_page = (fbe_eses_mode_sense_10_cdb_t *)cdb;    

    cdb_page->operation_code = FBE_SCSI_MODE_SENSE_10;
    cdb_page->pc = 0; // return current/saved values.
    cdb_page->pg_code = page_code;
    cdb_page->subpg_code = 0;   
    cdb_page->alloc_length_msbyte = ((response_buffer_size & 0xFF00) >> 8);
    cdb_page->alloc_length_lsbyte = (response_buffer_size & 0xFF);
    cdb_page->control = 0;

   return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_build_mode_select_10_cdb(fbe_u8_t *cdb, 
*                                        fbe_u16_t cdb_buffer_size, 
*                                        fbe_u16_t cmd_buffer_size)               
***************************************************************************
* @brief
*       This function verifies the size of the CDB and builds the CDB for the 
*       "mode select" command. This is the 10 bytes cdb.
*
* @param    cdb - The pointer to the cdb.
* @param    cdb_buffer_size - The buffer size of the cdb.
* @param    cmd_buffer_size - The number of bytes to be written to the EMA.
*       
* @return  fbe_status_t 
*        FBE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   03-Dec-2008 PHE - Created.
***************************************************************************/
fbe_status_t fbe_eses_build_mode_select_10_cdb(fbe_u8_t *cdb, 
                                                       fbe_u16_t cdb_buffer_size, 
                                                       fbe_u16_t cmd_buffer_size)
{
    fbe_eses_mode_select_10_cdb_t *cdb_page;

    if(cdb_buffer_size < FBE_SCSI_MODE_SELECT_10_CDB_SIZE) {
        return (FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_zero_memory(cdb, FBE_SCSI_MODE_SELECT_10_CDB_SIZE);
    cdb_page = (fbe_eses_mode_select_10_cdb_t *)cdb;

    cdb_page->operation_code = FBE_SCSI_MODE_SELECT_10;
    cdb_page->sp = 1; // return current/saved values.
    cdb_page->pf = 1;
    cdb_page->param_list_length_msbyte = ((cmd_buffer_size & 0xFF00) >> 8);
    cdb_page->param_list_length_lsbyte = (cmd_buffer_size & 0xFF);  

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_decode_trace_buf_action_status(fbe_eses_trace_buf_action_status_t * trace_buf_action_status)           
***************************************************************************
* @brief
*       This function decodes the trace_buf_action_status into a string.
*
* @param    trace_buf_action_status 
*       
* @return  char * - The pointer to the string. 
*
* NOTES
*
* HISTORY
*   27-Jan-2009 PHE - Created.
***************************************************************************/
char * fbe_eses_decode_trace_buf_action_status(fbe_eses_trace_buf_action_status_t trace_buf_action_status)
{
    switch (trace_buf_action_status)
    {
        case FBE_ESES_TRACE_BUF_ACTION_STATUS_ACTIVE:
            return("ACTIVE");
            break;
        case FBE_ESES_TRACE_BUF_ACTION_STATUS_UNEXPECTED_EXCEPTION:
            return("UNEXPECTED_EXCEPTION"); 
            break;
        case FBE_ESES_TRACE_BUF_ACTION_STATUS_ASSERTION_FAILURE_RESET:
            return("ASSERTION_FAILURE_RESET");
            break;
        case FBE_ESES_TRACE_BUF_ACTION_STATUS_CLIENT_INIT_SAVE_BUF:
            return("CLIENT_INIT_SAVE_BUF");
            break;
        case FBE_ESES_TRACE_BUF_ACTION_STATUS_RESET_ISSUED:
            return("RESET_ISSUED");
            break;
        case FBE_ESES_TRACE_BUF_ACTION_STATUS_WATCHDOG_FIRED:
            return("WATCHDOG_FIRED");
            break;
        case FBE_ESES_TRACE_BUF_ACTION_STATUS_NMI_OCCURRED:
            return("NMI_OCCURRED");
            break;
        case FBE_ESES_TRACE_BUF_ACTION_STATUS_ASSERTION_FAILURE_NO_RESET:
            return("ASSERTION_FAILURE_NO_RESET");
            break;
        case FBE_ESES_TRACE_BUF_ACTION_STATUS_UNUSED_BUF:
            return("UNUSED_BUF");
            break;
        default:
            return("UNKNOWN");
    }// End of switch
}// End of function fbe_eses_decode_trace_buf_action_status


/*!*************************************************************************
* @fn fbe_status_t fbe_eses_build_write_buf_cdb(fbe_u8_t *cdb, 
*                                                fbe_u8_t cdb_buffer_size, 
*                                                fbe_eses_write_buf_mode_t mode,
*                                                fbe_u8_t buf_id,
*                                                fbe_u32_t buf_offset,
*                                                fbe_u32_t response_buffer_size)        
***************************************************************************
* @brief
*       This function verifies the size of the CDB and builds the CDB to 
*       send "WRITE BUFFER" command from the EMA in the LCC to write a buffer 
*       in the enclosure. 
*
* @param   cdb - The pointer to the cdb.
* @param   cdb_buffer_size - The buffer size of the cdb.
* @param   mode - The mode of the write buffer command.
* @param   buf_id  - The identity of the buffer to write. 
* @param   data_offset - The date byte offset described by buffer id. Not used in all modes.  
* @param   data_buf_size - The number of bytes the client has allocated for the request.
*
* @return  fbe_status_t 
*        FBE_STATUS_OK - no error.
*        otherwise - error is found.
*
* NOTES
*
* HISTORY
*   03-Feb-2009 AS - Created.
***************************************************************************/
fbe_status_t fbe_eses_build_write_buf_cdb(fbe_u8_t *cdb, 
                                                fbe_u8_t cdb_buffer_size, 
                                                fbe_eses_write_buf_mode_t mode,
                                                fbe_u8_t buf_id,
                                                fbe_u32_t data_offset,
                                                fbe_u32_t data_buf_size)
{
    fbe_eses_write_buf_cdb_t *cdb_page;

    if(cdb_buffer_size < FBE_SCSI_WRITE_BUFFER_CDB_SIZE)
    {
        return (FBE_STATUS_GENERIC_FAILURE);
    }

    fbe_zero_memory(cdb, FBE_SCSI_WRITE_BUFFER_CDB_SIZE);
    cdb_page = (fbe_eses_write_buf_cdb_t *)cdb;
  
    cdb_page->operation_code = FBE_SCSI_WRITE_BUFFER;
    cdb_page->mode = mode;
    cdb_page->buf_id = buf_id;
    
    cdb_page->param_list_length_msbyte =  ((data_buf_size & 0xFF0000) >> 16);
    cdb_page->param_list_length_midbyte = ((data_buf_size & 0x00FF00) >> 8);; 
    cdb_page->param_list_length_lsbyte = (data_buf_size & 0xFF);

    cdb_page->buf_offset_msbyte =  ((data_offset & 0xFF0000) >> 16);
    cdb_page->buf_offset_midbyte = ((data_offset & 0x00FF00) >> 8);; 
    cdb_page->buf_offset_lsbyte = (data_offset & 0xFF);

    return FBE_STATUS_OK;
}

// End of file fbe_eses_main.c

