/***************************************************************************
 *  fbe_terminator_sas_drive_neit.c
 ***************************************************************************
 *
 *  Description
 *      Error Injection
 *
 *
 *  History:
 *            9/22/2008 Added JJB
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_physical_drive.h"
#include "terminator_sas_io_neit.h"
#include "fbe_terminator.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator_common.h"


/**********************************/
/*        Variables         */
/**********************************/

/* This variable maintains the state of the NEIT. It is initialized
 * to NEIT_NOT_INIT. Other states are init, start and stop.
 */
fbe_neit_state_monitor_s fbe_neit_state = {NEIT_NOT_INIT};


/* This variable points to the head of the list of
 * neit_drive_error_tables
 */
fbe_queue_head_t*  gneit_drive_error_table_head = NULL;

static fbe_u32_t fbe_terminator_get_elapsed_seconds(fbe_time_t timestamp);

/*************************/
/*     functions         */
/*************************/


fbe_status_t fbe_terminator_sas_payload_insert_error(fbe_payload_cdb_operation_t  *payload_cdb_operation,
                                                     fbe_terminator_device_ptr_t drive_ptr)
{
    /* This function is the external entry point for error insertion
       It uses fbe_payload_cdb_operation_t and the drive handle*/

    fbe_lba_t lba;
    fbe_lba_t bad_lba;
    fbe_u32_t cdb_length;
    fbe_block_count_t blocks;
    neit_drive_t  drive;
    fbe_u32_t sk_from_drive;
    fbe_u8_t op_code;

    fbe_neit_search_params_t search;

    fbe_terminator_neit_error_record_t* error_record_match = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;


    /* No point in getting into the function if the NEIT Start flag
     * is not turned on yet.
     */
    if(fbe_neit_state.current_state != NEIT_START)
    {
        return FBE_STATUS_OK;
    }

    /* Get the port number and the device ID from the drive handle */
    status = terminator_get_port_index(drive_ptr, &drive.port_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get port number thru drive handle\n", __FUNCTION__);
        return status;
    }
    status = terminator_get_drive_enclosure_number(drive_ptr, &drive.enclosure_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get enclosure_number thru drive handle\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return status;
    }
    status = terminator_get_drive_slot_number(drive_ptr, &drive.slot_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get slot_number thru drive handle\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return status;
    }

    sk_from_drive = (payload_cdb_operation->sense_info_buffer[2] & 0x0F);
    op_code = payload_cdb_operation->cdb[0];
    cdb_length = payload_cdb_operation->cdb_length;
    if (cdb_length == NEIT_SIX_BYTE_CDB)
    {
        lba = fbe_get_six_byte_cdb_lba(&payload_cdb_operation->cdb[0]);
        blocks = fbe_get_six_byte_cdb_blocks(&payload_cdb_operation->cdb[0]);
    }
    else if (cdb_length == NEIT_TEN_BYTE_CDB)
    {
        lba = fbe_get_ten_byte_cdb_lba(&payload_cdb_operation->cdb[0]);
        blocks = fbe_get_ten_byte_cdb_blocks(&payload_cdb_operation->cdb[0]);
    }
    else
    {
        // unsupported cdb length
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! Unsupported cdb length\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Don't inject an error if the drive itself has reported an error
       pass the real error through through. */
    if (sk_from_drive != 0)
    {
        return FBE_STATUS_OK;
    }
    /* Fill in the search structure from the data in the payload
       and drive handle.
     */
    fbe_drive_copy( &search.drive, &drive);
    search.lba_start = lba;
    search.lba_end = lba + (fbe_lba_t)blocks - 1;
    search.opcode = op_code;
    /* Search the error tables for a match to the error.
     * If none is found return since there is no need to
     * do error injection.
     */
    error_record_match = fbe_neit_search_error_rules(&search);
    if (error_record_match == NULL)
    {
        /* No error injection required - just return */
        return FBE_STATUS_OK;
    }

    /* An error record was found matching the search criteria
     * so continue on to error injection
     */

    /* Insert the error only if it is not a timeout error and
     * the error has not been inserted the specified number
     * of times already
     */
    if ( (error_record_match->times_inserted <=
              error_record_match->num_of_times_to_insert) &&
         (error_record_match->is_port_error == FBE_FALSE)  )
    {
        /* Find the bad lba to report in the sense data
         * If the search record is less than the error rule,
         * Then we start injecting errors at the error rule start address,
         * because this is the first address that should have an error
         * in the range of this I/O  (which is described by search_p).
         */
        if ( search.lba_start <= error_record_match->lba_start )
        {
            bad_lba = error_record_match->lba_start;
        }
        else
        {
            /* In all other cases we assume that the search_p
             * starts within the range of the error rule.
             * The first error address within the range of
             * the search record is search_p->lba_start.
             */
            bad_lba = search.lba_start;
        }

        /* Now modify the sense data to inject the error.
         */
        if ((error_record_match->error_sk == FBE_SCSI_SENSE_KEY_MEDIUM_ERROR &&
            error_record_match->error_asc == FBE_SCSI_ASC_VENDOR_SPECIFIC_FF &&
            error_record_match->error_ascq == FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB )) {
            /*this is here to allow us inject the FBE_SCSI_AUTO_SENSE_FAILED, if we don't set the 0
            here, the drive code would set a different error (see shay for details)*/
            payload_cdb_operation->sense_info_buffer[0] = 0x0;
        }else if ((error_record_match->error_sk == FBE_SCSI_SENSE_KEY_RECOVERED_ERROR) &&
                  (error_record_match->error_asc == FBE_SCSI_ASC_RECOVERED_WITH_ECC) &&
                  (error_record_match->error_ascq == FBE_SCSI_ASCQ_GENERAL_QUALIFIER)){
            /*this is here to allow us inject the FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP, if we don't set the 0
            here, the drive code would set a different error (see shay for details)*/
            payload_cdb_operation->sense_info_buffer[0] = 0x70;
        }else {
            /* The first byte of the sense data is filled in with
            * 0xF0 to indicate a valid LBA in bytes 3 through 6 and
            * to indicate that the error is for the current command.
            */
            payload_cdb_operation->sense_info_buffer[0] = 0xF0;
        }

        /* The sense key, additional sense code and additional sense code
         * qualifier are filled in from the error record.
         */
        payload_cdb_operation->sense_info_buffer[2] =
                                    error_record_match->error_sk;
        payload_cdb_operation->sense_info_buffer[12] =
                                    error_record_match->error_asc;
        payload_cdb_operation->sense_info_buffer[13] =
                                    error_record_match->error_ascq;

        /* The bad lba information is filled in.
         */
        payload_cdb_operation->sense_info_buffer[6] =
                                        (fbe_u8_t)(0xFF & bad_lba);
        payload_cdb_operation->sense_info_buffer[5] =
                                  (fbe_u8_t)(0xFF & (bad_lba >> 8));
        payload_cdb_operation->sense_info_buffer[4] =
                                  (fbe_u8_t)(0xFF & (bad_lba >> 16));
        payload_cdb_operation->sense_info_buffer[3] =
                                  (fbe_u8_t)(0xFF & (bad_lba >> 24));

        /* Set a check condition in the CDB SCSI status
         */
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);

        /* Increment the error counters in the error record.
         */
        error_record_match->times_inserted++;

        /* Check to see if we have we gone over our threshold.
         */
        if(error_record_match->times_inserted >=
                        error_record_match->num_of_times_to_insert)
        {
            /* Record the time that we went over the threshold.
             */
            error_record_match->max_times_hit_timestamp = fbe_get_time();
        }
    }else if ( (error_record_match->times_inserted <=
               error_record_match->num_of_times_to_insert) &&
               (error_record_match->is_port_error == FBE_TRUE)  )
    {
        /*we need to insert a port error on the payload*/
        fbe_payload_cdb_set_request_status (payload_cdb_operation, error_record_match->port_status);
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation, error_record_match->port_force_scsi_status);
        payload_cdb_operation->sense_info_buffer[0] = 0xF0;
        payload_cdb_operation->sense_info_buffer[2] = error_record_match->error_sk;

         /* Increment the error counters in the error record.
         */
        error_record_match->times_inserted++;

        /* Check to see if we have we gone over our threshold.
         */
        if(error_record_match->times_inserted >=
                        error_record_match->num_of_times_to_insert)
        {
            /* Record the time that we went over the threshold.
             */
            error_record_match->max_times_hit_timestamp = fbe_get_time();
        }
    }

    return FBE_STATUS_OK;

}

fbe_terminator_neit_error_record_t*  fbe_neit_search_error_rules(fbe_neit_search_params_t* search)
{
    /* This function will search through all of the error records
     * and look for a match to the search criteria specified in the
     * search structure.
     * If a matching record is found a pointer to it is returned
     * otherwise the pointer is NULL.
     */

    fbe_neit_drive_error_table_t*  drive_error_table = NULL;
    fbe_queue_element_t*  error_list_element = NULL;
    fbe_neit_err_list_t*  error_list = NULL;
    fbe_terminator_neit_error_record_t*  error_record_current = NULL;

    /* Create a generic error record pointer to potentially match a
     * a more general "any" range in an error record.
     * Initially set it equal to NULL.
     */
    fbe_terminator_neit_error_record_t* error_record_generic = NULL;


    /* Search the list of error tables for a matching drive */
    drive_error_table = fbe_neit_search_drive_table(search->drive);

    if (drive_error_table == NULL)
    {
        /* None was found so return NULL */
        return NULL;
    }

    /* The drive error table table is not NULL so get the first
     * error list element off of it. It will be NULL if empty */

    error_list_element =
                   fbe_queue_front( &(drive_error_table->neit_err_list_head) );

    while (error_list_element != NULL)
    {
        /* Cast the element to an error list object */
        error_list = (fbe_neit_err_list_t*)error_list_element;
        error_record_current = error_list->error_record;

        /* This error record may no longer be active but
         * may be eligible to be reactivated.
         */
        if ((error_record_current->times_inserted >=
                    error_record_current->num_of_times_to_insert) &&
            (error_record_current->secs_to_reactivate != NEIT_INVALID_SECONDS) &&
            (fbe_terminator_get_elapsed_seconds(error_record_current->max_times_hit_timestamp) >=
                    error_record_current->secs_to_reactivate) &&
            ( (error_record_current->num_of_times_to_reactivate == NEIT_ALWAYS_REACTIVATE) ||
              (error_record_current->times_reset < error_record_current->num_of_times_to_reactivate)  )  )
        {
            /* This record should be reactivated now by resetting
             * times_inserted.
             */
             error_record_current->times_inserted = 0;
            /* Increment the number of times we have reset
             * in the times_reset field.
             */
             error_record_current->times_reset++;
        }

        /* There is a match if the records overlap in some way
         * AND the record is active
         */
        if ( (search->lba_start <=
                            error_record_current->lba_end) &&
             (error_record_current->lba_start <=
                            search->lba_end) &&
             (error_record_current->times_inserted <
                    error_record_current->num_of_times_to_insert) )
        {
            /* There is a match of the specific LBA range. Now look for
             * specific opcode. If there is no match, check if there is
             * any thing with any range.
             */
            if(error_record_current->opcode == search->opcode)
            {
                /* We got an exact match here. This record can be returned.
                 */
                return (error_record_current);
            }
            else if ( ( error_record_current->opcode == NEIT_ANY_OPCODE) &&
                       ( error_record_generic == NULL ) )
            {
                /* We have match any opcode.  Take the first one
                 * we find and save it in the gen_ptr.
                 */
                error_record_generic = error_record_current;
            }
        }

        /* Goto the next error list element and continue searching */
        error_list_element =
                   fbe_queue_next( &(drive_error_table->neit_err_list_head), error_list_element );
    }

    /* If we get to here we haven't found an error record that matches
     * the specifics of the LBA/Opcode. So return the generic pointer.
     * Return the generic opcode containing error record.
     */
    return error_record_generic;
}


fbe_status_t  fbe_neit_init(void)
{
    if( fbe_neit_state.current_state == NEIT_NOT_INIT)
    {
        /* Create an empty drive error table list */
        gneit_drive_error_table_head = NULL;

        gneit_drive_error_table_head =
               (fbe_queue_head_t *)fbe_terminator_allocate_memory(sizeof(fbe_queue_head_t));

        if (gneit_drive_error_table_head == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_queue_init(gneit_drive_error_table_head);

        /* Init has worked so set the state */
        fbe_neit_state.current_state = NEIT_INIT;
    }

    return FBE_STATUS_OK;
}


fbe_status_t  fbe_neit_close(void)
{
    /* Free the entire drive error table and the head
     */

    fbe_queue_element_t* drive_error_table_element = NULL;

    /* If the current state is not initialized the drive error table
     * is already empty so just return ok
     */
    if (fbe_neit_state.current_state == NEIT_NOT_INIT)
    {
        return FBE_STATUS_OK;
    }

    /* Get the first drive_error_table_element.
     */
    drive_error_table_element =
                         fbe_queue_pop(gneit_drive_error_table_head);

    while (drive_error_table_element != NULL)
    {
        /* The drive error table element is not NULL so get the first
         * error list element off of it. It will be NULL if empty */

        fbe_queue_element_t* error_list_element =
                          fbe_queue_pop( &(((fbe_neit_drive_error_table_t*)drive_error_table_element)->neit_err_list_head) );

        while (error_list_element != NULL)
        {
           /* Free the error_record */
           fbe_terminator_free_memory(((fbe_neit_err_list_t*)error_list_element)->error_record);

           /* Free the memory for the error list object */
           fbe_terminator_free_memory((fbe_neit_err_list_t*)error_list_element);

           /* Pop the next error list element */
           error_list_element =
                          fbe_queue_pop( &(((fbe_neit_drive_error_table_t*)drive_error_table_element)->neit_err_list_head) );
        }

    /* Free the memory for the error list table object */
    fbe_terminator_free_memory((fbe_neit_drive_error_table_t*)drive_error_table_element);

    /* Pop the next error list element */
    drive_error_table_element = fbe_queue_pop(gneit_drive_error_table_head);
    }

    /* The table has been removed free the head.
     */
    fbe_terminator_free_memory(gneit_drive_error_table_head);
    gneit_drive_error_table_head = NULL;

    /* Change state back to NEIT_NOT_INIT
     */
    fbe_neit_state.current_state = NEIT_NOT_INIT;

    return FBE_STATUS_OK;
}


fbe_status_t  fbe_neit_error_injection_start(void)
{
    /* If the current state is init or stop (or start) it is
     * Ok to start (or continue) error injection. Otherwise
     * fail.
     */
    if( fbe_neit_state.current_state == NEIT_INIT   ||
        fbe_neit_state.current_state == NEIT_START  ||
        fbe_neit_state.current_state == NEIT_STOP )
    {
        fbe_neit_state.current_state = NEIT_START;
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}


fbe_status_t  fbe_neit_error_injection_stop(void)
{
    /* If the current state is init or start (or stopped) it is
     * Ok to stop (or continue to remain stopped) error injection.
     * Otherwise fail.
     */
    if( fbe_neit_state.current_state == NEIT_INIT   ||
        fbe_neit_state.current_state == NEIT_START  ||
        fbe_neit_state.current_state == NEIT_STOP )
    {
        fbe_neit_state.current_state = NEIT_STOP;
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}


fbe_status_t  fbe_neit_insert_error_record(fbe_terminator_neit_error_record_t  error_record_to_add)
{
    /* This is the main entry function to use to completely add an
     * error record.
     */

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* Pointer to the drive error table. */
    fbe_neit_drive_error_table_t*  err_tbl_ptr = NULL;

    /* The error record is copied in by value. Make a copy on the heap
     * to put on the list
     */

    /* Make a pointer to a new error record */
    fbe_terminator_neit_error_record_t* error_record_to_insert = NULL;

    /* If neit is not initialized we can't add an error record
     * so return an error.
     */
    if (fbe_neit_state.current_state == NEIT_NOT_INIT)
    {
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate a new neit error record object for it */
    error_record_to_insert =
       (fbe_terminator_neit_error_record_t*) fbe_terminator_allocate_memory(sizeof(fbe_terminator_neit_error_record_t));

    /* If we didn't get it fail */
    if (error_record_to_insert == NULL)
    {
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy the data from the error record passed into the function
     * into the error record on the heap
     */
    status = fbe_error_record_copy(error_record_to_insert, &error_record_to_add);

    if (status != FBE_STATUS_OK)
    {
        fbe_terminator_free_memory(error_record_to_insert);
        return status;
    }

    /* Check the error record for validity */
    status = fbe_neit_validate_new_record(error_record_to_insert);

    if (status != FBE_STATUS_OK)
    {
        fbe_terminator_free_memory(error_record_to_insert);
        return status;
    }

    /* Search the drive error table for the specified drive.  If
     * the specified drive is not found return NULL.
     */
    err_tbl_ptr = fbe_neit_search_drive_table(error_record_to_insert->drive);

    if (err_tbl_ptr == NULL)
    {
        /* There are no drive error tables matching this drive.
         * Insert a new drive error table for this drive.
         */

         if (fbe_neit_add_drive_table(error_record_to_insert) != FBE_STATUS_OK)
         {
             fbe_terminator_free_memory(error_record_to_insert);
             return FBE_STATUS_GENERIC_FAILURE;
         }
    }
    else
    {
        /* The specified drive was found in the drive error table. Add
         * a new error list to the specified drive's error table
         */
         if ( fbe_neit_add_err_list(err_tbl_ptr, error_record_to_insert) != FBE_STATUS_OK )
         {
             fbe_terminator_free_memory(error_record_to_insert);
             return FBE_STATUS_GENERIC_FAILURE;
         }
    }

    return FBE_STATUS_OK;
}



fbe_status_t  fbe_neit_validate_new_record(fbe_terminator_neit_error_record_t*  error_record)
{

    /* No validation code for the 9/26 delivery
     */

    return FBE_STATUS_OK;
}


fbe_neit_drive_error_table_t*  fbe_neit_search_drive_table(neit_drive_t drive_to_search_for)
{

    /* This function searches the drive error table list for a specific neit_drive_error_table
     * object that matches the specified drive.
     * If none is found NULL is returned.
     */

    /* Build a pointer to a a drive error table element */
    fbe_neit_drive_error_table_t* drive_error_table = NULL;

    /* Start searching at the head of the list */
    drive_error_table = (fbe_neit_drive_error_table_t*) fbe_queue_front(gneit_drive_error_table_head);

    /* Run through the drive error table and look for a match to
     * the drive.
     */
    while ( (drive_error_table != NULL) &&
            (fbe_drive_match( drive_error_table->drive, drive_to_search_for) != FBE_TRUE) )
    {
        /* Get the next drive error table on the list */
        drive_error_table =
          (fbe_neit_drive_error_table_t*)fbe_queue_next(gneit_drive_error_table_head,
                                             &(drive_error_table->queue_element));
    }

    /* Return the pointer to the drive error table for
     * the specified drive. Will be NULL if no matching
     * drive is found. */

    return drive_error_table;
}


fbe_status_t  fbe_neit_add_err_list(fbe_neit_drive_error_table_t*  error_table_addition_point,
                                fbe_terminator_neit_error_record_t*  error_record_to_add)
{

    /* Upon entry it has been determined that a neit_drive_error_table for the
     * correct drive already exists and a pointer to the table is passed in
     *
     * Given a pointer to an existing error record this function will
     * create, initialize an neit_err_list and add the error record to it
     * then add the neit_err_list object to the specified neit_drive_error_table
     */

    /* Make a pointer to a new error list */
    fbe_neit_err_list_t* new_error_list = NULL;

    /* Allocate a new neit error list object for it */
    new_error_list =
       (fbe_neit_err_list_t*) fbe_terminator_allocate_memory(sizeof(fbe_neit_err_list_t));

    /* If we didn't get it fail */
    if (new_error_list == NULL)
    {
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the queue element in the new error list object */
    fbe_queue_element_init(&(new_error_list->queue_element));

    /* Have the new error list object point to our error record
     * to be added
     */
    new_error_list->error_record = error_record_to_add;

    /* Everything worked so add the error list to the drive error table object */
    fbe_queue_push( &(error_table_addition_point->neit_err_list_head) , &(new_error_list->queue_element) );

    return FBE_STATUS_OK;
}


fbe_status_t  fbe_neit_add_drive_table(fbe_terminator_neit_error_record_t*  error_record_to_add)
{
    /* Upon entry it has been determined that a
     * neit_drive_error_table for the correct drive does NOT exist.
     *
     * This function will create and initialize a neit_drive_error_table object
     *
     * Then we call the function fbe_neit_add_err_list() to add the correct err_list
     * to this newly created neit_drive_error_table
     */

    /* Make a pointer to a new drive error table */
    fbe_neit_drive_error_table_t* new_drive_error_table = NULL;

    /* Allocate a new_error_record for it */
    new_drive_error_table =
       (fbe_neit_drive_error_table_t*) fbe_terminator_allocate_memory(sizeof(fbe_neit_drive_error_table_t));

    /* If we didn't get it fail */
    if (new_drive_error_table == NULL)
    {
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the queue element in the new drive error table */
    fbe_queue_element_init(&(new_drive_error_table->queue_element));

    /* Copy in the drive */
    fbe_drive_copy( &(new_drive_error_table->drive), &(error_record_to_add->drive) );

    /* Initialize the error list head */
    fbe_queue_init( &(new_drive_error_table->neit_err_list_head) );

    /* Now add the error list to the drive error table */
    if ( fbe_neit_add_err_list(new_drive_error_table, error_record_to_add) != FBE_STATUS_OK)
    {
        /* Adding the error list failed so free memory and
         * return failure.
         */
        fbe_terminator_free_memory(new_drive_error_table);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Everything worked so add the drive error table to the list */
    fbe_queue_push( gneit_drive_error_table_head, &(new_drive_error_table->queue_element) );

    return FBE_STATUS_OK;
}


fbe_status_t  fbe_error_record_init(fbe_terminator_neit_error_record_t*  record)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* The key element for a record is FRU number. If the user did not
     * enter a valid FRU number, the record should be dropped. Hence
     * the field is initialized with invalid FRU value so that if the
     * user did not enter the right value, it will be dropped.
     */
    status = fbe_drive_init( &(record->drive) );
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    /* LBA values will be initialized to 0. When adding the error
     * rule to the table, if these values are still 0s (which might be
     * the case when the user did not enter the right value) then
     * the entire user space will be considered for that FRU.
     */
    record->lba_start = 0;
    record->lba_end = 0;
    /* Opcode is defaulted to ANY. Which means if no opcode
     * is selected, this error will be injected with out considering
     * the opcode value.
     */
    record->opcode = NEIT_ANY_OPCODE;
    record->error_sk = NEIT_INVALID_SENSE;
    record->error_asc = 0;
    record->error_ascq = 0;
    /* By default, all error rules are error insertions. If it is
     * meant to be a port error, it should be explicitly
     * mentioned in this field.
     */
    record->is_port_error = FBE_FALSE;
    record->port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    record->port_force_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;

    /* Default the number of times to insert the error to 1.
     */
    record->num_of_times_to_insert = 1;
    /* Set the timestamp to something valid.
     */
    record->max_times_hit_timestamp = fbe_get_time();
    /* Set so that we will never reactivate a record.
     */
    record->secs_to_reactivate = NEIT_INVALID_SECONDS; 
    /* If secs_to_reactivate is valid, the default is that the record will
     * always be reactivated after the time interval is up.
     */
    record->num_of_times_to_reactivate = NEIT_ALWAYS_REACTIVATE;
    record->times_inserted = 0;
    record->times_reset = 0;

    return status;
}


fbe_status_t  fbe_error_record_copy(fbe_terminator_neit_error_record_t*  to_record, fbe_terminator_neit_error_record_t*  from_record)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_drive_copy(&(to_record->drive), &(from_record->drive));

    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    to_record->lba_start = from_record->lba_start;
    to_record->lba_end = from_record->lba_end;
    to_record->opcode = from_record->opcode;
    to_record->error_sk = from_record->error_sk;
    to_record->error_asc = from_record->error_asc;
    to_record->error_ascq = from_record->error_ascq;
    to_record->is_port_error = from_record->is_port_error;
    to_record->port_status = from_record->port_status;
    to_record->port_force_scsi_status = from_record->port_force_scsi_status;
    to_record->num_of_times_to_insert = from_record->num_of_times_to_insert;
    to_record->max_times_hit_timestamp = from_record->max_times_hit_timestamp;
    to_record->secs_to_reactivate = from_record->secs_to_reactivate;
    to_record->num_of_times_to_reactivate = from_record->num_of_times_to_reactivate;
    to_record->times_inserted = from_record->times_inserted;
    to_record->times_reset = from_record->times_reset;

    return status;
}


fbe_status_t  fbe_drive_init(neit_drive_t* drive)
{
    drive->port_number = NEIT_INVALID_POSITION;
    drive->enclosure_number = NEIT_INVALID_POSITION;
    drive->slot_number = NEIT_INVALID_POSITION;
    return FBE_STATUS_OK;
}


fbe_bool_t  fbe_drive_match(neit_drive_t drive_1, neit_drive_t drive_2)
{
    return ( (drive_1.port_number == drive_2.port_number) &&
             (drive_1.enclosure_number == drive_2.enclosure_number)&&
             (drive_1.slot_number == drive_2.slot_number) );
}


fbe_status_t  fbe_drive_copy(neit_drive_t* to_drive, neit_drive_t* from_drive)
{
    to_drive->port_number = from_drive->port_number;
    to_drive->enclosure_number = from_drive->enclosure_number;
    to_drive->slot_number = from_drive->slot_number;
    return FBE_STATUS_OK;
}
static fbe_u32_t fbe_terminator_get_elapsed_seconds(fbe_time_t timestamp)
{
    return (fbe_get_elapsed_seconds(timestamp));
}

