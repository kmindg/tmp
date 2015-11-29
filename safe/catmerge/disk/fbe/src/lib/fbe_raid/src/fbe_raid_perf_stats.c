/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_perf_stats.c
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
 *    4/27/2012:  Updated for compatibility with the FBE Perfstats service. Ryan Gadsby
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_perfstats_interface.h"

/*****************************************
 * LOCAL FUNCTION FORWARD DECLARATION
 *****************************************/

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/*!****************************************************************************
 * fbe_raid_perf_stats_inc_mr3_write()
 ******************************************************************************
 * @brief
 *  This function is used to increments the mr3_writes on the lun or unit
 *
 * @param lun_p     -   Pointer to the lun object.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_inc_mr3_write(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                  fbe_cpu_id_t cpu_id)
{
    lun_perf_stats_p->stripe_writes[cpu_id] += 1;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_inc_mr3_write()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_perf_stats_get_mr3_write()
 ******************************************************************************
 * @brief
 *  This function is used to get the mr3_writes on the lun or unit
 *
 * @param lun_perf_stats_p -   Pointer to the lun perf stats object.
 * @param mr3_write_p  -   Pointer to the mr3_write.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_get_mr3_write(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                  fbe_u64_t *mr3_write_p,
                                  fbe_cpu_id_t cpu_id)
{
    *mr3_write_p = lun_perf_stats_p->stripe_writes[cpu_id];
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_get_mr3_write()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_raid_perf_stats_inc_stripe_crossings()
 ******************************************************************************
 * @brief
 *  This function is used to increments the stripe crossings on the Lun or unit
 *
 * @param lun_p     -   Pointer to the lun object.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_inc_stripe_crossings(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                         fbe_cpu_id_t cpu_id)
{
    lun_perf_stats_p->stripe_crossings[cpu_id] += 1;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_inc_stripe_crossings()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_perf_stats_get_stripe_crossings()
 ******************************************************************************
 * @brief
 *  This function is used to get the stripe crossings on the lun or unit
 *
 * @param lun_perf_stats_p    -   Pointer to the lun performanc stats object.
 * @param stripe_crossings_p  -   Pointer to the stripe crossings.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_get_stripe_crossings(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                         fbe_u64_t *stripe_crossings_p,
                                         fbe_cpu_id_t cpu_id)
{
    *stripe_crossings_p = lun_perf_stats_p->stripe_crossings[cpu_id];
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_get_stripe_crossings()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_perf_stats_inc_disk_reads()
 ******************************************************************************
 * @brief
 *  This function is used to increments the disk read on the lun for given position
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param position  -    Position on the lun to increment.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_inc_disk_reads(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                   fbe_u32_t position,
                                   fbe_cpu_id_t cpu_id)
{
    lun_perf_stats_p->disk_reads[position][cpu_id] += 1;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_inc_disk_reads()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_perf_stats_get_disk_reads()
 ******************************************************************************
 * @brief
 *  This function is used to get the disk reads on the lun for given position
 *
 * @param lun_perf_stats_p -   Pointer to the lun perf stats object.
 * @param position  -    Position on the lun to increment.
 * @param disk_reads  -   Pointer to the disk_reads.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_get_disk_reads(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                   fbe_u32_t position,
                                   fbe_u64_t *disk_reads_p,
                                   fbe_cpu_id_t cpu_id)
{
    *disk_reads_p = lun_perf_stats_p->disk_reads[position][cpu_id];
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_get_disk_reads()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_perf_stats_inc_disk_writes()
 ******************************************************************************
 * @brief
 *  This function is used to increments the disk writes on the lun for given position
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param position  -    Position on the lun to increment.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_inc_disk_writes(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                    fbe_u32_t position,
                                    fbe_cpu_id_t cpu_id)
{
    lun_perf_stats_p->disk_writes[position][cpu_id] += 1;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_inc_disk_writes()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_perf_stats_get_disk_writes()
 ******************************************************************************
 * @brief
 *  This function is used to get the disk writes on the lun for given position
 *
 * @param lun_perf_stats_p -   Pointer to the lun perf stats object.
 * @param position  -    Position on the lun to increment.
 * @param disk_writes_p  -   Pointer to the disk writes
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_get_disk_writes(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                   fbe_u32_t position,
                                   fbe_u64_t *disk_writes_p,
                                   fbe_cpu_id_t cpu_id)
{
    *disk_writes_p = lun_perf_stats_p->disk_writes[position][cpu_id];
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_get_disk_reads()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_perf_stats_inc_disk_blocks_read()
 ******************************************************************************
 * @brief
 *  This function is used to increments the disk blocks read on the lun for given position
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param position  -    Position on the lun to increment.
 * @param blocks -       Blocks to increment by.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_inc_disk_blocks_read(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                         fbe_u32_t position,
                                         fbe_block_count_t blocks,
                                         fbe_cpu_id_t cpu_id)
{
    lun_perf_stats_p->disk_blocks_read[position][cpu_id] += blocks;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_inc_disk_blocks_read()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_perf_stats_get_disk_blocks_read()
 ******************************************************************************
 * @brief
 *  This function is used to get the disk blocks read on the lun for given position
 *
 * @param lun_perf_stats_p -   Pointer to the lun perf stats object.
 * @param position  -    Position on the lun to increment.
 * @param disk_blocks_read  -   Pointer to the disk_blocks_read_p.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_get_disk_blocks_read(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                         fbe_u32_t position,
                                         fbe_u64_t *disk_blocks_read_p,
                                         fbe_cpu_id_t cpu_id)
{
    *disk_blocks_read_p = lun_perf_stats_p->disk_blocks_read[position][cpu_id];
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_get_disk_blocks_read()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_perf_stats_inc_disk_blocks_written()
 ******************************************************************************
 * @brief
 *  This function is used to increments the disk block written on the lun for given position
 *
 * @param lun_p     -   Pointer to the lun object.
 * @param position  -    Position on the lun to increment.
 * @param blocks -       Blocks to increment by.
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_inc_disk_blocks_written(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                            fbe_u32_t position,
                                            fbe_block_count_t blocks,
                                            fbe_cpu_id_t cpu_id)
{
    lun_perf_stats_p->disk_blocks_written[position][cpu_id] += blocks;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_inc_disk_blocks_written()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_perf_stats_get_disk_blocks_written()
 ******************************************************************************
 * @brief
 *  This function is used to get the disk block written on the lun for given position
 *
 * @param lun_perf_stats_p -   Pointer to the lun perf stats object.
 * @param position  -          Position on the lun to increment.
 * @param disk_blocks_written_p-Pointer to the disk block written
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_get_disk_blocks_written(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                            fbe_u32_t position,
                                            fbe_u64_t *disk_blocks_written_p,
                                            fbe_cpu_id_t cpu_id)
{
    *disk_blocks_written_p = lun_perf_stats_p->disk_blocks_written[position][cpu_id];
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_get_disk_blocks_written()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_perf_stats_set_timestamp()
 ******************************************************************************
 * @brief
 *  This function is used to sets the timestamp on the lun or unit
 *
 * @param lun_p     -   Pointer to the lun object.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_set_timestamp(fbe_lun_performance_counters_t *lun_perf_stats_p)
{
    lun_perf_stats_p->timestamp = fbe_get_time();
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_set_timestamp()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_perf_stats_get_timestamp()
 ******************************************************************************
 * @brief
 *  This function is used to get the timestamp on the lun or unit
 *
 * @param lun_perf_stats_p -   Pointer to the lun perf stats object.
 * @param timestamp_p  -   Pointer to the mr3_write.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 * 04/27/2012:  Updated for per-core counter support. Ryan Gadsby
 ******************************************************************************/
fbe_status_t
fbe_raid_perf_stats_get_timestamp(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                  fbe_time_t *timestamp_p)
{
    *timestamp_p = lun_perf_stats_p->timestamp;
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_perf_stats_get_timestamp()
 ******************************************************************************/

/*******************************
 * end fbe_raid_perf_stats.c
 *******************************/
