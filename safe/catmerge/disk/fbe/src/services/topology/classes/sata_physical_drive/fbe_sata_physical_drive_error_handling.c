/**************************************************************************
*  
*  *****************************************************************
*  *** 2012_03_30 SATA DRIVES ARE FAULTED IF PLACED IN THE ARRAY ***
*  *****************************************************************
*      SATA drive support was added as black content to R30
*      but SATA drives were never supported for performance reasons.
*      The code has not been removed in the event that we wish to
*      re-address the use of SATA drives at some point in the future.
*
***************************************************************************/

#include "fbe_transport_memory.h"
#include "base_physical_drive_init.h"
#include "sata_physical_drive_private.h"
#include "base_physical_drive_private.h"

fbe_status_t 
fbe_sata_physical_drive_handle_block_operation_error(fbe_payload_ex_t * payload, fbe_port_request_status_t request_status)
{	
	fbe_payload_block_operation_t * block_operation = NULL;

	block_operation = fbe_payload_ex_get_prev_block_operation(payload);

    fbe_payload_block_set_status(block_operation,	FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
													FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
	switch(request_status)
    {
		case FBE_PORT_REQUEST_STATUS_INVALID_REQUEST:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_INVALIDREQUEST);
            break;
 
        case FBE_PORT_REQUEST_STATUS_BUSY:
        case FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_DEVICE_BUSY);
            break;

        case FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_DEVICE_NOT_PRESENT);
            break;

        case FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_BADBUSPHASE);
            break;

        case FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_IO_TIMEOUT_ABORT); //GJF Franklin had changed the error code to:  FBE_SCSI_SATA_IO_TIMEOUT_ABORT
            break;
        
        case FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR:
        case FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_SELECTIONTIMEOUT);
            break;   

        case FBE_PORT_REQUEST_STATUS_DATA_OVERRUN:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_TOOMUCHDATA);
            break;

        case FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_XFERCOUNTNOTZERO);
            break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_USERABORT);
		    
            break;

		case FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT:
			fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_INCIDENTAL_ABORT);
			break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE:
        case FBE_PORT_REQUEST_STATUS_REJECTED_NCQ_MODE:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_DRVABORT);
            break;        

		default:            

		    fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_UNKNOWNRESPONSE);
		    fbe_payload_block_set_status(block_operation,	FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
															FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
			break;
	}    

    return FBE_STATUS_OK;
}

fbe_status_t fbe_sata_physical_drive_map_port_status_to_scsi_error_code(fbe_port_request_status_t request_status, fbe_scsi_error_code_t * error)
{
    switch(request_status)
    {
		case FBE_PORT_REQUEST_STATUS_INVALID_REQUEST:

            *error = FBE_SCSI_INVALIDREQUEST;
            break;
 
        case FBE_PORT_REQUEST_STATUS_BUSY:
        case FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES:

            *error = FBE_SCSI_DEVICE_BUSY;
            break;

        case FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN:
            
            *error = FBE_SCSI_DEVICE_NOT_PRESENT;
            break;

        case FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR:
            *error = FBE_SCSI_BADBUSPHASE;
            break;

        case FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT:

            *error = FBE_SCSI_SATA_IO_TIMEOUT_ABORT;		    
            break;
        
        case FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR:
        case FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT:

            *error =  FBE_SCSI_SELECTIONTIMEOUT;
            break;   

        case FBE_PORT_REQUEST_STATUS_DATA_OVERRUN:
            
            *error = FBE_SCSI_TOOMUCHDATA;
            break;

        case FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN:

            *error = FBE_SCSI_XFERCOUNTNOTZERO;
            break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE:

            *error = FBE_SCSI_USERABORT;
            break;

		case FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT:
			*error = FBE_SCSI_INCIDENTAL_ABORT;
			break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE:
        case FBE_PORT_REQUEST_STATUS_REJECTED_NCQ_MODE:

            *error = FBE_SCSI_DRVABORT;
            break;


        case FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR:
            //Set FBE_SCSI_DRVABORT status JUST FOR NOW!!
            *error = FBE_SCSI_DRVABORT;

            break;

		default:

		    *error = FBE_SCSI_UNKNOWNRESPONSE;
			break;
	}

    return FBE_STATUS_OK;
}