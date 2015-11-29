/****************
 * Include Files
 ***************/

#include "fbe_transport_memory.h"
#include "base_physical_drive_init.h"

#include "sas_physical_drive_private.h"
#include "PhysicalPackageMessages.h"
#include "fbe/fbe_event_log_api.h"


typedef enum sas_pdo_remap_action_e
{
    SAS_PDO_REMAP_ACTION_NONE,
    SAS_PDO_REMAP_ACTION_HEALTH_CHECK,
    SAS_PDO_REMAP_ACTION_FAIL_DRIVE,

} sas_pdo_remap_action_t;


/******************************
 * Local function declarations
 *****************************/
static fbe_status_t sas_physical_drive_get_cc_error(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_payload_cdb_operation_t * payload_cdb_operation,
                                fbe_scsi_error_code_t * scsi_error, fbe_lba_t * bad_block, fbe_u32_t * sense_code);
static fbe_bool_t   sas_physical_drive_is_failable_op(fbe_payload_block_operation_opcode_t block_operation_opcode);
static void         sas_pdo_get_remap_recovery_action(fbe_bool_t remap_retries_exceeded, fbe_bool_t remap_time_exceeded, fbe_bool_t return_media_error, sas_pdo_remap_action_t *action);
static fbe_status_t sas_physical_drive_setup_remap_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);

/**************************************************************************
* sas_physical_drive_get_scsi_error_code()                  
***************************************************************************
*
* DESCRIPTION
*       This function gets called when a packet is returned from port object.
*       It returns SCSI error code based on CDB request status, SCSI status
*       and sense buffer.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       packet - Pointer to IO packet.
*       payload_cdb_operation - Pointer to the cdb payload.
*       scsi_error - Pointer to scsi error. Output.
*       bad_block - Pointer to bad lba. Output.
*       sense_code - Pointer to sense code. Output.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   10-Dec-2008 chenl6 - Created.
***************************************************************************/

fbe_status_t
sas_physical_drive_get_scsi_error_code(fbe_sas_physical_drive_t * sas_physical_drive, 
                                       fbe_packet_t * packet,
                                       fbe_payload_cdb_operation_t * payload_cdb_operation,
                                       fbe_scsi_error_code_t * error,
                                       fbe_lba_t * bad_lba,
                                       fbe_u32_t * sense_code)
{
    fbe_physcial_drive_serial_number_t  serial_number;
    fbe_bool_t no_bus_device_reset = FBE_FALSE;
    fbe_port_request_status_t cdb_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_payload_cdb_scsi_status_t cdb_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;
    fbe_bool_t lba_is_valid = FBE_FALSE;
    fbe_u8_t * sense_data = NULL;
    fbe_u8_t * cdb = NULL;
    fbe_port_number_t port;
    fbe_enclosure_number_t enclosure;
    fbe_slot_number_t slot;
    fbe_bool_t drive_ready_flag = FBE_FALSE;
    fbe_scsi_error_code_t port_error = 0;
    fbe_scsi_error_code_t scsi_error = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_scsi_sense_key_t sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE; 
    fbe_scsi_additional_sense_code_t asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO; 
    fbe_scsi_additional_sense_code_qualifier_t ascq = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
    fbe_lba_t lba;
    fbe_block_count_t blocks;

    fbe_payload_cdb_get_request_status(payload_cdb_operation, &cdb_request_status);
    fbe_payload_cdb_get_scsi_status(payload_cdb_operation, &cdb_scsi_status);
    *error = 0;    


	if((cdb_scsi_status == FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD) && (cdb_request_status == FBE_PORT_REQUEST_STATUS_SUCCESS)){
		return FBE_STATUS_OK;
	}

    lba = fbe_get_cdb_lba(payload_cdb_operation);
    blocks = fbe_get_cdb_blocks(payload_cdb_operation);

    fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_data);
    fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb);
    payload = fbe_transport_get_payload_ex(packet);

    if((bad_lba != NULL) && (sense_code != NULL)) {
        drive_ready_flag = FBE_TRUE;
    }

    /* Check cdb scsi status and cdb request status. */
    if(cdb_scsi_status != FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD)
    {
        /* Handle SCSI status 
         */
        switch (cdb_scsi_status)
        {
            case FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION:
            {
                /* Parse the sense data the parameters lba_is_valid and bad_lba
                 *  are checked in the function
                 */
                fbe_payload_cdb_parse_sense_data(sense_data, &sense_key, &asc, &ascq, &lba_is_valid, bad_lba);
                
                /* The follow code handles a special case.   Generally,  scsi cc errors take precedence over port errors.
                   However, if we have a recovered error (01/xx/xx), and we get either an underrun or overrun port error,
                   then port error takes precedence.   Not sure what issue this was attempting to resolve.   If someone knows,
                   please update this comment.   To further complicate things, there are certain cases where we still want to
                   process recovered errors, even if we meet these conditions.  Such as errors that cause drive EOL, specifically
                   the PFA errors.   The following code is a very specific fix and should someday be more generalized.   The problem
                   we actually hit (that this fixes) is during activate rotary, a mode sense was sent which returned on 01/5D along
                   with the expected underrun port error.   Previously the code was only returning the underrun and we never paco'd the
                   drive. -weg
                 */
                if((sense_key == FBE_SCSI_SENSE_KEY_RECOVERED_ERROR) && 
                   ((cdb_request_status == FBE_PORT_REQUEST_STATUS_DATA_OVERRUN) ||
                    (cdb_request_status == FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN)))
                {

                    sas_physical_drive_get_cc_error(sas_physical_drive, packet, payload_cdb_operation, &scsi_error, NULL, NULL);

                    if (FBE_SCSI_CC_PFA_THRESHOLD_REACHED == scsi_error) 
                    {
                        /* at this point we should call sas_physical_drive_process_scsi_error() so it can do the right
                           thing for us.   However this function is only intended to be used if IO was generated from
                           a block client.   This specific fix is for a case where IO is generated internally (i.e.
                           mode sense during drive init.  So well hardcode the same behavior that process_scsi_error
                           does. -weg */

                        /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
                        fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

                        fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, scsi_error, packet,
                                                          payload->physical_drive_transaction.last_error_code, payload_cdb_operation);
                    }

                    /* Handle overrun / underrun condition */
                    if (sas_physical_drive->get_port_error != NULL)
                    {
                        (sas_physical_drive->get_port_error)(sas_physical_drive, packet, payload_cdb_operation, &port_error);
                    }
                    else
                    {
                        fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t *)sas_physical_drive, 
                                            FBE_TRACE_LEVEL_ERROR,
                                            "%s get_misc_error virtual function never initialized.  code bug.\n",
                                            __FUNCTION__);                        
                    }
                    
                    *error = port_error;   /* port error takes precedence */
                }
                else
                {
                    /* Handle the various SCSI stat conditions here. */
                    sas_physical_drive_get_cc_error(sas_physical_drive, packet, payload_cdb_operation, &scsi_error, bad_lba, sense_code);
                    *error = scsi_error;
                }
                

                /* Log all check conditions except 06/29/00. */
                if ((sense_key != FBE_SCSI_SENSE_KEY_UNIT_ATTENTION) ||
                    (asc != FBE_SCSI_ASC_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED) ||
                    (ascq != FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO))
                {
				    fbe_u8_t dumpdata[FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE + FBE_PAYLOAD_CDB_CDB_SIZE];

                    no_bus_device_reset = FBE_TRUE;
                    /* Send SCSI buffer to Event log. */
                    fbe_copy_memory(dumpdata, cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
                    fbe_copy_memory(dumpdata+FBE_PAYLOAD_CDB_CDB_SIZE, sense_data, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
                    fbe_base_physical_drive_get_port_number((fbe_base_physical_drive_t *) sas_physical_drive, &port);
                    fbe_base_physical_drive_get_enclosure_number((fbe_base_physical_drive_t *) sas_physical_drive, &enclosure);
                    fbe_base_physical_drive_get_slot_number((fbe_base_physical_drive_t *) sas_physical_drive, &slot);
                    fbe_zero_memory(&serial_number, sizeof(fbe_physcial_drive_serial_number_t));
                    fbe_base_physical_drive_get_serial_number((fbe_base_physical_drive_t *) sas_physical_drive, &serial_number);
                    /*TODO: Add DIEH ratios to dumpdata or user msg string -wayne */
                    fbe_event_log_write(PHYSICAL_PACKAGE_INFO_SAS_SENSE,
                             dumpdata, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE + FBE_PAYLOAD_CDB_CDB_SIZE, 
                             "%d %d %d %s %s", port, enclosure, slot, serial_number.serial_number,
                                           fbe_base_physical_drive_get_error_initiator(payload->physical_drive_transaction.last_error_code));
                }                   
                break;
            }

            case FBE_PAYLOAD_CDB_SCSI_STATUS_BUSY:
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "scsi error, ScsiSt:0x%x PrtSt:%d Op:0x%x Lba:0x%x Blks:0x%x pkt:0x%llx svc:%d pri:%d\n", 
                                    cdb_scsi_status, cdb_request_status, (payload_cdb_operation->cdb)[0], (unsigned int)lba, (unsigned int)blocks, (unsigned long long)packet, (int)packet->physical_drive_service_time_ms, packet->packet_priority);

                *error = FBE_SCSI_DEVICE_BUSY;
                break;

            case FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT:
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "scsi error, ScsiSt:0x%x PrtSt:%d Op:0x%x Lba:0x%x Blks:0x%x pkt:0x%llx svc:%d pri:%d\n", 
                                    cdb_scsi_status, cdb_request_status, (payload_cdb_operation->cdb)[0], (unsigned int)lba, (unsigned int)blocks, (unsigned long long)packet, (int)packet->physical_drive_service_time_ms, packet->packet_priority);

                *error = FBE_SCSI_DEVICE_RESERVED;
                break;

#if 0
            case FBE_PAYLOAD_CDB_SCSI_STATUS_QUEUE_FULL:
                *error = FBE_SCSI_DEVICE_BUSY;
                break;
#endif

            default: 
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "scsi error, ScsiSt:0x%x PrtSt:%d Op:0x%x Lba:0x%x Blks:0x%x pkt:0x%llx svc:%d pri:%d\n", 
                                    cdb_scsi_status, cdb_request_status, (payload_cdb_operation->cdb)[0], (unsigned int)lba, (unsigned int)blocks, (unsigned long long)packet, (int)packet->physical_drive_service_time_ms, packet->packet_priority);

                /* We've receive a status that we do not understand.
                 */
                *error = FBE_SCSI_UNKNOWNRESPONSE;
        } /* end of Switch */
         
        if (drive_ready_flag == FBE_TRUE) {
            /* Handle Disk Collect when drive is ready. */
            fbe_sas_physical_drive_fill_dc_info(sas_physical_drive, payload_cdb_operation, sense_data, cdb, no_bus_device_reset); 
        }           
    }
    else if (cdb_request_status != FBE_PORT_REQUEST_STATUS_SUCCESS)
    {
        if (sas_physical_drive->get_port_error != NULL)
        {
            (sas_physical_drive->get_port_error)(sas_physical_drive, packet, payload_cdb_operation, error);
        }
        else
        {
            fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t *)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s get_misc_error virtual function never initialized.  code bug.\n",
                                __FUNCTION__);                        
        }

        /* Handle Disk Collect when drive is ready. */
        if (drive_ready_flag == FBE_TRUE) {
            /* We want to ignore these errors as are they are port related */
            if((cdb_request_status != FBE_PORT_REQUEST_STATUS_ENCRYPTION_NOT_ENABLED) &&
               (cdb_request_status != FBE_PORT_REQUEST_STATUS_ENCRYPTION_BAD_HANDLE) &&
               (cdb_request_status != FBE_PORT_REQUEST_STATUS_ENCRYPTION_KEY_WRAP_ERROR)){
                fbe_sas_physical_drive_fill_dc_info(sas_physical_drive, payload_cdb_operation, sense_data, cdb, FBE_FALSE);         
            }
        }
    }

    return FBE_STATUS_OK;
}


/**************************************************************************
* sas_physical_drive_get_cc_error()                  
***************************************************************************
*
* DESCRIPTION
*       This function gets called when SCSI status is check condition.
*       It returns check condition error code based on sense buffer.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       packet -   ptr for IO packet.
*       payload_cdb_operation - Pointer to the cdb payload.
*       scsi_error - Pointer to scsi error. This is an output but you need to pass
*                     in a non-NULL pointer.
*       bad_block - Pointer to bad lba. This is an output but you need to pass
*                    in a non-NULL pointer.
*       sense_code - Pointer to sense code. This is an output but you need to pass
*                     in a non_NULL pointer.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   10-Dec-2008 chenl6 - Created.
***************************************************************************/

fbe_status_t
sas_physical_drive_get_cc_error(fbe_sas_physical_drive_t* sas_physical_drive,        /* IN */
                                fbe_packet_t* packet,                                /* IN */
                                fbe_payload_cdb_operation_t* payload_cdb_operation,  /* IN */
                                fbe_scsi_error_code_t* scsi_error,                   /* OUT */
                                fbe_lba_t* bad_block,                                /* OUT */
                                fbe_u32_t* sense_code)                               /* OUT */
{
    fbe_u8_t * sense_data = NULL;
    fbe_scsi_sense_key_t sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE; 
    fbe_scsi_additional_sense_code_t asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO; 
    fbe_scsi_additional_sense_code_qualifier_t ascq = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
    fbe_scsi_error_code_t error = 0;
    fbe_bool_t lba_is_valid = FBE_FALSE;
    fbe_lba_t bad_lba = FBE_LBA_INVALID;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_port_request_status_t cdb_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_lba_t lba = fbe_get_cdb_lba(payload_cdb_operation);
    fbe_block_count_t blocks = fbe_get_cdb_blocks(payload_cdb_operation);
    fbe_bool_t lba_is_out_of_range = FBE_FALSE;
   
    fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_data);

    /* Function below parses the sense data */
    status = fbe_payload_cdb_parse_sense_data(sense_data, &sense_key, &asc, &ascq, &lba_is_valid, &bad_lba);
    
    /* Verify that the Request Sense data is valid. */
    if (status == FBE_STATUS_GENERIC_FAILURE) 
    {
        /* The data in the SCSI driver's Request Sense Buffer is invalid.
         */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "%s, invalid response code: 0x%x.\n",
                                    __FUNCTION__, (sense_data[0]& 0x7F));
        *scsi_error = FBE_SCSI_AUTO_SENSE_FAILED;
        sense_data[0] = 0xBB;
        return FBE_STATUS_OK;
    }

    /* If we have a bad LBA returned check to make sure that it falls
     *  within the range of the associated command
     */
    if (lba_is_valid == FBE_TRUE)  /* This is the status on the new function */ 
    {
        if((bad_lba < lba) || (bad_lba >= (lba + (fbe_lba_t)blocks )))
        {
             fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "%s, Bad LBA is out of range!!! LBA:0x%llX.\n",
                                    __FUNCTION__, (unsigned long long)bad_lba);
             lba_is_out_of_range = FBE_TRUE;
        }
    }
    
    // Fetch the port request status to print in trace message
    fbe_payload_cdb_get_request_status(payload_cdb_operation, &cdb_request_status);
 
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "cc error, %02x|%02x|%02x Prt:%d Op:0x%x Lba:0x%x Blks:0x%x Blba:0x%x pkt:0x%llx svc:%d pri:%d\n", 
                                    sense_key, asc, ascq, cdb_request_status, (payload_cdb_operation->cdb)[0], (unsigned int)lba, (unsigned int)blocks, 
                                    (unsigned int)bad_lba, (unsigned long long)packet, (int)packet->physical_drive_service_time_ms, packet->packet_priority);

    /* Check whether it is a deferred error.  Note that the LSB is set for
     *  deferred errors block descriptor format and fixed format sense data.
     */
    if (sense_data[0] & 0x01)
    {
        *scsi_error = FBE_SCSI_CC_DEFERRED_ERROR;
        return FBE_STATUS_OK;
    }

    /* Examine the sense data to determine the appropriate error status.
     */
    switch (sense_key)
    {
        case FBE_SCSI_SENSE_KEY_NO_SENSE:
            error = FBE_SCSI_CC_NOERR;
            
            if ( asc == FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO && ascq == FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO )
            {
                error = FBE_SCSI_CC_SENSE_DATA_MISSING;
            }
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
                error = FBE_SCSI_CC_AUTO_REMAPPED;
                break;                /* break out of sense_key switch */
            }

#if 0
            /* On some older drives, these are errors that could be returned 
             * with 01 sense key, but are 0f 04, 05 or 0B. Need to convert them 
             * to what they really are.
             */
            if ((asc == FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR && ascq == FBE_SCSI_ASCQ_FIFO_READ_ERROR)  ||
                (asc == FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR && ascq == FBE_SCSI_ASCQ_FIFO_WRITE_ERROR) ||
                (asc == FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR && ascq == FBE_SCSI_ASCQ_IOEDC_READ_ERROR) ||
                (asc == FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR && ascq == FBE_SCSI_ASCQ_HOST_PARITY_ERROR)||
                (asc == FBE_SCSI_ASC_SCSI_INTFC_PARITY_ERR  && ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER)||
                (asc == FBE_SCSI_ASC_SCSI_INTFC_PARITY_ERR  && ascq == FBE_SCSI_ASCQ_FIFO_READ_ERROR)  ||
                (asc == FBE_SCSI_ASC_INVALID_FIELD_IN_CDB   && ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER))
            {
                error = NTBEProcessSpecialSenseData(PScb, sense_data_ptr);
                break;
            }
#endif

            /* The recovered error was something other than an auto-remap.
             */
            switch (asc)
            {
                case FBE_SCSI_ASC_RECOVERED_WITH_ECC:
                    /* For Hitachi Ralston Peak drive */
                    if (ascq == FBE_SCSI_ASCQ_DIE_RETIREMENT_START)
                    {
                        error = FBE_SCSI_CC_DIE_RETIREMENT_START;
                        break;
                    }
                    else if (ascq == FBE_SCSI_ASCQ_DIE_RETIREMENT_END) 
                    {
                        error = FBE_SCSI_CC_DIE_RETIREMENT_END;
                        break;
                    }

                case FBE_SCSI_ASC_RECOVERED_WITH_RETRIES:
                case FBE_SCSI_ASC_ID_ADDR_MARK_MISSING:
                case FBE_SCSI_ASC_DATA_SYNC_MARK_MISSING:
                    if (lba_is_valid)
                    {
                        error = FBE_SCSI_CC_RECOVERED_BAD_BLOCK;
                        //NTBECheckLbaRange(PScb, sense_data_ptr, &error);
                    }
                    else
                    {
                        /* The drive did not report an LBA in the sense
                         * data.  Just report a recovered soft error so
                         * that we don't try to remap.
                         */
                        error = FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP;
                    }
                    break;

                case FBE_SCSI_ASC_PFA_THRESHOLD_REACHED:
                    error = FBE_SCSI_CC_PFA_THRESHOLD_REACHED;
                    break;

                case FBE_SCSI_ASC_SPINDLE_SYNCH_CHANGE:

                    /* Normally a spindle synch check condition will show up as
                     * a unit attention.  However, some manufacturers (notably
                     * IBM) may define a synch fail check condition (but not
                     * synch success) for other sense keys as well.
                     */
                    if (ascq == FBE_SCSI_ASCQ_SYNCH_FAIL)
                    {
                        error = FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH;
                    }
                    else
                    {
                        error = FBE_SCSI_CC_RECOVERED_ERR;
                    }
                    break;

                case FBE_SCSI_ASC_DEFECT_LIST_ERROR:
                case FBE_SCSI_ASC_PRIMARY_LIST_MISSING:
                    error = FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR;
                    break;

                case FBE_SCSI_ASC_DEV_WRITE_FAULT:
                    error = FBE_SCSI_CC_RECOVERED_WRITE_FAULT;
                    break;

                case FBE_SCSI_ASC_WARNING:
                    /* 01/0B/01 reports TEMPERATURE_EXCEEDED.
                     */
                    if (ascq == 0x01)
                    {
                        error = FBE_SCSI_CC_PFA_THRESHOLD_REACHED;
                    }
                    else
                    {
                        error = FBE_SCSI_CC_RECOVERED_ERR;
                    }
                    break;

                case FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR:
                    if (ascq == 0x30)
                    {
                        /* For FSSD drives, 01/80/30 reports Super Capacitor Failure.
                         */                     
                        error = FBE_SCSI_CC_SUPER_CAP_FAILURE;                    
                    }
                    else if (ascq == 0x33 || ascq == 0x4B)
                    /* For FSSD drives, 01/80/33, 01/80/4B report SMART data.
                     */                    
                    {
                        error = FBE_SCSI_CC_PFA_THRESHOLD_REACHED;
                    }
                    else
                    {
                        error = FBE_SCSI_CC_RECOVERED_ERR;
                    }
                    break;

                default:
                    error = FBE_SCSI_CC_RECOVERED_ERR;
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
                error = FBE_SCSI_CC_BECOMING_READY;
            }
            else if ((asc == FBE_SCSI_ASC_NOT_READY) &&
                     (ascq == FBE_SCSI_ASCQ_NOT_SPINNING))
            {
                /* This is the "Not Ready, Initializing Command Required"
                 * check condition. It specifically tells us that the drive
                 * is not spinning.
                 */
                error = FBE_SCSI_CC_NOT_SPINNING;
            }
            else if ((asc == FBE_SCSI_ASC_NOT_READY) && (ascq == 0x04))
            {
                error = FBE_SCSI_CC_FORMAT_IN_PROGRESS;
            }
            else if ((asc == FBE_SCSI_ASC_NOT_READY) && (ascq == 0x05))
            {
                /* For FSSD drives 02/04/05 reports a Drive Table Rebuild Error
                 */
                error = FBE_SCSI_CC_DRIVE_TABLE_REBUILD;
            }            
            else if (asc == FBE_SCSI_ASC_FORMAT_CORRUPTED)
            {
                /* This is the "Not Ready because of bad format" check
                 * condition.  There are a couple of possible ASCQ values;
                 * but they all tell us that the drive needs to be formatted.
                 */
                error = FBE_SCSI_CC_FORMAT_CORRUPTED;
            }
            else if (asc == FBE_SCSI_ASC_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION)
            {
                /* JO, 5-15-07. This has been added primarily to support Bigfoot/Mamba
                 * SATA drives. This error is returned by the LSI controller if LSI has 
                 * attempted initialization to a SATA and retried four times to no avail. 
                */
                error = FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION;    
            }
            else
            {
                /* All other "NOT READY"s:  Unfortunately, we can't tell
                 * whether or not the disk is spinning, if the drive was never
                 * started or if it was running before and then died, or if
                 * the NOT READY is a temporary or permanent condition.  Return
                 * a generic NOT READY status.  The SCSI operation will likely
                 * be retried.
                 */
                error = FBE_SCSI_CC_NOT_READY;
            }
            break;

        case FBE_SCSI_SENSE_KEY_MEDIUM_ERROR:
            if (asc == FBE_SCSI_ASC_FORMAT_CORRUPTED)
            {
                if (ascq == 0x80)
                {
                    error = FBE_SCSI_CC_SANITIZE_INTERRUPTED;
                }
                else
                {
                   error = FBE_SCSI_CC_FORMAT_CORRUPTED;
                }
            }
            else if (asc == FBE_SCSI_ASC_DEFECT_LIST_ERROR ||
                     asc == FBE_SCSI_ASC_PRIMARY_LIST_MISSING ||
                     asc == FBE_SCSI_ASC_NO_SPARES_LEFT)
            {
                error = FBE_SCSI_CC_DEFECT_LIST_ERROR;
            }
            else if (lba_is_valid)
            {
                if ((asc == FBE_SCSI_ASC_WRITE_ERROR) && 
                    (ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER))
                {
                    error = FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR;
                }
                else if (asc == FBE_SCSI_ASC_DEV_WRITE_FAULT)
                {
                    error = FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT;
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
                    error = FBE_SCSI_CC_HARD_BAD_BLOCK;
                }
                //NTBECheckLbaRange(PScb, sense_data_ptr, &error);
            }
            else
            {
                /* Drive did not report the bad LBA in the sense data.
                 */
                error = FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP;
            }
            break;

        case FBE_SCSI_SENSE_KEY_HARDWARE_ERROR:

            if (asc == FBE_SCSI_ASC_SCSI_INTFC_PARITY_ERR)
            {
                /* Record a more specific error code for a parity error.
                 */
                error = FBE_SCSI_CC_HARDWARE_ERROR_PARITY;

            }
            else if (asc == FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR)
            {
                error = FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE;
            }
            else if (asc == FBE_SCSI_ASC_NO_SPARES_LEFT)
            {
                error = FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE;
            }
            else if (asc == FBE_SCSI_ASC_DEFECT_LIST_ERROR)
            {
                error = FBE_SCSI_CC_DEFECT_LIST_ERROR;
            }
            else if (asc == FBE_SCSI_ASC_SEEK_ERROR)
            {
                error = FBE_SCSI_CC_SEEK_POSITIONING_ERROR;
            }
            else if (asc == FBE_SCSI_ASC_DEV_WRITE_FAULT)
            {
                error = FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT;
            }
            else if (asc == FBE_SCSI_ASC_INTERNAL_TARGET_FAILURE)
            {
                error = FBE_SCSI_CC_INTERNAL_TARGET_FAILURE;
            }
            else if (asc == FBE_SCSI_ASC_LOGICAL_UNIT_FAILURE && ascq == 0x03)
            {
                error = FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST;
            }
            else
            {
                error = FBE_SCSI_CC_HARDWARE_ERROR;
            }
            break;

        case FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST:

            if ((asc == FBE_SCSI_ASC_INVALID_LUN) && 
                (ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER))
            {
                error = FBE_SCSI_CC_INVALID_LUN;
            }
            else
            {
                error = FBE_SCSI_CC_ILLEGAL_REQUEST;
            }
            break;

        case FBE_SCSI_SENSE_KEY_UNIT_ATTENTION:

            if (asc == FBE_SCSI_ASC_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED)
            {
                error = FBE_SCSI_CC_DEV_RESET;
            }
            else if (asc == FBE_SCSI_ASC_MODE_PARAM_CHANGED)
            {
                error = FBE_SCSI_CC_MODE_SELECT_OCCURRED;
            }
            else if (asc == FBE_SCSI_ASC_SPINDLE_SYNCH_CHANGE)
            {
                if (ascq == FBE_SCSI_ASCQ_SYNCH_SUCCESS)
                {
                    error = FBE_SCSI_CC_SYNCH_SUCCESS;
                }
                else if (ascq == FBE_SCSI_ASCQ_SYNCH_FAIL)
                {
                    error = FBE_SCSI_CC_SYNCH_FAIL;
                }
            }
            else if (asc == FBE_SCSI_ASC_VENDOR_SPECIFIC_FF && 
                     (ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER || ascq == 0xFE))
            {
                error = FBE_SCSI_CC_SEL_ID_ERROR;
            }
            else
            {
                error = FBE_SCSI_CC_UNIT_ATTENTION;
            }
            break;

        case FBE_SCSI_SENSE_KEY_WRITE_PROTECT:
            {
                if(asc == FBE_SCSI_ASC_COMMAND_NOT_ALLOWED)
                {
                    error = FBE_SCSI_CC_WRITE_PROTECT;
                }
                else
                {
                    error = FBE_SCSI_CC_UNEXPECTED_SENSE_KEY;
                }
            }
            break;

        case FBE_SCSI_SENSE_KEY_VENDOR_SPECIFIC_09:
            if((asc == FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR) && 
               (ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER) )
            {
                if(((fbe_base_physical_drive_t *)sas_physical_drive)->drive_info.drive_vendor_id == FBE_DRIVE_VENDOR_SEAGATE)
                {
                    error = FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR;
                }
                else
                {
                    error = FBE_SCSI_CC_UNEXPECTED_SENSE_KEY;
                }
            }
            else
            {
                error = FBE_SCSI_CC_UNEXPECTED_SENSE_KEY;
            }
            break;

        case FBE_SCSI_SENSE_KEY_ABORTED_CMD:
            if (asc == FBE_SCSI_ASC_SCSI_INTFC_PARITY_ERR ||
                     asc == FBE_SCSI_ASC_DATA_PHASE_ERROR)
            {
                error = FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR;
            }
            else
            {
                error = FBE_SCSI_CC_ABORTED_CMD;
            }
            break;

        default:
            error = FBE_SCSI_CC_UNEXPECTED_SENSE_KEY;

    }
    
    /* If the deferred bit is not set and is a media error with a valid lba. We don't want to remap on the write IO when a LBA range is out of range.
    It will cause the read verify to get the CRC MultiBitCRC error later. */
    if (lba_is_out_of_range && fbe_sas_physical_drive_is_write_media_error(payload_cdb_operation, error))
    {                
        error = FBE_SCSI_INVALIDSCB;
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                FBE_TRACE_LEVEL_INFO,
                "%s Lba is out of range and error is a media with a write IO. Error is %d\n", __FUNCTION__, error);
    }
    
    /* Set media error lba and sense code here. */
    *scsi_error = error;

    if (bad_block)
    {
        *bad_block = bad_lba;
    }

    if (sense_code)
    {
        *sense_code = (((sense_key << 24) & 0xFF000000) | ((asc << 16) & 0x00FF0000) | ((ascq << 8) & 0x0000FF00));
    }

    return FBE_STATUS_OK;
}


/**************************************************************************
* sas_physical_drive_get_port_error()                  
***************************************************************************
*
* DESCRIPTION
*       This function gets called when request status is not good.
*       It returns check condition error code based on port request status.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       packet - IO packet.
*       payload_cdb_operation - Pointer to the cdb payload.
*       scsi_error - Pointer to scsi error.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   10-Dec-2008 chenl6 - Created.
***************************************************************************/

fbe_status_t
sas_physical_drive_get_port_error(fbe_sas_physical_drive_t * sas_physical_drive,
                                  fbe_packet_t * packet,
                                  fbe_payload_cdb_operation_t * payload_cdb_operation,
                                  fbe_scsi_error_code_t * scsi_error)
{
    fbe_port_request_status_t cdb_request_status;
    fbe_scsi_error_code_t error = 0;
    fbe_lba_t lba = FBE_LBA_INVALID;  
    fbe_block_count_t blocks = FBE_CDB_BLOCKS_INVALID;  
    
    fbe_payload_cdb_get_request_status(payload_cdb_operation, &cdb_request_status);           
    lba = fbe_get_cdb_lba(payload_cdb_operation);
    blocks = fbe_get_cdb_blocks(payload_cdb_operation);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "port error, PrtSt:%d Op:0x%x Lba:0x%x Blks:0x%x pkt:0x%llx svc:%lld pri:%d\n", 
                                    cdb_request_status, (payload_cdb_operation->cdb)[0], (unsigned int)lba, (unsigned int)blocks, (unsigned long long)packet, packet->physical_drive_service_time_ms, packet->packet_priority);

    switch (cdb_request_status)
    {
        case FBE_PORT_REQUEST_STATUS_SUCCESS:
            error = 0;
            break;
            
        case FBE_PORT_REQUEST_STATUS_INVALID_REQUEST:
            /* Inappropriate drive response
             */
            error = FBE_SCSI_INVALIDREQUEST;
            break;
            
        case FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES:
            error = FBE_SCSI_DEVICE_BUSY;
            break;
            
        case FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN:
            error = FBE_SCSI_DEVICE_NOT_PRESENT;
            break;
            
        case FBE_PORT_REQUEST_STATUS_BUSY:
            error = FBE_SCSI_DEVICE_BUSY;
            break;
            
        case FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR:
            error = FBE_SCSI_BADBUSPHASE;
            break;
            
        case FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT:
            error = FBE_SCSI_IO_TIMEOUT_ABORT;
            break;

        case FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT:
            error = FBE_SCSI_SELECTIONTIMEOUT;
            break;
            
        case FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR:
            error = FBE_SCSI_SELECTIONTIMEOUT;
            break;
            
        case FBE_PORT_REQUEST_STATUS_DATA_OVERRUN:
            error = FBE_SCSI_TOOMUCHDATA;
            break;

        case FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN:
            error = FBE_SCSI_XFERCOUNTNOTZERO;
            break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE:
            /* The request has timed out 
             */
            error = FBE_SCSI_DRVABORT;        
            break;

        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE:
            error = FBE_SCSI_DRVABORT;
            break;

        case FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT:
            error = FBE_SCSI_INCIDENTAL_ABORT;
            break;

        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_BAD_HANDLE:
            error = FBE_SCSI_ENCRYPTION_BAD_KEY_HANDLE;
            break;

        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_KEY_WRAP_ERROR:
            error = FBE_SCSI_ENCRYPTION_KEY_WRAP_ERROR;
            break;

        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_NOT_ENABLED:
            error = FBE_SCSI_ENCRYPTION_NOT_ENABLED;
            break;

        default:
            /* We've receive a status that we do not understand.
             */
            error = FBE_SCSI_UNKNOWNRESPONSE;
            
            break;
    }

    *scsi_error = error;

    return FBE_STATUS_OK;
}


/**************************************************************************
* sas_physical_drive_process_scsi_error()                  
***************************************************************************
*
* DESCRIPTION
*       This function processes SCSI errors for normal IOs.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       packet - Pointer to the packet.
*       error - SCSI error.
*
* RETURN VALUES
*       status. 
*
* NOTES
*       It expected that only IO that contains a block operation will
*       call this. i.e. IO generated from a block client, such as RAID.
*
* HISTORY
*   30-Dec-2008 chenl6 - Created.
***************************************************************************/

fbe_status_t
sas_physical_drive_process_scsi_error(fbe_sas_physical_drive_t * sas_physical_drive,
                                      fbe_packet_t * packet, fbe_scsi_error_code_t error,
                                      const fbe_payload_cdb_operation_t * payload_cdb_operation)
{
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)sas_physical_drive;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_u32_t block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    fbe_u32_t block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    fbe_payload_block_operation_opcode_t block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID; 

    payload = fbe_transport_get_payload_ex(packet);    

    /* If block_operation_p is NULL then packet was generated internally. This function should not be
       called by IO generated internally.   So log an error. */
    block_operation_p = fbe_payload_ex_get_block_operation(payload);

    if (NULL == block_operation_p)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                  "%s packet was generated internally.  \n", __FUNCTION__);
    }
    else /* a block client generated this packet */
    {
        fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);            
    }

    /* Update IO service time */
    fbe_transport_update_physical_drive_service_time(packet);

    switch(error)
    {
        case FBE_SCSI_CC_AUTO_REMAPPED:
        case FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP:
        case FBE_SCSI_CC_RECOVERED_ERR:
        case FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH:
        case FBE_SCSI_CC_NOERR:
        case FBE_SCSI_CC_DIE_RETIREMENT_START:
        case FBE_SCSI_CC_DIE_RETIREMENT_END:
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            break;

        case FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR:
        case FBE_SCSI_CC_SUPER_CAP_FAILURE:
            /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
            fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            break;

        case FBE_SCSI_CC_PFA_THRESHOLD_REACHED:
            /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
            fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;            
            break;
        
        case FBE_SCSI_CC_WRITE_PROTECT:
                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_WRITE_PROTECT);

                /* We need to set the BO:FAIL lifecycle condition */
                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          "%s can't set BO:FAIL condition, status: 0x%X\n",
                                          __FUNCTION__, status);
            
                }

                block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
                block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
            break;

        case FBE_SCSI_IO_TIMEOUT_ABORT:
            /* Check for Drive-Lockup case.   Certain situations can cause this with some drive types,
               such as pulling a back-end bus cable.  This can cause one of the drives ports to lose its link
               unexpectedly.  If the drive was in the middle of responding to the initiator, it may cause 
               it to lock-up.  The good port will then be unable to process the IOs and eventually miniport
               will time it out.  To solve this issue a hard-reset is sent to the drive.   Note, that
               Seagate indicates this behavior is by design.  Also Care has to be taken to not be too agressive
               at reseting the drive, since for other scenarios this has the affect of causing the peer to receive
               timeouts.  This could lead to a "ping-pong" affect, i.e.  spa reset, then spb reset, then spa
               reset, ...  Other situations can also cause timeouts, such as a result of drive fw bugs, which may
               cause drive lockup with similar behavior mentioned above.  A third situation, which is probably the
               most common, is a result of media errors. The drive can be busy trying to recover data and not allow
               other IOs to proceed.  
            */

            /* If Timeout Quiesce Handling is enabled then it handle the three scenario listed above by,
               1) Relies on drive nexus/IRT timer to clear drive lockup due to link loss.
               2) Relies on Health Check to handle lockup case to due drive fw bug.
               3) Avoid doing an agressive reset if due to media errors, since this can kick raid out.              
                  of its error handling, thereby getting it stuck retrying the same IOs.
               This error handling will Hold the transport Q to prevent additional IO from being sent to drive.
               Then it will wait for oustanding IO to drain.  On last IO it will resume the transport Q.  Doing this
               will help give time for drive to recover and allow time for nexus and IRT timers to kick in if this
               was a result of link loss.
            */
            if (fbe_dcs_param_is_enabled(FBE_DCS_DRIVE_TIMEOUT_QUIESCE_HANDLING))
            {
                fbe_base_physical_drive_lock(base_physical_drive);
                if (!fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_TIMEOUT_QUIESCE_HANDLING))
                {
                    fbe_base_physical_drive_set_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_TIMEOUT_QUIESCE_HANDLING);
                    fbe_base_physical_drive_unlock(base_physical_drive);

                    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                          "process_scsi_error: initiate Timeout draining. pkt:%p\n", packet);                    

                    base_physical_drive->quiesce_start_time_ms = fbe_get_time();

                    /* initiate quiesce */
                    fbe_block_transport_server_hold(&base_physical_drive->block_transport_server);  

                    /* this condition handles resuming IO after last IO drains.  Set it now to handle case where this
                       was the only IO to the drive */
                    fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive,                                                              
                                           FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESUME_IO);
                }
                else
                {
                    fbe_base_physical_drive_unlock(base_physical_drive);
                }
            }

            /* The solution if Drive Lockup Recovery is enable is to retry once, and if we still get a timeout then issue the reset.*/
            else if (fbe_dcs_param_is_enabled(FBE_DCS_DRIVE_LOCKUP_RECOVERY))
            {
                fbe_base_physical_drive_lock(base_physical_drive);
                if (!fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DRIVE_LOCKUP_RECOVERY))
                {
                    fbe_base_physical_drive_set_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DRIVE_LOCKUP_RECOVERY);

                    fbe_base_physical_drive_unlock(base_physical_drive);

                    /* Prevent any new or waiting IO from being sent to drive until this IO completes the Drive Lockup error handling.
                       This will prevent having to wait for outstanding IO at drive to be aborted if a reset is needed, which requires
                       going through pending-activate and draining */
                    fbe_block_transport_server_hold(&base_physical_drive->block_transport_server);  
                    
                    payload->physical_drive_transaction.retry_count++;

                    status = sas_physical_drive_process_resend_block_cdb(sas_physical_drive, packet);
                    if (FBE_STATUS_OK == status)
                    {                        
                        (void)fbe_base_physical_drive_log_event(base_physical_drive, error, packet, payload->physical_drive_transaction.last_error_code, payload_cdb_operation);
                        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
                    }
                    /* if previous cmd failed for whatever reason, just send back fail status to
                       RAID and let them retry. */
                }
                else if (payload->physical_drive_transaction.retry_count > 0) /* must be the IO we retried */
                {
                    /* If retry fails then we try a reset to clear drive lockup */

                    fbe_base_physical_drive_unlock(base_physical_drive);

                    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                          "%s initiate Drive Lockup recovery. pkt:%p\n", __FUNCTION__, packet);

                    fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                           FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESET);         
                }
                else /* This is a timeout from another IO.  We only care about the one we retried. */
                {
                    fbe_base_physical_drive_unlock(base_physical_drive);
                }
            }

            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;

            break;


        case FBE_SCSI_CC_SENSE_DATA_MISSING:
        case FBE_SCSI_CC_DEV_RESET:
        case FBE_SCSI_BUSRESET:
        case FBE_SCSI_SELECTIONTIMEOUT:
        case FBE_SCSI_DEVUNITIALIZED:
        case FBE_SCSI_AUTO_SENSE_FAILED:
        case FBE_SCSI_BADBUSPHASE:
        case FBE_SCSI_BADSTATUS:
        case FBE_SCSI_XFERCOUNTNOTZERO:
        case FBE_SCSI_INVALIDSCB:
        case FBE_SCSI_TOOMUCHDATA:
        case FBE_SCSI_CHANNEL_INACCESSIBLE:
        case FBE_SCSI_IO_LINKDOWN_ABORT:
        case FBE_SCSI_FCP_INVALIDRSP:
        case FBE_SCSI_UNKNOWNRESPONSE:
        case FBE_SCSI_CC_ILLEGAL_REQUEST:        
        case FBE_SCSI_CC_INVALID_LUN:
        case FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR:
        case FBE_SCSI_CC_ABORTED_CMD:
        case FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR:
        case FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE:
        case FBE_SCSI_CC_HARDWARE_ERROR_PARITY: 
        case FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST:                   
        case FBE_SCSI_CC_HARDWARE_ERROR:
        case FBE_SCSI_CC_MEDIA_ERR_DONT_REMAP:
        case FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP:
        case FBE_SCSI_CC_MODE_SELECT_OCCURRED:
        case FBE_SCSI_CC_UNIT_ATTENTION:
        case FBE_SCSI_CC_UNEXPECTED_SENSE_KEY:
        case FBE_SCSI_CHECK_COND_OTHER_SLOT:
        case FBE_SCSI_CC_SEEK_POSITIONING_ERROR:
        case FBE_SCSI_CC_SEL_ID_ERROR:
        case FBE_SCSI_SLOT_BUSY:
        case FBE_SCSI_DEVICE_BUSY:
        case FBE_SCSI_DEVICE_RESERVED:
        case FBE_SCSI_CC_NOT_READY:
        case FBE_SCSI_CC_FORMAT_IN_PROGRESS:
        case FBE_SCSI_DEVICE_NOT_READY:
        case FBE_SCSI_CC_BECOMING_READY:        
        case FBE_SCSI_SATA_IO_TIMEOUT_ABORT:
        case FBE_SCSI_CC_DATA_OFFSET_ERROR:
        case FBE_SCSI_CC_DEFERRED_ERROR:
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            break;

        case FBE_SCSI_INVALIDREQUEST:
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR;
            break;    
            
        case FBE_SCSI_CC_RECOVERED_WRITE_FAULT:
            /* TODO: PaCo */
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            break;

        case FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT:
            /* TODO: PaCo */
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            break;

        case FBE_SCSI_CC_INTERNAL_TARGET_FAILURE:
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            break;

        case FBE_SCSI_CC_NOT_SPINNING:
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            /* Set ACTIVATE condition. */
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      "%s can't set BO:ACTIVATE condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
            break;

        /* These error codes need keep sync with the list of error codes in fbe_sas_physical_drive_is_write_media_error().*/
        case FBE_SCSI_CC_RECOVERED_BAD_BLOCK:
        case FBE_SCSI_CC_HARD_BAD_BLOCK:
        case FBE_SCSI_CC_MEDIA_ERR_DO_REMAP:
        case FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT:
        case FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR:
            if ((block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
                (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME) ||
                (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY))
            {
                
                /* First log the event as we are going to return from this block itself */
                status = fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive,
                                                            error, 
                                                            packet,
                                                            payload->physical_drive_transaction.last_error_code,
                                                            payload_cdb_operation);

                /* We use remap strategy on WRITE operations. */
                return (sas_physical_drive_setup_remap(sas_physical_drive, packet));
            }
            else
            {
                /* For other operations we depend on RAID. */
                if (error == FBE_SCSI_CC_RECOVERED_BAD_BLOCK)
                {
                    block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
                    block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED;
                }
                else
                {
                    block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR;
                    block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST;
                }
            }
            break;

        case FBE_SCSI_CC_DEFECT_LIST_ERROR:
            if (sas_physical_drive_is_failable_op(block_operation_opcode))
            {
                /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
                fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

                block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR;
                block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP;
            }
            else
            {
                /* Fail the drive */

                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DEFECT_LIST_ERRORS);

                /* We need to set the BO:FAIL lifecycle condition */
                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          "%s can't set BO:FAIL condition, status: 0x%X\n",
                                          __FUNCTION__, status);
            
                }

                block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
                block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
            }
            break;

        case FBE_SCSI_INCIDENTAL_ABORT:
            /* This is considered a serious link error.   Go into Activate state to recover, which will wait until
               drive comes back.  This was added for a solution to AR 597523, where there was an expander power
               glitch and it failed to discover drives were logged out.  PDO continued to send IOs which got
               INCIDENTAL_ABORT errors.  These were a result of miniport failing for IT NEXUS LOSS due to OPEN RETRY
               THRESHOLD EXCEEDED.  Ultimately this resulted in failing all drives in the enclosure. */

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                  "%s FBE_SCSI_INCIDENTAL_ABORT - transitioning to ACTIVATE\n", __FUNCTION__);

            /* Notify monitor to set link_fault */
            fbe_base_pdo_set_flags_under_lock(&sas_physical_drive->base_physical_drive, FBE_PDO_FLAGS_SET_LINK_FAULT);            

            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                      "%s can't set BO:ACTIVATE condition, status: 0x%X, error: 0x%X\n",
                                      __FUNCTION__, status, error);
            }

            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            break;

        case FBE_SCSI_ENCRYPTION_BAD_KEY_HANDLE:
             fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                        "%s ENCRYPTION_BAD_KEY_HANDLE\n", __FUNCTION__);
             block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
             block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE;
            break;

        case FBE_SCSI_ENCRYPTION_KEY_WRAP_ERROR:
             fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                        "%s ENCRYPTION_KEY_WRAP_ERROR \n", __FUNCTION__);
             block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
             block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR;
            break;

        case FBE_SCSI_ENCRYPTION_NOT_ENABLED:
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                       "%s ENCRYPTION_NOT_ENABLED \n", __FUNCTION__);
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ENCRYPTION_NOT_ENABLED;
            break;

        case FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE:
        case FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION:
        case FBE_SCSI_CC_FORMAT_CORRUPTED:
        case FBE_SCSI_CC_SANITIZE_INTERRUPTED:
        case FBE_SCSI_BUSSHUTDOWN:
        default:
            /* Fail the drive */

            fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_UNEXPECTED_FAILURE);

            /* We need to set the BO:FAIL lifecycle condition */
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      "%s can't set BO:FAIL condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            
            }        
            block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            block_operation_status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
            break;
    }   

    /* Depends upon scsi error, log the event */
    status = fbe_base_physical_drive_log_event((fbe_base_physical_drive_t*)sas_physical_drive, error, packet, payload->physical_drive_transaction.last_error_code, payload_cdb_operation);

    if (NULL != block_operation_p)  /* packet generated by block client.  update return status.*/
    {   
        fbe_payload_block_set_status(block_operation_p, 
                                     block_operation_status,
                                     block_operation_status_qualifier); 
    }

    
    return FBE_STATUS_OK;
}


/**************************************************************************
* sas_physical_drive_set_control_status()                  
***************************************************************************
*
* DESCRIPTION
*       This function sets control operation status according to error.
*       It is used for monitor SCSI operations.
*       
* PARAMETERS
*       error - SCSI error.
*       payload_control_operation
*
* RETURN VALUES
*       Control operation status. 
*
* NOTES
*
* HISTORY
*   15-Dec-2008 chenl6 - Created.
***************************************************************************/

fbe_u32_t
sas_physical_drive_set_control_status(fbe_scsi_error_code_t error, fbe_payload_control_operation_t * payload_control_operation)
{
    fbe_payload_control_status_t control_status;
    fbe_payload_control_status_qualifier_t   control_status_qualifier;

    if ((error == 0) ||
        (error == FBE_SCSI_CC_PFA_THRESHOLD_REACHED) ||
        (error == FBE_SCSI_CC_SUPER_CAP_FAILURE) ||
        (error == FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR) ||
        (error == FBE_SCSI_CC_AUTO_REMAPPED) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP) ||
        (error == FBE_SCSI_CC_NOERR) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR) ||
        (error == FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH) ||
        (error == FBE_SCSI_CC_DIE_RETIREMENT_START)  ||
        (error == FBE_SCSI_CC_DIE_RETIREMENT_END))
    {
        /* No error or soft error */
        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
        control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
    }
    else if (error == FBE_SCSI_CC_NOT_SPINNING)
    {
        /* Not spinning */
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        control_status_qualifier = FBE_SAS_DRIVE_STATUS_NOT_SPINNING;
    }
    else if (error == FBE_SCSI_CC_BECOMING_READY)
    {
        /* Becoming ready  */
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        control_status_qualifier = FBE_SAS_DRIVE_STATUS_BECOMING_READY;
    }
    else if (error == FBE_SCSI_CC_FORMAT_IN_PROGRESS)  
    {   /* we should only ever get this during sanitization.*/
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        control_status_qualifier = FBE_SAS_DRIVE_STATUS_SANITIZE_IN_PROGRESS;
    }
    else if (error == FBE_SCSI_CC_SANITIZE_INTERRUPTED)  
    {   /* Santize was interrupted.  It needs to be re-issued or drive will continue to report errors..*/
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        control_status_qualifier = FBE_SAS_DRIVE_STATUS_SANITIZE_NEED_RESTART;
    }
    else if ((error == FBE_SCSI_CC_MODE_SELECT_OCCURRED) ||
             (error == FBE_SCSI_CC_UNIT_ATTENTION) || 
             (error == FBE_SCSI_CC_DEV_RESET))
    {
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        control_status_qualifier = FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE;
    }
    else
    {
        /* Other hard errors */
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        control_status_qualifier = FBE_SAS_DRIVE_STATUS_HARD_ERROR;
    }

    fbe_payload_control_set_status(payload_control_operation, control_status);
    fbe_payload_control_set_status_qualifier(payload_control_operation, control_status_qualifier);

    return control_status;
}


/**************************************************************************
* sas_physical_drive_setup_remap()                  
***************************************************************************
*
* DESCRIPTION
*       This function starts remap sequence for normal IOs.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       packet - Pointer to the packet.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   30-Dec-2008 chenl6 - Created.
*   01-Dec-2012  Wayne Garrett - major overhall for health check feature
***************************************************************************/

fbe_status_t
sas_physical_drive_setup_remap(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t *)sas_physical_drive;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_status_t status;
    fbe_payload_block_operation_opcode_t block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_memory_request_t * memory_request = NULL;
    fbe_bool_t  return_media_error = FBE_FALSE; /* init to fail IO as non-retryable*/
    fbe_bool_t  remap_retries_exceeded = FBE_FALSE;
    fbe_bool_t  remap_time_exceeded = FBE_FALSE;

    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload); 
    fbe_payload_block_get_opcode(block_operation, &block_operation_opcode);
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                            "%s retry count:%d pkt:%p svc:%lld pri:%d\n",
                            __FUNCTION__, payload->physical_drive_transaction.retry_count, packet, packet->physical_drive_service_time_ms, packet->packet_priority);

    base_pdo_payload_transaction_flag_set(&payload->physical_drive_transaction, BASE_PDO_TRANSACTION_STATE_IN_REMAP);

    /* The following code will verify if the remap threshold has been hit.   If so it will determine
     * how to fail the IO back to the client and what corrective action to take.
     * Note, with a write/verify we need to fail back with a media error after the 
     * retries fail.  The caller is trying to fix media errors and can handle a failure of this request.
     * Otherwise we report back a failed status.  The caller does not expect 
     * a media error on other requests like write or write same.  We recover by failing 
     * IO as non-retryable and either issue a HC (causing state change, which RAID expects on a 
     * non-retryable), or failing drive. 
    */

    /* 1. First check that a HealthCheck hasn't already been initiated.  If so, just fail this IO right away.
       If we don't do this then the remap handling could cause PDO to be stuck in activate_pending until it
       completes, significantly extending our service time.  This would then cause IOs to timeout in upper layers.
     */      
    if (fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_HEALTH_CHECK))
    {
        fbe_payload_block_set_status(block_operation, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        return FBE_STATUS_OK;
    }


    /* 2. now check for any thresholds exceeded
     */
    if (fbe_dcs_is_remap_retries_enabled()  && 
        payload->physical_drive_transaction.retry_count >= FBE_PAYLOAD_CDB_MAX_REMAPS_PER_REQ)
    {
        remap_retries_exceeded = FBE_TRUE;
        return_media_error = (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)? FBE_TRUE : FBE_FALSE;

    }
    
    if (fbe_base_drive_is_remap_service_time_exceeded(base_physical_drive, packet))
    {
        remap_time_exceeded = FBE_TRUE;
        return_media_error = (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)? FBE_TRUE : FBE_FALSE;
    }


    /* 3. Handle exceeded thresholds 
     */
    if (remap_retries_exceeded || remap_time_exceeded)
    { 
        sas_pdo_remap_action_t action = SAS_PDO_REMAP_ACTION_NONE;

        sas_pdo_get_remap_recovery_action(remap_retries_exceeded, remap_time_exceeded, return_media_error, &action);
        
        /* 3.1. set the appropriate block status
         */
        if (return_media_error)
        {
            fbe_payload_block_set_status(block_operation, 
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
        }
        else
        {
            /* If it's not a write_verify, then this is not a background op,  so we need to return fail/non-retriable.
               RAID will expect edge state change, or else they will hang, so either do HC or fail drive.
            */
            if (action == SAS_PDO_REMAP_ACTION_NONE)
            {
                fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                               "%s: Changing recovery action to Health Check. pkt: %p\n", __FUNCTION__, packet);
                action = SAS_PDO_REMAP_ACTION_HEALTH_CHECK;
            }
            fbe_payload_block_set_status(block_operation, 
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        }

        /* 3.2. take the appropriate action
         */
        switch(action)
        {
            case SAS_PDO_REMAP_ACTION_HEALTH_CHECK:
                /* If we are in a middle of a download when this happens, then easiest thing to do is abort it so we can process the health check.
                   However, due to complicated nature of download state machine, we can't enter Health Check yet.   So return IO
                   as retryable.  Eventually when download is aborted, this retried IO will trigger Health Check.
                   TODO: This is not a complete solution due to race conditions at PVD level.  This will be revisted during MR1.
                */
                if (fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD))
                {
                    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                               "setup_remap: abrt dl in progress.  ignore health check\n");
                    fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, (fbe_base_object_t*)base_physical_drive,
                                           FBE_BASE_DISCOVERED_LIFECYCLE_COND_ABORT_FW_DOWNLOAD);

                    /* tell RAID to retry */
                    fbe_payload_block_set_status(block_operation, 
                                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
                }
                else
                {
                    fbe_base_drive_initiate_health_check(base_physical_drive, FBE_TRUE /*is_service_timeout*/);
    
                    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                   "%s: initiated health check. pkt: %p\n", __FUNCTION__, packet);
                }
                break;

            case SAS_PDO_REMAP_ACTION_FAIL_DRIVE:
                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_MAX_REMAPS_EXCEEDED);

                /* We need to set the BO:FAIL lifecycle condition */
                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                          "%s can't set FAIL condition, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
                break;
        }

        return FBE_STATUS_OK;
    }

        
    /* 4. remap threshold not hit, so try another remap
     */
    payload->physical_drive_transaction.retry_count++;

    memory_request = fbe_transport_get_memory_request(packet);
    memory_request->request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_BUFFER;
    memory_request->number_of_objects = 2;
    memory_request->ptr = NULL;
    memory_request->completion_function = (fbe_memory_completion_function_t) sas_physical_drive_setup_remap_completion;
    memory_request->completion_context = sas_physical_drive;
    fbe_memory_allocate(memory_request);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/**************************************************************************
* sas_pdo_get_remap_recovery_action()                  
***************************************************************************
*
* DESCRIPTION
*       This function determines the recovery action when remap threshold
*       is exceeded.
*       
* PARAMETERS
*       remap_retries_exceeded - indicates if retries exceeded
*       remap_time_exceeded - indicates if service time exceeded
*       return_media_error -  indicates if we returning a media error
*       action -  action to be returned
*
* RETURN VALUES
*       none. 
*
* NOTES
*
* HISTORY
*   11/27/2012  Wayne Garrett - Created.
***************************************************************************/
static void sas_pdo_get_remap_recovery_action(fbe_bool_t remap_retries_exceeded, fbe_bool_t remap_time_exceeded, fbe_bool_t return_media_error, sas_pdo_remap_action_t *action)
{
    *action = SAS_PDO_REMAP_ACTION_NONE;


    /* note in the following code, the service time exceeded take precedence over retry counts, since this is probably the more
       critical error */

    if (return_media_error) /* i.e. write/verify opcode */
    {
        if (remap_time_exceeded)
        {
            *action = SAS_PDO_REMAP_ACTION_HEALTH_CHECK;
        }
        else if (remap_retries_exceeded)
        {
            *action = SAS_PDO_REMAP_ACTION_NONE;
        }
    }
    else /* return fail/non_retryable */
    {
        /* for either service time exceeded or retries exceeded, we'll do the same thing*/
        *action = fbe_dcs_use_hc_for_remap_non_retryable() ? SAS_PDO_REMAP_ACTION_HEALTH_CHECK : SAS_PDO_REMAP_ACTION_FAIL_DRIVE;
    }
}


/**************************************************************************
* sas_physical_drive_setup_remap_completion()                  
***************************************************************************
*
* DESCRIPTION
*       This function starts remap sequence for normal IOs.
*       
* PARAMETERS
*       memory_request - Pointer to memory request.
*       context - Context of the completion routine.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   30-Dec-2008 chenl6 - Created.
***************************************************************************/

fbe_status_t
sas_physical_drive_setup_remap_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_sas_physical_drive_t * sas_physical_drive = NULL;
    fbe_packet_t * remap_packet = NULL;  /* pkt used for handling reserve, reassign and release*/
    fbe_packet_t * packet = NULL;
    fbe_payload_ex_t * payload = NULL, * master_payload = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_status_t status;
    void ** chunk_array = memory_request->ptr;

    sas_physical_drive = (fbe_sas_physical_drive_t *)context;

    packet = fbe_transport_memory_request_to_packet(memory_request);
    remap_packet = chunk_array[0];
    sg_list = chunk_array[1];
    fbe_sg_element_terminate(sg_list);

    /* Initialize sub packet. */
    fbe_transport_initialize_packet(remap_packet);
    payload = fbe_transport_get_payload_ex(remap_packet);
    master_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_set_sg_list(payload, sg_list, 1);
    payload->physical_drive_transaction.retry_count = 0;
    payload->physical_drive_transaction.last_error_code = master_payload->physical_drive_transaction.last_error_code;

    /* We need to assosiate newly allocated packet with original one */
    fbe_transport_add_subpacket(packet, remap_packet);

    /* It is possible that this is an encrypted system and key handle might be set, irrespective 
     * since this command has a partthat is sent as part of data phase we dont want to encrypt that
     * e.g. LBA to remap is set in the sg list and we dont want to encrypt that. So set the key
     * handle to be invalid */
    fbe_payload_ex_set_key_handle(payload, FBE_INVALID_KEY_HANDLE);

    /* Put packet on the termination queue while we wait for the subpacket to complete. */
    /* NOTE, the cancelling will not work for this packet.   The base PDOs cancel condition, which is called by fbe_base_object_packet_cancel_function,
       will only cancel pkts on the transport queue.   Not the terminator queue.  Currently as it stands,  even if RAID cancels it,
       the remap will continue to completion.  It has worked this way since day one. */
    /* TODO: Need to talk to RAID to see if okay for remap pkt to not be cancelled. The solution would be to provide a different
       cancel function */
    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)sas_physical_drive);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)sas_physical_drive, packet);

    status = fbe_sas_physical_drive_send_reserve(sas_physical_drive, remap_packet);
    return status;
}


/**************************************************************************
* sas_physical_drive_end_remap()                  
***************************************************************************
*
* DESCRIPTION
*       This function ends remap sequence for normal IOs.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       packet - Pointer to the packet.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   30-Dec-2008 chenl6 - Created.
***************************************************************************/

fbe_status_t
sas_physical_drive_end_remap(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_packet_t * master_packet = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_block_operation_status_t block_status;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "%s \n", __FUNCTION__);

    /* Release sg_list. */
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    fbe_memory_release(sg_list);

    /* Free up the io packet we allocated. */
    fbe_transport_remove_subpacket(packet);
    fbe_transport_release_packet(packet);

    status = fbe_transport_get_status_code(master_packet);
    payload = fbe_transport_get_payload_ex(master_packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);       
    fbe_payload_block_get_status(block_operation, &block_status);

    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)sas_physical_drive, master_packet);

    if ((status != FBE_STATUS_OK) ||         /* Transport failure during reserve, reassign or release. */
        (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED))          /* Reassign failed. */
    {
        /* Complete master packet here. */
        fbe_transport_update_physical_drive_service_time(packet);
        fbe_transport_complete_packet(master_packet);
    }
    else
    {
        /* Remap succeeds. Retry master packet. */
        status = sas_physical_drive_process_resend_block_cdb(sas_physical_drive, master_packet);
    }

    return FBE_STATUS_OK;
}


/**************************************************************************
* sas_physical_drive_is_failable_op()                  
***************************************************************************
*
* DESCRIPTION
*       This function ends remap sequence for normal IOs.
*       
* PARAMETERS
*       block_operation_opcode - block operation opcode.
*
* RETURN VALUES
*       Whether the block operation is failable. 
*
* NOTES
*
* HISTORY
*   30-Dec-2008 chenl6 - Created.
***************************************************************************/

fbe_bool_t 
sas_physical_drive_is_failable_op(fbe_payload_block_operation_opcode_t block_operation_opcode)
{
    fbe_bool_t failable = FALSE;

    switch (block_operation_opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            failable = TRUE;
            break;

        default:
            failable = FALSE;
            break;
    }

    return failable;
}

/**************************************************************************
* sas_physical_drive_set_dc_control_status()                  
***************************************************************************
*
* DESCRIPTION
*       This function sets control operation status for disk collect according to error.
*       It is used for monitor SCSI operations.
*       
* PARAMETERS
*       error - SCSI error.
*       payload_control_operation
*
* RETURN VALUES
*       Control operation status. 
*
* NOTES
*
* HISTORY
*   11-March-2010 Christina Chiang - Created.
***************************************************************************/

fbe_u32_t
sas_physical_drive_set_dc_control_status(fbe_sas_physical_drive_t * sas_physical_drive, fbe_scsi_error_code_t error, fbe_payload_control_operation_t * payload_control_operation)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_status_t control_status;
    fbe_payload_control_status_qualifier_t   control_status_qualifier;

    switch(error)
    {
        case 0:
        case FBE_SCSI_CC_AUTO_REMAPPED:
        case FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP:
        case FBE_SCSI_CC_RECOVERED_ERR:
        case FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH:
        case FBE_SCSI_CC_NOERR:
        case FBE_SCSI_CC_RECOVERED_WRITE_FAULT:
        case FBE_SCSI_CC_DIE_RETIREMENT_START:
        case FBE_SCSI_CC_DIE_RETIREMENT_END:
            /* No error, soft error, retry possible error. */
            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
            control_status_qualifier = FBE_SAS_DRIVE_STATUS_OK;
            break;

        case FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR:
        case FBE_SCSI_CC_SUPER_CAP_FAILURE:
            /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
            fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
            control_status_qualifier = FBE_SAS_DRIVE_STATUS_SET_PROACTIVE_SPARE;
            break;

        case FBE_SCSI_CC_PFA_THRESHOLD_REACHED:
            /*force the drive to act as going to die soon, this should trigger proactive sparing at upper levels*/
            fbe_base_physical_drive_set_path_attr((fbe_base_physical_drive_t*)sas_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);
            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
            control_status_qualifier = FBE_SAS_DRIVE_STATUS_SET_PROACTIVE_SPARE;
            break;
        
        case FBE_SCSI_CC_WRITE_PROTECT:
                fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_WRITE_PROTECT);

                /* We need to set the BO:FAIL lifecycle condition */
                status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                                FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

                if (status != FBE_STATUS_OK) {
                    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          "%s can't set BO:FAIL condition, status: 0x%X\n",
                                          __FUNCTION__, status);
            
                }

            control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
            control_status_qualifier = FBE_SAS_DRIVE_STATUS_HARD_ERROR;
            break;

        case FBE_SCSI_CC_SENSE_DATA_MISSING:
        case FBE_SCSI_CC_DEV_RESET:
        case FBE_SCSI_BUSRESET:
        case FBE_SCSI_SELECTIONTIMEOUT:
        case FBE_SCSI_DEVUNITIALIZED:
        case FBE_SCSI_AUTO_SENSE_FAILED:
        case FBE_SCSI_BADBUSPHASE:
        case FBE_SCSI_BADSTATUS:
        case FBE_SCSI_XFERCOUNTNOTZERO:
        case FBE_SCSI_INVALIDSCB:
        case FBE_SCSI_TOOMUCHDATA:
        case FBE_SCSI_CHANNEL_INACCESSIBLE:
        case FBE_SCSI_IO_LINKDOWN_ABORT:
        case FBE_SCSI_FCP_INVALIDRSP:
        case FBE_SCSI_UNKNOWNRESPONSE:
        case FBE_SCSI_CC_ILLEGAL_REQUEST:
        case FBE_SCSI_INVALIDREQUEST:
        case FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR:
        case FBE_SCSI_CC_ABORTED_CMD:
        case FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR:
        case FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE:
        case FBE_SCSI_CC_HARDWARE_ERROR_PARITY:    
        case FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST:                
        case FBE_SCSI_CC_HARDWARE_ERROR:
        case FBE_SCSI_CC_MEDIA_ERR_DONT_REMAP:
        case FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP:
        case FBE_SCSI_CC_MODE_SELECT_OCCURRED:
        case FBE_SCSI_CC_UNIT_ATTENTION:
        case FBE_SCSI_CC_UNEXPECTED_SENSE_KEY:
        case FBE_SCSI_CHECK_COND_OTHER_SLOT:
        case FBE_SCSI_CC_SEEK_POSITIONING_ERROR:
        case FBE_SCSI_CC_SEL_ID_ERROR:
        case FBE_SCSI_SLOT_BUSY:
        case FBE_SCSI_DEVICE_BUSY:
        case FBE_SCSI_DEVICE_RESERVED:
        case FBE_SCSI_CC_NOT_READY:
        case FBE_SCSI_CC_FORMAT_IN_PROGRESS:
        case FBE_SCSI_DEVICE_NOT_READY:
        case FBE_SCSI_CC_BECOMING_READY:
        case FBE_SCSI_IO_TIMEOUT_ABORT:
        case FBE_SCSI_SATA_IO_TIMEOUT_ABORT:
        case FBE_SCSI_CC_DATA_OFFSET_ERROR:
        case FBE_SCSI_CC_DEFERRED_ERROR:
        case FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT:
        case FBE_SCSI_CC_INTERNAL_TARGET_FAILURE:
        case FBE_SCSI_CC_NOT_SPINNING:
        case FBE_SCSI_INCIDENTAL_ABORT:        
        /* normaly this error fails the drive.  If for whatever reason DC tries to log expected errors during a sanitize glitch,  we should not shoot the drive.*/          
        case FBE_SCSI_CC_SANITIZE_INTERRUPTED:
            control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
            control_status_qualifier = FBE_SAS_DRIVE_STATUS_NEED_RETRY;
            break;
    
        case FBE_SCSI_CC_RECOVERED_BAD_BLOCK:
        case FBE_SCSI_CC_HARD_BAD_BLOCK:
        case FBE_SCSI_CC_MEDIA_ERR_DO_REMAP:
        case FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT:
        case FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR:
            /* We use remap strategy on WRITE operations. */
            control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
            control_status_qualifier = FBE_SAS_DRIVE_STATUS_NEED_REMAP;
            break;           

        case FBE_SCSI_CC_DEFECT_LIST_ERROR:              
        case FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE:        
        case FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION:                
        case FBE_SCSI_CC_FORMAT_CORRUPTED: 
        case FBE_SCSI_BUSSHUTDOWN:
        default:

            /* Fail the drive */
            fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_UNEXPECTED_FAILURE);
            
            /* We need to set the BO:FAIL lifecycle condition */
            status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      "%s can't set BO:FAIL condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            
            }        
            control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
            control_status_qualifier = FBE_SAS_DRIVE_STATUS_HARD_ERROR;
            break;
    }

    fbe_payload_control_set_status(payload_control_operation, control_status);
    fbe_payload_control_set_status_qualifier(payload_control_operation, control_status_qualifier);

    return control_status;
}

/**************************************************************************
* sas_physical_drive_setup_dc_remap()                  
***************************************************************************
*
* DESCRIPTION
*       This function starts remap sequence for disk collect IOs.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       packet - Pointer to the packet.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   11-March-2010 CLC - Created.
***************************************************************************/

fbe_status_t
sas_physical_drive_setup_dc_remap(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "%s retry count: %d\n",
                            __FUNCTION__, payload->physical_drive_transaction.retry_count);

    if (payload->physical_drive_transaction.retry_count >= FBE_PAYLOAD_CDB_MAX_REMAPS_PER_REQ)
    {
         fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)sas_physical_drive, FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_MAX_REMAPS_EXCEEDED);
         
         /* We need to set the BO:FAIL lifecycle condition */
         status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, (fbe_base_object_t*)sas_physical_drive, 
                                         FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);

         if (status != FBE_STATUS_OK) {
             fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  "%s can't set BO:FAIL condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        
        return FBE_STATUS_OK;
    }
    payload->physical_drive_transaction.retry_count++;
    status = fbe_sas_physical_drive_send_reserve(sas_physical_drive, packet);

    return status;
}
/**************************************************************************
* sas_physical_drive_end_dc_remap()                  
***************************************************************************
*
* DESCRIPTION
*       This function ends remap sequence for Disk Collect IOs.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       packet - Pointer to the packet.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   26-July-2010 CLC - Created.
***************************************************************************/

fbe_status_t
sas_physical_drive_end_dc_remap(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "%s\n",__FUNCTION__);

    /* Release sg_list. */
    payload = fbe_transport_get_payload_ex(packet);
    //fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    //fbe_memory_release(sg_list);

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        return FBE_STATUS_OK; 
    }
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s fbe DC set flush data.\n", __FUNCTION__);

    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                             (fbe_base_object_t*)sas_physical_drive, 
                             FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DISK_COLLECT_FLUSH);
                             
    return FBE_STATUS_OK;

}

/**************************************************************************
* sas_physical_drive_set_qst_control_status()                  
***************************************************************************
*
* DESCRIPTION
*       This function sets health check control operation status according to error.
*       It is used for monitor SCSI operations.
*       
* PARAMETERS
*       error - SCSI error.
*       payload_control_operation
*
* RETURN VALUES
*       Control operation status. 
*
* NOTES
*       This function is ONLY intented for QST (Quick Self Test) cmd as a
*       result of service timeout.   Don't use for QST maintanence feature.
*       I suggest renaming these functions later (when maintanence feature
*       is added) to make it clear for which case it covers.
*
* HISTORY
*   27-Feb-2013 chianc1 - Created.
*   10-May-2013 Wayne Garrett - added default error handling.
***************************************************************************/

fbe_u32_t
sas_physical_drive_set_qst_control_status(fbe_sas_physical_drive_t * sas_physical_drive, fbe_scsi_error_code_t error, fbe_payload_control_operation_t * payload_control_operation)
{
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;    
    fbe_drive_error_handling_bitmap_t       cmd_status = FBE_DRIVE_CMD_STATUS_INVALID;
    fbe_physical_drive_death_reason_t       death_reason = FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_INVALID;

    sas_physical_drive_get_default_error_handling(error, &cmd_status, &death_reason);
   
    /* convert drive cmd status to control status
     */
    if (cmd_status & FBE_DRIVE_CMD_STATUS_SUCCESS)
    {
        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else if (cmd_status & FBE_DRIVE_CMD_STATUS_FAIL_RETRYABLE)
    {
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }
    else if (cmd_status & FBE_DRIVE_CMD_STATUS_FAIL_NON_RETRYABLE)
    {
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }
    
    /* override exceptions */
    switch (error)
    {
        case FBE_SCSI_IO_TIMEOUT_ABORT:
            /* If we got a cmd timeout, then good chance drive fw is hung.  A
               hard reset is needed to fix this issue. */
            control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
            cmd_status = FBE_DRIVE_CMD_STATUS_RESET;
            break;
    }

    fbe_payload_control_set_status(payload_control_operation, control_status);
    fbe_payload_control_set_status_qualifier(payload_control_operation, cmd_status);

    return control_status;
}

        
/**************************************************************************
* sas_physical_drive_get_default_error_handling()                  
***************************************************************************
*
* DESCRIPTION
*       This function gets the default error handling
*       
* PARAMETERS
*       error        - IN  - the rolled up fbe scsi error code
*       cmd_status   - OUT - bitmap indicating how to handle the error
*       death_reason - OUT - If drive should be failed, then death
*                            reason is set.
*
* RETURN VALUES
*       void. 
*
* NOTES
*
* HISTORY
*   5/8/2013 : Wayne Garrett - Created.
***************************************************************************/
void sas_physical_drive_get_default_error_handling(fbe_scsi_error_code_t error, fbe_drive_error_handling_bitmap_t *cmd_status, fbe_physical_drive_death_reason_t *death_reason)
{
    *cmd_status = FBE_DRIVE_CMD_STATUS_INVALID;
    *death_reason = FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_INVALID;


    switch(error)
    {
        /* No error. */
        case 0:
        case FBE_SCSI_CC_NOERR:        
        case FBE_SCSI_CC_AUTO_REMAPPED:
        case FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP:
        case FBE_SCSI_CC_RECOVERED_ERR:
        case FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH:
        case FBE_SCSI_CC_RECOVERED_WRITE_FAULT:
        case FBE_SCSI_CC_RECOVERED_BAD_BLOCK:
        case FBE_SCSI_CC_DIE_RETIREMENT_START:
        case FBE_SCSI_CC_DIE_RETIREMENT_END:             
            *cmd_status = FBE_DRIVE_CMD_STATUS_SUCCESS;
            break;

        /* No error, but triggers EOL */
        case FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR:          
        case FBE_SCSI_CC_SUPER_CAP_FAILURE:                     
            *cmd_status = ( FBE_DRIVE_CMD_STATUS_SUCCESS |
                            FBE_DRIVE_CMD_STATUS_EOL );
            break;

        case FBE_SCSI_CC_PFA_THRESHOLD_REACHED: 
            *cmd_status = FBE_DRIVE_CMD_STATUS_SUCCESS;
            if (fbe_dcs_param_is_enabled(FBE_DCS_PFA_HANDLING))
            {
                *cmd_status |= FBE_DRIVE_CMD_STATUS_EOL;
            }
            break;
        
        /* retry link errors*/
        case FBE_SCSI_BUSRESET:
        case FBE_SCSI_IO_TIMEOUT_ABORT:
        case FBE_SCSI_SELECTIONTIMEOUT:
        case FBE_SCSI_DEVUNITIALIZED:
        case FBE_SCSI_AUTO_SENSE_FAILED:
        case FBE_SCSI_BADBUSPHASE:
        case FBE_SCSI_BADSTATUS:
        case FBE_SCSI_XFERCOUNTNOTZERO:
        case FBE_SCSI_INVALIDSCB:
        case FBE_SCSI_TOOMUCHDATA:
        case FBE_SCSI_CHANNEL_INACCESSIBLE:
        case FBE_SCSI_IO_LINKDOWN_ABORT:
        case FBE_SCSI_FCP_INVALIDRSP:
        case FBE_SCSI_UNKNOWNRESPONSE:
        case FBE_SCSI_INVALIDREQUEST:        
        case FBE_SCSI_CHECK_COND_OTHER_SLOT:
        case FBE_SCSI_SLOT_BUSY:
        case FBE_SCSI_DEVICE_BUSY:
        case FBE_SCSI_DEVICE_RESERVED:
        case FBE_SCSI_DEVICE_NOT_READY:
        case FBE_SCSI_SATA_IO_TIMEOUT_ABORT:
        /* retry 02 not ready errors.  */
        case FBE_SCSI_CC_NOT_READY:
        case FBE_SCSI_CC_FORMAT_IN_PROGRESS:
        case FBE_SCSI_CC_BECOMING_READY: 
        case FBE_SCSI_CC_NOT_SPINNING:
        /*retry 03 unrecovered media erros */
        case FBE_SCSI_CC_MEDIA_ERR_DO_REMAP:
        case FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT:
        case FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR:
        case FBE_SCSI_CC_MEDIA_ERR_DONT_REMAP:
        case FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP:
        case FBE_SCSI_CC_HARD_BAD_BLOCK:
        /* retry 04 hw errors */
        case FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE:
        case FBE_SCSI_CC_SEEK_POSITIONING_ERROR:
        case FBE_SCSI_CC_INTERNAL_TARGET_FAILURE:
        case FBE_SCSI_CC_HARDWARE_ERROR_PARITY:
        case FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT:
        case FBE_SCSI_CC_HARDWARE_ERROR:
        /* retry 05 illegal cmds.  All drives have to support this cmd, so this should never happen unless sw bug, which will be caught during testing*/
        case FBE_SCSI_CC_ILLEGAL_REQUEST:   
        /* retry 06 unit attention & deferred errors*/    
        case FBE_SCSI_CC_MODE_SELECT_OCCURRED:
        case FBE_SCSI_CC_UNIT_ATTENTION:
        case FBE_SCSI_CC_DEV_RESET:
        case FBE_SCSI_CC_DEFERRED_ERROR:
        /* retry 0B aborted cmds */
        case FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR:
        case FBE_SCSI_CC_ABORTED_CMD:
        /* retry 09 errors*/
        case FBE_SCSI_CC_SEL_ID_ERROR: 
        case FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR:
        /* retry unexpected */
        case FBE_SCSI_CC_UNEXPECTED_SENSE_KEY: 
        case FBE_SCSI_CC_DATA_OFFSET_ERROR: /* obsolete */
        case FBE_SCSI_CC_SENSE_DATA_MISSING:        

            *cmd_status = FBE_DRIVE_CMD_STATUS_FAIL_RETRYABLE;
            break;

        /* TODO: If we get self_test failure during TUR then GDE (aka SBMT) recommends retry of 5
           times before shooting.  This is a future ehancement.  Currently today (5/10/13) we will shoot the
           drive after retrying TUR for about 3 minutes. -wayne */
        case FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST:
            *cmd_status = (FBE_DRIVE_CMD_STATUS_FAIL_RETRYABLE |
                           FBE_DRIVE_CMD_STATUS_EOL);
            break;

        /* fail any of the remaining errors since this indicates we have a serious issue.
        */
        case FBE_SCSI_CC_WRITE_PROTECT: 
            *cmd_status = (FBE_DRIVE_CMD_STATUS_FAIL_NON_RETRYABLE | FBE_DRIVE_CMD_STATUS_KILL);
            *death_reason = FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_WRITE_PROTECT;
            break;

        case FBE_SCSI_CC_DEFECT_LIST_ERROR:
            *cmd_status = (FBE_DRIVE_CMD_STATUS_FAIL_NON_RETRYABLE | FBE_DRIVE_CMD_STATUS_KILL);
            *death_reason = FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DEFECT_LIST_ERRORS;
            break;
                                                      
        case FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE:
        case FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION:
        case FBE_SCSI_CC_DRIVE_TABLE_REBUILD:
        case FBE_SCSI_CC_SYNCH_SUCCESS:
        case FBE_SCSI_CC_SYNCH_FAIL:
        case FBE_SCSI_CC_FORMAT_CORRUPTED: 
        case FBE_SCSI_CC_SANITIZE_INTERRUPTED:
        case FBE_SCSI_BUSSHUTDOWN:  /* obsolete */       
        default:
            *cmd_status = (FBE_DRIVE_CMD_STATUS_FAIL_NON_RETRYABLE | FBE_DRIVE_CMD_STATUS_KILL);
            *death_reason = FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_UNEXPECTED_FAILURE;
            break;
    }
}

