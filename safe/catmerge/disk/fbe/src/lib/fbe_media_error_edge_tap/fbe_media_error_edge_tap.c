/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_media_error_edge_tap.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains functions of the edge tap object used for
 *  injecting media errors.
 *
 * HISTORY
 *   1/12/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_topology.h"
#include "fbe/fbe_payload_ex.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_media_error_edge_tap.h"
#include "fbe/fbe_api_common_transport.h"


/************************* 
 * GLOBAL  DEFINITIONS 
 *************************/

/*! @var media_error_edge_tap_object
 *  @brief Object we defined for the edge tap.
 */
static fbe_media_error_edge_tap_object_t media_error_edge_tap_object;

/*! @var fbe_media_error_edge_tap_debug
 *  @brief Global to control how verbose we are in tracing.
 */
static fbe_bool_t fbe_media_error_edge_tap_debug = FBE_FALSE;

static fbe_status_t media_error_edge_tap_for_ssp(fbe_packet_t * packet_p);

static void fbe_media_error_edge_tap_object_init(fbe_media_error_edge_tap_object_t *const edge_tap_p);
static fbe_media_error_edge_tap_object_t *fbe_media_error_edge_tap_object_get(void);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!**************************************************************
 * @fn fbe_media_error_edge_tap_object_create(
 *                fbe_api_edge_tap_create_info_t *edge_tap_info_p)
 ****************************************************************
 * @brief
 *  This creates the media error edge tap object and returns
 *  a pointer to information on the edge tap..
 * 
 * @param edge_tap_info_p - The edge tap object info ptr.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/19/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_media_error_edge_tap_object_create(fbe_api_edge_tap_create_info_t *edge_tap_info_p)
{
    /* Get ptr to edge tap and initialize it. 
     * For now we have a single instance, but we might change this in the future. 
     */
    fbe_media_error_edge_tap_object_t *edge_tap_p;
    edge_tap_p = fbe_media_error_edge_tap_object_get();
    fbe_media_error_edge_tap_object_init(edge_tap_p);

    /* Fill out info structure.
     */
    edge_tap_info_p->edge_tap_fn = media_error_edge_tap_for_ssp;
    edge_tap_info_p->edge_tap_object_p = edge_tap_p;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_media_error_edge_tap_object_create()
 **************************************/
/*!**************************************************************
 * @fn fbe_media_error_edge_tap_object_get(void)
 ****************************************************************
 * @brief
 *  This is an accessor for the above media_error_edge_tap_object.
 *  This allows us to separate the of the above object from its use.
 * 
 * @return fbe_media_error_edge_tap_object_t*
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

static fbe_media_error_edge_tap_object_t *fbe_media_error_edge_tap_object_get(void)
{
    return &media_error_edge_tap_object;
}
/**************************************
 * end fbe_media_error_edge_tap_object_get()
 **************************************/

/*!**************************************************************
 * @fn fbe_media_error_edge_tap_object_init(void)
 ****************************************************************
 * @brief
 *   Simply initialize the input edge tap object by clearing out
 *   its contents.
 *
 * @param  - None.              
 *
 * @return - None.
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

static void fbe_media_error_edge_tap_object_init(fbe_media_error_edge_tap_object_t *const edge_tap_p)
{
    /* Simply clear out all the memory in the edge tap. 
     * this sets all the error records to uninitialized. 
     */
    fbe_zero_memory(edge_tap_p, sizeof(fbe_media_error_edge_tap_object_t));
    return;
}
/******************************************
 * end fbe_media_error_edge_tap_object_init()
 ******************************************/

/*!**************************************************************
 * @fn fbe_media_error_edge_tap_object_get_stats(   
 *            fbe_media_error_edge_tap_object_t *const edge_tap_p,
 *            fbe_media_error_edge_tap_stats_t  *const stats_p)
 ****************************************************************
 * @brief
 *   Get the statistics on the media error edge tap object.
 *  
 * @param edge_tap_p - Edge tap object to report on.
 * @param stats_p - The statistics object to fill out.
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE on error.
 *                        FBE_STATUS_OK on success.
 *
 * HISTORY:
 *  1/19/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_media_error_edge_tap_object_get_stats(fbe_media_error_edge_tap_object_t *const edge_tap_p,
                                                       fbe_media_error_edge_tap_stats_t *const stats_p)
{
    if (stats_p == NULL || edge_tap_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy the pointer to the records.
     */
    stats_p->record_p = edge_tap_p->records;

    /* Copy the remaining fields.
     */
    stats_p->count = FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS;
    stats_p->packet_count = edge_tap_p->packet_received_count;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_media_error_edge_tap_object_get_stats()
 **************************************/
/*!**************************************************************
 * @fn fbe_media_error_edge_tap_object_find_overlap(fbe_media_error_edge_tap_object_t *const edge_tap_p,
 *                                                  const fbe_lba_t lba,
 *                                                  const fbe_block_count_t blocks,
 *                                                  fbe_error_record_type_t error)
 ****************************************************************
 * @brief
 *   Find an overlap between the input lba range and the error
 *   records that already exist on the given edge tap.
 *  
 *  
 * @param edge_tap_p - The edge tap object to search.
 * @param lba - Start logical block address of range to search for.
 * @param blocks - Length of range to search for.
 * @param error - The type of error to inject.
 * 
 * @return fbe_error_record_t* - The first record in the input range.
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

fbe_error_record_t * fbe_media_error_edge_tap_object_find_overlap(fbe_media_error_edge_tap_object_t *const edge_tap_p,
                                             const fbe_lba_t lba,
                                             const fbe_block_count_t blocks,
                                             fbe_error_record_type_t error)
{
    fbe_u32_t index;

    /* Search among all records.
     */
    for (index = 0; 
          (index < FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS);
          index++)
    {
        fbe_bool_t b_type_match = FBE_FALSE;

        if ((error == FBE_ERROR_RECORD_TYPE_ALL_ERRORS) &&
            (edge_tap_p->records[index].error_type != FBE_ERROR_RECORD_TYPE_UNINITIALIZED))
        {
            /* We are searching for any valid error (everything but uninitialized), 
             * and we have a match. 
             */
            b_type_match = FBE_TRUE;
        }
        else if (edge_tap_p->records[index].error_type == error)
        {
            /* Searching for other error types and found match.
             */
            b_type_match = FBE_TRUE;
        }

        /* If we find a record in the range we are searching for, then return the first
         * one we find. 
         */
        if ( edge_tap_p->records[index].b_active &&
             b_type_match &&
            (edge_tap_p->records[index].lba >= lba) &&
            (edge_tap_p->records[index].lba < lba + blocks))
        {
            return &edge_tap_p->records[index];
        }
    } /* for all indexes. */

    /* Not found, return NULL.
     */ 
    return NULL;
}
/**************************************
 * end fbe_media_error_edge_tap_object_find_overlap()
 **************************************/

/*!**************************************************************
 * @fn fbe_media_error_edge_tap_object_add(fbe_media_error_edge_tap_object_t *const edge_tap_p,
 *                                         const fbe_lba_t lba,
 *                                         const fbe_block_count_t blocks,
 *                                         fbe_error_record_type_t error)
 ****************************************************************
 * @brief
 *  Add an error record to the input edge tap object.
 *  
 * @param edge_tap_p - The edge tap object to add to.
 * @param lba - Start logical block address to begin error at.
 * @param blocks - Length of range to inject errors on.
 * @param error - The type of error to inject.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_media_error_edge_tap_object_add(fbe_media_error_edge_tap_object_t *const edge_tap_p,
                                                 const fbe_lba_t lba,
                                                 const fbe_block_count_t blocks,
                                                 fbe_error_record_type_t error)
{
    fbe_u32_t index;
    fbe_lba_t current_lba = lba;

    /* If we already have records in this range, return an error. 
     * We only allow one record per lba. 
     */
    if (fbe_media_error_edge_tap_object_find_overlap(edge_tap_p, lba, blocks, error) != NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Simply loop over all the error records and add this error where there is space.
     * If we run out of space, then return an error. 
     */
    for (index = 0; 
          (index < FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS);
          index++)
    {
        /* If we find an uninitialized record, then add the error here.
         */
        if (edge_tap_p->records[index].error_type == FBE_ERROR_RECORD_TYPE_UNINITIALIZED)
        {
            edge_tap_p->records[index].error_type = error;
            edge_tap_p->records[index].lba = current_lba;
            edge_tap_p->records[index].b_active = FBE_TRUE;
            current_lba++;
            if (current_lba >= lba + blocks)
            {
                break;
            }
        }
    } /* for all indexes and current lba less than the range we are inserting. */

    if (index == FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS)
    {
        /* Not enough free records.
         */
        printf("FBE Edge Tap, not enough free records.\n");
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_media_error_edge_tap_object_add()
 **************************************/
/*!**************************************************************
 * @fn fbe_media_error_edge_tap_object_is_active(fbe_media_error_edge_tap_object_t *const edge_tap_p,
 *                                         const fbe_lba_t lba,
 *                                         const fbe_block_count_t blocks,
 *                                         fbe_error_record_type_t error)
 ****************************************************************
 * @brief
 *  Add an error record to the input edge tap object.
 *  
 * @param edge_tap_p - The edge tap object to add to.
 * @param lba - Start logical block address to begin error at.
 * @param blocks - Length of range to inject errors on.
 * @param error - The type of error to inject.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_media_error_edge_tap_object_is_active(fbe_media_error_edge_tap_object_t *const edge_tap_p,
                                                       const fbe_lba_t lba,
                                                       const fbe_block_count_t blocks,
                                                       fbe_error_record_type_t error)
{
    fbe_u32_t index;

    /* Simply loop over all the error records and search for a match.
     * If we don't find a match, then return an error. 
     */
    for (index = 0; 
          (index < FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS);
          index++)
    {
        /* If we find an active record return true.
         * An active record is one where: 
         * 1) the record is marked active. 
         * 2) the error type is set. 
         * 3) the lba range matches that passed in. 
         */
        if (edge_tap_p->records[index].b_active &&
            (edge_tap_p->records[index].error_type != FBE_ERROR_RECORD_TYPE_UNINITIALIZED) &&
            ((edge_tap_p->records[index].lba >= lba) && (edge_tap_p->records[index].lba <= (lba + blocks - 1))))
        {
            return FBE_STATUS_OK;
        }
    } /* for all indexes and current lba less than the range we are inserting. */

    return FBE_STATUS_FAILED;
}
/**************************************
 * end fbe_media_error_edge_tap_object_is_active()
 **************************************/
/*!**************************************************************
 * @fn fbe_media_error_edge_tap_object_clear(fbe_media_error_edge_tap_object_t *const edge_tap_p,
 *                                           const fbe_lba_t lba,
 *                                           const fbe_block_count_t blocks)
 ****************************************************************
 * @brief
 *  This clears out the contents of the media error edge tap
 *  so that there are no error records in the input lba range.
 *  
 * @param edge_tap_p - The edge tap object to work with.
 * @param lba - Logical block start address to clear. 
 * @param blocks - The number of blocks in range to clear.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_media_error_edge_tap_object_clear(fbe_media_error_edge_tap_object_t *const edge_tap_p,
                                                   const fbe_lba_t lba,
                                                   const fbe_block_count_t blocks)
{
    fbe_u32_t index;

    for (index = 0; 
          (index < FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS);
          index++)
    {
        /* If we find a record in the range we are clearing, then set it to inactive.
         */
        if ((edge_tap_p->records[index].lba >= lba) &&
            (edge_tap_p->records[index].lba < lba + blocks))
        {
            edge_tap_p->records[index].b_active = FBE_FALSE;
        }
    } /* for all indexes. */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_media_error_edge_tap_object_clear()
 **************************************/


/*!**************************************************************
 * @fn fbe_media_error_edge_tap_get_cdb_info(fbe_payload_ex_t *const payload_p,
 *                                           fbe_media_edge_tap_info_t *const info_p)
 ****************************************************************
 * @brief
 *  Get the information from the cdb and fill in the edge tap
 *  information structure.
 *  This structure isolates the caller from the details of the
 *  payload.
 *  
 * @param payload_p - Payload to get details from.
 * @param info_p - Ptr to structure to fill info into.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_media_error_edge_tap_get_cdb_info(fbe_payload_ex_t *const payload_p,
                                                   fbe_media_edge_tap_info_t *const info_p)
{
    fbe_status_t  status;
    fbe_payload_cdb_operation_t *payload_cdb_operation_p = NULL;
    fbe_u8_t * cdb_p; /* Pointer to cdb. */
    fbe_u8_t cdb_opcode; /* The opcode from the cdb. */

    /* Get the pointer to our cdb payload.
     */
    payload_cdb_operation_p = fbe_payload_ex_get_cdb_operation(payload_p);

    if (payload_cdb_operation_p == NULL)
    {
        /* We always expect a cdb payload, return an error.
         */
        KvTrace("%s:Error fbe_payload_ex_get_cdb_operation failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation_p, &cdb_p);
    if (status != FBE_STATUS_OK)
    {
        KvTrace("%s:Error !! fbe_payload_cdb_operation_get_cdb failed\n", __FUNCTION__);
        return status;
    }

    /* Fetch the ptr to the sense data.
     */
    status = fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation_p,
                                                        &info_p->sense_buffer_p);
    if (status != FBE_STATUS_OK)
    {
        KvTrace("%s:Error fbe_payload_cdb_operation_get_sense_buffer failed\n", __FUNCTION__);
        return status;
    }

    /* Fetch the opcode from the cdb.  It is always the first entry.  
     */ 
    cdb_opcode = cdb_p[0];

    /* Save the opcode to be returned.
     */
    info_p->opcode = cdb_opcode;

    switch (cdb_opcode)
    {
        case FBE_SCSI_WRITE_6:
        case FBE_SCSI_READ_6:
            info_p->lba = fbe_get_six_byte_cdb_lba(cdb_p);
            info_p->blocks = fbe_get_six_byte_cdb_blocks(cdb_p);
            break;

        case FBE_SCSI_REASSIGN_BLOCKS: /* Ten Byte. */
            {
                /* For reassign, the lba to be remapped is set inside the 
                 * defect information, which is in the payload sg. 
                 */
                fbe_sg_element_t *sg_p = NULL;
                fbe_u8_t * defect_list_p = NULL;
                status = fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);
                //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                //MUT_ASSERT_NOT_NULL(sg_p);
                defect_list_p = sg_p[0].address;
                //MUT_ASSERT_NOT_NULL(defect_list_p);

                /* Calculate the lba, just assume a single block.
                 */
                if (cdb_p[1]) {
                    info_p->lba = swap64(*((fbe_u64_t *) (defect_list_p+4)));
                } else {
                    info_p->lba = (fbe_u64_t)swap32(*((fbe_u32_t *) (defect_list_p+4)));
                }
            }
            info_p->blocks = 1;
            break;

        case FBE_SCSI_READ_10:
        case FBE_SCSI_WRITE_10:
        case FBE_SCSI_WRITE_VERIFY:    /* Ten Byte. */
            /* Calculate the lba and blocks.
             */
            info_p->lba = fbe_get_ten_byte_cdb_lba(cdb_p);
            info_p->blocks = fbe_get_ten_byte_cdb_blocks(cdb_p);
            break;

        case FBE_SCSI_READ_16:
        case FBE_SCSI_WRITE_16:
        case FBE_SCSI_WRITE_VERIFY_16:    /* Sixteen Byte. */
            /* Calculate the lba and blocks.
             */
            info_p->lba = fbe_get_sixteen_byte_cdb_lba(cdb_p);
            info_p->blocks = fbe_get_sixteen_byte_cdb_blocks(cdb_p);
            break;

        case FBE_SCSI_READ_12:
        case FBE_SCSI_INQUIRY:
        case FBE_SCSI_READ_CAPACITY:
        case FBE_SCSI_MODE_SELECT_6:
        case FBE_SCSI_MODE_SENSE_6:
        case FBE_SCSI_START_STOP_UNIT:
        case FBE_SCSI_TEST_UNIT_READY:       
        case FBE_SCSI_WRITE_SAME:    /* Ten Byte. */
        case FBE_SCSI_WRITE_SAME_16:
        case FBE_SCSI_RESERVE:
        case FBE_SCSI_RESERVE_10:
        case FBE_SCSI_RELEASE:
        case FBE_SCSI_RELEASE_10:
        case FBE_SCSI_VERIFY:
        case FBE_SCSI_SEND_DIAGNOSTIC:
        default:
            /* We do not inject errors for these opcodes.
             * Return error so the caller knows to inject errors. 
             */
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_media_error_edge_tap_get_cdb_info()
 **************************************/

/*!**************************************************************
 * @fn fbe_media_error_edge_tap_insert_error_in_payload(
 *                  fbe_media_error_edge_tap_object_t *const tap_p,
 *                  fbe_error_record_t *const error_record_p,
 *                  fbe_payload_ex_t *const payload_p,
 *                  fbe_media_edge_tap_info_t *const info_p)
 ****************************************************************
 * @brief
 *  Insert the error described by the error record into the
 *  payload passed in.
 *  
 * @param payload_p - Payload to inject error in.
 * @param error_record_p - Error record to inject on.
 * @param info_p - Information on the request.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_media_error_edge_tap_insert_error_in_payload(fbe_payload_ex_t *const payload_p,
                                                 fbe_error_record_t *const error_record_p,
                                                 fbe_media_edge_tap_info_t *const info_p)
{
    fbe_u32_t index;
    fbe_status_t  status;
    fbe_payload_cdb_operation_t *payload_cdb_operation_p = NULL;
    fbe_u8_t * sense_buffer_p; /* Pointer to cdb payload's sense buffer. */

    /* Get the pointer to our cdb payload.
     */
    payload_cdb_operation_p = fbe_payload_ex_get_cdb_operation(payload_p);

    if (payload_cdb_operation_p == NULL)
    {
        /* We always expect a cdb payload, return an error.
         */
        KvTrace("%s:Error fbe_payload_ex_get_cdb_operation failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fetch the ptr to the sense data.
     */
    status = fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation_p,
                                                        &sense_buffer_p);
    if (status != FBE_STATUS_OK)
    {
        KvTrace("%s:Error fbe_payload_cdb_operation_get_sense_buffer failed\n", __FUNCTION__);
        return status;
    }

    /* Handle a media error.
     */
    if (error_record_p->error_type == FBE_ERROR_RECORD_TYPE_MEDIA_ERROR)
    {
        /* Setup the scsi and request status to indicate error.
         */
        fbe_payload_cdb_set_scsi_status(payload_cdb_operation_p, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
        fbe_payload_cdb_set_request_status(payload_cdb_operation_p, FBE_PORT_REQUEST_STATUS_ERROR);

        /* Setup the sense data with relevant error info. 
         * First set check condition.
         */
        sense_buffer_p[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] = FBE_SCSI_SENSE_KEY_MEDIUM_ERROR;
        sense_buffer_p[FBE_SCSI_SENSE_DATA_ASC_OFFSET] = FBE_SCSI_ASC_READ_DATA_ERROR;
        sense_buffer_p[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET] = FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO;

        /* Set the sense buffer error code, this is always 0x70 per spec.
         */
        sense_buffer_p[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] = 0x70;

        /* Set the valid bit indicating the sense data has extra bad lba info.
         */
        sense_buffer_p[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] |= 0x80;
        sense_buffer_p[FBE_SCSI_SENSE_DATA_ADDL_SENSE_LENGTH_OFFSET] = 10; /* Always 10. */

        /* Set the bad lba into the sense data at 3-6.
         */
        sense_buffer_p[3] = (fbe_u8_t)((error_record_p->lba >>  24) & 0xFF);
        sense_buffer_p[4] |= (fbe_u8_t)((error_record_p->lba >> 16) & 0xFF);
        sense_buffer_p[5] |= (fbe_u8_t)((error_record_p->lba >>  8) & 0xFF);  
        sense_buffer_p[6] |= (fbe_u8_t)(error_record_p->lba & 0xFF);

        /* Display the contents of the sense buffer.
         */
        if (fbe_media_error_edge_tap_debug)
        {
            for (index = 0; index <= FBE_SCSI_SENSE_DATA_ADDL_SENSE_LENGTH_OFFSET; index++)
            {
                //mut_printf(MUT_LOG_TEST_STATUS, "Sense info[%d] = 0x%02x", index, sense_buffer_p[index]);
            }
        }
    } /* If error is media error. */

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_media_error_edge_tap_insert_error_in_payload()
 **************************************/

/*!**************************************************************
 * @fn media_error_edge_tap_for_ssp(void * packet_p)
 ****************************************************************
 * @brief
 *  This is the media error edge tap for injecting errors
 *  on a SSP edge.
 * 
 *  Packets will redirect to this function, where packets are filtered and processed here
 *  things we can do here are:
 *  - change the content of a packet and forward it on.
 *  - change the content of a packet and complete it.
 *  - change the completion function, so the packet can be intercepted on the way back.
 *  - add packet to a queue so we can add delay or change order of execution.
 *  
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

static fbe_status_t media_error_edge_tap_for_ssp(fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_media_error_edge_tap_object_t *tap_p = &media_error_edge_tap_object;
    fbe_error_record_t *error_record_p;
    fbe_media_edge_tap_info_t info;
    fbe_payload_ex_t *payload_p;

    /* Count the packets.
     */
    tap_p->packet_received_count++;
    /* Tooooooooo noisy
    //mut_printf(MUT_LOG_TEST_STATUS, "*** %s: receives %d packets. ***", __FUNCTION__, tap_p->packet_received_count);
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Fetch the details of this operation
     */
    status = fbe_media_error_edge_tap_get_cdb_info(payload_p, &info);

    /* A good status means that we understand the opcode and need to intercept and process
     * the payload. 
     */
    if (status == FBE_STATUS_OK)
    {
        error_record_p = fbe_media_error_edge_tap_object_find_overlap(tap_p, 
                                                                      info.lba, 
                                                                      info.blocks, 
                                                                      FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    
        if (error_record_p != NULL)
        {
            /* We are injecting errorrs for this lba range.
             */
            switch (info.opcode)
            {
                case FBE_SCSI_WRITE_6:
                case FBE_SCSI_WRITE_10:
                case FBE_SCSI_WRITE_16:
                case FBE_SCSI_READ_6:
                case FBE_SCSI_READ_10:
                case FBE_SCSI_READ_16:
                case FBE_SCSI_WRITE_VERIFY:    /* Ten Byte. */
                case FBE_SCSI_WRITE_VERIFY_16:
                    /* Inject the media error at the first lba of the I/O that has an 
                     * error.  This lba is represented by the info.lba. 
                     */
                    if (fbe_media_error_edge_tap_debug)
                    {
                        printf("Media error edge tap injecting media error on op:0x%x at lba:0x%llx lba:0x%llx block:0x%llx\n", 
                               info.opcode,
			       (unsigned long long)error_record_p->lba,
			       (unsigned long long)info.lba,
			       (unsigned long long)info.blocks);
                    }

                    fbe_media_error_edge_tap_insert_error_in_payload(payload_p, error_record_p, &info);

                    /* We return good packet status since the packet was delivered OK. 
                     * The payload status was set to invalid above. 
                     */
                    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(packet_p);
                    return FBE_STATUS_OK;
                    break;
                case FBE_SCSI_REASSIGN_BLOCKS:    /* Ten Byte. */
                    
                    /* Simulate the fixing of the media error at the first lba of the
                     * reassign that has an error. 
                     * This lba is represented by the info.lba. 
                     *  
                     * We simulate the fixing of the error by simply clearing the active 
                     * bit. 
                     *  
                     * We do not support reassigning an entire range since the 
                     * PDO does not reassign more than one block at a time. 
                     */
                    error_record_p->b_active = FBE_FALSE;
                    if (fbe_media_error_edge_tap_debug)
                    {
                        printf("Media error edge remap on op:0x%x at lba:0x%llx lba:0x%llx block:0x%llx\n", 
                               info.opcode,
			       (unsigned long long)error_record_p->lba,
			       (unsigned long long)info.lba,
			       (unsigned long long)info.blocks);
                    }
                    break;

                default:
                    //MUT_ASSERT_TRUE_MSG(FALSE, "Unknown opcode in media error edge tap");
                    break;
            }; /* end switch on opcode */

        } /* end if error_record_p != NULL */

    } /* status == FBE_STATUS_OK */

    /* Send the io packet 
     */
    /* the media_error_edge_tap is in the test space, we need to send this packet to Physical Package
     * use fbe_api to send this packet to physical package .*/
    status = fbe_api_common_send_io_packet_to_driver(packet_p);
    return status;
}
/**************************************
 * end media_error_edge_tap_for_ssp()
 **************************************/

/*!**************************************************************
 * @fn fbe_media_edge_tap_unit_test()
 ****************************************************************
 * @brief
 *  This is the unit test for testing all aspects of the
 *  media error edge tap.
 *
 * @param - None.               
 *
 * @return - None.
 *
 * HISTORY:
 *  1/12/2009 - Created. RPF
 *
 ****************************************************************/

void fbe_media_edge_tap_unit_test(void)
{
    fbe_api_edge_tap_create_info_t create_info;
    fbe_media_error_edge_tap_object_t *tap_p = NULL;
    fbe_error_record_t *record_p = NULL;
    fbe_status_t status;
    fbe_media_error_edge_tap_stats_t stats;

    /* Initialize our edge tap object.
     */
    status = fbe_media_error_edge_tap_object_create(&create_info);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    tap_p = create_info.edge_tap_object_p;

    status = fbe_media_error_edge_tap_object_add(tap_p, 
                                                 0, /* lba */ 
                                                 FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS, /* blocks */ 
                                                 FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Can't add if we already have this lba added.
     */
    status = fbe_media_error_edge_tap_object_add(tap_p, 0, 1, FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /* Can't add if we do not have any free entries.
     */
    status = fbe_media_error_edge_tap_object_add(tap_p, 500, 1, FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_INSUFFICIENT_RESOURCES);

    /* Check cases where record is found. 
     * Check boundaries and middle cases. 
     */
    record_p = fbe_media_error_edge_tap_object_find_overlap(tap_p, 0, 1, FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    //MUT_ASSERT_NOT_NULL(record_p);
    //MUT_ASSERT_TRUE(record_p->lba == 0);

    record_p = fbe_media_error_edge_tap_object_find_overlap(tap_p, 5, 12, 
                                                            FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    //MUT_ASSERT_NOT_NULL(record_p);
    //MUT_ASSERT_TRUE(record_p->lba == 5);

    record_p = fbe_media_error_edge_tap_object_find_overlap(tap_p, 99, 100, 
                                                            FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    //MUT_ASSERT_NOT_NULL(record_p);
    //MUT_ASSERT_TRUE(record_p->lba == 99);

    /* Check case where we search by any error type and record is found.
     */
    record_p = fbe_media_error_edge_tap_object_find_overlap(tap_p, 0, 1, FBE_ERROR_RECORD_TYPE_ALL_ERRORS);
    //MUT_ASSERT_NOT_NULL(record_p);
    //MUT_ASSERT_TRUE(record_p->lba == 0);

    /* Check case where record is not in range.
     */
    record_p = fbe_media_error_edge_tap_object_find_overlap(tap_p, 999, 5,
                                                            FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    //MUT_ASSERT_NULL(record_p);

    /* Test clear.
     */
    status = fbe_media_error_edge_tap_object_clear(tap_p, 
                                                   0, /* lba */ 
                                                   FBE_MEDIA_ERROR_EDGE_TAP_MAX_RECORDS /* blocks */ );
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure all records are cleared.
     */
    record_p = fbe_media_error_edge_tap_object_find_overlap(tap_p, 0, 0xFFFFFFFF, 
                                                            FBE_ERROR_RECORD_TYPE_MEDIA_ERROR);
    //MUT_ASSERT_NULL(record_p);

    /* Use the get stats function.
     */
    status = fbe_media_error_edge_tap_object_get_stats(tap_p, &stats);

    /* Make sure there are records returned.
     */
    //MUT_ASSERT_NOT_NULL(stats.record_p);
    //MUT_ASSERT_INT_NOT_EQUAL(stats.count, 0);
    return;
}
/******************************************
 * end fbe_media_edge_tap_unit_test()
 ******************************************/

/*************************
 * end file fbe_media_error_edge_tap.c
 *************************/


