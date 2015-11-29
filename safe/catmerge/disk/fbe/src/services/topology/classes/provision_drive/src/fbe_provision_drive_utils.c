/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code which provides generic function interface
 *  for common functionality.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   07/16/2010:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_provision_drive.h"
#include "base_object_private.h"
#include "fbe_provision_drive_private.h"
#include "fbe_raid_library.h"
#include "fbe_raid_library_proto.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/
static fbe_provision_drive_error_precedence_t fbe_provision_drive_utils_get_block_operation_error_precedece(
                                                    fbe_provision_drive_t * provision_drive_p,
                                                    fbe_payload_block_operation_status_t    error);

static fbe_status_t fbe_provision_drive_utils_is_block_status_has_precedence(
                                fbe_provision_drive_error_precedence_t  master_error_precedence,
                                fbe_lba_t                               master_media_error_lba,
                                fbe_time_t                              master_retry_wait_msec,
                                fbe_provision_drive_error_precedence_t  sub_error_precedence,
                                fbe_lba_t                               sub_media_error_lba,
                                fbe_time_t                              sub_retry_wait_msec,
                                fbe_payload_block_operation_status_t    subpacket_operation_status,
                                fbe_payload_block_operation_qualifier_t subpacket_qualifier,
                                fbe_bool_t                              *b_is_precedence_p);

static fbe_status_t fbe_provision_drive_utils_determine_master_block_status(fbe_status_t   status,
                                               fbe_payload_block_operation_status_t sub_packet_block_status,
                                               fbe_payload_block_operation_qualifier_t sub_packet_status_qualifier,
                                               fbe_payload_block_operation_status_t* master_block_status_p,
                                               fbe_payload_block_operation_qualifier_t* master_block_qualifier_p);
static fbe_status_t 
fbe_provision_drive_utils_ask_verify_invalidate_permission_completion(fbe_event_t * event_p,
                                                                      fbe_event_completion_context_t context);
/*!****************************************************************************
 * fbe_provision_drive_utils_is_io_request_aligned()
 ******************************************************************************
 *
 * @brief
 *  This function is used to determine whether i/o request (start_lba and
 *  (start_lba + block_count) is aligned to passed-in optimum block size or
 *  not.
 *
 * @param   start_lba               -  start lba of the i/o request.
 * @param   block_count             -  block count of the i/o request.
 * @param   optimum_block_size      -  optimum block size.   
 * @param   is_request_aligned_p    -  returns whether request is aligned to
 *                                     optimum block size or not.
 *
 * @return  fbe_status_t          -  status of the operation.
 *
 * @version
 *    23/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_is_io_request_aligned(fbe_lba_t start_lba,
                                                fbe_block_count_t block_count,
                                                fbe_optimum_block_size_t optimum_block_size,
                                                fbe_bool_t * is_request_aligned_p)
{

    /* Initialize request as not aligned initially. */
    *is_request_aligned_p = FBE_TRUE;

    /* verify whether start lba of the i/o request is aligned. */
    if(start_lba % optimum_block_size)
    {
        *is_request_aligned_p = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /* verify whether end address of the i/o request is aligned. */
    if((start_lba + block_count) % optimum_block_size)
    {
        *is_request_aligned_p = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_is_io_request_aligned()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_calculate_chunk_range()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the chunk range for the given lba range
 *  for the i/o operation, it rounds the edged on the bounday which is not
 *  aligned and consider it as full chunk.
 *
 * @param start_lba                     - start lba of the range.
 * @param block_count                   - block count of i/o operation.
 * @param chunk_size                    - chunk size.
 * @param start_chunk_index_p           - pointer to the start chunk index (out).
 * @param chunk_count_p                 - pointer to the number of chunks (out).
 * @param chunk_index_p                 - pointer to chunk index (out).
 *
 * @return fbe_status_t                 - status of the operation.
 *  
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_calculate_chunk_range(fbe_lba_t start_lba,
                                                fbe_block_count_t block_count,
                                                fbe_u32_t chunk_size,
                                                fbe_chunk_index_t * start_chunk_index_p,
                                                fbe_chunk_count_t * chunk_count_p)
{
    fbe_chunk_index_t   start_chunk_index;
    fbe_chunk_index_t   end_chunk_index;

    /* initialize ths start chunk indes as invaid. */
    *start_chunk_index_p = FBE_CHUNK_INDEX_INVALID;

    /* If lba range is invalid then return error. */
    if((start_lba == FBE_LBA_INVALID) || (block_count == 0))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the chunk index for the given start lba and end lba. */
    start_chunk_index = (start_lba - (start_lba % chunk_size)) / chunk_size;

    /* get the end chunk index to find the stripe lock range. */
    if((start_lba + block_count) % chunk_size)
    {
        end_chunk_index = (start_lba + block_count + chunk_size - ((start_lba + block_count) % chunk_size)) / chunk_size;
    }
    else
    {
        end_chunk_index = (start_lba + block_count) / chunk_size;
    }

    /* get the chunk count. */
    *chunk_count_p = (fbe_chunk_count_t) (end_chunk_index - start_chunk_index);

    if(*chunk_count_p)
    {
        /* store the start chunk index as return value. */
        *start_chunk_index_p = start_chunk_index;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_calculate_chunk_range()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_calculate_chunk_range_without_edges()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the chunk range for the given lba range
 *  for the i/o operation, it only covers the chunk which is entirely covered
 *  in lba range and it ignores the edges which is not aligned.
 *
 * @param provision_drive_p             - Provision drive object.
 * @param start_lba                     - start lba of the range.
 * @param block_count                   - block count of i/o operation.
 * @param chunk_size                    - chunk size.
 * @param start_chunk_index_p           - pointer to the start chunk index (out).
 * @param chunk_count_p                 - pointer to the number of chunks (out).
 * @param chunk_index_p                 - pointer to chunk index (out).
 *
 * @return fbe_status_t                 - status of the operation.
 *  
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_calculate_chunk_range_without_edges(fbe_lba_t start_lba,
                                                              fbe_block_count_t block_count,
                                                              fbe_u32_t chunk_size,
                                                              fbe_chunk_index_t * start_chunk_index_p,
                                                              fbe_chunk_count_t * chunk_count_p)
{
    fbe_chunk_index_t   start_chunk_index;
    fbe_chunk_index_t   end_chunk_index;

    /* initialize ths start chunk indes as invaid. */
    *start_chunk_index_p = FBE_CHUNK_INDEX_INVALID;
    *chunk_count_p = 0;

    /* If lba range is invalid then return error. */
    if(start_lba == FBE_LBA_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(block_count < chunk_size)
    {
        /* if block count is less than chunk then it does not cover the full chunk. */
        return FBE_STATUS_OK;
    }

    /* Get the chunk index for the given start lba and end lba. */
    if(start_lba % chunk_size)
    {
        start_chunk_index = (start_lba + chunk_size - (start_lba % chunk_size)) / chunk_size;
    }
    else
    {
        start_chunk_index = start_lba / chunk_size;
    }

    end_chunk_index = (start_lba + block_count - ((start_lba + block_count) % chunk_size)) / chunk_size;

    /* get the chunk count. */
    if(end_chunk_index >= start_chunk_index)
    {
        *chunk_count_p = (fbe_chunk_count_t) (end_chunk_index - start_chunk_index);
        *start_chunk_index_p = start_chunk_index;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_calculate_chunk_range_without_edges()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_calculate_number_of_unaligned_edges()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the number of edges which is not covered
 *  in chunk boundary range.
 *
 * @param provision_drive_p             - Provision drive object.
 * @param start_lba                     - start lba of the range.
 * @param block_count                   - block count of i/o operation.
 * @param chunk_size                    - chunk size.
 * @param number_of_edges_p             - no of edges not covered part of the
 *                                        chunk boundary (out).
 *
 * @return fbe_status_t                 - status of the operation.
 *  
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_calculate_number_of_unaligned_edges(fbe_lba_t start_lba,
                                                               fbe_block_count_t block_count,
                                                               fbe_u32_t chunk_size,
                                                               fbe_u32_t * number_of_edges_p)
{
    fbe_chunk_index_t   start_chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_count_t   chunk_count = 0;

    /* initialize number of edge which is not aligned as zero. */
    *number_of_edges_p = 0;

    /* If lba range is invalid then return error. */
    if(start_lba == FBE_LBA_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if block count less than or equal zero then return error. */
    if(block_count <= 0)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* determine the chunk range from the given lba range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    chunk_size,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* if start lba and start lba + block count both are not aligned then increment
     * the number of edge by one if it has one chunk else increment two times.
     */
    if((start_lba % chunk_size) && ((start_lba + block_count) % chunk_size))
    {
        /* if there is only one chunk then we have only pre edge range. */
        if(chunk_count == 1)
        {
            (*number_of_edges_p)++;
        }
        else
        {
            (*number_of_edges_p)++;
            (*number_of_edges_p)++;
        }
        return FBE_STATUS_OK;
    }

    /* if start lba is not aligned to chunk size.
     */
    if(start_lba % chunk_size)
    {
        (*number_of_edges_p)++;
        return FBE_STATUS_OK;
    }

    /* if start lba + block_count is not aligned to chunk size.
     */
    if((start_lba + block_count) % chunk_size)
    {
        (*number_of_edges_p)++;
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_calculate_number_of_unaligned_edges()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_calculate_unaligned_edges_lba_range()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the lba range for the pre and post edge
 *  if exists.
 *
 * @param provision_drive_p             - Provision drive object.
 * @param start_lba                     - start lba of the range.
 * @param block_count                   - block count of i/o operation.
 * @param chunk_size                    - chunk size.
 * @param pre_edge_start_lba_p          - pre edge start lba.
 * @param pre_edge_block_count_p        - pre edge block count.
 * @param post_edge_start_lba_p         - post edge start lba.
 * @param post_edge_block_count_p       - post edge block count.
 * @param number_of_edges_p             - no of edges not covered part of the
 *                                        chunk boundary (out).
 *
 * @return fbe_status_t                 - status of the operation.
 *  
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_calculate_unaligned_edges_lba_range(fbe_lba_t start_lba,
                                                              fbe_block_count_t block_count,
                                                              fbe_u32_t chunk_size,
                                                              fbe_lba_t * pre_edge_start_lba_p,
                                                              fbe_block_count_t * pre_edge_block_count_p,
                                                              fbe_lba_t * post_edge_start_lba_p,
                                                              fbe_block_count_t * post_edge_block_count_p,
                                                              fbe_u32_t * number_of_edges_p)
{
    fbe_chunk_index_t   start_chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_count_t   chunk_count = 0;

    /* initialize number of edges which is not aligned as zero. */
    *number_of_edges_p = 0;
    *pre_edge_start_lba_p = FBE_LBA_INVALID;
    *pre_edge_block_count_p = 0;
    *post_edge_start_lba_p = FBE_LBA_INVALID;
    *post_edge_block_count_p = 0;

    /* If lba range is invalid then return error. */
    if((start_lba == FBE_LBA_INVALID) || (block_count == 0))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* determine the chunk range from the given lba range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    chunk_size,
                                                    &start_chunk_index,
                                                    &chunk_count);


    /* if start lba and start lba + block count are not aligned to chunk size then 
     * increment the number of chunk by one if it has only one chunk else by two.
     */
    if((start_lba % chunk_size) && ((start_lba + block_count) % chunk_size))
    {
        /* if there is only one chunk then we have only pre edge range. */
        if(chunk_count == 1)
        {
            *pre_edge_start_lba_p = start_lba;
            *pre_edge_block_count_p = block_count;
            (*number_of_edges_p)++;
        }

        else
        {
            *pre_edge_start_lba_p = start_lba;
            *pre_edge_block_count_p = chunk_size - (start_lba % chunk_size);
            (*number_of_edges_p)++;
            *post_edge_start_lba_p = start_lba + block_count - ((start_lba + block_count) % chunk_size);
            *post_edge_block_count_p = (fbe_block_count_t) (start_lba + block_count - (*post_edge_start_lba_p));
            (*number_of_edges_p)++;
        }
        return FBE_STATUS_OK;
    }

    /* if start lba is not aligned to chunk size then calculate the pre edge range. */
    if(start_lba % chunk_size)
    {
        *pre_edge_start_lba_p = start_lba;
        *pre_edge_block_count_p = chunk_size - (start_lba % chunk_size); 
        (*number_of_edges_p)++;
        return FBE_STATUS_OK;
    }

    /* if start lba + block_count is not aligned then calculate the post edge range. */
    if((start_lba + block_count) % chunk_size)
    {
        *post_edge_start_lba_p = start_lba + block_count - ((start_lba + block_count) % chunk_size);
        *post_edge_block_count_p = (fbe_block_count_t) (start_lba + block_count - (*post_edge_start_lba_p));
        (*number_of_edges_p)++;
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_calculate_unaligned_edges_lba_range()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_calc_number_of_edge_chunks_needs_process()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the number of edges (pre and post)
 *  which is not aligned to chunk that needs extra processing.
 *
 * @param provision_drive_p             - Provision drive object.
 * @param start_lba                     - start lba of the range.
 * @param block_count                   - block count of i/o operation.
 * @param chunk_size                    - chunk size.
 * @param edge_handling_func            - Function to handle the kind of processing
 * @param number_of_edges_need_process_p   - number of edges not aligned to chunk
 *                                        and need extra processing
 *
 * @return fbe_status_t                 - status of the operation.
 *  
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *  03/06/2012 - Ashok Tamilarasan - Updated to include invalidation case
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_calc_number_of_edge_chunks_needs_process(fbe_lba_t start_lba,
                                                                   fbe_block_count_t block_count,
                                                                   fbe_u32_t chunk_size,
                                                                   fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                   fbe_provision_drive_edge_handling_check_func_t edge_handling_check_func,
                                                                   fbe_chunk_count_t * number_of_edge_chunks_need_process_p)
{
    fbe_bool_t          is_chunk_needs_process = FBE_FALSE;
    fbe_chunk_index_t   start_chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_count_t   chunk_count = 0;

    /* initialize number of edge which needs zero as zero. */
    *number_of_edge_chunks_need_process_p = 0;

    /* If lba range is invalid then return error. */
    if(start_lba == FBE_LBA_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if block count less than or equal zero then return error. */
    if(block_count <= 0)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* determine the chunk range from the given lba range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    chunk_size,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* if start lba is not aligned and (start + block count) is not aligned to chunk size 
     * then  increment the pre and post edge both if it covers more than one chunk and both
     * edges are set for zero else set only pre edge.
     */
    if((start_lba % chunk_size) && ((start_lba + block_count) % chunk_size))
    {
        if(chunk_count == 1)
        {
            /* look at the first chunk to determine if it need to perform zeroing. */
            is_chunk_needs_process = FBE_FALSE;
            edge_handling_check_func(paged_data_bits_p, &is_chunk_needs_process);
            if(is_chunk_needs_process)
            {
                (*number_of_edge_chunks_need_process_p)++;
            }
        }
        else
        {
            /* look at the first chunk to determine if it need to perform zeroing. */
            is_chunk_needs_process = FBE_FALSE;
            edge_handling_check_func(paged_data_bits_p, &is_chunk_needs_process);
            if(is_chunk_needs_process)
            {
                (*number_of_edge_chunks_need_process_p)++;
            }

            is_chunk_needs_process = FBE_FALSE;
            edge_handling_check_func(&paged_data_bits_p[chunk_count - 1], &is_chunk_needs_process);
            if(is_chunk_needs_process)
            {
                (*number_of_edge_chunks_need_process_p)++;
            }
        }
        return FBE_STATUS_OK;
    }

    /* if start lba is not aligned and (start + block count) is aligned to chunk size 
     * then  increment the edge (pre_edge) if it set for the zero on demand.
     */
    if(start_lba % chunk_size)
    {
        is_chunk_needs_process = FBE_FALSE;
        edge_handling_check_func(paged_data_bits_p, &is_chunk_needs_process);
        if(is_chunk_needs_process)
        {
            (*number_of_edge_chunks_need_process_p)++;
        }
        return FBE_STATUS_OK;
    }

    /* if start lba is aligned and (start + block count) is not aligned to chunk size 
     * then  increment the edge (post_edge) if it set for the zero on demand.
     */
    if((start_lba + block_count) % chunk_size)
    {
        is_chunk_needs_process = FBE_FALSE;
        edge_handling_check_func(&paged_data_bits_p[chunk_count - 1], &is_chunk_needs_process);
        if(is_chunk_needs_process)
        {
            (*number_of_edge_chunks_need_process_p)++;
        }
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_calc_number_of_edge_chunks_needs_process()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_calculate_edge_chunk_lba_range_which_need_process()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the lba range for the edges (pre and post)
 *  which is not aligned to chunk size and also needs extra processing for the
 *  edge range.
 *
 * @param provision_drive_p             - Provision drive object.
 * @param start_lba                     - start lba of the range.
 * @param block_count                   - block count of i/o operation.
 * @param chunk_size                    - chunk size.
 * @param pre_edge_start_lba_p          - pre edge start lba.
 * @param pre_edge_block_count_p        - pre edge block count.
 * @param post_edge_start_lba_p         - post edge start lba.
 * @param post_edge_block_count_p       - post edge block count.
 * @param number_of_edges_need_process_p- no of edges not covered part of the
 *                                        chunk boundary and need extra processing.
 *
 * @return fbe_status_t                 - status of the operation.
 *  
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *  03/06/2012 - Ashok Tamilarasan - Update to handle the invalidate case
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_calculate_edge_chunk_lba_range_which_need_process(fbe_lba_t start_lba,
                                                                            fbe_block_count_t block_count,
                                                                            fbe_chunk_size_t chunk_size,
                                                                            fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                            fbe_lba_t * pre_edge_chunk_start_lba_p,
                                                                            fbe_block_count_t * pre_edge_chunk_block_count_p,
                                                                            fbe_lba_t * post_edge_chunk_start_lba_p,
                                                                            fbe_block_count_t * post_edge_chunk_block_count_p,
                                                                            fbe_chunk_count_t * number_of_edge_chunks_need_process_p)
{
    fbe_bool_t          is_chunk_needs_process = FBE_FALSE;
    fbe_chunk_index_t   start_chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_count_t   chunk_count = 0;
    fbe_provision_drive_edge_handling_check_func_t edge_handling_check_function;
    fbe_bool_t          is_paged_data_valid;

    /* initialize number of edges which need processing. */
    *number_of_edge_chunks_need_process_p = 0;

    /* initialize the pre edge and post edge range as invalid. */
    *pre_edge_chunk_start_lba_p = FBE_LBA_INVALID;
    *pre_edge_chunk_block_count_p = 0;
    *post_edge_chunk_start_lba_p = FBE_LBA_INVALID;
    *post_edge_chunk_block_count_p = 0;

    /* If lba range is invalid then return error. */
    if(start_lba == FBE_LBA_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if block count less than or equal zero then return error. */
    if(block_count <= 0)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* determine the chunk range from the given lba range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    chunk_size,
                                                    &start_chunk_index,
                                                    &chunk_count);

    fbe_provision_drive_metadata_is_paged_metadata_valid(start_lba,
                                                         block_count, paged_data_bits_p, &is_paged_data_valid);
    if(!is_paged_data_valid)
    {
        /* There are area where the paged data is not valid and so we will have get the pre and post
         * using a different edge check function
         */
        edge_handling_check_function = fbe_provision_drive_bitmap_is_paged_data_invalid;
    }
    else
    {
        edge_handling_check_function = fbe_provision_drive_bitmap_is_chunk_marked_for_zero;
    }

    /* if start lba is not aligned and (start + block count) is not aligned to chunk size 
     * then  increment the pre and post edge both if it covers more than one chunk and both
     * edges are set for zero or not valid else set only pre edge.
     */
    if((start_lba % chunk_size) && ((start_lba + block_count) % chunk_size))
    {
        /* if there is only one chunk then we have only pre edge range. */
        if(chunk_count == 1)
        {
            /* look at the first chunk to determine if it need to perform zeroing/invalidation. */
            is_chunk_needs_process = FBE_FALSE;
            edge_handling_check_function(paged_data_bits_p, &is_chunk_needs_process);
            if(is_chunk_needs_process)
            {
                *pre_edge_chunk_start_lba_p = start_lba - (start_lba % chunk_size);
                *pre_edge_chunk_block_count_p = chunk_size;
                (*number_of_edge_chunks_need_process_p)++;
            }
        }
        else
        {
            /* look at the first chunk to determine if it need to perform zeroing/invalidation. */
            is_chunk_needs_process = FBE_FALSE;
            edge_handling_check_function(paged_data_bits_p, &is_chunk_needs_process);
            if(is_chunk_needs_process)
            {
                *pre_edge_chunk_start_lba_p = start_lba - (start_lba % chunk_size);
                *pre_edge_chunk_block_count_p = chunk_size;
                (*number_of_edge_chunks_need_process_p)++;
            }

            is_chunk_needs_process = FBE_FALSE;
            edge_handling_check_function(&paged_data_bits_p[chunk_count - 1], &is_chunk_needs_process);
            if(is_chunk_needs_process)
            {
                *post_edge_chunk_start_lba_p = start_lba + block_count - ((start_lba + block_count) % chunk_size);
                *post_edge_chunk_block_count_p = chunk_size;
                (*number_of_edge_chunks_need_process_p)++;
            }
        }
        return FBE_STATUS_OK;
    }

    /* if start lba is not aligned and (start + block count) is aligned to chunk size 
     * then  increment the edge (pre_edge) if it set for the zero on demand or page not valid.
     */
    if(start_lba % chunk_size)
    {
        /* look at the first chunk to determine if it need to perform zeroing/invalidating. */
        is_chunk_needs_process = FBE_FALSE;
        edge_handling_check_function(paged_data_bits_p, &is_chunk_needs_process);
        if(is_chunk_needs_process)
        {
            *pre_edge_chunk_start_lba_p = start_lba - (start_lba % chunk_size);
            *pre_edge_chunk_block_count_p = chunk_size; 
            (*number_of_edge_chunks_need_process_p)++;
        }
        return FBE_STATUS_OK;
    }

    /* if start lba is aligned and (start + block count) is not aligned to chunk size 
     * then  increment the edge (post_edge) if it set for the zero on demand or page not valid
     */
    if((start_lba + block_count) % chunk_size)
    {
        /* look at the last chunk to determine if it need to perform zeroing/invalidation. */
        is_chunk_needs_process = FBE_FALSE;
        edge_handling_check_function(&paged_data_bits_p[chunk_count - 1], &is_chunk_needs_process);
        if(is_chunk_needs_process)
        {
            *post_edge_chunk_start_lba_p = start_lba + block_count - ((start_lba + block_count) % chunk_size);
            *post_edge_chunk_block_count_p = chunk_size;
            (*number_of_edge_chunks_need_process_p)++;
        }
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_calculate_edge_chunk_lba_range_which_need_process()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_utils_get_packet_and_sg_list()
 ******************************************************************************
 * @brief
 *  This function is used to get the array of packet pointers and and sg list 
 *  pointer from the preallocated memory, it return error if preallocated 
 *  memory is not enough to satisfy the request.
 *
 * @param provision_drive_p     - provision drive object pointer.
 * @param memory_request_p      - preallocated memory request pointer.
 * @param new_packet_p          - pointer to subpacket array.
 * @param number_of_subpackets  - number of subpackets client wants to allocate.
 * @param sg_list_p             - pointer to the sg list
 * @param sg_element_count      - number of sg element client need for the sg list.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_get_packet_and_sg_list(fbe_provision_drive_t * provision_drive_p,
                                                 fbe_memory_request_t * memory_request_p,
                                                 fbe_packet_t * new_packet_p[],
                                                 fbe_u32_t number_of_subpackets,
                                                 fbe_sg_element_t ** sg_list_p,
                                                 fbe_u32_t sg_element_count)
{
    fbe_u32_t               sg_list_size = 0;
    fbe_u32_t               subpacket_size = 0;
    fbe_u32_t               subpacket_index = 0;
    fbe_memory_header_t *   master_memory_header = NULL;
    fbe_memory_header_t *   current_memory_header = NULL;
    fbe_u32_t               used_memory_in_chunk = 0;
    fbe_u32_t               number_of_used_chunks = 0;
    fbe_u8_t *              buffer_p = NULL;

    /* If memory request is null then return error. */
    if(memory_request_p->ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s:memory request pointer is null, num_pkts:0x%x, sg_count:0x%x.\n",
                              __FUNCTION__, number_of_subpackets, sg_element_count);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get the pointer to the allocated memory from the memory request. */
    master_memory_header = current_memory_header = memory_request_p->ptr;
    buffer_p = current_memory_header->data;

    if((sg_list_p != NULL) && (sg_element_count != 0))
    {
       /* if sg list size is greater than chunk size of the the memory then return the
        * failure, we cannot break the sg list across the two chunks.
        */
        sg_list_size = (sizeof(fbe_sg_element_t) * sg_element_count);

        /* round the sg list size to align with 64-bit boundary. */
        sg_list_size += (sg_list_size % sizeof(fbe_u64_t));

        if(sg_list_size > (fbe_u32_t) master_memory_header->memory_chunk_size)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s:sg list size exceeded memory chunk size, sg_list_size:0x%x, mem_chk_size:0x%x.\n",
                                  __FUNCTION__, sg_list_size, master_memory_header->memory_chunk_size);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            *sg_list_p = (fbe_sg_element_t *) buffer_p;
            buffer_p = buffer_p + sg_list_size;
            used_memory_in_chunk = sg_list_size;
        }
    }

    /* reserve the memory for the subpackets. */
    subpacket_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    
    /* Divide the allocated memory in chunks to get the array of packets, SG
     * lists and data buffers.
     */
    subpacket_index = 0;
    while(subpacket_index < number_of_subpackets)
    {
        if ((used_memory_in_chunk + subpacket_size) <=
            ((fbe_u32_t) master_memory_header->memory_chunk_size))
        {
            *new_packet_p = (fbe_packet_t *) buffer_p;
            buffer_p = buffer_p + FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;

            /* increment the used memory in chunks with single chunk size.*/
            used_memory_in_chunk += subpacket_size;
            subpacket_index++;
            new_packet_p++;
        }
        else
        {
            /* increment the number of chunks used. */
            number_of_used_chunks++;

            /* if number of chunks used less than number of chunks allocated
             * then shift the pointer otherwise complete the packet with error.
             */
            if(number_of_used_chunks < master_memory_header->number_of_chunks)
            {
                /* get the next memory header. */
                current_memory_header = current_memory_header->u.hptr.next_header;
                buffer_p = current_memory_header->data;
                used_memory_in_chunk = 0;
            }
            else
            {
                /* we do not have preallocated sufficient memory. */
                fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s:allocated memory not sufficient, num_pkts:0x%x, sg_cnt:0x%x, chk_size:0x%x. num_chk:0x%x.\n",
                                      __FUNCTION__, number_of_subpackets, sg_element_count,
                                      master_memory_header->memory_chunk_size, master_memory_header->number_of_chunks);
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_get_packet_and_sg_list()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_destroy_iots_completion()
 ******************************************************************************
 * @brief
 *  This function is used to simply destroy the iots.
 * 
 * @param   packet_p        - pointer to disk zeroing IO packet
 * @param   context         - context, which is a pointer to the pvd object.
 *
 * @return fbe_status_t    - status of the operation.
 *
 * @author
 *  9/2/2010 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_destroy_iots_completion(fbe_packet_t * packet_p,
                                            fbe_packet_completion_context_t context)
{
    fbe_raid_iots_t   *iots_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_basic_destroy(iots_p);
    /* Return OK to complete the packet to the next level.
     */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_provision_drive_destroy_iots_completion
 **************************************/
/*!****************************************************************************
 * fbe_provision_drive_utils_initialize_iots()
 ******************************************************************************
 * @brief
 *  This function is used to initialize an iots before we process the incoming
 *  packet to make sure we initialize it.
 *
 * @note
 *  Provision drive object uses iots only for utilzing some of the common
 *  functionality which assumes iots set to initialize properly.
 * 
 * @param provision_drive_p 	- pointer to provision drive object.
 * @param packet_p 				- packet to start the iots for.
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/25/2010 - Created. Dhaval Patel (Derived from existing RAID function). 
 *
 ******************************************************************************/

fbe_status_t fbe_provision_drive_utils_initialize_iots(fbe_provision_drive_t * provision_drive_p, 
                                                       fbe_packet_t * packet_p)
{
    fbe_raid_iots_t     *					iots_p = NULL;
    fbe_payload_ex_t *						sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *			block_operation_p = NULL;
    fbe_lba_t 								lba;
    fbe_block_count_t 						blocks;
    fbe_payload_block_operation_opcode_t 	opcode;

    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    if (block_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s block operation is null.\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize and kick off iots. */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* Initialize the iots with default state. */
    fbe_raid_iots_basic_init(iots_p, packet_p, NULL);
    fbe_raid_iots_basic_init_common(iots_p);
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_NOT_USED);

    /* Set the completion function so the iots is eventually destroyed.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_destroy_iots_completion, provision_drive_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_initialize_iots()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_quiesce_io_if_needed()
 ******************************************************************************
 * @brief
 *   This function determines whether i/o is quiesce or not, if it is not
 *   quiesce then it sets the provision drive quiesce condition to quiesce and
 *   then unquiesce later.
 * 
 * @param provision_drive_p         -   pointer to a provision drive object.
 * @param is_already_quiesce_b_p    -   returns TRUE if i/o is already quiesced
 *                                      else return FALSE.
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   06/25/2010 - Created. Dhaval Patel (Derived from existing RAID function). 
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_quiesce_io_if_needed(fbe_provision_drive_t * provision_drive_p,
                                               fbe_bool_t * is_already_quiesce_b_p)
{

    /* Initialize the is already quiesce as false. */
    *is_already_quiesce_b_p = FBE_FALSE;

    /*  If provision drive object is is already quiesced then we do not need to
     *  do more here and return the good status.
     */
    if(fbe_base_config_is_clustered_flag_set((fbe_base_config_t *) provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED) == FBE_TRUE)
    {
        *is_already_quiesce_b_p = FBE_TRUE;
        return FBE_STATUS_OK; 
    }

    /* Set a condition to quiese. */
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);

    /*  Set a condition to unquiese, it is located after the quiesce condition in rotary and
     *  assumption is all the condition which needs a quiesce operation will be added in
     *  between these two condition in rotary.
     */ 
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);

    /* Reschedule monitor immediately. */
    fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                             (fbe_base_object_t *) provision_drive_p, 0);

    /* Return good status. */     
    return FBE_STATUS_OK; 
}
/******************************************************************************
 * end fbe_provision_drive_quiesce_io_if_needed()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_complete_packet()
 ******************************************************************************
 * @brief
 *  This generic function is used to complete the packet and it also update the
 *  status of the packet. 
 *
 * @param   packet_p            - pointer to packet.
 * @param   status              - status of the operation.
 * @param   status_qualifier    - status qualifier.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_complete_packet(fbe_packet_t * packet_p,
                                          fbe_status_t status, fbe_u32_t status_qualifier)
{
    fbe_transport_set_status(packet_p, status, status_qualifier);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_complete_packet()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_block_transport_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the completion for the subpacket for the
 *  the provisiion drive object.
 * 
 * @note
 *  Provision drive object should use this function whenever it creates
 *  subpacket and send it to the downstream edge, it is to make sure that error
 *  precedence take into account while updating master packet status.
 *
 * @param   packet_p        - pointer to disk zeroing IO packet
 * @param   context         - context, which is a pointer to the pvd object.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_block_transport_subpacket_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_packet_t *                          master_packet_p = NULL;
    fbe_payload_ex_t *                         new_payload_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         master_block_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_queue_head_t						tmp_queue;
	fbe_bool_t is_empty;

    /* get the master packet. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);

    sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    new_payload_p = fbe_transport_get_payload_ex(packet_p);
    
    /* get the subpacket and master packet's block operation. */
    master_block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    block_operation_p = fbe_payload_ex_get_block_operation(new_payload_p);

    /* update the master packet status only if it is required. */
    fbe_provision_drive_utils_process_block_status(provision_drive_p, master_packet_p, packet_p);

    /* release the block operation. */
    fbe_payload_ex_release_block_operation(new_payload_p, block_operation_p);

    /* remove the subpacket from master packet. */
    //fbe_transport_remove_subpacket(packet_p);
	fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* destroy the packet. */
    //fbe_transport_destroy_packet(packet_p);

    /* when the queue is empty, all subpackets have completed. */
    if(is_empty)
    {
		fbe_transport_destroy_subpackets(master_packet_p);
        /* release the preallocated memory for the zero on demand request. */
        //fbe_memory_request_get_entry_mark_free(&master_packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(&master_packet_p->memory_request);

        /* remove the master packet from terminator queue and complete it. */
        fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) provision_drive_p, master_packet_p);

        /* initialize the queue. */
        fbe_queue_init(&tmp_queue);

        /* enqueue this packet temporary queue before we enqueue it to run queue. */
        fbe_queue_push(&tmp_queue, &master_packet_p->queue_element);

        /*!@note Queue this request to run queue to break the i/o context to avoid the stack
        * overflow, later this needs to be queued with the thread running in same core.
        */
        fbe_transport_run_queue_push(&tmp_queue,  NULL,  NULL);

    }
    else
    {
        /* not done with processing sub packets.
         */
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_block_transport_subpacket_completion()
 ******************************************************************************/

/*!****************************************************************************
* fbe_provision_drive_release_block_op_completion()
******************************************************************************
* @brief
*  This is a completion function for where we have a block operation that
*  was allocated on a packet where there is also another master block
*  operation above this one.
* 
 * @param   packet_p        - pointer to disk zeroing IO packet
* @param   context         - context, which is a pointer to the pvd object.
*
* @return fbe_status_t.    - status of the operation.
*
* @author
*  9/14/2012 - Created. Rob Foley
*
******************************************************************************/
fbe_status_t 
fbe_provision_drive_release_block_op_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t * master_block_operation_p = NULL;
    fbe_payload_block_operation_t * block_operation_p = NULL;
    payload = fbe_transport_get_payload_ex(packet);
    block_operation_p =  fbe_payload_ex_get_block_operation(payload);

    fbe_payload_ex_release_block_operation(payload, block_operation_p);

    master_block_operation_p = fbe_payload_ex_get_present_block_operation(payload);

    fbe_payload_block_operation_copy_status(block_operation_p, master_block_operation_p);
   
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_provision_drive_release_block_op_completion()
 **************************************/

/*!****************************************************************************
 *          fbe_provision_drive_utils_trace()
 ******************************************************************************
 * @brief
 * This function is used to enhance the debugging functionality for provision 
 * drive. It wraps over the "fbe_base_object_trace" and prints additional 
 * debugging information using trace level FBE_TRACE_LEVEL_INFO. 
 * Caller should pass the debug flag based on the functionality. 
 * 
 * @param   provision_drive_p   - pointer to provision drive object
 * @param   trace_level         - base object trace level
 * @param   message_id          - base object trace message id
 * @param   debug_flag          - caller specified debug flag
 * @param   fmt                 - pointer to formatted string
 *
 * @return void
 * 
 * Note: Max capacity to print string format is 188 characters
 *
 * @author
 *   09/02/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
void
fbe_provision_drive_utils_trace(fbe_provision_drive_t * provision_drive_p, 
                                fbe_trace_level_t trace_level,
                                fbe_u32_t message_id,
                                fbe_provision_drive_debug_flags_t debug_flag,
                                const fbe_char_t * fmt, ...)
{

    
    char buffer[FBE_PROVISION_DRIVE_MAX_TRACE_CHARS];
    va_list argList;
    fbe_u32_t num_chars_in_string = 0;
    fbe_bool_t b_is_debug_flag_set = FBE_FALSE;

    /* check if debug flag is set either for this PVD or globally for PVD class */
    if(fbe_provision_drive_class_is_debug_flag_set(debug_flag) || 
       fbe_provision_drive_is_pvd_debug_flag_set(provision_drive_p, debug_flag))
    {
        b_is_debug_flag_set = FBE_TRUE;
        if (trace_level > ((fbe_base_object_t*)provision_drive_p)->trace_level)
        {
            trace_level = FBE_TRACE_LEVEL_INFO;
        }
    }

    /* if pvd debug flag and caller specified debug flag both are set and the trace level is one of the _DEBUG_ 
     * level then overwrite the trace level with FBE_TRACE_LEVEL_INFO 
     */
    if ( ( b_is_debug_flag_set && 
           ((trace_level == FBE_TRACE_LEVEL_DEBUG_LOW) ||
            (trace_level == FBE_TRACE_LEVEL_DEBUG_MEDIUM) ||
            (trace_level == FBE_TRACE_LEVEL_DEBUG_HIGH)) ) ||
         (trace_level <= ((fbe_base_object_t*)provision_drive_p)->trace_level) )
    {
		buffer[0] = '\0';
		if (fmt != NULL){    
			va_start(argList, fmt);
			num_chars_in_string = _vsnprintf(buffer, FBE_PROVISION_DRIVE_MAX_TRACE_CHARS, fmt, argList);
			buffer[FBE_PROVISION_DRIVE_MAX_TRACE_CHARS - 1] = '\0';
			va_end(argList);
		}
        /* associate debug flag is set for the caller specified flag, use the trace level as _TRACE_LEVEL_INFO */
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, trace_level, message_id, buffer); 
        
        return;
    }

    return;
}

/*!****************************************************************************
 * fbe_provision_drive_utils_get_block_operation_error_precedece()
 ******************************************************************************
 * @brief
 *  This function determines how to rate the block status error.
 * 
 *
 * @param   provision_drive_p   - pointer to the object
 *                                This is only used for tracing purpose.
 *  TODO: we need to review this code to see if we really need this function.
 * 
 *
 * @param   error               - block operation status
 *
 * @return
 *  A number with a relative precedence value.
 *  This number may be compared with other precedence values from this
 *  function.
 *
 * @author
 *   09/15/2010: - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_provision_drive_error_precedence_t fbe_provision_drive_utils_get_block_operation_error_precedece(
                                                    fbe_provision_drive_t * provision_drive_p,
                                                    fbe_payload_block_operation_status_t    error)
{

    fbe_provision_drive_error_precedence_t priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_NO_ERROR;


    /* Determine the priority of this error
     * Certain errors are more significant than others.
     */
    switch (error)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
            /* Set precedence.
             */
            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_NO_ERROR;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID:
            /* The only way this could occur is if the I/O was never
             * executed.  Assumption is that it is retryable.
             */
            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_INVALID_REQUEST;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
            /* Request was not properly formed, etc. Retryable.
             */
            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_INVALID_REQUEST;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            /* Set precedence.
             */
            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_MEDIA_ERROR;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY:
            /* With support of drive spin-down this may occur and depending
             * on the situation (i.e. if there is redundancy etc.) the I/O
             * may be retried.
             */
            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_NOT_READY;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
            /* The request was purposfully aborted.  It is up to the
             * originator to retry the request.
             */
            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_ABORTED;
            break;


        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
            /* May not want to retry this request.  Up to the client.
             */
            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_TIMEOUT;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
            /* Device not usable therefore don't retry (at least until the
             * device is replaced).
             */
            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_DEVICE_DEAD;
            break;

        default:
            /* We don't expect this so use the highest priority error.
             */
            fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s :error 0x%x != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID\n",
                                   __FUNCTION__, error);

            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_UNKNOWN;
            break;
    }

    return priority;
}
/******************************************************************************
 * end fbe_provision_drive_utils_get_block_operation_error_precedece()
 ******************************************************************************/



/*!****************************************************************************
 * fbe_provision_drive_utils_process_block_status()
 ******************************************************************************
 * @brief
 * This function is used to update the master block status if 
 * subpacket block status has precedence.
 *
 * @param   provision_drive_p           - pointer to provision drive object
 * @param   master_packet_p             - pointer to block operation master
 *                                        packet 
 * @param   master_block_operation_p    - master block operation status
 * @param   packet_p                    - pointer to block operation sub 
 *                                        packet
 * @param   block_operation_p           - subpacket block operation status
 *
 * 
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   09/13/2010 - Amit Dhaduk
 *              
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_process_block_status(fbe_provision_drive_t * provision_drive_p,
			                                    fbe_packet_t * master_packet_p,
												fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *						payload_p = NULL;
	fbe_payload_ex_t *						new_payload_p = NULL;
    fbe_payload_block_operation_t *         master_block_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;

    fbe_payload_block_operation_status_t        sub_block_operation_status;
    fbe_payload_block_operation_qualifier_t     sub_block_operation_qualifier;
    fbe_time_t                                  sub_block_operation_retry_wait_msecs;
    fbe_payload_block_operation_status_t        master_block_operation_status;
    fbe_payload_block_operation_qualifier_t     master_block_operation_qualifier;
    fbe_payload_block_operation_status_t        master_block_status;
    fbe_payload_block_operation_qualifier_t     master_block_qualifier;
    fbe_time_t                                  master_block_operation_retry_wait_msecs;
    fbe_provision_drive_error_precedence_t      master_packet_error_precedence;
    fbe_provision_drive_error_precedence_t      sub_packet_error_precedence;    
    fbe_payload_block_operation_opcode_t        sub_packet_opcode;
    fbe_lba_t                                   start_lba;
    fbe_block_count_t                           block_count;
    fbe_status_t                                status;
    fbe_status_t                                function_status;
    fbe_u32_t                                   status_qualifier;
    fbe_lba_t                                   master_block_operation_media_error_lba = FBE_LBA_INVALID;
    fbe_lba_t                                   sub_block_operation_media_error_lba = FBE_LBA_INVALID;
    fbe_bool_t                                  is_subpacket_has_precedence = FBE_FALSE;
    //fbe_object_id_t                             pvd_object_id;

    payload_p = fbe_transport_get_payload_ex(master_packet_p);
    new_payload_p = fbe_transport_get_payload_ex(packet_p);
    
    /* get the subpacket and master packet's block operation. */
    master_block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);
    block_operation_p = fbe_payload_ex_get_block_operation(new_payload_p);

    status = fbe_transport_get_status_code(packet_p); 
	if(status == FBE_STATUS_INVALID){
        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "pvd_prc_blk_stat: pkt status: INVALID \n");
	}

    /* get the packet status */
	status = fbe_transport_get_status_code(packet_p); 
	status_qualifier = fbe_transport_get_status_qualifier(packet_p);


    fbe_payload_block_get_lba(master_block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(master_block_operation_p, &block_count); 
    //fbe_payload_ex_get_pvd_object_id(new_payload_p, &pvd_object_id);
    /* get the subpacket status and details */
    fbe_payload_block_get_status(block_operation_p, &sub_block_operation_status);
    fbe_payload_block_get_opcode(block_operation_p, &sub_packet_opcode);
    fbe_payload_block_get_qualifier(block_operation_p, &sub_block_operation_qualifier);
    fbe_payload_ex_get_media_error_lba(new_payload_p, &sub_block_operation_media_error_lba);
    fbe_payload_ex_get_retry_wait_msecs(new_payload_p, &sub_block_operation_retry_wait_msecs);


    /* trace the I/O error details */
    if((status != FBE_STATUS_OK) ||
       (sub_block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "pvd_prc_blk_stat:lba:0x%llx, blk:0x%llx, stat:0x%x, blk_stat:0x%x \n",
                                        (unsigned long long)start_lba, (unsigned long long)block_count, status, sub_block_operation_status);

        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "cnt: blk_qual:0x%x opcode:0x%x, packet:%p m_packet:%p.\n",
                                        sub_block_operation_qualifier, sub_packet_opcode, packet_p, master_packet_p);
    }    

    /* get the master packet status and details */
    fbe_payload_block_get_status(master_block_operation_p, &master_block_operation_status);
    fbe_payload_block_get_qualifier(master_block_operation_p, &master_block_operation_qualifier);
    fbe_payload_ex_get_media_error_lba(payload_p, &master_block_operation_media_error_lba);
    fbe_payload_ex_get_retry_wait_msecs(payload_p, &master_block_operation_retry_wait_msecs);

    /* following code keep commented for testing purpose
     * check that subpacket status is valid 
     *    if (( sub_block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) ||
     *             (sub_block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID))
     *    {
     *   fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
     *                         FBE_TRACE_LEVEL_ERROR,
     *                         FBE_TRACE_MESSAGE_ID_INFO,
     *                         "%s:Invalid subpacket status, provision_drive_p:%p.\n",
     *                         __FUNCTION__, provision_drive_p); 
     *
     *   
     *        fbe_provision_drive_unlock(provision_drive_p);
     *
     *   return FBE_STATUS_GENERIC_FAILURE;
     * }
     */

    /* check if this is the subpacket needs SLF processing */
    if(packet_p->packet_attr & FBE_PACKET_FLAG_SLF_REQUIRED){
        fbe_transport_set_packet_attr(master_packet_p, FBE_PACKET_FLAG_SLF_REQUIRED);
    }


    /* check if this is the first subpacket get processing */
    if (master_block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)
    {
        /* We have not yet saved any status in the master packet, 
         * just set the current subpacket status in the master packet
         * There is no precedence to determine.
         */ 
        fbe_payload_ex_set_media_error_lba(payload_p, sub_block_operation_media_error_lba);

        fbe_payload_ex_set_retry_wait_msecs(payload_p, sub_block_operation_retry_wait_msecs);
        //fbe_payload_ex_set_pvd_object_id(payload_p, pvd_object_id);


        fbe_provision_drive_utils_determine_master_block_status(status, 
                                                                sub_block_operation_status, 
                                                                sub_block_operation_qualifier,
                                                                &master_block_status,
                                                                &master_block_qualifier);

        fbe_payload_block_set_status(master_block_operation_p, master_block_status, master_block_qualifier);
        
        fbe_transport_set_status(master_packet_p, status, status_qualifier);

        fbe_transport_propagate_physical_drive_service_time(master_packet_p, packet_p);
        return FBE_STATUS_OK;

    }


    /* determine the precedence of errors for subpacket block status.
     * Only if the subpacket block status takes precedence we do update the master packet block status.
     */

    /* get the block status error precedence details */
    master_packet_error_precedence = fbe_provision_drive_utils_get_block_operation_error_precedece(
                                                    provision_drive_p, master_block_operation_status);

    sub_packet_error_precedence = fbe_provision_drive_utils_get_block_operation_error_precedece(
                                                    provision_drive_p, sub_block_operation_status);

    function_status = fbe_provision_drive_utils_is_block_status_has_precedence(
                                            master_packet_error_precedence,
                                            master_block_operation_media_error_lba,
                                            master_block_operation_retry_wait_msecs,
                                            sub_packet_error_precedence,
                                            sub_block_operation_media_error_lba,
                                            sub_block_operation_retry_wait_msecs,
                                            sub_block_operation_status,
                                            sub_block_operation_qualifier,
                                            &is_subpacket_has_precedence);


    /* check if subpacket block status has precedence */
    if( is_subpacket_has_precedence ) 
    {
       /* The subpacket block status clearly takes precedence,
        * overwrite the master packet block status
        */
        fbe_payload_ex_set_media_error_lba(payload_p, sub_block_operation_media_error_lba);
        fbe_payload_ex_set_retry_wait_msecs(payload_p, sub_block_operation_retry_wait_msecs);
        //fbe_payload_ex_set_pvd_object_id(payload_p, pvd_object_id);
        fbe_payload_block_set_status(master_block_operation_p, sub_block_operation_status, sub_block_operation_qualifier);
        fbe_transport_set_status(master_packet_p, status, status_qualifier);
    }

    if (packet_p->physical_drive_service_time_ms > master_packet_p->physical_drive_service_time_ms)
    {
        fbe_transport_propagate_physical_drive_service_time(master_packet_p, packet_p);
    }

    return FBE_STATUS_OK;
    
}
/******************************************************************************
 * end fbe_provision_drive_utils_process_block_status()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_is_block_status_has_precedence()
 ******************************************************************************
 * @brief
 * This function determines if subpacket block status has precedence or not. 
 * 
 *
 * @param   master_error_precedence           - pointer to provision drive object
 * @param   master_media_error_lba             - pointer to block operation master
 * @param   master_retry_wait_msec    - master block operation status
 * @param   sub_error_precedence                    - pointer to block operation sub 
 * @param   sub_media_error_lba           - pointer to provision drive object
 * @param   sub_retry_wait_msec             - pointer to block operation master
 * @param   sub_operation_status    - master block operation status
 * @param   sub_qualifier                    - pointer to block operation sub 
 * 
 * @return 
 *  fbe_status_t.                    - status of the operation.
 *  b_is_subpacket_has_precedence_p  - 
 *                  FBE_TRUE - if subpacket block status has precedence
 *                  FBE_FALSE - if subpacket block status has no precedence
 *
 * @author
 *   09/13/2010 - Amit Dhaduk
 *              
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_utils_is_block_status_has_precedence(
                    fbe_provision_drive_error_precedence_t      master_error_precedence,
                    fbe_lba_t                                   master_media_error_lba,
                    fbe_time_t                                  master_retry_wait_msec,
                    fbe_provision_drive_error_precedence_t      sub_error_precedence,
                    fbe_lba_t                                   sub_media_error_lba,
                    fbe_time_t                                  sub_retry_wait_msec,
                    fbe_payload_block_operation_status_t        sub_operation_status,
                    fbe_payload_block_operation_qualifier_t     sub_qualifier,
                    fbe_bool_t                                  *b_is_subpacket_has_precedence_p)
{
    *b_is_subpacket_has_precedence_p = FBE_FALSE;

    if( master_error_precedence > sub_error_precedence)
    {
        /* subpacket has no error precedence */
        *b_is_subpacket_has_precedence_p = FBE_FALSE;
    }
    else if( master_error_precedence < sub_error_precedence)
    {
        /* subpacket has error precedence */
        *b_is_subpacket_has_precedence_p = FBE_TRUE;        
    }
    else if( sub_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /* master packet and subpacket has same error precedence
         * and block status is MEDIA error
         * Let check if block status has lower lba
         */
        if( master_media_error_lba > sub_media_error_lba )
        {
            /* block status has higher precedence */
            *b_is_subpacket_has_precedence_p = FBE_TRUE;            
        }
    }
    else if(( sub_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
                (sub_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE))
    {
        /* master packet and subpacket has same error precedence
         * block status is IO failed and qualifier is retry possible
         * if subpacket has greater retry wait time then will get the precedence
         */
        if( master_retry_wait_msec < sub_retry_wait_msec )
        {
            *b_is_subpacket_has_precedence_p = FBE_TRUE;            
        }
    }

    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_provision_drive_utils_is_block_status_has_precedence()
 ******************************************************************************/

/*!**************************************************************
 * fbe_provision_drive_check_hook()
 ****************************************************************
 * @brief
 *  This is the function that calls the hook for the pvd.
 *
 * @param provision_drive_p - Object.
 * @param state - Current state of the monitor for the caller.
 * @param substate - Sub state of this monitor op for the caller.
 * @param status - status of the hook to return.
 * 
 * @return fbe_status_t Overall status of the operation.
 *
 * @author
 *  10/6/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_provision_drive_check_hook(fbe_provision_drive_t *provision_drive_p,
                                            fbe_u32_t state,
                                            fbe_u32_t substate,
                                            fbe_u64_t val2,
                                            fbe_scheduler_hook_status_t *status)
{

	fbe_status_t ret_status;
    ret_status = fbe_base_object_check_hook((fbe_base_object_t *)provision_drive_p,
                                            state,
                                            substate,
                                            val2,
                                            status);
    return ret_status;
}
/******************************************
 * end fbe_provision_drive_check_hook()
 ******************************************/

/*!***********************************************************
 *          fbe_provision_drive_is_background_request()
 *************************************************************
 * @brief
 *  Determines if this is a background (i.e. request has been
 *  issued by the provision drive monitor or not).  This method is 
 *  just look at block opcode to see if it is from monitor or not.
 * 
 * @param opcode - block operation code
 * 
 * @return FBE_TRUE - Is a background request
 *        FBE_FALSE - Normal IO request
 * 
 ************************************************************/

fbe_bool_t fbe_provision_drive_is_background_request(fbe_payload_block_operation_opcode_t opcode)
{
    fbe_bool_t b_return;

    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY))
    {
        b_return = FBE_TRUE;
    }
    else
    {
        b_return = FBE_FALSE;
    }
    return b_return;
}

/*!****************************************************************************
 * fbe_provision_drive_utils_get_lba_for_chunk_index()
 ******************************************************************************
 * @brief
 *   This function will get the LBA that corresponds to the given chunk index.  
 *
 * @param provision_drive_p      
 * @param in_chunk_index        - chunk index for which to find the LBA.
 * @param out_lba_p             - pointer which gets set to the LBA of the chunk
 *                                for the input index; only valid when function
 *                                returns success 

 *
 * @return  fbe_status_t  
 *
 * @author
 *   02/23/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_utils_get_lba_for_chunk_index(
                                    fbe_provision_drive_t*   provision_drive_p,
                                    fbe_chunk_index_t        chunk_index,
                                    fbe_lba_t*               lba_p)          
{
    
    fbe_chunk_count_t               total_chunks;
    fbe_lba_t                       exported_capacity;
    fbe_lba_t                       total_capacity;
    fbe_lba_t                       paged_capacity;

    //fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);
    fbe_provision_drive_get_user_capacity(provision_drive_p, &exported_capacity);
    
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *) provision_drive_p, &paged_capacity);

    total_capacity = exported_capacity + paged_capacity;
    total_chunks = (fbe_chunk_count_t) (total_capacity / FBE_PROVISION_DRIVE_CHUNK_SIZE);

    if (chunk_index > total_chunks)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: chunk index:0x%llx greater than total chunks: 0x%llx\n", 
                              __FUNCTION__, (unsigned long long)chunk_index,
			      (unsigned long long)total_chunks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    *lba_p = (fbe_lba_t) (chunk_index * FBE_PROVISION_DRIVE_CHUNK_SIZE); 

    return FBE_STATUS_OK; 

} // End fbe_provision_drive_utils_get_lba_for_chunk_index()


/*!****************************************************************************
 * fbe_provision_drive_utils_get_num_of_exported_chunks()
 ******************************************************************************
 * @brief
 *   This function will return of number of chunks based on the exported capacity  
 *
 * @param provision_drive_p      
 * @param out_num_of_chunks        - Number of chunks
 
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_utils_get_num_of_exported_chunks(fbe_provision_drive_t*   provision_drive_p,
                                                                  fbe_chunk_count_t      * out_num_of_chunks)          
{
    fbe_lba_t                       exported_capacity;
    
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);
    
    *out_num_of_chunks = (fbe_chunk_count_t) (exported_capacity / FBE_PROVISION_DRIVE_CHUNK_SIZE);

    return FBE_STATUS_OK; 

} // End fbe_provision_drive_utils_get_num_of_exported_chunks()

/*!***********************************************************
 *   fbe_provision_drive_utils_determine_master_block_status()
 *************************************************************
 * @brief
 *  Determines the master block status depending upon the subpacket
 *  block status and packet status
 * 
 * @param opcode - block operation code
 * 
 * @return FBE_TRUE - Is a background request
 *        FBE_FALSE - Normal IO request
 * 
 ************************************************************/

static fbe_status_t fbe_provision_drive_utils_determine_master_block_status(fbe_status_t   status,
                                               fbe_payload_block_operation_status_t sub_packet_block_status,
                                               fbe_payload_block_operation_qualifier_t sub_packet_status_qualifier,
                                               fbe_payload_block_operation_status_t* master_block_status_p,
                                               fbe_payload_block_operation_qualifier_t* master_block_qualifier_p)
{
    /* This means that IO was not even executed */
    if( (status != FBE_STATUS_OK) && (sub_packet_block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) )
    {
        *master_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
        *master_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;

    }

    else
    {
        *master_block_status_p = sub_packet_block_status;
        *master_block_qualifier_p = sub_packet_status_qualifier;
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_provision_drive_utils_can_verify_invalidate_start()
 ******************************************************************************
 * @brief
 *  This function is used to determine whether we can start the verify 
 *  invalidate operation.
 *
 * @param provision_drive_p     - pointer to the provision drive object.
 * @param can_verify_invalidate_start_p   - retuns whether it is ok to start operation.
 *
 * @return  fbe_status_t           
 *
 * @author
 *   04/02/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_can_verify_invalidate_start(fbe_provision_drive_t * provision_drive_p,
                                                      fbe_bool_t * can_verify_invalidate_start_p)
{

    fbe_bool_t  is_load_okay_b = FBE_FALSE;
    fbe_base_config_downstream_health_state_t   downstream_health_state;

    /* initialiez can zero start as false. */
    *can_verify_invalidate_start_p = FBE_FALSE;

    /* can we perform the zero operation with current downstream health? */
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);
    if(downstream_health_state != FBE_DOWNSTREAM_HEALTH_OPTIMAL) {
         return FBE_STATUS_OK;
    }

    /*  See if we are allowed to do a zero I/O based on the current load. */
    fbe_provision_drive_utils_verify_invalidate_check_permission_based_on_current_load(provision_drive_p, &is_load_okay_b);
    if (is_load_okay_b == FBE_FALSE) {
         return FBE_STATUS_OK;
    }

    /* all the checks succeed, we can start zeroing. */
    *can_verify_invalidate_start_p = FBE_TRUE;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_can_verify_invalidate_start()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_ask_verify_invalidate_permission()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the event and send it to upstream object
 *  to get the permission for the verify invalidate operation.
 * 
 * @param provision_drive_p - Provision Drive object.
 * @param packet_p        - FBE packet pointer.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 04/02/2012 - Created. Ashok Tamilarasan
 * 
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_utils_ask_verify_invalidate_permission(fbe_provision_drive_t * provision_drive_p,
                                                          fbe_lba_t lba,
                                                          fbe_block_count_t block_count,
                                                          fbe_packet_t * packet_p)
{
    fbe_event_t *               event_p = NULL;
    fbe_event_stack_t *         event_stack = NULL;
    fbe_event_permit_request_t  permit_request = {0};
    fbe_lba_t                       next_consumed_lba = FBE_LBA_INVALID;
    fbe_lba_t                       default_offset = FBE_LBA_INVALID;


    /* For system RGs the PVD directly connects to RAID. So calculate the next consumed lba
       and set the sniff checkpoint
    */
    fbe_block_transport_server_find_next_consumed_lba(&provision_drive_p->base_config.block_transport_server,
                                                      lba,
                                                      &next_consumed_lba);

    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);

    if((next_consumed_lba != lba) && (lba < default_offset))
    {
        fbe_provision_drive_verify_update_invalidate_checkpoint(provision_drive_p, packet_p, lba, next_consumed_lba,
                                                           block_count);
        return FBE_STATUS_OK;
    }

    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    if(event_p == NULL)
    { 
        /* Do not worry we will send it later */
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                        "%s event allocatil failed", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize the event. */
    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);

    /* setup the zero permit request event. */
    permit_request.permit_event_type = FBE_PERMIT_EVENT_TYPE_VERIFY_INVALIDATE_REQUEST;
    fbe_event_set_permit_request_data(event_p, &permit_request);

    /* allocate the event stack. */
    event_stack = fbe_event_allocate_stack(event_p);

    /* fill the LBA range to request for the zero operation. */
    fbe_event_stack_set_extent(event_stack, lba, block_count);

 #if 0 /* Use default priority for now*/
    /* Set medic action priority. */
    fbe_medic_get_action_priority(FBE_MEDIC_ACTION_VERIFY_INVALIDATE, &medic_action_priority);
    fbe_event_set_medic_action_priority(event_p, medic_action_priority);
#endif 
    /* set the event completion function. */
    fbe_event_stack_set_completion_function(event_stack,
                                            fbe_provision_drive_utils_ask_verify_invalidate_permission_completion,
                                            provision_drive_p);

    /* send the zero permit request to upstream object. */
    fbe_base_config_send_event((fbe_base_config_t *)provision_drive_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_ask_verify_invalidate_permission()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_ask_verify_invalidate_permission_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the event to ask for the 
 *  verify invalidate request permission to upstream object.
 * 
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 04/03/2012 - Created. Ashok Tamilarasan
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_utils_ask_verify_invalidate_permission_completion(fbe_event_t * event_p,
                                                                      fbe_event_completion_context_t context)
{
    fbe_event_stack_t *         event_stack_p = NULL;
    fbe_packet_t *              packet_p = NULL;
    fbe_provision_drive_t *     provision_drive_p = NULL;
    fbe_object_id_t             object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                  b_is_system_drive = FBE_FALSE;
    fbe_u32_t                   event_flags = 0;
    fbe_event_status_t          event_status = FBE_EVENT_STATUS_INVALID;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_lba_t                   verify_invalidate_checkpoint = FBE_LBA_INVALID;
    fbe_block_count_t           chunk_size = 0;
    fbe_lba_t                   default_offset = FBE_LBA_INVALID;
    fbe_scheduler_hook_status_t hook_status;
    fbe_block_count_t unconsumed_blocks = 0;


    /* Get the provision drive object pointer and object id.
     */
    provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);

    /* Get flag and status of the event. */
    event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_get_master_packet(event_p, &packet_p);
    fbe_event_get_flags (event_p, &event_flags);
    fbe_event_get_status(event_p, &event_status);

    /*! @note Get the verify invalidate checkpoint from the ask permission request.  
     * Do not use the provision drive verify invalidate checkpoint since it may have been
     * modified while we were waiting for permission.
     */
    verify_invalidate_checkpoint = event_stack_p->lba;
    chunk_size = event_stack_p->block_count;
    unconsumed_blocks = event_p->event_data.permit_request.unconsumed_block_count;

    /* Release the event.  */
    fbe_event_release_stack(event_p, event_stack_p);
    fbe_event_destroy(event_p);
    fbe_memory_ex_release(event_p);

    /*! @note We have already set the completion which will
     * simply re-schedules the run of verify invalidate operation.
     */

    /* First check the event status.
     */
    if(event_status == FBE_EVENT_STATUS_BUSY)
    {
        /* Our upstream client is busy, we will need to try again later. */
        fbe_transport_set_status(packet_p, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_BUSY;
    }
    else if((event_status != FBE_EVENT_STATUS_OK) && 
            (event_status != FBE_EVENT_STATUS_NO_USER_DATA))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s received bad status(0x%x), can't proceed\n",
                              __FUNCTION__, event_status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Trace under debug.
     */
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO, 
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                    "verify_invalidate_permit.Status:%d checkpoint:0x%llx\n",
                                    event_status, verify_invalidate_checkpoint);

    /* Send the zero request to executor if event flag is not set to deny. 
     */
    if (event_flags & FBE_EVENT_FLAG_DENY)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s received deny status from upstream object, event_flags:0x%x\n",
                              __FUNCTION__, event_flags);

        /* If upstream object deny the zeroing request the complete the packet with error response. */
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if (event_status == FBE_EVENT_STATUS_NO_USER_DATA)
    {
        /* Else if the current verify invalidate checkpoint is below the default
         * offset (this should only occur for the system drive) and there
         * there is no user data, advance the checkpoint.
         */ 
        if (verify_invalidate_checkpoint < default_offset)
        {
            /* System drives can have areas that are below the default_offset.
             */
            if (b_is_system_drive == FBE_TRUE)
            {
                /* Trace a debug message, set the completion function (done above) and 
                 * invoke method to update the checkpoint to the next chunk.
                 */
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_INFO,
                                                FBE_TRACE_MESSAGE_ID_INFO, 
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                                "vr_inv_permit_no_user_data:chk:0x%llx dflt_offset:0x%llx,unconsumed:0x%llx\n",
                                                verify_invalidate_checkpoint, default_offset,
                                                unconsumed_blocks);

                if (unconsumed_blocks == FBE_LBA_INVALID)
                {
                    /* If we find that there is nothing else consumed here,
                     * then advance the checkpoint to the end of the default offset to get beyond the unconsumed area. 
                     * We will continue verify invalidate unbound area beyond the offset.
                     */
                    fbe_provision_drive_metadata_set_verify_invalidate_checkpoint(provision_drive_p, packet_p, default_offset);
                }
                else
                {
                    if(unconsumed_blocks == 0)
                    {
                        /* We should not be getting unconsumed block of Zero*/
                        fbe_provision_drive_utils_trace(provision_drive_p,
                                                        FBE_TRACE_LEVEL_ERROR,
                                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                                        "vr_inv_no_user_data: Unconsumed of 0 unexp. chk:0x%llx offset:0x%llx\n",
                                                        verify_invalidate_checkpoint, default_offset);

                        /* Just move on to the next chunk instead of just getting stuck */
                        unconsumed_blocks += chunk_size;
                        
                    }
                    /* System drive area is below default_offset and not consumed. Advance
                     * checkpoint to next chunk.
                     */
                    fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint(provision_drive_p, 
                                                                                   packet_p, 
                                                                                   verify_invalidate_checkpoint,
                                                                                   unconsumed_blocks);
                }
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
            else
            {
                /* Else the checkpoint should never be set below the 
                 * default_offset.  Log an error and fail the request.
                 */
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "verify invalidate checkpoint: 0x%llx below default_offset: 0x%llx unexpected.\n",
                                      (unsigned long long)verify_invalidate_checkpoint, (unsigned long long)default_offset);
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }

    fbe_provision_drive_check_hook(provision_drive_p,  
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE, 
                                   FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT, 
                                   verify_invalidate_checkpoint, 
                                   &hook_status);

    /* Else proceed with verify invalidate operation.
     */
    status = fbe_provision_drive_send_monitor_packet(provision_drive_p,
                                                     packet_p,
                                                     FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE,
                                                     verify_invalidate_checkpoint,
                                                     FBE_PROVISION_DRIVE_CHUNK_SIZE);

    /* Return the event processing status.
     */
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_utils_ask_verify_invalidate_permission_completion()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_utils_verify_invalidate_check_permission_based_on_current_load()
 ******************************************************************************
 * @brief
 *   This function will check if a verify invalidate I/O is allowed based on the current
 *   I/O load and scheduler credits available for it.
 *
 * @param provision_drive_p           - pointer to the provision drive object.
 * @param is_ok_to_perform           - retuns whether it is ok to perform the
 *                                      operation.
 *
 * @return  fbe_status_t           
 *
 * @author
 *   04/03/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_utils_verify_invalidate_check_permission_based_on_current_load(fbe_provision_drive_t * provision_drive_p,
                                                                                   fbe_bool_t * is_ok_to_perform)
{
    
    fbe_base_config_operation_permission_t  permission_data;
    
    /* Initialize the output parameter. By default, the verify invalidate can not proceed. */
    *is_ok_to_perform = FBE_FALSE;

    /* Set the zero priority in the permission request. */
    permission_data.operation_priority = fbe_provision_drive_get_zero_priority(provision_drive_p);

    /* Set up the CPU and mbytes parameters for the permission request. */
    permission_data.credit_requests.cpu_operations_per_second = fbe_provision_drive_class_get_verify_invalidate_cpu_rate();
    permission_data.credit_requests.mega_bytes_consumption     = 0;
    permission_data.io_credits                                 = 1;

    /* Make the permission request and set the result in the output data. */ 
    fbe_base_config_get_operation_permission((fbe_base_config_t*) provision_drive_p,
                                             &permission_data,
											 FBE_TRUE,
                                             is_ok_to_perform);

	if(*is_ok_to_perform == FBE_FALSE){
		fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
								 (fbe_base_object_t *) provision_drive_p,
								 (fbe_lifecycle_timer_msec_t) 500);
	}
    /* return status. */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_verify_invalidate_check_permission_based_on_current_load()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_verify_update_invalidate_checkpoint
 ******************************************************************************
 *
 * @brief
 *   This function calculates the unconsumed blocks and checks whether it is
 *   a system drive or not and updates the checkpoint.
 *
 * @param   provision_drive_p    
 * @param   packet_p
 * @param   sniff_checkpoint
 * @param   next_consumed_lba
 *
 * @return  fbe_status_t     
 *                           
 *
 * @version
 *    04/26/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_verify_update_invalidate_checkpoint(fbe_provision_drive_t*  provision_drive_p,
                                                   fbe_packet_t*  packet_p,
                                                 fbe_lba_t     invalidate_checkpoint,
                                                 fbe_lba_t     next_consumed_lba,
                                                 fbe_block_count_t   blocks)
{
    fbe_lba_t                   unconsumed_blocks = 0;
    fbe_lba_t                   default_offset = FBE_LBA_INVALID;
    fbe_object_id_t             object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                  b_is_system_drive = FBE_FALSE;


    if ((next_consumed_lba != FBE_LBA_INVALID) && (next_consumed_lba > invalidate_checkpoint))
    {
        unconsumed_blocks = (next_consumed_lba - invalidate_checkpoint);
    }
    else if (next_consumed_lba == FBE_LBA_INVALID)
    {
        unconsumed_blocks = FBE_LBA_INVALID;
    }

    
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);

    /* System drives can have areas that are below the default_offset.
     */
    if (b_is_system_drive == FBE_TRUE)
    {
        /* Trace a debug message, set the completion function (done above) and 
         * invoke method to update the sniff checkpoint to the next chunk.
         */
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                        "VERIFY INVALIDATE: chkpt: 0x%llx < def_off: 0x%llx unconsumed: 0x%llx .\n",
                                        invalidate_checkpoint, default_offset, unconsumed_blocks);

        if (unconsumed_blocks == FBE_LBA_INVALID)
        {
            /* If we find that there is nothing else consumed here,
             * then advance the checkpoint to the end of the default offset to get beyond the unconsumed area. 
             * We will continue sniffing unbound area beyond the offset.
             */
            fbe_provision_drive_metadata_set_verify_invalidate_checkpoint(provision_drive_p, packet_p, default_offset);
            return FBE_STATUS_OK;
        }
        else
        {
            if(unconsumed_blocks == 0)
            {
                /* We should not be getting unconsumed block of Zero*/
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                                "VERIFY INVALIDATE: Unconsumed of 0 unexp. chk:0x%llx offset:0x%llx\n",
                                                invalidate_checkpoint, default_offset
                                                );

                /* Just move on to the next chunk instead of just getting stuck */
                unconsumed_blocks += blocks;
                
            }

            /* System drive area is below default_offset and not consumed. Advance
             * checkpoint to next chunk.
             */
            invalidate_checkpoint += unconsumed_blocks;
            fbe_provision_drive_metadata_set_verify_invalidate_checkpoint(provision_drive_p, packet_p, invalidate_checkpoint);
            return FBE_STATUS_OK;
        }
    }
    else
    {
        /* It is expected we are unconsumed below the checkpoint.
         */
        fbe_provision_drive_metadata_set_verify_invalidate_checkpoint(provision_drive_p, packet_p, default_offset);
        return FBE_STATUS_OK;
    }


    return FBE_STATUS_OK;

}

/*************************************************************
 * end fbe_provision_drive_verify_update_invalidate_checkpoint()
 *************************************************************/

/*!**************************************************************
 * fbe_provision_drive_is_location_valid()
 ****************************************************************
 * @brief
 *  This function checks if the BES information is valid:
 *     BES in nonpaged is valid
 *     BES in metadata memory is valid
 *     BES in nonpaged and metadata memory is matched.
 *
 * @param provision_drive_p - Object.
 *
 * @return True if the location is valid.
 *
 * @author
 *  10/09/2014 - Created. Jamin Kang
 *
 ****************************************************************/

fbe_bool_t fbe_provision_drive_is_location_valid(fbe_provision_drive_t *provision_drive_p)
{

	fbe_status_t status;
    fbe_provision_drive_nonpaged_metadata_drive_info_t np_drive_info;
    fbe_u32_t port_num, encl_num, slot_num;

    status = fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive_p,
                                                                           &np_drive_info);
    if (status != FBE_STATUS_OK) {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s: Failed to get nonpaged information\n",
                                        __FUNCTION__);
        return FBE_FALSE;
    }

    status = fbe_provision_drive_get_drive_location(provision_drive_p, &port_num, &encl_num, &slot_num);
    if (status != FBE_STATUS_OK) {
        return FBE_FALSE;
    }

    if(port_num == FBE_PORT_NUMBER_INVALID || encl_num == FBE_ENCLOSURE_NUMBER_INVALID ||
       slot_num == FBE_SLOT_NUMBER_INVALID) {
        fbe_provision_drive_utils_trace(
            provision_drive_p,
            FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
            "%s: BES in metadata memory is invalid: %u_%u_%u\n",
            __FUNCTION__, port_num, encl_num, slot_num);
        return FBE_FALSE;
    }

    if (np_drive_info.port_number == FBE_PORT_NUMBER_INVALID ||
        np_drive_info.enclosure_number == FBE_ENCLOSURE_NUMBER_INVALID ||
        np_drive_info.slot_number == FBE_SLOT_NUMBER_INVALID ||
        np_drive_info.drive_type == FBE_DRIVE_TYPE_INVALID) {
        fbe_provision_drive_utils_trace(
            provision_drive_p,
            FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
            "%s: BES in nonpaged is invalid: %u_%u_%u, type: %u\n",
            __FUNCTION__,
            np_drive_info.port_number, np_drive_info.enclosure_number, np_drive_info.slot_number,
            np_drive_info.drive_type);
        return FBE_FALSE;
    }

    if (np_drive_info.port_number != port_num ||
        np_drive_info.enclosure_number != encl_num ||
        np_drive_info.slot_number != slot_num) {
        fbe_provision_drive_utils_trace(
            provision_drive_p,
            FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
            "%s: BES is unmatched: np: %u_%u_%u, mm: %u_%u_%u\n",
            __FUNCTION__, np_drive_info.port_number, np_drive_info.enclosure_number, np_drive_info.slot_number,
            port_num, encl_num, slot_num);
        return FBE_FALSE;
    }
    return FBE_TRUE;
}
/******************************************
 * end fbe_provision_drive_is_location_valid()
 ******************************************/


/*******************************
 * end fbe_provision_drive_utils.c
 *******************************/

