#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"

/**************************************************************************
* fbe_payload_diplex_build_opeartion()                  
***************************************************************************
*
* DESCRIPTION
*       This function fill in diplex operation.
*
* PARAMETERS
*       diplex_operation - The pointer to the diplex_operation.
*       timeout - scsi timeout value.
*       rqst_length - commnad length.
*       usr_rqst_ptr - point the user request command.  If this pointer 
*                      is the same as diplex_buffer, meaning user already
*                      have the request setted up.
*       rsp_buffer_length - size of the response buffer.
*       usr_rsp_ptr - point to the buffer user provide for response.  If it's
*                     NULL, internally, we will use free space from diplex buffer
*                     to hold response.
*
* RETURN VALUES
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   2-Sept-2009 nchiu - Created.
***************************************************************************/
fbe_status_t 
fbe_payload_diplex_build_opeartion(fbe_payload_diplex_operation_t * diplex_operation, 
                                fbe_time_t timeout,
                                fbe_u8_t rqst_length,
                                fbe_u8_t *usr_rqst_ptr,
                                fbe_u8_t rsp_buffer_length,
                                fbe_u8_t *usr_rsp_ptr)
{
    if(rqst_length > FBE_PAYLOAD_DIPLEX_BUF_SIZE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    diplex_operation->timeout = timeout;

    /* we don't have to copy it again, if the same buffer already been filled by user */
    if (usr_rqst_ptr != diplex_operation->diplex_buffer)
    {
        fbe_copy_memory(diplex_operation->diplex_buffer, usr_rqst_ptr, rqst_length);
    }
    diplex_operation->cmd_length = rqst_length;

    /* use user provided response buffer if not NULL */
    if (usr_rsp_ptr != NULL)
    {
        diplex_operation->rsp_ptr = usr_rsp_ptr;
        diplex_operation->rsp_buffer_length = rsp_buffer_length;
    }
    else
    {
        diplex_operation->rsp_ptr = diplex_operation->diplex_buffer + rqst_length;
        diplex_operation->rsp_buffer_length = FBE_PAYLOAD_DIPLEX_BUF_SIZE - rqst_length;
    }

    if(diplex_operation->rsp_buffer_length < FBE_PAYLOAD_DIPLEX_MIN_BUF_SIZE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_diplex_set_request_status(fbe_payload_diplex_operation_t * diplex_operation, fbe_port_request_status_t request_status)
{
    diplex_operation->port_request_status = request_status;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_diplex_get_request_status(fbe_payload_diplex_operation_t * diplex_operation, fbe_port_request_status_t * request_status)
{
    *request_status = diplex_operation->port_request_status;
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_payload_diplex_operation_get_request_buffer(fbe_payload_diplex_operation_t * const diplex_operation, fbe_u8_t **rqst)
{
    *rqst = diplex_operation->diplex_buffer;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_diplex_operation_get_response_buffer(fbe_payload_diplex_operation_t * const diplex_operation, fbe_u8_t **rsp)
{
    *rsp = diplex_operation->rsp_ptr;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_diplex_set_transferred_count(fbe_payload_diplex_operation_t * diplex_operation, fbe_u32_t transferred_count)
{
    diplex_operation->transferred_count = transferred_count;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_diplex_get_transferred_count(fbe_payload_diplex_operation_t * diplex_operation, fbe_u32_t * transferred_count)
{
    *transferred_count = diplex_operation->transferred_count;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_payload_diplex_operation_get_response_data(fbe_payload_diplex_operation_t * const diplex_operation, 
                                               fbe_u8_t rsp_buf_length, 
                                               fbe_u8_t *rsp_ptr)
{
    if(diplex_operation->transferred_count > rsp_buf_length)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* we don't have to copy it again, if the same buffer already been filled by user */
    if (rsp_ptr != diplex_operation->rsp_ptr)
    {
        fbe_copy_memory(rsp_ptr, diplex_operation->rsp_ptr, diplex_operation->transferred_count);
    }

    return FBE_STATUS_OK;
}

