/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_bvd_interface_perf_stats.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the BVD object's performance statistics incremental, get
 *  and set functions.
 * 
 * @ingroup bvd_class_files
 * 
 * @version
 *   12/14/2010:  Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_sep_shim.h"
#include "fbe_bvd_interface_private.h"

/*****************************************
 * LOCAL FUNCTION FORWARD DECLARATION
 *****************************************/

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/*!****************************************************************************
 * fbe_bvd_interface_perf_stats_update_perf_counters()
 ******************************************************************************
 * @brief
 *  This function is used to update performance counters from sep shim
 *
 * @param bvd_interface_p     -   Pointer to the BVD object.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 *
 ******************************************************************************/
fbe_status_t
fbe_bvd_interface_perf_stats_update_perf_counters(fbe_bvd_interface_t * bvd_interface_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_sep_shim_perf_stat_t                    perf_stats;
    status = fbe_sep_shim_get_perf_stat(&perf_stats);
    /* copy the statistics data in bvd interface structure*/
    bvd_interface_p->sp_performace_stats.arrivals_to_nonzero_q = perf_stats.arrivals_to_nonzero_q;
    bvd_interface_p->sp_performace_stats.sum_q_length_arrival = perf_stats.sum_q_length_arrival;
    bvd_interface_p->sp_performace_stats.timestamp = perf_stats.timestamp;
    return status;
}
/******************************************************************************
 * end fbe_bvd_interface_perf_stats_update_perf_counters()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_bvd_interface_perf_stats_inc_arrivals_to_nonzero_q()
 ******************************************************************************
 * @brief
 *  This function is used to increments the arrivals_to_nonzero_q on the lun
 *
 * @param bvd_interface_p     -   Pointer to the BVD object.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 *
 ******************************************************************************/
fbe_status_t
fbe_bvd_interface_perf_stats_inc_arrivals_to_nonzero_q(fbe_bvd_interface_t * bvd_interface_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    if (bvd_interface_p->b_perf_stats_enabled)
    {
        fbe_atomic_32_increment(&(bvd_interface_p->sp_performace_stats.arrivals_to_nonzero_q));
    }
     return status;
}
/******************************************************************************
 * end fbe_bvd_interface_perf_stats_inc_arrivals_to_nonzero_q()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_bvd_interface_perf_stats_get_arrivals_to_nonzero_q()
 ******************************************************************************
 * @brief
 *  This function is used to get the arrivals_to_nonzero_q on the lun.
 *
 * @param lun_p        -   Pointer to the lun object.
 * @param arrivals_to_nonzero_q  -   Pointer to the arrivals_to_nonzero_q.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 *
 ******************************************************************************/
fbe_status_t
fbe_bvd_interface_perf_stats_get_arrivals_to_nonzero_q(fbe_bvd_interface_t * bvd_interface_p, 
                                                       fbe_atomic_32_t *arrivals_to_nonzero_q_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    if (bvd_interface_p->b_perf_stats_enabled)
    {
        *arrivals_to_nonzero_q_p = bvd_interface_p->sp_performace_stats.arrivals_to_nonzero_q;
    }
     return status;
}
/******************************************************************************
 * end fbe_bvd_interface_perf_stats_get_arrivals_to_nonzero_q()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_bvd_interface_perf_stats_inc_sum_q_length_arrival()
 ******************************************************************************
 * @brief
 *  This function is used to increments the sum_q_length_arrival on the lun
 *
 * @param bvd_interface_p     -   Pointer to the BVD object.
 * @param blocks -       Blocks to increment by.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 *
 ******************************************************************************/
fbe_status_t
fbe_bvd_interface_perf_stats_inc_sum_q_length_arrival(fbe_bvd_interface_t * bvd_interface_p,
                                                      fbe_u32_t blocks)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    if (bvd_interface_p->b_perf_stats_enabled)
    {
        fbe_atomic_add(&(bvd_interface_p->sp_performace_stats.sum_q_length_arrival), blocks);
    }
     return status;
}
/******************************************************************************
 * end fbe_bvd_interface_perf_stats_inc_sum_q_length_arrival()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_bvd_interface_perf_stats_get_sum_q_length_arrival()
 ******************************************************************************
 * @brief
 *  This function is used to get the sum_of_queue_lengths on the lun.
 *
 * @param bvd_interface_p     -   Pointer to the BVD object.
 * @param sum_q_length_arrival  -   Pointer to the arrivals_to_nonzero_q.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 *
 ******************************************************************************/
fbe_status_t
fbe_bvd_interface_perf_stats_get_sum_q_length_arrival(fbe_bvd_interface_t * bvd_interface_p, 
                                                       fbe_atomic_t *sum_q_length_arrival)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    if (bvd_interface_p->b_perf_stats_enabled)
    {
        *sum_q_length_arrival = bvd_interface_p->sp_performace_stats.sum_q_length_arrival;
    }
     return status;
}
/******************************************************************************
 * end fbe_bvd_interface_perf_stats_get_sum_q_length_arrival()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_bvd_interface_perf_stats_set_timestamp()
 ******************************************************************************
 * @brief
 *  This function is used to sets the timestamp on the BVD
 *
 * @param bvd_interface_p     -   Pointer to the BVD object.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 *
 ******************************************************************************/
fbe_status_t
fbe_bvd_interface_perf_stats_set_timestamp(fbe_bvd_interface_t * bvd_interface_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    /* used to store the current time */
    fbe_time_t                                  current_time;
    /* Set the timestamps */
    current_time = fbe_get_time();

    if (bvd_interface_p->b_perf_stats_enabled)
    {
        bvd_interface_p->sp_performace_stats.timestamp = current_time;
    }

    return status;
}
/******************************************************************************
 * end fbe_bvd_interface_perf_stats_set_timestamp()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_bvd_interface_perf_stats_get_timestamp()
 ******************************************************************************
 * @brief
 *  This function is used to get the timestamp on the BVD
 *
 * @param lun_perf_stats_p -   Pointer to the lun perf stats object.
 * @param timestamp_p  -   Pointer to the mr3_write.
 *
 * @return fbe_status_t
 *
 * @author
 * 12/14/2010:  Created. Swati Fursule
 *
 ******************************************************************************/
fbe_status_t
fbe_bvd_interface_perf_stats_get_timestamp(fbe_bvd_interface_t * bvd_interface_p, 
                                  fbe_atomic_t *timestamp_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    *timestamp_p = bvd_interface_p->sp_performace_stats.timestamp;
     return status;
}
/******************************************************************************
 * end fbe_bvd_interface_perf_stats_get_timestamp()
 ******************************************************************************/

/*******************************
 * end fbe_bvd_interface_perf_stats.c
 *******************************/
