#include "fbe/fbe_stat.h"
#include "fbe/fbe_stat_api.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_library_interface.h"


typedef struct fbe_stat_weight_change_components_s
{
    fbe_u8_t opcode;
    fbe_u32_t port_status;
    fbe_u32_t sk, asc, ascq;
}fbe_stat_weight_change_components_t;

static fbe_status_t 
fbe_stat_get_recovery_action(fbe_drive_stat_t * drive_stat,
                             fbe_drive_error_t * drive_error,
                             fbe_payload_cdb_fis_error_flags_t error_flags,
                             fbe_stats_control_flag_t  control_flag,
                             fbe_stat_action_t * stat_action);

static void 
fbe_stat_get_recovery_action2(fbe_stat_actions_state_t  * actions_state,
                              fbe_stat_t * stat,                              
                              fbe_u64_t error_tag,
                              fbe_atomic_t io_counter, 
                              fbe_stat_action_t * stat_action);


static fbe_status_t fbe_stat_recovery_tag_reduce(fbe_stat_t * stat, fbe_u32_t recovery_tag_reduce, fbe_u64_t io_counter, fbe_u64_t * tag);
static void         fbe_stat_get_external_configuration_weight_change(const fbe_stat_t * stat_p, const fbe_stat_weight_change_components_t *wt_comps, fbe_u32_t *change_p);
static void         fbe_stat_get_opcode_weight_change(const fbe_stat_t * stat_p, const fbe_u8_t opcode, fbe_u32_t *change_p);
static void         fbe_stat_get_port_error_weight_change(const fbe_stat_t * stat_p, const fbe_u32_t port_status, fbe_u32_t *change_p);
static void         fbe_stat_get_sense_code_weight_change(const fbe_stat_t * stat_p, const fbe_u8_t sk, const fbe_u8_t asc, const fbe_u8_t ascq, fbe_u32_t *change_p);
static fbe_bool_t   fbe_stat_action_is_tripped(const fbe_stat_actions_state_t * actions, fbe_u32_t entry);
static void         fbe_stat_action_set_tripped(fbe_stat_actions_state_t * actions, fbe_u32_t entry);
static void         fbe_stat_action_clear_tripped(fbe_stat_actions_state_t * actions, fbe_u32_t entry);
static void         fbe_stat_handle_drive_error(fbe_stat_t *stat_p, fbe_stat_actions_state_t *pdo_actions_state_p, fbe_u64_t *error_tag_p, fbe_atomic_t io_counter, fbe_u32_t *pdo_ratio_p, fbe_u32_t burst_weight_reduce, const fbe_stat_weight_change_components_t *external_config_weight_adjust_p, fbe_u32_t addl_weight_adjust);

fbe_status_t 
fbe_stat_init(fbe_stat_t * stat, fbe_u32_t interval, fbe_u32_t weight, fbe_u32_t threshold)
{
    stat->interval = interval;
    stat->weight = weight;
    stat->threshold = threshold;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_stat_destroy(fbe_stat_t * stat)
{
    stat->interval = 0;
    stat->weight = 0;
    stat->threshold = 0;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_stat_io_fail(fbe_stat_t * stat, fbe_atomic_t io_counter, fbe_u64_t * error_tag, fbe_u32_t  burst_weight_reduce, fbe_u32_t weight_change)
{
    fbe_u64_t   err = 0;
    fbe_u64_t   historical_shift = 0; /* Added when the tag is below interval watermark */
    fbe_u64_t   diff = 0;
    double      wr = 0; /* weight ratio */
    fbe_u64_t   real_weight = stat->weight;/*to start with we take that as default value*/
    double      p = 0;

    err = *error_tag;
    historical_shift = 0;

    if(err + 2 * stat->interval < (fbe_u64_t)io_counter){ /* We have no recent history */
        err = io_counter - stat->interval;
    } else if(err < io_counter - stat->interval +  stat->weight) { /* We are below the interval mark */
        diff = err - (io_counter - 2 * stat->interval);
        wr = (double)(diff) / (double)(stat->interval + stat->weight);
        historical_shift = (fbe_u64_t)((double)(stat->weight) * wr);
        err = io_counter - stat->interval + historical_shift;
    }

    /*calculate the real wieght which is the weight , less the possible reduction*/
    if (burst_weight_reduce > 0) {
        p = (double)(stat->weight) / 100.0; /* one percent of weight */
        diff = (fbe_u64_t)(p * (double)burst_weight_reduce);
        real_weight = stat->weight - diff;

        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                "fbe_stat: burst_weight_reduce %d, starting weight %d, new weight %d\n", burst_weight_reduce, (int)stat->weight, (int)real_weight);

    }

    /* for all other cases where we want to change the weight*/    
    if (weight_change != FBE_STAT_WEIGHT_CHANGE_NONE)
    {
        real_weight = (fbe_u64_t)((double)real_weight * ((double)weight_change/100.0));

        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                "fbe_stat: weight_reduce %d, starting weight %d, new weight %d \n", weight_change, (int)stat->weight, (int)real_weight);
    }

    err += real_weight;
    if(err > (fbe_u64_t)io_counter){
        err = io_counter;
    }

    *error_tag = err;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_stat_get_ratio(fbe_stat_t * stat, fbe_atomic_t io_counter, fbe_u64_t error_tag, fbe_u32_t * ratio)
{
    fbe_u64_t diff;
    double p;
    fbe_u32_t r = 0;
    double wr; /* weight ratio */

    if(stat->interval == 0){
        *ratio = r;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(error_tag + 2 * stat->interval < (fbe_u64_t)io_counter){ /* We have no recent history */
        r = 0; /* No recent errors */
    } else if(error_tag < io_counter - stat->interval +  stat->weight) {
        /* We are below the interval mark */
        p = 100.0 * (stat->weight / stat->interval); /* How many % brungs one error */
        diff = error_tag - (io_counter - 2 * stat->interval);
        wr = (double)(diff) / (double)(stat->interval + stat->weight);
        r = (fbe_u32_t)(p * wr); 
    } else {
        p = 100.0 / (double)(stat->interval);
        diff = io_counter - error_tag;
        r = (fbe_u32_t)(100 - p*(double)diff); 
    }

    *ratio = r;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_stat_io_completion(fbe_object_id_t                        obj_id,        /*in*/
                       fbe_drive_configuration_record_t     * threshold_rec, /*in*/
                       fbe_drive_dieh_state_t               * dieh_state, /*in*/
                       fbe_drive_error_t                    * drive_error, /*out*/
                       fbe_payload_cdb_fis_error_flags_t      error_flags, /*in*/
                       fbe_u8_t                               opcode, /*in*/
                       fbe_u32_t                              port_status, /*in*/
                       fbe_u32_t sk, fbe_u32_t asc, fbe_u32_t ascq,  /*in*/
                       fbe_bool_t                             is_emeh, /*in*/
                       fbe_stat_action_t                    * stat_action) /*out*/
{
    fbe_u32_t ratio;
    fbe_time_t current_time;
    fbe_u32_t  burst_weight_reduce;
    fbe_stat_weight_change_components_t weight_change_components = {opcode, port_status, sk, asc, ascq};


    /* If we are in the Extended Media Error Handling (EMEH) mode then we need to freeze dieh ratios 
       until we come out.  This is to prevent drives from crossing the fail threshold and causing a 
       broken RG when we exit EMEH. The only bucket we do not want to freeze is the link bucket,  since
       if it is really a link error then we want to shoot the drive and let SLF kick in.  */


    *stat_action = FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION;

    if(NULL == threshold_rec) 
    {
        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: threshold_rec is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We should look at simple cases first */
    if(error_flags == FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR){
        return FBE_STATUS_OK;
    }

    /* Some error returned by the port and did not reach the actual device and
     * so action needs to be taken at this level */
    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_PORT) {
        *stat_action = FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION;
        return FBE_STATUS_OK;
    }
    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL){
        *stat_action = FBE_STAT_ACTION_FLAG_FLAG_FAIL;
        return FBE_STATUS_OK;
    }

    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NOT_SPINNING){
        *stat_action = FBE_STAT_ACTION_FLAG_FLAG_SPINUP; /* Spinup condition in activate state */
        return FBE_STATUS_OK;
    }

    /* Update time signature */
    burst_weight_reduce = 0;
    current_time = fbe_get_time();
    if(fbe_get_elapsed_milliseconds(drive_error->last_error_time) < threshold_rec->threshold_info.error_burst_delta){
        burst_weight_reduce = threshold_rec->threshold_info.error_burst_weight_reduce;
    }
    

    drive_error->last_error_time = current_time;

    /* Check io_error (cumulative) ratio */
    {
        fbe_stat_t *io_stat_p = &threshold_rec->threshold_info.io_stat;
                
        fbe_stat_handle_drive_error(io_stat_p, &dieh_state->category.io, &drive_error->io_error_tag, 
                                    drive_error->io_counter, &ratio,
                                    burst_weight_reduce, &weight_change_components, (is_emeh?0:dieh_state->media_weight_adjust));  /* media errors can affect this stat */
        
        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Cumulative(%d)- Obj:0x%x, IoCnt:0x%llX, Et:0x%llX, r:%d, rstEt:0x%llX\r\n", 
                                error_flags, obj_id, (unsigned long long)drive_error->io_counter, (unsigned long long)drive_error->io_error_tag, ratio, (unsigned long long)drive_error->reset_error_tag);    
    
        fbe_stat_get_recovery_action2(&dieh_state->category.io, io_stat_p, drive_error->io_error_tag, drive_error->io_counter, stat_action);
    }

    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED){
        /* Check recovered ratio */
        fbe_stat_t *recov_stat_p = &threshold_rec->threshold_info.recovered_stat;

        fbe_stat_handle_drive_error(recov_stat_p, &dieh_state->category.recovered, &drive_error->recovered_error_tag, 
                                    drive_error->io_counter, &ratio, 
                                    burst_weight_reduce, &weight_change_components, (is_emeh?0:dieh_state->media_weight_adjust)); /* unrecovered errors are mostly media errors */

        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                                "Recovered(%d) - Obj:0x%x, IoCnt:0x%llX, Et:0x%llX, r:%d, rstEt:0x%llX\r\n", 
                                error_flags, obj_id, (unsigned long long)drive_error->io_counter, (unsigned long long)drive_error->recovered_error_tag, ratio, (unsigned long long)drive_error->reset_error_tag);

        fbe_stat_get_recovery_action2(&dieh_state->category.recovered, recov_stat_p, drive_error->recovered_error_tag, drive_error->io_counter, stat_action);

        /* Also clear link stats. Since we got a response from a drive we know the link is good. This is to guard against the case where we 
           get command timeouts due to drive taking too long recovering data. */
        drive_error->link_error_tag = 0;
    }

    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA){
        /* Check media ratio */
        fbe_stat_t *media_stat_p = &threshold_rec->threshold_info.media_stat;

        fbe_stat_handle_drive_error(media_stat_p, &dieh_state->category.media, &drive_error->media_error_tag, 
                                    drive_error->io_counter, &ratio,
                                    burst_weight_reduce, &weight_change_components, (is_emeh?0:dieh_state->media_weight_adjust));

        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                                "Media(%d) - Obj:0x%x, IoCnt:0x%llX, Et:0x%llX, r:%d, rstEt:0x%llX\r\n", 
                                error_flags, obj_id, (unsigned long long)drive_error->io_counter, (unsigned long long)drive_error->media_error_tag, ratio, (unsigned long long)drive_error->reset_error_tag);

        fbe_stat_get_recovery_action2(&dieh_state->category.media, media_stat_p, drive_error->media_error_tag, drive_error->io_counter, stat_action);            

        /* Also clear link stats. Since we got a response from a drive we know the link is good. This is to guard against the case where we 
           get command timeouts due to drive taking too long recovering data. */
        drive_error->link_error_tag = 0;
    }

    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE){
        /* Check hardware ratio */
        fbe_stat_t *hw_stat_p = &threshold_rec->threshold_info.hardware_stat;

        fbe_stat_handle_drive_error(hw_stat_p, &dieh_state->category.hardware, &drive_error->hardware_error_tag, 
                                    drive_error->io_counter, &ratio,
                                    burst_weight_reduce, &weight_change_components, (is_emeh?0:FBE_STAT_WEIGHT_CHANGE_NONE));

        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                                "Hardware(%d) - Obj:0x%x, IoCnt:0x%llX, Et:0x%llX, r:%d, rstEt:0x%llX\r\n", 
                                error_flags, obj_id, (unsigned long long)drive_error->io_counter, (unsigned long long)drive_error->hardware_error_tag, ratio, (unsigned long long)drive_error->reset_error_tag);

        fbe_stat_get_recovery_action2(&dieh_state->category.hardware, hw_stat_p, drive_error->hardware_error_tag, drive_error->io_counter, stat_action);
    }

    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HEALTHCHECK){
        /* Check healthcheck ratio */
        fbe_stat_t *hc_stat_p = &threshold_rec->threshold_info.healthCheck_stat;

        fbe_stat_handle_drive_error(hc_stat_p, &dieh_state->category.healthCheck, &drive_error->healthCheck_error_tag, 
                                    drive_error->io_counter, &ratio,
                                    burst_weight_reduce, &weight_change_components, (is_emeh?0:dieh_state->media_weight_adjust));   /* media errors can affect this stat. */

        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                                "HealthCheck(%d) - Obj:0x%x, IoCnt:0x%llX, Et:0x%llX, r:%d, rstEt:0x%llX\r\n", 
                                error_flags, obj_id, (unsigned long long)drive_error->io_counter, (unsigned long long)drive_error->healthCheck_error_tag, ratio, (unsigned long long)drive_error->reset_error_tag);

        fbe_stat_get_recovery_action2(&dieh_state->category.healthCheck, hc_stat_p, drive_error->healthCheck_error_tag, drive_error->io_counter, stat_action);
    }

    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK){
        /* Check link ratio */
        fbe_stat_t *link_stat_p = &threshold_rec->threshold_info.link_stat;

        fbe_stat_handle_drive_error(link_stat_p, &dieh_state->category.link, &drive_error->link_error_tag, 
                                    drive_error->io_counter, &ratio,
                                    burst_weight_reduce, &weight_change_components, FBE_STAT_WEIGHT_CHANGE_NONE);

        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                                "Link(%d) - Obj:0x%x, IoCnt:0x%llX, Et:0x%llX, r:%d, rstEt:0x%llX\r\n", 
                                error_flags, obj_id, (unsigned long long)drive_error->io_counter, (unsigned long long)drive_error->link_error_tag, ratio, (unsigned long long)drive_error->reset_error_tag);

        fbe_stat_get_recovery_action2(&dieh_state->category.link, link_stat_p, drive_error->link_error_tag, drive_error->io_counter, stat_action); 
    }

    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_DATA){
        /* Check data ratio */
        fbe_stat_t *data_stat_p = &threshold_rec->threshold_info.data_stat;

        fbe_stat_handle_drive_error(data_stat_p, &dieh_state->category.data, &drive_error->data_error_tag, 
                                    drive_error->io_counter, &ratio,
                                    burst_weight_reduce, &weight_change_components, (is_emeh?0:FBE_STAT_WEIGHT_CHANGE_NONE));

        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                                "Data(%d) - Obj:0x%x, IoCnt:0x%llX, Et:0x%llX, r:%d, rstEt:0x%llX\r\n", 
                                error_flags, obj_id, (unsigned long long)drive_error->io_counter, (unsigned long long)drive_error->data_error_tag, ratio, (unsigned long long)drive_error->reset_error_tag);

        fbe_stat_get_recovery_action2(&dieh_state->category.data, data_stat_p, drive_error->data_error_tag, drive_error->io_counter, stat_action);
    }

    /* Process exceptions */
    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE){
        (*stat_action) |= FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE; 
    }

    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_END_OF_LIFE_CALLHOME){
        (*stat_action) |= FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE_CALL_HOME; 
    }

    if(error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL_CALLHOME){
        (*stat_action) |= FBE_STAT_ACTION_FLAG_FLAG_FAIL_CALL_HOME; 
    }


    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_stat_calculate_new_weight_change
 ****************************************************************
 * @brief
 *  Calculates the new weight_change value.   The value will adjust
 *  the current records weight, as a percentage.   For instance, if
 *  weight_change = 20, and current weight = 1000, then new weight
 *  will be 200.  Note, weights can also be increased by providing
 *  a weight_change > 100.
 *
 * @param current_percent   - (IN) current weight_change value.
 * @param adjust_by_percent - (IN) Adjust current weight_change value by
 *                              this percentage.
 * @return fbe_u32_t - the new current weight_change value.
 *
 * @author 
 *  12/03/2012 -  Wayne Garrett - Created
 *
 ****************************************************************/
fbe_u32_t fbe_stat_calculate_new_weight_change(fbe_u32_t current_percent, fbe_u32_t adjust_by_percent)
{
    if (adjust_by_percent != FBE_STAT_WEIGHT_CHANGE_NONE)
    {
        return (fbe_u32_t)((double)current_percent * (double)adjust_by_percent/100.0);
    }

    return current_percent;
}

/* This is the 2nd generation DIEH introduced in R31. */
static fbe_status_t 
fbe_stat_get_recovery_action(fbe_drive_stat_t * drive_stat,
                             fbe_drive_error_t * drive_error,
                             fbe_payload_cdb_fis_error_flags_t error_flags,
                             fbe_stats_control_flag_t  control_flag,
                             fbe_stat_action_t * stat_action)

{
    fbe_u32_t ratio;

    /* Check reset ratio */
    fbe_stat_get_ratio(&drive_stat->reset_stat, drive_error->io_counter, drive_error->reset_error_tag, &ratio);
    if(ratio > drive_stat->reset_stat.threshold){
        /* Check power cycle ratio */
        fbe_stat_get_ratio(&drive_stat->power_cycle_stat, drive_error->io_counter, drive_error->power_cycle_error_tag, &ratio);
        if(/*(drive_stat->power_cycle_stat.threshold == 0) || */(ratio > drive_stat->power_cycle_stat.threshold)){
            if(control_flag & FBE_STATS_CNTRL_FLAG_DRIVE_KILL) {            
                (*stat_action) |= FBE_STAT_ACTION_FLAG_FLAG_FAIL;           
            }
            if(control_flag & FBE_STATS_CNTRL_FLAG_DRIVE_KILL_CALL_HOME) {            
                (*stat_action) |= FBE_STAT_ACTION_FLAG_FLAG_FAIL_CALL_HOME; 
            }
        } else {
            /*for now we don't do power cycles, we just force the tag to be big enough so it would force the next
            time we are here to fail the drive and not take this branch*/
            drive_error->power_cycle_error_tag = drive_error->io_counter;
            (*stat_action) |= FBE_STAT_ACTION_FLAG_FLAG_RESET; 

            if(control_flag & FBE_STATS_CNTRL_FLAG_EOL) {
                (*stat_action) |= FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE;
            }
            if(control_flag & FBE_STATS_CNTRL_FLAG_EOL_CALL_HOME) {
                (*stat_action) |= FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE_CALL_HOME;
            }
        }
    } else {
        (*stat_action) |= FBE_STAT_ACTION_FLAG_FLAG_RESET;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_stat_get_recovery_action2
 ****************************************************************
 * @brief
 *  Return the recovery actions for a given error category.   An
 *  action will be returned if the current ratio and geater than
 *  a defined action and that action hasn't already been tripped.
 *  More than one action can be returned.
 *
 * @param pdo_actions_state  - (IN) Current state of the actions for a specific error category.
 * @param thrsh_cfg - (IN) Threshold configuration info for a particual error category.
 * @param error_tag  - (IN) Health of drive.
 * @param io_counter  - (IN) Current number of IOs sent to drive
 * @param stat_action - (OUT) Recovery actions to be returned.
 *
 * @return none.
 *
 * @author 
 *  06/11/2012 -  Wayne Garrett - Redesigned DIEH (3rd generation)
 *
 ****************************************************************/
static void 
fbe_stat_get_recovery_action2(fbe_stat_actions_state_t  * pdo_actions_state_p, 
                              fbe_stat_t * thrsh_cfg,                              
                              fbe_u64_t error_tag,
                              fbe_atomic_t io_counter, 
                              fbe_stat_action_t * stat_action)
{
    fbe_u32_t ratio;
    fbe_u32_t i;

    if (NULL == pdo_actions_state_p ||
        NULL == thrsh_cfg ||
        NULL == stat_action )
    {
        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s null paramater. actions_state=0x%llx, thrsh_cfg=0x%llx, stat_actoin=0x%llx",  __FUNCTION__, (unsigned long long)pdo_actions_state_p, (unsigned long long)thrsh_cfg, (unsigned long long)stat_action);
        return;
    }

    fbe_stat_get_ratio(thrsh_cfg, io_counter, error_tag, &ratio);

    /* Loop through all actions.  Trip each action that has exceeded the ratio.
       An action can only be tripped once.  This may change some day.
     */
    for(i=0; i < thrsh_cfg->actions.num; i++)
    {
        if (! fbe_stat_action_is_tripped(pdo_actions_state_p, i))
        {
            if (ratio >= thrsh_cfg->actions.entry[i].ratio)
            {
                (*stat_action) |= thrsh_cfg->actions.entry[i].flag;
                fbe_stat_action_set_tripped(pdo_actions_state_p, i);
            }
        }
    }

}


fbe_status_t 
fbe_stat_drive_error_init(fbe_drive_error_t * drive_error)
{
    drive_error->io_counter = 2 * FBE_STAT_MAX_INTERVAL;    
    drive_error->last_error_time = 0;
    fbe_stat_drive_error_clear(drive_error);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_stat_drive_error_clear
 ****************************************************************
 * @brief
 *  Resets/Clears the Error Tags to zero, effectively resetting
 *  Error Ratio back to 0.
 *
 *
 ****************************************************************/

void
fbe_stat_drive_error_clear(fbe_drive_error_t * drive_error)
{
    drive_error->io_error_tag = 0;
    drive_error->recovered_error_tag = 0;
    drive_error->media_error_tag = 0;
    drive_error->hardware_error_tag = 0;
    drive_error->healthCheck_error_tag = 0;
    drive_error->link_error_tag = 0;
    drive_error->reset_error_tag = 0;
    drive_error->power_cycle_error_tag = 0;
    drive_error->data_error_tag = 0;
}


fbe_status_t 
fbe_stat_drive_reset(fbe_drive_configuration_handle_t drive_configuration_handle, 
                     fbe_drive_error_t * drive_error)
{
    fbe_status_t status;
    fbe_drive_configuration_record_t * threshold_rec = NULL;

    status = fbe_drive_configuration_get_threshold_info_ptr(drive_configuration_handle, &threshold_rec);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Can't get threshold Info.  Invalid handle.\n", __FUNCTION__);            
        return status;
    }

    fbe_stat_io_fail(&threshold_rec->threshold_info.reset_stat, drive_error->io_counter, &drive_error->reset_error_tag, 0, FBE_STAT_WEIGHT_CHANGE_NONE);
        
    return FBE_STATUS_OK;
}



static fbe_status_t 
fbe_stat_recovery_tag_reduce(fbe_stat_t * stat, fbe_u32_t recovery_tag_reduce, fbe_u64_t io_counter, fbe_u64_t * tag)
{
    fbe_u64_t diff;
    double p;
    fbe_u64_t error_tag = 0;

    if(stat->interval == 0){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    error_tag = *tag;

    if(error_tag + 2 * stat->interval < io_counter){ /* We have no recent history */
        return FBE_STATUS_OK;; /* No recent errors */
    } else {
        p = (double)(stat->interval) / 100.0; /* one percent of interval */
        diff = (fbe_u64_t)(p * (double)recovery_tag_reduce);
        if((error_tag - diff) > 0){
            error_tag -= diff;
        }else {
            error_tag = 0;
        }
    }

    *tag = error_tag;
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_stat_get_external_configuration_weight_change
 ****************************************************************
 * @brief
 *  Return any special weight_change value.  If no special overrides
 *  then weight_change arg it will not be modified.
 *
 * @param stat_p   - (IN) fbe_stat record
 * @param wt_comps - (IN) Components which are used to check for
 *                    special weight changes.
 * @return weight_change_percent - the new weight_change value.
 *
 * @author 
 *  12/03/2012 -  Wayne Garrett - Created
 *
 ****************************************************************/
static void fbe_stat_get_external_configuration_weight_change(const fbe_stat_t * stat_p, const fbe_stat_weight_change_components_t *wt_comps, fbe_u32_t *weight_change_percent)
{
    fbe_u32_t adjust_by;

    fbe_stat_get_opcode_weight_change(stat_p, wt_comps->opcode, &adjust_by);
    *weight_change_percent = fbe_stat_calculate_new_weight_change(*weight_change_percent, adjust_by);

    fbe_stat_get_port_error_weight_change(stat_p, wt_comps->port_status, &adjust_by);
    *weight_change_percent = fbe_stat_calculate_new_weight_change(*weight_change_percent, adjust_by);

    fbe_stat_get_sense_code_weight_change(stat_p, wt_comps->sk, wt_comps->asc, wt_comps->ascq, &adjust_by);
    *weight_change_percent = fbe_stat_calculate_new_weight_change(*weight_change_percent, adjust_by);
}


/*!**************************************************************
 * @fn fbe_stat_get_opcode_weight_change
 ****************************************************************
 * @brief
 *  Return weight_change value for a matching opcode.  If no
 *  special overrides then FBE_STAT_WEIGHT_CHANGE_NONE is returned.
 *
 * @param stat_p   - (IN) fbe_stat record
 * @param opcode   - (IN) opcode to check for.
 * @return change_p - (OUT) the new weight_change value.
 *
 * @author 
 *  12/03/2012 -  Wayne Garrett - Created
 *
 ****************************************************************/
static void fbe_stat_get_opcode_weight_change(const fbe_stat_t * stat_p, const fbe_u8_t opcode, fbe_u32_t *change_p)
{
    fbe_u32_t i=0;

    /* loop through record search for match.  return No Change if nothing is found. */
    *change_p = FBE_STAT_WEIGHT_CHANGE_NONE;

    for (i=0; i<FBE_STAT_MAX_WEIGHT_EXCEPTION_CODES; i++)
    {
        if (FBE_STAT_WEIGHT_EXCEPTION_OPCODE == stat_p->weight_exceptions[i].type &&
            opcode == stat_p->weight_exceptions[i].u.opcode)
        {
            *change_p = stat_p->weight_exceptions[i].change;
            break;
        }
        else if (FBE_STAT_WEIGHT_EXCEPTION_INVALID == stat_p->weight_exceptions[i].type)
        {
            break; /*no matches found.*/
        }
    }
}

/*!**************************************************************
 * @fn fbe_stat_get_port_error_weight_change
 ****************************************************************
 * @brief
 *  Return weight_change value for a matching port error.  If no
 *  special overrides then FBE_STAT_WEIGHT_CHANGE_NONE is returned.
 *
 * @param stat_p      - (IN) fbe_stat record
 * @param port_status - (IN) port status to check for.
 * @return change_p   - (OUT) the new weight_change value.
 *
 * @author 
 *  12/03/2012 -  Wayne Garrett - Created
 *
 ****************************************************************/
static void fbe_stat_get_port_error_weight_change(const fbe_stat_t * stat_p, const fbe_u32_t port_status, fbe_u32_t *change_p)
{
    fbe_u32_t i=0;

    /* loop through record search for match.  return No Change if nothing is found. */
    *change_p = FBE_STAT_WEIGHT_CHANGE_NONE;

    for (i=0; i<FBE_STAT_MAX_WEIGHT_EXCEPTION_CODES; i++)
    {
        if (FBE_STAT_WEIGHT_EXCEPTION_PORT_ERROR == stat_p->weight_exceptions[i].type &&
            port_status == stat_p->weight_exceptions[i].u.port_error)
        {
            *change_p = stat_p->weight_exceptions[i].change;
            break;
        }
        else if (FBE_STAT_WEIGHT_EXCEPTION_INVALID == stat_p->weight_exceptions[i].type)
        {
            break; /*no matches found.*/
        }
    }
}

/*!**************************************************************
 * @fn fbe_stat_get_sense_code_weight_change
 ****************************************************************
 * @brief
 *  Return weight_change value for a matching sense code.  If no
 *  special overrides then FBE_STAT_WEIGHT_CHANGE_NONE is returned.
 *
 * @param stat_p      - (IN) fbe_stat record
 * @param sense code  - (IN) sense code to check for.
 * @return change_p   - (OUT) the new weight_change value.
 *
 * @author 
 *  12/03/2012 -  Wayne Garrett - Created
 *
 ****************************************************************/
static void fbe_stat_get_sense_code_weight_change(const fbe_stat_t * stat_p, const fbe_u8_t sk, const fbe_u8_t asc, const fbe_u8_t ascq, fbe_u32_t *change_p)
{
    fbe_u32_t i=0;

    /* loop through record search for match.  return No Change if nothing is found. */
    *change_p = FBE_STAT_WEIGHT_CHANGE_NONE;

    for (i=0; i<FBE_STAT_MAX_WEIGHT_EXCEPTION_CODES; i++)
    {
        if (FBE_STAT_WEIGHT_EXCEPTION_CC_ERROR == stat_p->weight_exceptions[i].type &&
            sk ==   stat_p->weight_exceptions[i].u.sense_code.sense_key  &&
            asc ==  stat_p->weight_exceptions[i].u.sense_code.asc        &&            
            ascq >= stat_p->weight_exceptions[i].u.sense_code.ascq_start &&
            ascq <= stat_p->weight_exceptions[i].u.sense_code.ascq_end) 
        {
            *change_p = stat_p->weight_exceptions[i].change;
            break;
        }
        else if (FBE_STAT_WEIGHT_EXCEPTION_INVALID == stat_p->weight_exceptions[i].type)
        {
            break; /*no matches found.*/
        }
    }
}

/*!**************************************************************
 * @fn fbe_stat_action_is_tripped
 ****************************************************************
 * @brief
 *  Return true if action has already been tripped.
 * 
 *  @param actions - bitmap of tripped actions
 *  @param entry -  action entry to check in bitmap
 * 
 *  @return fbe_bool_t - true if tripped
 * 
 * @author 
 *  02/14/2013 -  Wayne Garrett - Created
 *
 ****************************************************************/
static fbe_bool_t fbe_stat_action_is_tripped(const fbe_stat_actions_state_t * actions, fbe_u32_t entry)
{
    if (entry > 31)
    {
        fbe_base_library_trace (FBE_LIBRARY_ID_DRIVE_STAT, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s invalid entry %d\n", __FUNCTION__, entry);
        return FBE_FALSE;
    }
    return (actions->tripped_bitmap & (1<<entry))? FBE_TRUE : FBE_FALSE;
}

/*!**************************************************************
 * @fn fbe_stat_action_set_tripped
 ****************************************************************
 * @brief
 *  Mark action as being tripped.
 * 
 *  @param actions - bitmap of tripped actions
 *  @param entry -  entry in action array to check
 * 
 * @author 
 *  02/14/2013 -  Wayne Garrett - Created
 *
 ****************************************************************/
static void fbe_stat_action_set_tripped(fbe_stat_actions_state_t * actions, fbe_u32_t entry)
{
    actions->tripped_bitmap |= (1<<entry);
}

/*!**************************************************************
 * @fn fbe_stat_action_clear_tripped
 ****************************************************************
 * @brief
 *  Clears a tripped action.
 * 
 *  @param actions - bitmap of tripped actions
 *  @param entry -  entry in action array to check
 * 
 * @author 
 *  02/14/2013 -  Wayne Garrett - Created
 *
 ****************************************************************/
static void fbe_stat_action_clear_tripped(fbe_stat_actions_state_t * actions, fbe_u32_t entry)
{
    actions->tripped_bitmap &= ~(1<<entry);
}

/*!**************************************************************
 * @fn fbe_stat_reactivate_action_flags
 ****************************************************************
 * @brief
 *  Reactivate all actions that have a ratio which dropped below
 *  the low water mark.
 * 
 *  @param stat_p - DIEH Record
 *  @param pdo_action_state_p -  PDO state for DIEH actions taken
 *  @param pdo_ratio -  current DIEH ratio of PDO.
 * 
 * @author 
 *  02/14/2013 -  Wayne Garrett - Created
 *
 ****************************************************************/
void fbe_stat_reactivate_action_flags(fbe_stat_t* stat_p, fbe_stat_actions_state_t *pdo_action_state_p, fbe_u32_t pdo_ratio)
{
    fbe_u32_t i;

    for (i=0; i < stat_p->actions.num; i++)
    {
        if (stat_p->actions.entry[i].reactivate_ratio != FBE_U32_MAX &&
            pdo_ratio <= stat_p->actions.entry[i].reactivate_ratio)  /* is below water mark?*/
        {
            fbe_stat_action_clear_tripped(pdo_action_state_p, i);
        }
    }
}

/*!**************************************************************
 * @fn fbe_stat_handle_drive_error
 ****************************************************************
 * @brief
 *  Handle the drive error by updating the DIEH stats.
 * 
 *  @param stat_p - DIEH Record for specific error bucket
 *  @param pdo_action_state_p -  PDO state for DIEH actions taken for a
 *                  specific error bucket.
 *  @param error_tag_p - Error Tag for specific error bucket.
 *  @param pdo_ratio -  current DIEH ratio of PDO for a specific error
 *                  bucket .
 *  @param burst_weight_reduce - How much to reduce weight by if errors come
 *                  in bursts
 *  @param external_config_weight_adjust_p - Contains values provided by
 *                  DIEH external configuration for adjusting the weight
 *                  of an error. Value is a percentage muliplier.
 *  @param addl_weight_adjust -  Additional weight adjustment.   Value is a
 *                  percentage muliplier.
 * 
 * @author 
 *  02/14/2013 -  Wayne Garrett - Created
 *
 ****************************************************************/
static void fbe_stat_handle_drive_error(fbe_stat_t *stat_p, fbe_stat_actions_state_t *pdo_actions_state_p, fbe_u64_t *error_tag_p, fbe_atomic_t io_counter, fbe_u32_t *pdo_ratio_p, fbe_u32_t burst_weight_reduce, const fbe_stat_weight_change_components_t *external_config_weight_adjust_p, fbe_u32_t addl_weight_adjust)
{
    fbe_u32_t  weight_change_percent = FBE_STAT_WEIGHT_CHANGE_NONE;  
    fbe_u32_t  prev_ratio = FBE_U32_MAX;

    fbe_stat_get_external_configuration_weight_change(stat_p, external_config_weight_adjust_p, &weight_change_percent);  

    /* adjust weight further based on caller's request. */
    weight_change_percent = (fbe_u32_t)(weight_change_percent * addl_weight_adjust/100.0);

    /* before failing IO, reactivate any flags if we dropped below our low watermark */
    fbe_stat_get_ratio(stat_p, io_counter, *error_tag_p, &prev_ratio);  /* get previous ratio */
    fbe_stat_reactivate_action_flags(stat_p, pdo_actions_state_p, prev_ratio);

    /* update the provided error bucket's error_tag and calculate new ratio */
    fbe_stat_io_fail(stat_p, io_counter, error_tag_p, burst_weight_reduce, weight_change_percent);
    fbe_stat_get_ratio(stat_p, io_counter, *error_tag_p, pdo_ratio_p);
}
