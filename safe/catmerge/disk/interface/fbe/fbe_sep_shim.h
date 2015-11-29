#ifndef FBE_SEP_SHIM_H
#define FBE_SEP_SHIM_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"

fbe_status_t fbe_sep_shim_init(void);
fbe_status_t fbe_sep_shim_destroy(void);


/*!********************************************************************* 
 * @struct fbe_sep_shim_perf_stat_per_core_t
 *  
 * @brief 
 *   The struct sep shim performance statistics counters
 **********************************************************************/
typedef struct fbe_sep_shim_perf_stat_per_core_s{
	/*! @todo: Make sure that all IOs (like PSM) 
	 *  are core affined to save locking and atomic operations here
	 */
    fbe_u64_t       total_ios;/*ios since last reset*/
    fbe_u64_t   	current_ios_in_progress;/*how many ios in flight we have now*/
    fbe_u64_t       max_io_q_depth;/*what was the biggest number of outstanding IOs*/

	fbe_transport_run_queue_stats_t runq_stats;
}fbe_sep_shim_perf_stat_per_core_t;

typedef struct fbe_sep_shim_get_perf_stat_s{
    fbe_sep_shim_perf_stat_per_core_t	core_stat[FBE_CPU_ID_MAX];
	fbe_u32_t							structures_per_core;
}fbe_sep_shim_get_perf_stat_t;

typedef struct fbe_sep_shim_set_rq_method_s{
	fbe_transport_rq_method_t  rq_method;
}fbe_sep_shim_set_rq_method_t;

typedef struct fbe_sep_shim_set_alert_time_s{
	fbe_u32_t  alert_time;
}fbe_sep_shim_set_alert_time_t;

/*!***************************************************************************
 *          fbe_sep_shim_get_perf_stat()
 *****************************************************************************
 *
 * @brief   Get the performace statistics from sep shim .
 *
 * @param   stat - Pointer to statistics structure
 *
 * @return fbe_status_t   
 *
 * @author
 *  2/15/2011 - Created. Swati Fursule
 *
 *****************************************************************************/
fbe_status_t fbe_sep_shim_get_perf_stat(fbe_sep_shim_get_perf_stat_t *stat);
void fbe_sep_shim_perf_stat_enable(void);
void fbe_sep_shim_perf_stat_disable(void);
void fbe_sep_shim_perf_stat_clear(void);

fbe_status_t fbe_sep_shim_set_async_io(fbe_bool_t async_io);
fbe_status_t fbe_sep_shim_set_async_io_compl(fbe_bool_t async_io);
fbe_status_t fbe_sep_shim_set_rq_method(fbe_transport_rq_method_t rq_method);
fbe_status_t fbe_sep_shim_set_alert_time(fbe_u32_t alert_time);
fbe_status_t fbe_sep_shim_shutdown_bvd(void);
#endif /*FBE_SEP_SHIM_H*/
