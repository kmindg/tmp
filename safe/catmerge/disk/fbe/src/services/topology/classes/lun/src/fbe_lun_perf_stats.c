/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_perf_stats.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the lun object's performance statistics incremental, get
 *  and set functions.
 * 
 * @ingroup lun_class_files
 * 
 * @version
 *   12/14/2010:  Created. Swati Fursule
 *   04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_atomic.h"
#include "fbe_lun_private.h"
#include "fbe/fbe_perfstats_interface.h"

/*****************************************
 * LOCAL FUNCTION FORWARD DECLARATION
 *****************************************/

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_hist_index()
 ******************************************************************************
 * @brief
 *   This function takes as input a number of sectors from 1 to
 *   FBE_HISTOGRAM_OVF_SIZE-1 and returns the correct index in the log array for the
 *   count of reads/writes to be incremented for the input sector count. It is
 *   the responsibility of the caller to test for the overflow size and not call
 *   this routine if the sector count is that or larger
 *
 * @param num -  sector count.
 *
 * @return fbe_u32_t - index
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for new histogram format. Ryan Gadsby
 ******************************************************************************/
fbe_u32_t fbe_lun_perf_stats_get_hist_index(fbe_u32_t num)
{
    fbe_u32_t index;

    for (index = 0; index <= FBE_PERFSTATS_HISTOGRAM_BUCKET_1024_BLOCK_PLUS_OVERFLOW; index++)
    {
        if ((num & (0x1 << index)) != 0)
        {
            return (index);
        }
    }
    return FBE_PERFSTATS_HISTOGRAM_BUCKET_1024_BLOCK_PLUS_OVERFLOW;
}


/*!****************************************************************************
 * fbe_lun_perf_stats_update_perf_counters_io_completion()
 ******************************************************************************
 * @brief
 *  This function is used to update perfomance counters after io completion
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param block_operation_p     -   Pointer to the block operation .
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_update_perf_counters_io_completion(fbe_lun_t * lun_p,
                                                      fbe_payload_block_operation_t *block_operation_p,
                                                      fbe_cpu_id_t cpu_id,
                                                      fbe_payload_ex_t *payload_p)
{
    fbe_u32_t index = 0;
    if (lun_p->b_perf_stats_enabled && lun_p->performance_stats.counter_ptr.lun_counters)
    {
        fbe_u64_t elapsed_time = 0;
        /* Update Cum_Read_Response_Time, Cum_Write_Response_Time
         * Update LUN Blocks read and written along with histogram
         * Also update number of blocks read or written.
         */
        /*how long IO was continued?*/
        if (payload_p->start_time != 0)
        {
            elapsed_time = fbe_get_elapsed_milliseconds(payload_p->start_time);
        }
        
        lun_p->performance_stats.counter_ptr.lun_counters->timestamp = fbe_get_time();

        if(block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
        {
            if (elapsed_time != 0)
            {
                fbe_lun_perf_stats_inc_cum_read_time(lun_p, elapsed_time + 1, cpu_id);
            }
            fbe_lun_perf_stats_inc_read_requests(lun_p, cpu_id);
            fbe_lun_perf_stats_inc_blocks_read(lun_p, (fbe_u64_t)block_operation_p->block_count, cpu_id);
           
            index = fbe_lun_perf_stats_get_hist_index((fbe_u32_t)block_operation_p->block_count);
            fbe_lun_perf_stats_inc_read_histogram(lun_p, index, cpu_id);       
        }
        else if(block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE ||
                block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED)
        {
            if (elapsed_time != 0)
            {
                fbe_lun_perf_stats_inc_cum_write_time(lun_p, elapsed_time + 1, cpu_id);
            }
            fbe_lun_perf_stats_inc_write_requests(lun_p, cpu_id);
            fbe_lun_perf_stats_inc_blocks_written(lun_p, (fbe_u64_t)block_operation_p->block_count, cpu_id);

            index = fbe_lun_perf_stats_get_hist_index((fbe_u32_t)block_operation_p->block_count);
            fbe_lun_perf_stats_inc_write_histogram(lun_p, index, cpu_id);            
        }
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_update_perf_counters_io_completion()
 ******************************************************************************/



/*!****************************************************************************
 * fbe_lun_perf_stats_inc_cum_read_time()
 ******************************************************************************
 * @brief
 *  This function is used to increments the cum_read_time on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param elapsed_time     -   elapsed time for this read
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_cum_read_time(fbe_lun_t * lun_p,
                                     fbe_u64_t elapsed_time,
                                     fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        lun_p->performance_stats.counter_ptr.lun_counters->cumulative_read_response_time[cpu_id] += elapsed_time;
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_cum_read_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_cum_read_time()
 ******************************************************************************
 * @brief
 *  This function is used to get the cum_read_time on the lun.
 *
 * @param lun_p        -   Pointer to the lun object.
 * @param cum_read_time_p  -   Pointer to the cum_read_time.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_cum_read_time(fbe_lun_t * lun_p, 
                                     fbe_u64_t *cum_read_time_p,
                                     fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        *cum_read_time_p = lun_p->performance_stats.counter_ptr.lun_counters->cumulative_read_response_time[cpu_id];
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_cum_read_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_inc_cum_write_time()
 ******************************************************************************
 * @brief
 *  This function is used to increments the cum_write_time on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_cum_write_time(fbe_lun_t * lun_p,
                                      fbe_u64_t elapsed_time,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        lun_p->performance_stats.counter_ptr.lun_counters->cumulative_write_response_time[cpu_id] += elapsed_time;
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_cum_write_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_cum_write_time()
 ******************************************************************************
 * @brief
 *  This function is used to get the cum_write_time on the lun or unit
 *
 * @param lun_p        -   Pointer to the lun object.
 * @param cum_write_time_p  -   Pointer to the cum_write_time.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_cum_write_time(fbe_lun_t * lun_p,
                                      fbe_u64_t *cum_write_time_p,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        *cum_write_time_p = lun_p->performance_stats.counter_ptr.lun_counters->cumulative_write_response_time[cpu_id];
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_cum_write_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_inc_blocks_read()
 ******************************************************************************
 * @brief
 *  This function is used to increments the blocks_read on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param blocks -       Blocks to increment by.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_blocks_read(fbe_lun_t * lun_p,
                                   fbe_u64_t blocks,
                                   fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
         lun_p->performance_stats.counter_ptr.lun_counters->lun_blocks_read[cpu_id] += blocks;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_blocks_read()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_blocks_read()
 ******************************************************************************
 * @brief
 *  This function is used to get the blocks_read on the lun or unit
 *
 * @param lun_p        -   Pointer to the lun object.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_blocks_read(fbe_lun_t * lun_p,
                                   fbe_u64_t *blocks_read_p,
                                   fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        *blocks_read_p = lun_p->performance_stats.counter_ptr.lun_counters->lun_blocks_read[cpu_id];
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_blocks_read()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_inc_blocks_written()
 ******************************************************************************
 * @brief
 *  This function is used to increments the blocks_written on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param blocks -       Blocks to increment by.
*
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_blocks_written(fbe_lun_t * lun_p,
                                      fbe_u64_t blocks,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
         lun_p->performance_stats.counter_ptr.lun_counters->lun_blocks_written[cpu_id] += blocks;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_blocks_written()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_blocks_written()
 ******************************************************************************
 * @brief
 *  This function is used to get the blocks_written on the lun.
 *
 * @param lun_p        -   Pointer to the lun object.
 * @param blocks_written_p  -   Pointer to the blocks_written.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_blocks_written(fbe_lun_t * lun_p, 
                                      fbe_u64_t *blocks_written_p,
                                      fbe_cpu_id_t cpu_id)
{

    if (lun_p->b_perf_stats_enabled)
    {
        *blocks_written_p = lun_p->performance_stats.counter_ptr.lun_counters->lun_blocks_written[cpu_id];
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_blocks_written()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_inc_read_requests()
 ******************************************************************************
 * @brief
 *  This function is used to increments the read_requests on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param cpu_id    - CPU ID of the core the IO completed on
*
 * @return fbe_status_t
 *
 * @author
 * 7/6/2012 - Added support for new counter. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_read_requests(fbe_lun_t * lun_p,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
         lun_p->performance_stats.counter_ptr.lun_counters->lun_read_requests[cpu_id]++;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_read_requests()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_read_requests()
 ******************************************************************************
 * @brief
 *  This function is used to get the read_requests on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param read_requests_p - total number of reads (return)
 * @param cpu_id    - CPU ID of the core the IO completed on
 *
 * @return fbe_status_t
 *
 * @author
 * 7/6/2012 - Added support for new counter. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_read_requests(fbe_lun_t * lun_p,
                                      fbe_u64_t * read_requests_p,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
         *read_requests_p = lun_p->performance_stats.counter_ptr.lun_counters->lun_read_requests[cpu_id];
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_read_requests()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_lun_perf_stats_inc_write_requests()
 ******************************************************************************
 * @brief
 *  This function is used to increments the write_requests on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param cpu_id    - CPU ID of the core the IO completed on
*
 * @return fbe_status_t
 *
 * @author
 * 7/6/2012 - Added support for new counter. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_write_requests(fbe_lun_t * lun_p,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
         lun_p->performance_stats.counter_ptr.lun_counters->lun_write_requests[cpu_id]++;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_write_requests()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_write_requests()
 ******************************************************************************
 * @brief
 *  This function is used to get the write_requests on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param write_requests_p - total number of writes(return)
 * @param cpu_id    - CPU ID of the core the IO completed on
 *
 * @return fbe_status_t
 *
 * @author
 * 7/6/2012 - Added support for new counter. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_write_requests(fbe_lun_t * lun_p,
                                      fbe_u64_t * write_requests_p,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
         *write_requests_p = lun_p->performance_stats.counter_ptr.lun_counters->lun_write_requests[cpu_id];
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_write_requests()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_inc_arrivals_to_nonzero_q()
 ******************************************************************************
 * @brief
 *  This function is used to increments the arrivals_to_nonzero_q on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_arrivals_to_nonzero_q(fbe_lun_t * lun_p,
                                             fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
       lun_p->performance_stats.counter_ptr.lun_counters->non_zero_queue_arrivals[cpu_id]++;
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_cum_read_time()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_arrivals_to_nonzero_q()
 ******************************************************************************
 * @brief
 *  This function is used to get the arrivals_to_nonzero_q on the lun.
 *
 * @param lun_p        -   Pointer to the lun object.
 * @param arrivals_to_nonzero_q_p  -   Pointer to the arrivals_to_nonzero_q.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_arrivals_to_nonzero_q(fbe_lun_t * lun_p,
                                             fbe_u64_t *arrivals_to_nonzero_q_p,
                                             fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        *arrivals_to_nonzero_q_p = lun_p->performance_stats.counter_ptr.lun_counters->non_zero_queue_arrivals[cpu_id];
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_arrivals_to_nonzero_q()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_inc_sum_q_length_arrival()
 ******************************************************************************
 * @brief
 *  This function is used to increments the sum_q_length_arrival on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param blocks -       Blocks to increment by.
*
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_sum_q_length_arrival(fbe_lun_t * lun_p,
                                            fbe_u64_t blocks,
                                            fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        lun_p->performance_stats.counter_ptr.lun_counters->sum_arrival_queue_length[cpu_id] += blocks;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_sum_q_length_arrival()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_sum_q_length_arrival()
 ******************************************************************************
 * @brief
 *  This function is used to get the sum_q_length_arrival on the lun.
 *
 * @param lun_p        -   Pointer to the lun object.
 * @param sum_q_length_arrival_p  -   Pointer to the sum_q_length_arrival.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_sum_q_length_arrival(fbe_lun_t * lun_p,
                                            fbe_u64_t *sum_q_length_arrival_p,
                                            fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        *sum_q_length_arrival_p = lun_p->performance_stats.counter_ptr.lun_counters->sum_arrival_queue_length[cpu_id];
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_sum_q_length_arrival()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_inc_read_histogram()
 ******************************************************************************
 * @brief
 *  This function is used to increments the read_histogram on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param hist_index  -   Histogram index
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_read_histogram(fbe_lun_t * lun_p,
                                      fbe_u32_t hist_index,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        lun_p->performance_stats.counter_ptr.lun_counters->lun_io_size_read_histogram[hist_index][cpu_id]++;
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_read_histogram()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_read_histogram()
 ******************************************************************************
 * @brief
 *  This function is used to get the read_histogram on the lun.
 *
 * @param lun_p        -   Pointer to the lun object.
 * @param hist_index  -   Histogram index 
 * @param read_histogram_p  -   Pointer to the read_histogram.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_read_histogram(fbe_lun_t * lun_p, 
                                      fbe_u32_t hist_index,
                                      fbe_u64_t *read_histogram_p,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        *read_histogram_p = lun_p->performance_stats.counter_ptr.lun_counters->lun_io_size_read_histogram[hist_index][cpu_id];
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_read_histogram()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_inc_write_histogram()
 ******************************************************************************
 * @brief
 *  This function is used to increments the write_histogram on the lun
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param hist_index  -   Histogram index
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_inc_write_histogram(fbe_lun_t * lun_p,
                                      fbe_u32_t hist_index,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        lun_p->performance_stats.counter_ptr.lun_counters->lun_io_size_write_histogram[hist_index][cpu_id]++;
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_inc_write_histogram()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_perf_stats_get_write_histogram()
 ******************************************************************************
 * @brief
 *  This function is used to get the write_histogram on the lun.
 *
 * @param lun_p        -   Pointer to the lun object.
 * @param hist_index  -   Histogram index 
 * @param write_histogram_p  -   Pointer to the write_histogram.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/19/2012:  Modified for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_lun_perf_stats_get_write_histogram(fbe_lun_t * lun_p, 
                                      fbe_u32_t hist_index,
                                      fbe_u64_t *write_histogram_p,
                                      fbe_cpu_id_t cpu_id)
{
    if (lun_p->b_perf_stats_enabled)
    {
        *write_histogram_p = lun_p->performance_stats.counter_ptr.lun_counters->lun_io_size_write_histogram[hist_index][cpu_id];
    }
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_perf_stats_get_write_histogram()
 ******************************************************************************/

/*******************************
 * end fbe_lun_perf_stats.c
 *******************************/
