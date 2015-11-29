#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"
#include "fbe_scsi.h"
#include "fbe/fbe_payload_cdb_fis.h"
#include "fbe/fbe_drive_configuration_interface.h"

static fbe_status_t 
fbe_payload_cdb_process_scsi_status(fbe_payload_cdb_operation_t * cdb_operation, /* IN */
                                    fbe_drive_configuration_record_t * threshold_rec, /* IN */
                                    fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                    fbe_payload_cdb_fis_error_t * error_type, /* OUT */
                                    fbe_lba_t * bad_lba); /* OUT */

static fbe_status_t 
fbe_payload_cdb_process_data_underrun(fbe_payload_cdb_operation_t * cdb_operation, /* IN */
                                      fbe_drive_configuration_record_t * threshold_rec, /* IN */
                                      fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                      fbe_payload_cdb_fis_error_t * error_type); /* OUT */

static fbe_status_t 
fbe_payload_cdb_process_scsi_check_condition(fbe_payload_cdb_operation_t * cdb_operation, /* IN */
                                             fbe_drive_configuration_record_t * threshold_rec, /* IN */
                                             fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                             fbe_payload_cdb_fis_error_t * error_type, /* OUT */
                                             fbe_lba_t * bad_lba); /* OUT */

fbe_status_t
fbe_payload_cdb_build_read_capacity_10(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;

    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 10;
    /* OPERATION CODE (25h)*/
    cdb_operation->cdb[0] = FBE_SCSI_READ_CAPACITY;

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_cdb_build_read_capacity_16(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;

    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 16;
    /* OPERATION CODE (9Eh)*/
    cdb_operation->cdb[0] = FBE_SCSI_READ_CAPACITY_16;
    /* Service Action */
    cdb_operation->cdb[1] = 0x10;
    /* Allocation Length */
    cdb_operation->cdb[13] = FBE_SCSI_READ_CAPACITY_DATA_SIZE_16;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_cdb_build_tur(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER;


    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (00h)*/
    cdb_operation->cdb[0] = FBE_SCSI_TEST_UNIT_READY;

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_cdb_build_start_stop_unit(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_bool_t start)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER;


    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (1Bh)*/
    cdb_operation->cdb[0] = FBE_SCSI_START_STOP_UNIT;
    cdb_operation->cdb[1] = 1;
    if (start)
    {
        /* start unit */
        cdb_operation->cdb[4] = 1;
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_cdb_build_mode_sense(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_bool_t get_default)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (1Ah)*/
    cdb_operation->cdb[0] = FBE_SCSI_MODE_SENSE_6;
    if (get_default)
    {
        /* default mode pages */
        cdb_operation->cdb[2] = 0xBF;
    } 
    else
    {
        cdb_operation->cdb[2] = 0xFF;
    }
    cdb_operation->cdb[4] = 0xFF;

    return FBE_STATUS_OK;
}


fbe_status_t
fbe_payload_cdb_build_mode_sense_page(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout,
                                      fbe_u8_t page)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
    cdb_operation->timeout = timeout;

    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (1Ah)*/
    cdb_operation->cdb[0] = FBE_SCSI_MODE_SENSE_6;
    /* Do not return block descriptor information */
    cdb_operation->cdb[1] = FBE_SCSI_MODE_SENSE_RETURN_NO_BLOCK_DESCRIPTOR;
    /* Page code */
    cdb_operation->cdb[2] = page;
    /* Subpage code */
    if (page == 0x19)
    {
        cdb_operation->cdb[3] = FBE_SCSI_MODE_SENSE_SUBPAGE_0x01;
    }
    /* Maximum size */
    cdb_operation->cdb[4] = FBE_SCSI_MAX_MODE_SENSE_SIZE;

    return FBE_STATUS_OK;
}


fbe_status_t
fbe_payload_cdb_build_page_0x19_mode_sense(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (1Ah)*/
    cdb_operation->cdb[0] = FBE_SCSI_MODE_SENSE_6;
    /* Do not return block descriptor information */
    cdb_operation->cdb[1] = FBE_SCSI_MODE_SENSE_RETURN_NO_BLOCK_DESCRIPTOR;
    /* Ask for current mode page 0x19 with SAS PHY information */
    cdb_operation->cdb[2] = FBE_SCSI_MODE_SENSE_REQUEST_PAGE_0x19;
    /* Ask for subpage code 0x01 which is the long format of the page */
    cdb_operation->cdb[3] = FBE_SCSI_MODE_SENSE_SUBPAGE_0x01;    
    /* Maximum size -- we will underrun */
    cdb_operation->cdb[4] = FBE_SCSI_MAX_MODE_SENSE_SIZE;
    /* cdb[5] has been cleared to zeros above */    

    return FBE_STATUS_OK;
}


fbe_status_t
fbe_payload_cdb_build_mode_select(fbe_payload_cdb_operation_t * cdb_operation, 
                           fbe_time_t timeout,
                           fbe_u32_t data_length)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;


    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (15h)*/
    cdb_operation->cdb[0] = FBE_SCSI_MODE_SELECT_6;
    cdb_operation->cdb[1] = 0x11;
    cdb_operation->cdb[4] = data_length & 0xFF;

    return FBE_STATUS_OK;
}

/**************************************************************************
* fbe_payload_cdb_build_mode_sense_10()                  
***************************************************************************
*
* DESCRIPTION
*       This function fill in CDB operation for mode_sense_10
*
* PARAMETERS
*       cdb_operation - The pointer to the cdb_operation.
*       timeout - scsi timeout value.
*       get_default - get default pages.
*       buffer_size - buffer size.
*
* RETURN VALUES
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   01-Sept-2010 chenl6 - Created.
***************************************************************************/
fbe_status_t
fbe_payload_cdb_build_mode_sense_10(fbe_payload_cdb_operation_t * cdb_operation, 
                                    fbe_time_t timeout, 
                                    fbe_bool_t get_default,
                                    fbe_u16_t buffer_size)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;
    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 10;
    /* OPERATION CODE (5Ah)*/
    cdb_operation->cdb[0] = FBE_SCSI_MODE_SENSE_10;
    if (get_default)
    {
        /* default mode pages */
        cdb_operation->cdb[2] = 0xBF;
    } 
    else
    {
        cdb_operation->cdb[2] = 0xFF;
    }
    cdb_operation->cdb[7] = ((buffer_size & 0xFF00) >> 8);
    cdb_operation->cdb[8] = (buffer_size & 0xFF);

    return FBE_STATUS_OK;
}

/**************************************************************************
* fbe_payload_cdb_build_mode_sense_10()                  
***************************************************************************
*
* DESCRIPTION
*       This function fill in CDB operation for mode_select_10
*
* PARAMETERS
*       cdb_operation - The pointer to the cdb_operation.
*       timeout - scsi timeout value.
*       data_length - data length.
*
* RETURN VALUES
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   01-Sept-2010 chenl6 - Created.
***************************************************************************/
fbe_status_t
fbe_payload_cdb_build_mode_select_10(fbe_payload_cdb_operation_t * cdb_operation, 
                           fbe_time_t timeout,
                           fbe_u32_t data_length)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_OUT;
    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 10;
    /* OPERATION CODE (55h)*/
    cdb_operation->cdb[0] = FBE_SCSI_MODE_SELECT_10;
    cdb_operation->cdb[1] = 0x11;
    cdb_operation->cdb[7] = ((data_length & 0xFF00) >> 8);
    cdb_operation->cdb[8] = (data_length & 0xFF);

    return FBE_STATUS_OK;
}

/**************************************************************************
* fbe_payload_cdb_set_parameters()                  
***************************************************************************
*
* DESCRIPTION
*       This function fill in CDB operation without touching cdb data itself
*
* PARAMETERS
*       cdb_operation - The pointer to the cdb_operation.
*       timeout - scsi timeout value.
*       attribute - cdb task attribute.
*       flags - cdb task flags.
*       cdb_length - cdb length.
*
* RETURN VALUES
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   30-Sept-2008 nchiu - Created.
***************************************************************************/
fbe_status_t 
fbe_payload_cdb_set_parameters(fbe_payload_cdb_operation_t * cdb_operation, 
                                fbe_time_t timeout,
                                fbe_payload_cdb_task_attribute_t attribute,
                                fbe_payload_cdb_flags_t flags,
                                fbe_u32_t cdb_length)
{
    if(cdb_length > FBE_PAYLOAD_CDB_CDB_SIZE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(cdb_operation->cdb, cdb_length);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = attribute;
    cdb_operation->payload_cdb_flags = flags;
    cdb_operation->timeout = timeout;
    cdb_operation->cdb_length = cdb_length;

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_cdb_build_inquiry(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;


    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (12h)*/
    cdb_operation->cdb[0] = FBE_SCSI_INQUIRY;

    /* ALLOCATION LENGTH */
    cdb_operation->cdb[3] = 0; /* MSB */
    cdb_operation->cdb[4] = FBE_SCSI_INQUIRY_DATA_SIZE; /* LSB */

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_cdb_build_vpd_inquiry(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u8_t page_code)
{
    fbe_zero_memory(cdb_operation->cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
    fbe_zero_memory(cdb_operation->sense_info_buffer, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);

    cdb_operation->payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    cdb_operation->payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;

    cdb_operation->timeout = timeout;

    /* constract cdb */
    cdb_operation->cdb_length = 6;
    /* OPERATION CODE (12h)*/
    cdb_operation->cdb[0] = FBE_SCSI_INQUIRY;
    /* EVPD */
    cdb_operation->cdb[1] = 1;
    /* PAGE CODE (00h, 83h, 86h or C0h - DFh) */
    cdb_operation->cdb[2] = page_code;
    /* ALLOCATION LENGTH */
    cdb_operation->cdb[3] = 0; /* MSB */
    cdb_operation->cdb[4] = FBE_SCSI_INQUIRY_DATA_SIZE; /* LSB */

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_cdb_set_request_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_port_request_status_t request_status)
{
    cdb_operation->port_request_status = request_status;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_cdb_get_request_status(const fbe_payload_cdb_operation_t * cdb_operation, fbe_port_request_status_t * request_status)
{
    *request_status = cdb_operation->port_request_status;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_cdb_set_recovery_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_port_recovery_status_t recovery_status)
{
    cdb_operation->port_recovery_status = recovery_status;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_cdb_get_recovery_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_port_recovery_status_t * recovery_status)
{
    *recovery_status = cdb_operation->port_recovery_status;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_cdb_set_scsi_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_payload_cdb_scsi_status_t scsi_status)
{
    cdb_operation->payload_cdb_scsi_status = scsi_status;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_cdb_get_scsi_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_payload_cdb_scsi_status_t * scsi_status)
{
    *scsi_status = cdb_operation->payload_cdb_scsi_status;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_cdb_operation_get_cdb(fbe_payload_cdb_operation_t * const cdb_operation, fbe_u8_t **cdb)
{
    *cdb = cdb_operation->cdb;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_cdb_operation_get_sense_buffer(fbe_payload_cdb_operation_t * const cdb_operation, fbe_u8_t **sense_buffer)
{
    *sense_buffer = cdb_operation->sense_info_buffer;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_cdb_set_transfer_count(fbe_payload_cdb_operation_t * cdb_operation, fbe_u32_t transfer_count)
{
    cdb_operation->transfer_count = transfer_count;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_cdb_get_transfer_count(fbe_payload_cdb_operation_t * cdb_operation, fbe_u32_t * transfer_count)
{
    *transfer_count = cdb_operation->transfer_count;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_cdb_set_transferred_count(fbe_payload_cdb_operation_t * cdb_operation, fbe_u32_t transferred_count)
{
    cdb_operation->transferred_count = transferred_count;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_cdb_get_transferred_count(fbe_payload_cdb_operation_t * cdb_operation, fbe_u32_t * transferred_count)
{
    *transferred_count = cdb_operation->transferred_count;
    return FBE_STATUS_OK;
}

/****************************************************************
 * fbe_get_cdb_lba()
 ****************************************************************
 * DESCRIPTION:
 *  This function extracts the lba from a cdb.  
 *
 * PARAMETERS:
 *  payload_cdb_operation - CDB packet payload
 *
 * RETURNS:
 *  The lba value. FBE_LBA_INVALID is return on error.
 *
 * HISTORY:
 *  09/26/12  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_lba_t
fbe_get_cdb_lba(const fbe_payload_cdb_operation_t * payload_cdb_operation)
{
    fbe_lba_t lba = FBE_LBA_INVALID;  

    if(payload_cdb_operation->cdb_length == 6)
    {
        lba = fbe_get_six_byte_cdb_lba(&payload_cdb_operation->cdb[0]);
    }
    else if (payload_cdb_operation->cdb_length == 10)
    {
        lba = fbe_get_ten_byte_cdb_lba(&payload_cdb_operation->cdb[0]);
    }
    else if (payload_cdb_operation->cdb_length == 16)
    {
        lba = fbe_get_sixteen_byte_cdb_lba(&payload_cdb_operation->cdb[0]);
    }   
    else
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Invalid cdb length %d\n", 
                                 __FUNCTION__, payload_cdb_operation->cdb_length);
        lba = FBE_LBA_INVALID;
    }

    return lba;
}

/****************************************************************
 * fbe_get_cdb_blocks()
 ****************************************************************
 * DESCRIPTION:
 *  This function extracts the block count from a cdb.  
 *
 * PARAMETERS:
 *  payload_cdb_operation - CDB packet payload
 *
 * RETURNS:
 *  The number of blocks.   FBE_CDB_BLOCKS_INVALID is return on error.
 *
 * HISTORY:
 *  09/26/12  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_block_count_t
fbe_get_cdb_blocks(const fbe_payload_cdb_operation_t * payload_cdb_operation)
{
    fbe_block_count_t blocks = FBE_CDB_BLOCKS_INVALID;  

    if(payload_cdb_operation->cdb_length == 6)
    {
        blocks = fbe_get_six_byte_cdb_blocks(&payload_cdb_operation->cdb[0]);
    }
    else if (payload_cdb_operation->cdb_length == 10)
    {
        blocks = fbe_get_ten_byte_cdb_blocks(&payload_cdb_operation->cdb[0]);
    }
    else if (payload_cdb_operation->cdb_length == 16)
    {
        blocks = fbe_get_sixteen_byte_cdb_blocks(&payload_cdb_operation->cdb[0]);
    }   
    else
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Invalid cdb length %d\n", 
                                 __FUNCTION__, payload_cdb_operation->cdb_length);
        blocks = FBE_CDB_BLOCKS_INVALID;
    }

    return blocks;
}

/* Deprecated functions */
/****************************************************************
 * fbe_get_six_byte_cdb_lba()
 ****************************************************************
 * DESCRIPTION:
 *  This function extracts the lba from a 6 byte cdb.
 *
 * PARAMETERS:
 *  cdb_p, [I] - The cdb to get the lba from.
 *
 * RETURNS:
 *  The lba value.
 *
 * HISTORY:
 *  11/13/07 - Created. RPF
 *
 ****************************************************************/
fbe_lba_t 
fbe_get_six_byte_cdb_lba(const fbe_u8_t *const cdb_p)
{
    fbe_lba_t lba;
    
    lba = (fbe_lba_t) cdb_p[1] & 0x1F;
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[2];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[3];
    return lba;
}
/* end fbe_get_six_byte_cdb_lba() */

/****************************************************************
 * fbe_get_six_byte_cdb_blocks()
 ****************************************************************
 * DESCRIPTION:
 *  This function extracts the block count from a 6 byte cdb.
 *
 * PARAMETERS:
 *  cdb_p, [I] - The cdb to get the block count from.
 *
 * RETURNS:
 *  The block count value.
 *
 * HISTORY:
 *  11/13/07 - Created. RPF
 *
 ****************************************************************/
fbe_block_count_t 
fbe_get_six_byte_cdb_blocks(const fbe_u8_t *const cdb_p)
{
    fbe_block_count_t blocks;
    blocks = cdb_p[4];
    return blocks;
}
/* end fbe_get_six_byte_cdb_blocks() */

/****************************************************************
 * fbe_get_ten_byte_cdb_lba()
 ****************************************************************
 * DESCRIPTION:
 *  This function extracts the lba from a 10 byte cdb.
 *
 * PARAMETERS:
 *  cdb_p, [I] - The cdb to get the lba from.
 *
 * RETURNS:
 *  The lba value.
 *
 * HISTORY:
 *  11/13/07 - Created. RPF
 *
 ****************************************************************/
fbe_lba_t 
fbe_get_ten_byte_cdb_lba(const fbe_u8_t *const cdb_p)
{
    fbe_lba_t lba;
    
    lba = (fbe_lba_t) cdb_p[2];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[3];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[4];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[5];
    return lba;
}
/* end fbe_get_ten_byte_cdb_lba() */

/****************************************************************
 * fbe_get_ten_byte_cdb_blocks()
 ****************************************************************
 * DESCRIPTION:
 *  This function extracts the block count from a 10 byte cdb.
 *
 * PARAMETERS:
 *  cdb_p, [I] - The cdb to get the block count from.
 *
 * RETURNS:
 *  The block count value.
 *
 * HISTORY:
 *  11/13/07 - Created. RPF
 *
 ****************************************************************/
fbe_block_count_t 
fbe_get_ten_byte_cdb_blocks(const fbe_u8_t *const cdb_p)
{
    fbe_block_count_t blocks;
    blocks = (cdb_p[7] << 8) | cdb_p[8];
    return blocks;
}
/* end fbe_get_ten_byte_cdb_blocks() */

/****************************************************************
 * fbe_set_ten_byte_cdb_blocks()
 ****************************************************************
 * DESCRIPTION:
 *  This function sets the block count from a 10 byte cdb.
 *
 * PARAMETERS:
 *  cdb_p, [I] - The cdb to set the block count to.
 *
 * RETURNS:
 *  The block count value.
 *
 * HISTORY:
 *  06/07/12 - Created. Vamsi V
 *
 ****************************************************************/
void 
fbe_set_ten_byte_cdb_blocks(fbe_u8_t * cdb_p, fbe_block_count_t blk_cnt)
{
	cdb_p[7] = (fbe_u8_t)((blk_cnt >> 8) & 0xFF); /* MSB */
	cdb_p[8] = (fbe_u8_t)(blk_cnt & 0xFF); /* LSB */
	return;
}
/* end fbe_set_ten_byte_cdb_blocks() */


fbe_status_t 
fbe_payload_cdb_completion(fbe_payload_cdb_operation_t * cdb_operation, /* IN */
                           fbe_drive_configuration_record_t * threshold_rec, /* IN */
                           fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                           fbe_payload_cdb_fis_error_t * error_type, /* OUT */
                           fbe_lba_t * bad_lba) /* OUT */
{
    fbe_port_request_status_t cdb_request_status;
    fbe_status_t status = FBE_STATUS_OK;

    *bad_lba = FBE_LBA_INVALID;
    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;

    fbe_payload_cdb_get_request_status(cdb_operation, &cdb_request_status);

    switch (cdb_request_status)
    {
        case FBE_PORT_REQUEST_STATUS_SUCCESS:
            status = fbe_payload_cdb_process_scsi_status(cdb_operation, threshold_rec, io_status, error_type, bad_lba); 
            break;
            
        case FBE_PORT_REQUEST_STATUS_INVALID_REQUEST:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            break;
            
        case FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;
            
        case FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;
            
        case FBE_PORT_REQUEST_STATUS_BUSY:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;
            
        case FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            break;
            
        case FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT:
        case FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT:            
        case FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR:
            /* Check port and enclosure error stat and make a decision */
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK;
            break;
            
        case FBE_PORT_REQUEST_STATUS_DATA_OVERRUN:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            break;

        case FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN:            
            status = fbe_payload_cdb_process_data_underrun(cdb_operation, threshold_rec, io_status, error_type);                        
            break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE:        
        case FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT: /*aborted by miniport due to a severe error*/
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE:        
            /* Check port and enclosure error stat and make a decision */
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK;
            break;

        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_BAD_HANDLE:
        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_KEY_WRAP_ERROR:
        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_NOT_ENABLED:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_PORT;
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

static fbe_status_t 
fbe_payload_cdb_process_scsi_status(fbe_payload_cdb_operation_t * cdb_operation, /* IN */
                                    fbe_drive_configuration_record_t * threshold_rec, /* IN */    
                                    fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                    fbe_payload_cdb_fis_error_t * error_type, /* OUT */
                                    fbe_lba_t * bad_lba) /* OUT */
{
    fbe_payload_cdb_scsi_status_t cdb_scsi_status;
    fbe_status_t status = FBE_STATUS_OK;

    /* We have a valid request status, so let's look at scsi status */
    fbe_payload_cdb_get_scsi_status(cdb_operation, &cdb_scsi_status);

    switch (cdb_scsi_status) {
        case FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION:
            status = fbe_payload_cdb_process_scsi_check_condition(cdb_operation, threshold_rec, io_status, error_type, bad_lba); 
            break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_BUSY:
        case FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;

        default: 
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_UKNOWN;
    } /* end of Switch */
        
    return status;
}

/* NOTE: data underruns are expected for many unrecovered errors. */
static fbe_status_t 
fbe_payload_cdb_process_data_underrun(fbe_payload_cdb_operation_t * cdb_operation, /* IN */
                                      fbe_drive_configuration_record_t * threshold_rec, /* IN */
                                      fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                      fbe_payload_cdb_fis_error_t * error_type) /* OUT */
{
    fbe_payload_cdb_scsi_status_t cdb_scsi_status;
    fbe_status_t status = FBE_STATUS_OK; 
    fbe_lba_t bad_lba = FBE_LBA_INVALID;
    
    // fetch SCSI status
    fbe_payload_cdb_get_scsi_status(cdb_operation, &cdb_scsi_status);
    
    switch (cdb_scsi_status) {        
        case FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION:
            // AR 416493 - if SCSI check condition is detected, use DIEH mapping based on check condition
            status = fbe_payload_cdb_process_scsi_check_condition(cdb_operation, threshold_rec, io_status, error_type, &bad_lba); 
            break;        
                    
        case FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT:
            /* a reservation conflict is not a link error.  This is expected when peer is remapping. Set to retry/no_error*/
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;

        default: 
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK;
    } 
    
    

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_payload_cdb_process_scsi_check_condition(fbe_payload_cdb_operation_t * cdb_operation, /* IN */                                            
                                             fbe_drive_configuration_record_t * threshold_rec, /* IN */
                                             fbe_payload_cdb_fis_io_status_t * io_status, /* OUT */
                                             fbe_payload_cdb_fis_error_t * error_type, /* OUT */
                                             fbe_lba_t * bad_lba) /* OUT */
{

	fbe_u8_t * sense_data = NULL;
    fbe_scsi_sense_key_t sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE; 
    fbe_scsi_additional_sense_code_t asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO; 
    fbe_scsi_additional_sense_code_qualifier_t ascq = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
    fbe_bool_t lba_is_valid = FBE_FALSE;
#if 0
    fbe_scsi_error_code_t error = 0;
#endif
	fbe_u8_t *cdb;
	fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;

 
	fbe_payload_cdb_operation_get_sense_buffer(cdb_operation, &sense_data);
	fbe_payload_cdb_operation_get_cdb(cdb_operation, &cdb);

    status = fbe_payload_cdb_parse_sense_data(sense_data, &sense_key, &asc, &ascq, &lba_is_valid, bad_lba);

    /* Verify that the Request Sense data is valid. */
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {    
        /* Sense data is not valid - set error conditions */
		*io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
		*error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
        return FBE_STATUS_OK;
    }

	/*at this point, before we make the mapping, we want to make sure there are no exceptions in the table,
     * these would take presedence over the hard coded onces
     */
	status = fbe_drive_configuration_get_scsi_exception_action(threshold_rec,
															   cdb_operation,
															   io_status,
															   error_type);

	/*if we had an error, let's try the hard coded mapping so we can at least try and make some sense of the error*/
    if (status != FBE_STATUS_CONTINUE && status != FBE_STATUS_GENERIC_FAILURE)
    {
		return status;/*we are done and had a match*/
	}


    /* Examine the sense data to determine the appropriate error status. */
    switch (sense_key)
    {
        case FBE_SCSI_SENSE_KEY_NO_SENSE:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            break;

        case FBE_SCSI_SENSE_KEY_RECOVERED_ERROR:

            /* Some drives may have auto reallocation enabled, which means
             * they will do remaps automatically when they decide it's
             * necessary.  They report a recovered error when this occurs.
             * If it does happen, we want to be certain that Flare does NOT
             * tell the drive to do a remap again!
             * Before we enter the inner switch (for ASC), first test
             * for an assortment of ASC/ASCQ combinations which indicates the
             * drive did a successful auto remap.
             */
            if (((asc == FBE_SCSI_ASC_WRITE_ERROR)               && (ascq == 1)) ||
                ((asc == FBE_SCSI_ASC_RECORD_MISSING)            && (ascq == 6)) ||
                ((asc == FBE_SCSI_ASC_DATA_SYNC_MARK_MISSING)    && (ascq == 3)) ||
                ((asc == FBE_SCSI_ASC_RECOVERED_WITH_RETRIES)    && (ascq == 6)) ||
                ((asc == FBE_SCSI_ASC_RECOVERED_WITH_ECC)        && (ascq == 2)))
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED;
                (*error_type) |= FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_REMAPPED;
                break;                /* break out of sense_key switch */
            }

            /* The recovered error was something other than an auto-remap. */
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED;

            switch (asc)
            {
                case FBE_SCSI_ASC_DEFECT_LIST_ERROR:
                case FBE_SCSI_ASC_PRIMARY_LIST_MISSING:
                    (*error_type) |= FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE;
                    break;

                case FBE_SCSI_ASC_PFA_THRESHOLD_REACHED:
                    if (fbe_dcs_param_is_enabled(FBE_DCS_PFA_HANDLING)){
                        (*error_type) |= FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE;
                    }
                    else{
                        fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                 "%s %02x|%02x|%02x PFA HANDLING DISABLED\n", __FUNCTION__, sense_key, asc, ascq); 
                    }
                    break;

                case FBE_SCSI_ASC_WARNING:
                    /* 01/0B/01 reports TEMPERATURE_EXCEEDED. */
                    if (ascq == 0x01) {
                        if (fbe_dcs_param_is_enabled(FBE_DCS_PFA_HANDLING)){
                           (*error_type) |= FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE;
                        }
                        else{
                            fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                     "%s %02x|%02x|%02x  PFA HANDLING DISABLED\n", __FUNCTION__, sense_key, asc, ascq); 
                        }
                    }
                    break;

                case FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR:
                    if (ascq == 0x30) {
                        /* For FSSD drives, 01/80/30 reports Super Capacitor Failure. */
                        (*error_type) |= FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE;
                    } else if (ascq == 0x33 || ascq == 0x4B){
                        /* For FSSD drives, 01/80/33, 01/80/4B report SMART data. */   
                        if (fbe_dcs_param_is_enabled(FBE_DCS_PFA_HANDLING)){                 
                            (*error_type) |= FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE;
                        }
                        else{
                            fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                     "%s %02x|%02x|%02x  PFA HANDLING DISABLED\n", __FUNCTION__, sense_key, asc, ascq); 
                        }
                    }
                    break;
            }
            break;

        case FBE_SCSI_SENSE_KEY_NOT_READY:

            if ((asc == FBE_SCSI_ASC_NOT_READY) && 
                (ascq == FBE_SCSI_ASCQ_BECOMING_READY) || 
                (ascq == FBE_SCSI_ASCQ_NOTIFY_ENABLE_SPINUP))
            {
                /* This is the "Not Ready, In the process of becoming ready,
                 * or waiting for a an enable spinup primitive"
                 * check condition.  It occurs during drive spinup, as well
                 * as when going into spindle synch mode on IBM drives.
                 */
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            }
            else if ((asc == FBE_SCSI_ASC_NOT_READY) && (ascq == FBE_SCSI_ASCQ_NOT_SPINNING))
            {
                /* This is the "Not Ready, Initializing Command Required"
                 * check condition. It specifically tells us that the drive
                 * is not spinning.
                 */
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NOT_SPINNING;
            }
#if 0 /* We need to understand what to do with this */
            else if ((asc == FBE_SCSI_ASC_NOT_READY) && (ascq == 0x05))
            {
                /* For FSSD drives 02/04/05 reports a Drive Table Rebuild Error */
                error = FBE_SCSI_CC_DRIVE_TABLE_REBUILD;
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL;
            }            
#endif
            else if (asc == FBE_SCSI_ASC_FORMAT_CORRUPTED)
            {
                /* This is the "Not Ready because of bad format" check
                 * condition.  There are a couple of possible ASCQ values;
                 * but they all tell us that the drive needs to be formatted.
                 */
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL;
            }
#if 0
            else if (asc == FBE_SCSI_ASC_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION)
            {
                /* JO, 5-15-07. This has been added primarily to support Bigfoot/Mamba
                 * SATA drives. This error is returned by the LSI controller if LSI has 
                 * attempted initialization to a SATA and retried four times to no avail. 
                */
                error = FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION;    
            }
#endif
            else
            {
                /* All other "NOT READY"s:  Unfortunately, we can't tell
                 * whether or not the disk is spinning, if the drive was never
                 * started or if it was running before and then died, or if
                 * the NOT READY is a temporary or permanent condition.  Return
                 * a generic NOT READY status.  The SCSI operation will likely
                 * be retried.
                 */
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            break;

        case FBE_SCSI_SENSE_KEY_MEDIUM_ERROR:
            if (asc == FBE_SCSI_ASC_FORMAT_CORRUPTED) {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL;
            }
            else if (asc == FBE_SCSI_ASC_DEFECT_LIST_ERROR ||
                     asc == FBE_SCSI_ASC_PRIMARY_LIST_MISSING ||
                     asc == FBE_SCSI_ASC_NO_SPARES_LEFT)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL;             
            }
            else if (lba_is_valid) {
                if(cdb[0] == FBE_SCSI_REASSIGN_BLOCKS){
                    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY;
                    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL;
                } else if ((asc == FBE_SCSI_ASC_WRITE_ERROR) && 
                    (ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER))
                {
                    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA;
                }
                else if (asc == FBE_SCSI_ASC_DEV_WRITE_FAULT)
                {
                    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA;
                }
#if 0
                else if (asc == FBE_SCSI_ASC_READ_DATA_ERROR && 
                         ascq == FBE_SCSI_ASCQ_SYNCH_FAIL)
                {
                    /* This is the Fujutsu Buffer CRC Error (03/11/02) case. */
                    error = NTBEProcessSpecialSenseData(PScb,
                                                        sense_data_ptr);
                }
#endif
                else
                {
                    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA;
                }
            }
            else {
                /* Drive did not report the bad LBA in the sense data. */
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA;
            }
            break;

        case FBE_SCSI_SENSE_KEY_HARDWARE_ERROR:

            if (asc == FBE_SCSI_ASC_SCSI_INTFC_PARITY_ERR) {
                /* Record a more specific error code for a parity error. */
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            else if (asc == FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_NO_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL;
            }
            else if (asc == FBE_SCSI_ASC_NO_SPARES_LEFT)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            else if (asc == FBE_SCSI_ASC_DEFECT_LIST_ERROR)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
                (*error_type) |= FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE;
            }
            else if (asc == FBE_SCSI_ASC_SEEK_ERROR)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            else if (asc == FBE_SCSI_ASC_DEV_WRITE_FAULT)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            else if (asc == FBE_SCSI_ASC_INTERNAL_TARGET_FAILURE)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            else
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            break;

        case FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST:

            if ((asc == FBE_SCSI_ASC_INVALID_LUN) && 
                (ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER))
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            else
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            break;

        case FBE_SCSI_SENSE_KEY_UNIT_ATTENTION:

            if (asc == FBE_SCSI_ASC_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK; 
            }
            else if (asc == FBE_SCSI_ASC_MODE_PARAM_CHANGED)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            }
            else if (asc == FBE_SCSI_ASC_SPINDLE_SYNCH_CHANGE)
            {
                if (ascq == FBE_SCSI_ASCQ_SYNCH_SUCCESS)
                {
                    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
                    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
                }
                else if (ascq == FBE_SCSI_ASCQ_SYNCH_FAIL)
                {
                    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
                    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
                }
            }
            else if (asc == FBE_SCSI_ASC_VENDOR_SPECIFIC_FF && 
                     (ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER || ascq == 0xFE))
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            else
            {
                    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
                    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
            }
            break;

        case FBE_SCSI_SENSE_KEY_VENDOR_SPECIFIC_09:
            if((asc == FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR) && 
               (ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER) )
            {
                    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                    *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            else
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            break;

        case FBE_SCSI_SENSE_KEY_ABORTED_CMD:
            if (asc == FBE_SCSI_ASC_SCSI_INTFC_PARITY_ERR ||
                asc == FBE_SCSI_ASC_DATA_PHASE_ERROR || 
                asc == FBE_SCSI_ASC_BRIDGED_DRIVE_ABORT_CMD_ERROR)
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK;
            }
            else
            {
                *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
                *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;
            }
            break;

        default:
            *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_FAIL_RETRY;
            *error_type = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE;

    }
    
    return FBE_STATUS_OK;
}


fbe_status_t fbe_payload_cdb_parse_sense_data(fbe_u8_t* sense_data,  /* IN */
                                              fbe_scsi_sense_key_t* sense_key,  /* OUT */
                                              fbe_scsi_additional_sense_code_t* asc,  /* OUT */
                                              fbe_scsi_additional_sense_code_qualifier_t* ascq,  /* OUT */
                                              fbe_bool_t*  lba_is_valid,  /* OUT */
                                              fbe_lba_t*  bad_lba)  /* OUT */
{
    fbe_status_t  status = FBE_STATUS_OK;

    /* All functions arguments except sense_data are outputs
     *  initialize approiately */
     *sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE;
     *asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
     *ascq = FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO;
     if (lba_is_valid != NULL)
     {
         *lba_is_valid = FBE_FALSE;
     }
     if (bad_lba != NULL)
     {
         *bad_lba = 0;
     }

    /* Verify that the request Sense data is valid. If not then return FBE_STATUS_GENERIC_FAILURE */
    if ((sense_data[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] & 0x70) != 0x70) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Invalid Data Response Code 0x%x\n", 
                                 __FUNCTION__, sense_data[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET]);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine if the request sense data is fixed format or descriptor format */
    if ((sense_data[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] & 0x02) == 0x02)
    {
         fbe_u8_t current_location = 0x00;
         fbe_u8_t descriptor_type = 0x00; 
         fbe_u8_t total_sense_length = 0x00;
         fbe_u8_t new_location = 0x00;
    
        /* The sense data is returned in descriptor format */
        /* Extract the sense_key, the asc and the ascq */
         *sense_key = sense_data[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET_DESCRIPTOR_FORMAT] & FBE_SCSI_SENSE_KEY_MASK;
         *asc = sense_data[FBE_SCSI_SENSE_DATA_ASC_OFFSET_DESCRIPTOR_FORMAT];
         *ascq = sense_data[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET_DESCRIPTOR_FORMAT];

        /* Now check to see if we have a valid bad LBA to report */
         total_sense_length = sense_data[0x07] + 8;
         
         /* Check that the total length of the returned sense data is <= 48 bytes
          *  as required by the EMC specification.  If not return FBE_STATUS_GENERIC_FAILURE
          *  to indicate that the sense data is not valid.
          */
         if (total_sense_length > 48)
         {
             fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Sense Length > 48. len:%d\n", 
                                 __FUNCTION__, total_sense_length);
             return FBE_STATUS_GENERIC_FAILURE;         
         }
          
         if (total_sense_length > 7)
         {
       
             /* There are 1 or more sense data descriptors in the returned data
              * Look at the beginning of the first one. */
              current_location = 0x08;

             while (current_location < total_sense_length)
             {

                 descriptor_type = sense_data[current_location];
                 /* information sense data descriptor */
                 if ( (descriptor_type == 0x00) && (bad_lba != NULL) && (lba_is_valid != NULL) )
                 {
                     /* The "information" (i.e. the bad LBA information field) field of the
                      *  sense data is valid and contains a bad LBA so report the bad LBA information */
                     *bad_lba = (fbe_lba_t)(sense_data[current_location + 4]) << 56 |            
                                (fbe_lba_t)(sense_data[current_location + 5]) << 48 |            
                                (fbe_lba_t)(sense_data[current_location + 6]) << 40 |
                                (fbe_lba_t)(sense_data[current_location + 7]) << 32 |                                     
                                (fbe_lba_t)(sense_data[current_location + 8]) << 24 |
                                (fbe_lba_t)(sense_data[current_location + 9]) << 16 |
                                (fbe_lba_t)(sense_data[current_location + 10]) <<  8 |
                                (fbe_lba_t)(sense_data[current_location + 11]);
                     *lba_is_valid = FBE_TRUE;
                     break;
                 }
                 /* Point to the next potential descriptor */                 
                 new_location = current_location + sense_data[current_location + 1] + 2;

                 /* Confirm that malformed sense information does not cause us to go in 
                  * an infinite loop.
                  */
                 if (new_location <= current_location){
                     /* New location should have moved us further in the sense buffer.
                      * If this is not happening, it is time to bail.
                      */
                    fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Invalid Data sense info. Offset New: 0x%x Old: 0x%x\n", 
                                             __FUNCTION__,new_location, current_location);
                    status = FBE_STATUS_GENERIC_FAILURE;
                    break;
                 }else{
                     current_location = new_location;
                 }
             }
         }
    }
    else
    {
        /* The sense data is returned in fixed format */
         *sense_key = sense_data[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] & FBE_SCSI_SENSE_KEY_MASK;
         *asc = sense_data[FBE_SCSI_SENSE_DATA_ASC_OFFSET];
         *ascq = sense_data[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET];
        /* Check to see if we have valid lba information */
         if ( ((sense_data[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] & 0x80) == 0x80) &&
              (bad_lba != NULL) && (lba_is_valid != NULL) )
         {
              /* The "information" (i.e. the bad LBA information field) field of the
               *  sense data is valid and contains a bad LBA so report the bad LBA information */
              *bad_lba = (fbe_lba_t)(sense_data[3]) << 24 |
                         (fbe_lba_t)(sense_data[4]) << 16 |
                         (fbe_lba_t)(sense_data[5]) <<  8 |
                         (fbe_lba_t)(sense_data[6]);
              *lba_is_valid = FBE_TRUE;
         }
    }

    return status;
}

/****************************************************************
 * fbe_payload_cdb_parse_sense_data_for_sanitization_percent()
 ****************************************************************
 * DESCRIPTION:
 *  This function extracts the santization percent from sense data.
 *  This is only valid for 02/04/04 errors.
 *
 * HISTORY:
 *  07/25/2014 - Created.   Wayne Garrett
 *
 ****************************************************************/
void fbe_payload_cdb_parse_sense_data_for_sanitization_percent(const fbe_u8_t* const sense_data, fbe_bool_t *is_sksv_set, fbe_u16_t *percent)
{
    if(sense_data[15] & 0x80)  /* Sense Key Specific Field (SKSV) bit */
    {
        *is_sksv_set = FBE_TRUE;

        (*percent)  = (fbe_u16_t)sense_data[16] << 8;
        (*percent) |= (fbe_u16_t)sense_data[17];
    }
    else
    {
        *is_sksv_set = FBE_FALSE;
    }           
}



/****************************************************************
 * fbe_get_sixteen_byte_cdb_lba()
 ****************************************************************
 * DESCRIPTION:
 *  This function extracts the lba from a 16 byte cdb.
 *
 * PARAMETERS:
 *  cdb_p, [I] - The cdb to get the lba from.
 *
 * RETURNS:
 *  The lba value.
 *
 * HISTORY:
 *  12/23/08 - Created. CLC
 *
 ****************************************************************/
fbe_lba_t 
fbe_get_sixteen_byte_cdb_lba(const fbe_u8_t *const cdb_p)
{
    fbe_lba_t lba;
    
    lba = (fbe_lba_t) cdb_p[2];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[3];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[4];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[5];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[6];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[7];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[8];
    lba <<= 8;
    lba |= (fbe_lba_t) cdb_p[9];
    return lba;
}
/* end fbe_get_sixteen_byte_cdb_lba() */

/****************************************************************
 * fbe_get_sixteen_byte_cdb_blocks()
 ****************************************************************
 * DESCRIPTION:
 *  This function extracts the block count from a 16 byte cdb.
 *
 * PARAMETERS:
 *  cdb_p, [I] - The cdb to get the block count from.
 *
 * RETURNS:
 *  The block count value.
 *
 * HISTORY:
 *  12/23/08 - Created. CLC
 *
 ****************************************************************/
fbe_block_count_t 
fbe_get_sixteen_byte_cdb_blocks(const fbe_u8_t *const cdb_p)
{
    fbe_block_count_t blocks;
    blocks = cdb_p[10];
    blocks <<= 8;
    blocks |= cdb_p[11];
    blocks <<= 8;
    blocks |= cdb_p[12];
    blocks <<= 8;
    blocks |= cdb_p[13];
    return blocks;
}
/* end fbe_get_sixteen_byte_cdb_blocks() */

/****************************************************************
 * fbe_set_sixteen_byte_cdb_blocks()
 ****************************************************************
 * DESCRIPTION:
 *  This function sets the block count from a 16 byte cdb.
 *
 * PARAMETERS:
 *  cdb_p, [I] - The cdb to set the block count to.
 *
 * RETURNS:
 *  The block count value.
 *
 * HISTORY:
 *  06/07/12 - Created. Vamsi V
 *
 ****************************************************************/
void 
fbe_set_sixteen_byte_cdb_blocks(fbe_u8_t * cdb_p, fbe_block_count_t blk_cnt)
{
	cdb_p[10] = (fbe_u8_t)((blk_cnt >> 24) & 0xFF);	/* MSB */
	cdb_p[11] = (fbe_u8_t)((blk_cnt >> 16) & 0xFF); 
	cdb_p[12] = (fbe_u8_t)((blk_cnt >> 8) & 0xFF);
	cdb_p[13] = (fbe_u8_t)(blk_cnt & 0xFF);	/* LSB */
	return;
}
/* end fbe_set_sixteen_byte_cdb_blocks() */

