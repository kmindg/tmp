#ifndef FBE_LIFECYCLE_PRIVATE_H
#define FBE_LIFECYCLE_PRIVATE_H

#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_base.h"
#include "fbe_trace.h"
#include "fbe_lifecycle.h"
#include "base_object_private.h"


#define FBE_LIFECYCLE_DEFAULT_TIMER 3000 /* 3 sec. by default */


/*--- Private Inlines -------------------------------------------------------------*/

__forceinline static void
lifecycle_lock_state(fbe_lifecycle_inst_base_t * p_inst_base)
{
    fbe_spinlock_lock(&p_inst_base->state_lock);
}

__forceinline static void
lifecycle_unlock_state(fbe_lifecycle_inst_base_t * p_inst_base)
{
    fbe_spinlock_unlock(&p_inst_base->state_lock);
}

__forceinline static void
lifecycle_lock_trace(fbe_lifecycle_inst_base_t * p_inst_base)
{
    fbe_spinlock_lock(&p_inst_base->trace_lock);
}

__forceinline static void
lifecycle_unlock_trace(fbe_lifecycle_inst_base_t * p_inst_base)
{
    fbe_spinlock_unlock(&p_inst_base->trace_lock);
}

__forceinline static void
lifecycle_lock_cond(fbe_lifecycle_inst_base_t * p_inst_base)
{
    fbe_spinlock_lock(&p_inst_base->cond_lock);
}

__forceinline static void
lifecycle_unlock_cond(fbe_lifecycle_inst_base_t * p_inst_base)
{
    fbe_spinlock_unlock(&p_inst_base->cond_lock);
}

__forceinline static fbe_bool_t
lifecycle_is_trace_enabled(fbe_lifecycle_inst_base_t * p_inst_base)
{
    return (p_inst_base->trace_flags != 0) ? FBE_TRUE : FBE_FALSE;
}

__forceinline static fbe_u32_t
lifecycle_crank_interval(fbe_lifecycle_inst_base_t * p_inst_base)
{
    fbe_time_t now_time;
    fbe_u32_t now_interval;

    now_interval = 0;
    if (p_inst_base != NULL) {
        now_time = fbe_get_time();
        if (now_time > p_inst_base->last_crank_time) {
            now_interval = (fbe_u32_t)((now_time - p_inst_base->last_crank_time)/10);
        }
    }
    return now_interval;
}

/*--- condition prototypes --------------------------------------------------------*/

void
lifecycle_set_timer_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                         fbe_lifecycle_inst_cond_t * p_cond_inst,
                         fbe_u32_t interval);

void lifecycle_initialize_a_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                                 fbe_lifecycle_inst_cond_t * p_cond_inst);

fbe_status_t lifecycle_is_cond_set(fbe_lifecycle_const_t * p_class_const,
                                   fbe_lifecycle_inst_base_t * p_inst_base,
                                   fbe_lifecycle_cond_id_t cond_id,
                                   fbe_bool_t * p_cond_is_set,
                                   fbe_lifecycle_inst_cond_t ** pp_cond_inst);

void lifecycle_clear_this_cond(fbe_lifecycle_inst_base_t * p_inst_base,
                               fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                               fbe_lifecycle_inst_cond_t * p_cond_inst,
							   fbe_bool_t clear_a_timer);

fbe_status_t lifecycle_clear_a_cond(fbe_lifecycle_const_t * p_class_const,
                                    fbe_lifecycle_inst_base_t * p_inst_base,
                                    fbe_lifecycle_cond_id_t cond_id);

fbe_status_t lifecycle_set_a_cond(fbe_lifecycle_const_t * p_class_const,
                                  fbe_base_object_t * p_object,
                                  fbe_lifecycle_cond_id_t cond_id);

fbe_status_t lifecycle_do_cond_presets(fbe_lifecycle_const_t * p_class_const,
                                       fbe_lifecycle_const_rotary_t * p_rotary,
                                       fbe_lifecycle_inst_base_t * p_inst_base);

fbe_status_t lifecycle_do_all_cond_presets(fbe_lifecycle_const_t * p_leaf_class_const,
                                           fbe_lifecycle_inst_base_t * p_inst_base,
                                           fbe_lifecycle_state_t this_state);

fbe_status_t lifecycle_decrement_all_timer_cond(fbe_lifecycle_const_t * p_leaf_class_const,
                                                fbe_lifecycle_inst_base_t * p_inst_base);

fbe_status_t lifecycle_run_cond_func(fbe_lifecycle_inst_base_t * p_inst_base,
                                     fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                                     fbe_lifecycle_inst_cond_t * p_cond_inst,
                                     const fbe_lifecycle_func_cond_t cond_func);

fbe_status_t lifecycle_stop_timer(fbe_lifecycle_const_t * p_class_const,
                                    fbe_lifecycle_inst_base_t * p_inst_base,
                                    fbe_lifecycle_cond_id_t cond_id);

/*--- crank prototypes ------------------------------------------------------------*/

fbe_status_t lifecycle_reschedule(fbe_base_object_t * p_object,
                                  fbe_lifecycle_inst_base_t * p_inst_base,
                                  fbe_u32_t msecs);

fbe_status_t lifecycle_crank_pending_object(fbe_lifecycle_const_t * p_leaf_class_const,
                                            fbe_lifecycle_inst_base_t * p_inst_base,
                                            fbe_lifecycle_const_base_state_t * p_base_state);

fbe_status_t lifecycle_crank_non_pending_object(fbe_lifecycle_const_t * p_leaf_class_const,
                                                fbe_lifecycle_inst_base_t * p_inst_base,
                                                fbe_lifecycle_const_base_state_t * p_base_state);

/*--- getter prototypes -----------------------------------------------------------*/

fbe_status_t lifecycle_get_const_base_data(fbe_lifecycle_const_base_t ** pp_const_base);

fbe_status_t lifecycle_get_inst_base_data(fbe_base_object_t * p_object,
                                          fbe_lifecycle_inst_base_t ** pp_inst_base);

fbe_status_t lifecycle_get_base_state_data(fbe_lifecycle_inst_base_t * p_inst_base,
                                           fbe_lifecycle_state_t state,
                                           fbe_lifecycle_const_base_state_t ** pp_base_state);

fbe_status_t lifecycle_get_class_const_data(fbe_lifecycle_const_t * p_class_const,
                                            fbe_class_id_t class_id,
                                            fbe_lifecycle_const_t ** pp_class_const);

fbe_status_t lifecycle_get_inst_cond_data(fbe_lifecycle_const_t * p_lower_class_const,
                                          fbe_base_object_t * p_object,
                                          fbe_lifecycle_cond_id_t cond_id,
                                          fbe_lifecycle_inst_cond_t ** pp_cond_inst);

fbe_status_t lifecycle_get_cond_data(fbe_lifecycle_const_rotary_t * p_rotary,
                                     fbe_lifecycle_cond_id_t cond_id,
                                     fbe_lifecycle_const_rotary_cond_t ** pp_const_rotary_cond,
                                     fbe_lifecycle_const_cond_t ** pp_const_cond);

fbe_status_t lifecycle_get_rotary_cond_data(fbe_lifecycle_const_t * p_class_const,
                                            fbe_lifecycle_state_t state,
                                            fbe_lifecycle_cond_id_t cond_id,
                                            fbe_lifecycle_const_t ** pp_class_const,
                                            fbe_lifecycle_const_rotary_cond_t ** pp_const_rotary_cond,
                                            fbe_lifecycle_const_cond_t ** pp_const_cond);

fbe_status_t lifecycle_get_base_cond_data(fbe_lifecycle_const_t * p_leaf_class_const,
                                          fbe_lifecycle_cond_id_t cond_id,
                                          fbe_lifecycle_const_t ** pp_class_const,
                                          fbe_lifecycle_const_base_cond_t ** pp_const_base_cond);

fbe_status_t lifecycle_get_derived_class_const(fbe_lifecycle_const_t * p_leaf_class_const,
                                               fbe_lifecycle_const_t * p_super_class_const,
                                               fbe_lifecycle_const_t ** pp_derived_class_const);

fbe_status_t lifecycle_get_class_rotary(fbe_lifecycle_const_t * p_class_const,
                                        fbe_lifecycle_state_t state,
                                        fbe_lifecycle_const_rotary_t ** pp_rotary);

/*--- init prototypes -------------------------------------------------------------*/

fbe_status_t lifecycle_init_inst_data(fbe_lifecycle_const_t * p_class_const,
                                      fbe_lifecycle_inst_t * p_class_inst,
                                      struct fbe_base_object_s * p_object);

/*--- init prototypes -------------------------------------------------------------*/

fbe_status_t lifecycle_init_inst_data(fbe_lifecycle_const_t * p_class_const,
                                      fbe_lifecycle_inst_t * p_class_inst,
                                      struct fbe_base_object_s * p_object);

/*--- pending prototypes ----------------------------------------------------------*/

fbe_lifecycle_status_t
lifecycle_run_pending_func(fbe_lifecycle_inst_base_t * p_inst_base,
                           const fbe_lifecycle_func_pending_t pending_func);

/*--- state prototypes ------------------------------------------------------------*/

fbe_status_t
lifecycle_change_state(fbe_lifecycle_inst_base_t * p_inst_base,
                       fbe_lifecycle_const_base_state_t * p_old_base_state,
                       fbe_lifecycle_state_t requested_state,
                       fbe_lifecycle_trace_type_t trace_type);

/*--- trace prototypes ------------------------------------------------------------*/

void lifecycle_error(fbe_lifecycle_inst_base_t * p_inst_base,
                     const char * fmt, ...)
                     __attribute__((format(__printf_func__,2,3)));

void lifecycle_warning(fbe_lifecycle_inst_base_t * p_inst_base,
                     const char * fmt, ...)
                     __attribute__((format(__printf_func__,2,3)));

void lifecycle_log_info(fbe_lifecycle_inst_base_t * p_inst_base,
                        const char * fmt, ...)
                     __attribute__((format(__printf_func__,2,3)));

void lifecycle_log_trace_entry(fbe_lifecycle_inst_base_t * p_inst_base,
                               fbe_lifecycle_trace_entry_t * p_entry);

void lifecycle_trace(fbe_lifecycle_inst_base_t * p_inst_base,
                     fbe_lifecycle_trace_entry_t * p_trace_entry);

/*--- verify prototypes -----------------------------------------------------------*/

fbe_status_t lifecycle_verify(fbe_lifecycle_const_t * p_class_const,
                              fbe_base_object_t * p_object);

fbe_status_t lifecycle_verify_class_const_data(fbe_lifecycle_const_t * p_leaf_class_const);

#endif /*FBE_LIFECYCLE_PRIVATE_H */
