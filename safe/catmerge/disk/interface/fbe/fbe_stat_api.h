#ifndef FBE_STAT_API_H
#define FBE_STAT_API_H

#include "fbe/fbe_atomic.h"
#include "fbe/fbe_payload_cdb_fis.h"
#include "fbe/fbe_drive_configuration_interface.h"


#define FBE_STAT_INVALID_RATIO 0xffffffff

enum 
{
    FBE_STAT_WEIGHT_CHANGE_DISABLE = 0,   /* effectively disables error bucket by reducing weight to 0*/
    FBE_STAT_WEIGHT_CHANGE_NONE    = 100,   /*as percentage.  100 means stay the same, 200 means double, 50 means half */    
    FBE_STAT_WEIGHT_CHANGE_DOUBLE  = 200, 
    FBE_STAT_WEIGHT_CHANGE_HALF    = 50, 
};

static __inline fbe_u32_t fbe_physical_drive_convert_percent_increase_to_weight_adjust(fbe_u32_t current_weight_adjust,
                                                                                       fbe_u32_t percent_threshold_increase)
{
    fbe_u32_t   increased_threshold;
/* following values are used when threshold cmd is INCREASE.  Values represent a percentage multipler. 
   100 = stay the same, 200 = double, 50 means reduce by half
*/
    increased_threshold = (fbe_u32_t)(FBE_STAT_WEIGHT_CHANGE_NONE + (FBE_STAT_WEIGHT_CHANGE_NONE * (percent_threshold_increase / 100.0)));

/* An increase in threshold is equivalent to a decrease in weight.  So reducing weight by the percentage that is being increased.  */
    return (fbe_u32_t)(current_weight_adjust*(100.0/(double)increased_threshold));
}

/*!***************************************************************************
 *          fbe_physical_drive_convert_weight_adjust_to_percent_increase()
 *****************************************************************************
 * @brief
 *  Determine the percentage changed using the weight adjust and default values.
 *
 * @param   current_weight_adjust - The current weight adjust value
 * @param   b_is_disabled_p - Address of `is thresholds disabled' bool
 * @param   b_is_increase_p - Address of `is threshold increased' bool
 * @param   b_is_decrease_p - Address if `is threshold decreased' bool
 * @param   adjust_default_p - Address of `adjust default'
 *
 * @return  percentage increased - 0 - If disabled - adjust is 0
 *                                     else no change
 *
 * @author
 *  05/12/2015  Ron Proulx
 *
 *****************************************************************************/
static __inline fbe_u32_t fbe_physical_drive_convert_weight_adjust_to_percent_increase(fbe_u32_t current_weight_adjust,
                                                                                       fbe_bool_t *b_is_disabled_p,
                                                                                       fbe_bool_t *b_is_increase_p,
                                                                                       fbe_bool_t *b_is_decrease_p,
                                                                                       fbe_u32_t *adjust_default_p)
{
    fbe_u32_t   threshold;

    *b_is_disabled_p = FBE_FALSE;
    *b_is_increase_p = FBE_FALSE;
    *b_is_decrease_p = FBE_FALSE;
    *adjust_default_p = FBE_STAT_WEIGHT_CHANGE_NONE;

    /* If disabled set disabled and return 0 */
    if (current_weight_adjust == FBE_STAT_WEIGHT_CHANGE_DISABLE)
    {
        *b_is_disabled_p = FBE_TRUE;
        return 0;
    }
/* following values are used when threshold cmd is INCREASE.  Values represent a percentage multipler. 
   100 = stay the same, 200 = double, 50 means reduce by half
*/
    threshold = (fbe_u32_t)(FBE_STAT_WEIGHT_CHANGE_NONE * (100.0 / (double)current_weight_adjust));
    if (threshold < 100)
    {
        *b_is_decrease_p = FBE_TRUE;
        return (fbe_u32_t)(((100.0 / (double)threshold) * 100) - 100);
    }
    else if (threshold > 100)
    {
        /* An increase in threshold is equivalent to a decrease in weight.  So reducing weight by the percentage that is being increased.  */
        *b_is_increase_p = FBE_TRUE;
        return (fbe_u32_t)(threshold - 100);
    }
    return 0;
}
fbe_status_t fbe_stat_init(fbe_stat_t * stat, fbe_u32_t interval, fbe_u32_t weight, fbe_u32_t threshold);
fbe_status_t fbe_stat_destroy(fbe_stat_t * stat);

fbe_status_t fbe_stat_io_fail(fbe_stat_t * stat, fbe_atomic_t io_counter, fbe_u64_t * error_tag, fbe_u32_t  burst_weight_reduce, fbe_u32_t weight_reduce);
fbe_status_t fbe_stat_get_ratio(fbe_stat_t * stat, fbe_atomic_t io_counter, fbe_u64_t error_tag, fbe_u32_t * ratio);


fbe_status_t fbe_stat_io_completion(fbe_object_id_t obj_id,
                                    fbe_drive_configuration_record_t * threshold_rec,
                                    fbe_drive_dieh_state_t  * dieh_state,
									fbe_drive_error_t * drive_error,
									fbe_payload_cdb_fis_error_flags_t error_flags,
                                    fbe_u8_t opcode,
                                    fbe_u32_t port_status,
                                    fbe_u32_t sk, fbe_u32_t asc, fbe_u32_t ascq, 
                                    fbe_bool_t is_emeh,  /*extended media error handling*/
                                    fbe_stat_action_t * stat_action);

fbe_u32_t fbe_stat_calculate_new_weight_change(fbe_u32_t current_percent, fbe_u32_t adjust_by_percent);

fbe_status_t    fbe_stat_drive_error_init(fbe_drive_error_t * drive_error);
void            fbe_stat_drive_error_clear(fbe_drive_error_t * drive_error);

fbe_status_t fbe_stat_drive_reset(fbe_drive_configuration_handle_t drive_configuration_handle, 
								  fbe_drive_error_t * drive_error);

void fbe_stat_reactivate_action_flags(fbe_stat_t* stat_p, fbe_stat_actions_state_t *pdo_action_state_p, fbe_u32_t pdo_ratio);

#endif /* FBE_STAT_API_H */
