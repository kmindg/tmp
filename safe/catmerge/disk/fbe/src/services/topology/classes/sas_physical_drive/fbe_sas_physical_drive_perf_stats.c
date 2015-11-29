/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sas_physical_drive_perf_stats.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the SAS physical drive object's (PDO's) performance
 *  statistics increment and get functions.
 * 
 * @ingroup sas_physical_drive_class_files
 * 
 * @version
 *   05/07/2012:  Created. Darren Insko
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_atomic.h"
#include "sas_physical_drive_private.h"

/*****************************************
 * LOCAL FUNCTION FORWARD DECLARATION
 *****************************************/

fbe_u32_t
fbe_sas_physical_drive_perf_stats_get_srv_time_hist_index(fbe_u64_t delta_us_time);

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
#define MILLISECONDS_PER_TICK    100 /* refers to a "system tick" (100ms), not a HW timer tick */
#define BIT(b)  (1 << (b))

/*!****************************************************************************
 * fbe_sas_physical_drive_perf_stats_get_srv_time_hist_index()
 ******************************************************************************
 * @brief
 *  This function takes as input the begin and end time
 *  and returns the correct index of the histogram array for the service time interval count to be incremented.
 *
 * @param delta_us_time - Difference in time in microseconds
 *
 * @return fbe_u32_t - index
 *
 * @author
 * 09/22/2012:  Created. Darren Insko
 *
 ******************************************************************************/
fbe_u32_t fbe_sas_physical_drive_perf_stats_get_srv_time_hist_index(fbe_u64_t delta_us_time)
{
    fbe_u32_t index = 0;
    fbe_u32_t delta = (fbe_u32_t)(delta_us_time/FBE_PERFSTATS_SRV_TIME_HISTOGRAM_MIN_TIME); //scale to 100us buckets

/****************************************************************************************************/
/***** NOTE: Numerical "BIT" position valudes are currently used in this function.  However, I  *****/
/*****       could replace them with defined enumeration values in fbe_perfstats_interface.h    *****/
/*****       such as FBE_PERFSTATS_SRV_TIME_HISTOGRAM_BUCKET_GTE_{0,100,200,400,800,etc.}_US.   *****/
/*****       Using non-numerical literals is normally a perferred coding practice, but this     *****/
/*****       function may be more difficult to understand than when using the numerical "BIT"   *****/
/*****       position values.  Please provide feedback on your preference in the code review.   *****/
/*****                                                                                          *****/
/***** ALSE NOTE: This algorithm produces buckets that contain slightly different numbers than  *****/
/*****            what was originally mentioned in Gerry's proposal document.                   *****/
/*****            For example: bucket 0 will contain 0 - 99 instead of 0 - 100                  *****/
/*****                         bucket 1 will contain 100 - 199 instead of 101 - 200             *****/
/*****                         :                                                                *****/
/*****                         bucket 19 will contain 26,214,400us instead of > 26,214,400us    *****/
/*****            However, this change in bucket contents simplifies the algorithm, because     *****/
/*****            the remainder of the original delta_us_time/100 doesn't have to be accounted  *****/
/*****            for in deciding what bucket a number should be placed.                        *****/
/****************************************************************************************************/
    if (delta >= BIT(9)) //between 10 and 19
    {
        index += 10;
        if (delta >= BIT(14)) //between 15 and 19
        {
            index += 5;
            if (delta >= BIT(16)) //between 17 and 19
            {
                index += 2 + ((delta >= BIT(18)) ? 2 : /* is 19 */
                    ((delta >= BIT(17)) ? 1 /* is 18 */ : 0 /* is 17 */));
            }
            else //between 15 and 16
            {
                index += ((delta >= BIT(15)) ? 1 /* 16 */ : 0 /* is 15 */);
            }
        }
        else //between 10 and 14
        {
            if (delta >= BIT(11)) //between 12 and 14
            {
                index += 2 + ((delta >= BIT(13)) ? 2 : /* is 14 */
                    ((delta >= BIT(12)) ? 1 /* is 13 */ : 0 /* is 12 */));
            }
            else //between 10 and 11
            {
                index += ((delta >= BIT(10)) ? 1 /* is 11 */ : 0 /* is 10 */);
            }
        }
    }
    else //between 0 and 9
    {
        if (delta >= BIT(4)) //between 5 and 9
        {
            index += 5;
            if (delta >= BIT(6)) //between 7 and 9
            {
                index += 2 + ((delta >= BIT(8)) ? 2 : /* is 9 */
                    ((delta >= BIT(7)) ? 1 /* is 8 */ : 0 /* is 7 */));
            }
            else //between 5 and 6
            {
                index += ((delta >= BIT(5)) ? 1 /* is 6 */ : 0 /* is 5 */);
            }
        }
        else //between 0 and 4
        {
            if (delta >= BIT(1)) //between 2 and 4
            {
                index += 2 + ((delta >= BIT(3)) ? 2 : /* is 4 */
                    ((delta >= BIT(2)) ? 1 /* is 3 */ : 0 /* is 2 */));
            }
            else //between 0 and 1
            {
                index += ((delta >= BIT(0)) ? 1 /* is 1 */ : 0 /* is 0 */);
            }
        }
    }

    return index;
}
/******************************************************************
 * end fbe_sas_physical_drive_perf_stats_get_srv_time_hist_index()
 ******************************************************************/

/*!****************************************************************************
 * fbe_sas_physical_drive_perf_stats_update_counters_io_completion()
 ******************************************************************************
 * @brief
 *  This function updates the PDO's per-core perfomance counters upon io
 *  completion.
 *
 * @param sas_physical_drive_p - Pointer to the PDO.
 * @param block_operation_p -   Pointer to the block operation.
 * @param cpu_id - Core (CPU) identifier.
 *
 * @return fbe_status_t
 *
 * @author
 * 05/07/2012:  Created. Darren Insko
 * 11/07/2013:  Check for NULL pointer to pdo_counters and collapse some single
 *              line functions that are not called anywhere else.  Darren Insko
 *
 ******************************************************************************/
fbe_status_t
fbe_sas_physical_drive_perf_stats_update_counters_io_completion(fbe_sas_physical_drive_t * sas_physical_drive_p,
                                                                fbe_payload_block_operation_t *block_operation_p,
                                                                fbe_cpu_id_t cpu_id,
                                                                const fbe_payload_cdb_operation_t * payload_cdb_operation)
{
    fbe_pdo_performance_counters_t * temp_pdo_counters = NULL;
    fbe_bool_t b_read = FBE_FALSE;
    fbe_bool_t b_write = FBE_FALSE;
    fbe_lba_t  temp_lba;
    INT_32     seekdiff = 0;
    fbe_u32_t  index = 0;

    /* Save a local copy of the pointer to the pdo_counters in case it's set to NULL before we access it */
    temp_pdo_counters = sas_physical_drive_p->performance_stats.counter_ptr.pdo_counters;
    if (temp_pdo_counters)
    {
        /* Update PDO Blocks read or written. */    
        if ((block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) ||
            (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
            (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WITH_BUFFER))
        {
            b_read = FBE_TRUE;
            /* Increment the PDO's per-core disk_blocks_read counter. */
            temp_pdo_counters->disk_blocks_read[cpu_id] += (fbe_u32_t)block_operation_p->block_count;
            /* Increment the PDO's per-core disk_reads counter. */
            temp_pdo_counters->disk_reads[cpu_id]++;
        }
        else if ((block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
                 (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY))
        {
            b_write = FBE_TRUE;
            /*  Increment the PDO's per-core disk_blocks_written counter. */
            temp_pdo_counters->disk_blocks_written[cpu_id] += (fbe_u32_t)block_operation_p->block_count;
            /*  Increment the PDO's per-core disk_writes counter. */
            temp_pdo_counters->disk_writes[cpu_id]++;
        }

        if (b_read || b_write)
        {
            temp_lba = block_operation_p->lba;
            seekdiff = (UINT_32)(sas_physical_drive_p->base_physical_drive.prev_start_lba - temp_lba);
            /* We want a difference here, so flip the sign if */
            /* the above math has given us a negative number. */
            if (seekdiff < 0)
            {
                seekdiff = -seekdiff;
            }
            /* Save the current start LBA for the next time we're here */
            sas_physical_drive_p->base_physical_drive.prev_start_lba = temp_lba;
            /*  Increment the PDO's per-core sum_blocks_seeked counter. */
            temp_pdo_counters->sum_blocks_seeked[cpu_id] += (fbe_u32_t)seekdiff;
        }

        index = fbe_sas_physical_drive_perf_stats_get_srv_time_hist_index(payload_cdb_operation->service_end_time_us -
                                                                          payload_cdb_operation->service_start_time_us);
        /* Increment the PDO's per-core disk_srv_time_histogram counter. */
        temp_pdo_counters->disk_srv_time_histogram[cpu_id][index]++;
    }
    
    return FBE_STATUS_OK;
}
/***********************************************************************
 * end fbe_sas_physical_drive_perf_stats_update_counters_io_completion()
 ***********************************************************************/

/*!****************************************************************************
 * fbe_sas_physical_drive_perf_stats_inc_num_io_in_progress()
 ******************************************************************************
 * @brief
 *  This function increments the PDO's number of IOs in progress and starts the
 *  busy time if the drive is idle.
 *
 * @param sas_physical_drive_p - Pointer to the PDO.
 *
 * @return None
 *
 * @author
 * 02/20/2013:  Created. Darren Insko
 * 11/07/2013:  Check for NULL pointer to pdo_counters and collapse some single
 *              line functions that are not called anywhere else.  Darren Insko
 *
 ******************************************************************************/
void
fbe_sas_physical_drive_perf_stats_inc_num_io_in_progress(fbe_sas_physical_drive_t * sas_physical_drive_p,
                                                         fbe_cpu_id_t cpu_id)
{
    fbe_atomic_t io_count;
    fbe_base_physical_drive_t * base_physical_drive_p = (fbe_base_physical_drive_t *)sas_physical_drive_p;
    fbe_pdo_performance_counters_t * temp_pdo_counters = NULL;
    fbe_time_t current_time;
    fbe_u64_t idle_ticks;

    io_count = fbe_atomic_increment(&base_physical_drive_p->stats_num_io_in_progress);       

    /* Save a local copy of the pointer to the pdo_counters in case it's set to NULL before we access it */
    temp_pdo_counters = sas_physical_drive_p->performance_stats.counter_ptr.pdo_counters;
    if (temp_pdo_counters)
    {
        /* Increment the PDO's per-core sum of arrival queue lengths */
        temp_pdo_counters->sum_arrival_queue_length[cpu_id] += io_count;

        /* Number of outstanding_IOs present in block transport does not include this io */
        if (io_count > 1)
        {
            /* Increment the PDO's per-core number of arrivals that occur with a nonzero queue */
            temp_pdo_counters->non_zero_queue_arrivals[cpu_id]++;
        }

        if (io_count == 1) // Idle->Busy
        {
            current_time = fbe_get_time_in_us();        
 
            /* Calculate idle ticks, 1 tick is 1 microsecond. */
            idle_ticks = (current_time - base_physical_drive_p->start_idle_timestamp);        
 
            temp_pdo_counters->timestamp = current_time;
            temp_pdo_counters->idle_ticks[0] += idle_ticks;
            base_physical_drive_p->start_busy_timestamp = current_time;

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive_p, 
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                "PERFSTAT_INC_IO curTime - startITime(%llu) is %llu, totalITicks(%llu)\n",
                                base_physical_drive_p->start_idle_timestamp,
                                idle_ticks, 
                                temp_pdo_counters->idle_ticks[0]);
        }
    }

    return;
}
/******************************************************************************
 * end fbe_sas_physical_drive_perf_stats_inc_num_io_in_progress()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_sas_physical_drive_perf_stats_dec_num_io_in_progress()
 ******************************************************************************
 * @brief
 *  This function decrements the PDO's number of IOs in progress and stops the
 *  busy time if the drive is idle.
 *
 * @param sas_physical_drive_p - Pointer to the PDO.
 *
 * @return None
 *
 * @author
 * 02/20/2013:  Created. Darren Insko
 * 06/24/2013:  Armando Vega - Added calculation of busy ticks.
 * 11/07/2013:  Check for NULL pointer to pdo_counters.  Darren Insko
 *
 ******************************************************************************/
void
fbe_sas_physical_drive_perf_stats_dec_num_io_in_progress(fbe_sas_physical_drive_t * sas_physical_drive_p)
{
    fbe_atomic_t io_count;
    fbe_pdo_performance_counters_t * temp_pdo_counters = NULL;
    fbe_time_t current_time;
    fbe_u64_t busy_ticks;
    fbe_base_physical_drive_t * base_physical_drive_p = (fbe_base_physical_drive_t *)sas_physical_drive_p;
    
    io_count = fbe_atomic_decrement(&base_physical_drive_p->stats_num_io_in_progress);      

    /* Save a local copy of the pointer to the pdo_counters in case it's set to NULL before we access it */
    temp_pdo_counters = sas_physical_drive_p->performance_stats.counter_ptr.pdo_counters;
    if (temp_pdo_counters)
    {
        if (io_count == 0) // Busy->Idle
        {           
            current_time = fbe_get_time_in_us();
            busy_ticks = (current_time - base_physical_drive_p->start_busy_timestamp);                    

            temp_pdo_counters->timestamp = current_time;
            temp_pdo_counters->busy_ticks[0] += busy_ticks;
            base_physical_drive_p->start_idle_timestamp = current_time;

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive_p, 
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                "PERFSTAT_DEC_IO curTime - startBTime(%llu) is %llu, totalBTicks: %llu\n",
                                base_physical_drive_p->start_busy_timestamp,
                                busy_ticks,
                                temp_pdo_counters->busy_ticks[0]);
        }
        else if (io_count > 0x7FFFFFFFFFFFFFFFULL) //higher than this value is considered negative
        {
            /* To safeguard against case where multiple threads could have outstanding IOs 
               upon perftstas being enabled. These IOs could trigger a decrement and set the
               count erroneously. Increment value back.
             */
            fbe_atomic_increment(&base_physical_drive_p->stats_num_io_in_progress);   

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive_p, 
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                "PERFSTAT_DEC_IO Bad IO count, disregard for analysis: %llu\n",
                                io_count);
        }
    }

    return;
}
/******************************************************************************
 * end fbe_sas_physical_drive_perf_stats_dec_num_io_in_progress()
 ******************************************************************************/

/******************************************
 * end fbe_sas_physical_drive_perf_stats.c
 ******************************************/
