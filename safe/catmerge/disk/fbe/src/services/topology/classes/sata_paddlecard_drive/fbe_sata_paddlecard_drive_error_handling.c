/****************
 * Include Files
 ***************/
#include "sata_paddlecard_drive_private.h"



/**************************************************************************
* fbe_sata_paddlecard_drive_get_port_error()                  
***************************************************************************
*
* DESCRIPTION
*       This is an override function which will return a specific error
*       code for this subclass.  
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       packet - IO packet
*       payload_cdb_operation - CDB operation
*       scsi_error - Error that is returned
*
* RETURN VALUES
*       status. 
*
* NOTES
*       This is based on a Template Method design pattern.
*
* HISTORY
*   1/6/2010 Wayne Garrett - Created.
***************************************************************************/
fbe_status_t
fbe_sata_paddlecard_drive_get_port_error(fbe_sas_physical_drive_t * sas_physical_drive,
                                         fbe_packet_t * packet,
                                         fbe_payload_cdb_operation_t * payload_cdb_operation,
                                         fbe_scsi_error_code_t * scsi_error)
{
    fbe_status_t status;
   
    status = sas_physical_drive_get_port_error(sas_physical_drive, packet, payload_cdb_operation, scsi_error);

    /* If a timeout error was returned then change it to indicate that it really came from a SATA drive.  Error
       handling for SATA drives are different due to the increased response time.
    */
    if (*scsi_error == FBE_SCSI_IO_TIMEOUT_ABORT) {
        *scsi_error = FBE_SCSI_SATA_IO_TIMEOUT_ABORT;
    }

    return status;
}
