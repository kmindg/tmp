#include "fbe_lifecycle_private.h"

/*
 * This is a utility function for rescheduling the monitor
 *
 * Note!  This is called from within the current monitor cycle as well as from other
 *         contexts, e.g., when setting a conditions or forcing a state change.
 */

fbe_status_t
lifecycle_reschedule(fbe_base_object_t * p_object, fbe_lifecycle_inst_base_t * p_inst_base, fbe_u32_t msecs)
{
    fbe_status_t status;

    if (msecs == 0) {
        /* Reschedule to run as soon as the scheduler will allow. */
        status = fbe_base_object_run_request(p_inst_base->p_object);
        if (status != FBE_STATUS_OK) {
            lifecycle_error(NULL,
                "%s: Object run request failed, status: 0x%X\n",
                __FUNCTION__, status);
        }
    }
    else {
        /* Reschedule to run after waiting a while. */
        if (msecs > FBE_LIFECYCLE_TIMER_RESCHED_MAX) {
            msecs = FBE_LIFECYCLE_TIMER_RESCHED_MAX;
        }
        status = fbe_base_object_reschedule_request(p_inst_base->p_object, msecs);
        if (status != FBE_STATUS_OK) {
            lifecycle_error(NULL,
                "%s: Object reschedule request failed, msecs: 0x%X, status: 0x%X\n",
                __FUNCTION__, msecs, status);
        }
    }

    /* Trace this? */
    if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
        fbe_lifecycle_trace_entry_t trace_entry;
        trace_entry.type = FBE_LIFECYCLE_TRACE_TYPE_RESCHEDULE_MONITOR;
        trace_entry.u.reschedule_monitor.msecs = msecs;
        lifecycle_trace(p_inst_base, &trace_entry);
    }

    return status;
}

/*
 * This is a utility function for running the monitor, when the object is in a
 * pending state.  This function runs the pending functions of a class.
 *
 * We want to traverse the class hierarchy from the leaf class to the base class.
 * For each class we call its pending function.
 */

fbe_status_t
lifecycle_crank_pending_object(fbe_lifecycle_const_t * p_leaf_class_const,
                               fbe_lifecycle_inst_base_t * p_inst_base,
                               fbe_lifecycle_const_base_state_t * p_base_state)
{
    fbe_lifecycle_const_t * p_this_class_const;
    fbe_lifecycle_status_t this_lifecycle_status;
    fbe_lifecycle_status_t final_lifecycle_status;
    fbe_status_t status;

    final_lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;

    /* Traverse the class hierarchy. */
    p_this_class_const = p_leaf_class_const;
    while (p_this_class_const != NULL) {
        if (p_this_class_const->p_callback->pending_func != NULL) {
            /* Call the pending function of this class. */
            this_lifecycle_status = lifecycle_run_pending_func(p_inst_base, p_this_class_const->p_callback->pending_func);
            switch (this_lifecycle_status) {
                case FBE_LIFECYCLE_STATUS_PENDING:
                    if (final_lifecycle_status != FBE_LIFECYCLE_STATUS_RESCHEDULE) {
                        final_lifecycle_status = this_lifecycle_status;
                    }
                    break;
                case FBE_LIFECYCLE_STATUS_RESCHEDULE:
                    final_lifecycle_status = this_lifecycle_status;
                    break;
                case FBE_LIFECYCLE_STATUS_DONE:
                default:
                    break;
            }
        }
        p_this_class_const = p_this_class_const->p_super;
    }

    /* Are we done with this pending state? */
    if (final_lifecycle_status == FBE_LIFECYCLE_STATUS_DONE) {
        p_inst_base->reschedule_msecs = 0;
        status = lifecycle_change_state(p_inst_base,
                                        p_base_state,
                                        p_base_state->next_state,
                                        FBE_LIFECYCLE_TRACE_TYPE_STATE_CHANGE);
        if (status != FBE_STATUS_OK) {
            return status;
        }
    }

    return FBE_STATUS_OK;
}

/*
 * This is a utility function for running the monitor, when the object is not in a
 * pending state.  This function cranks the rotary for the current state.
 *
 * We want to traverse the class hierarchy from the base class to the leaf class.
 * For each class instance we look for a rotary for the current offline state.
 * If we find a matching rotary we check to see if any of its conditions are set.
 * If we find a set condition, we run the condition function and then stop this cycle.
 */

fbe_status_t
lifecycle_crank_non_pending_object(fbe_lifecycle_const_t * p_leaf_class_const,
                                   fbe_lifecycle_inst_base_t * p_inst_base,
                                   fbe_lifecycle_const_base_state_t * p_base_state)
{
    fbe_lifecycle_const_base_t * p_const_base;
    fbe_lifecycle_const_t * p_derived_class_const;
    fbe_lifecycle_const_t * p_this_class_const;
    fbe_lifecycle_const_rotary_t * p_rotary;
    fbe_lifecycle_const_rotary_cond_t * p_rotary_cond;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_const_cond_t * p_const_cond;
    fbe_lifecycle_inst_cond_t * p_cond_inst;
    fbe_u32_t ii, rotary_cond_max;
    fbe_lifecycle_cond_id_t cond_id;
    fbe_bool_t cond_is_set;
    fbe_lifecycle_state_t this_state;
    fbe_status_t status;

    cond_is_set = FBE_FALSE;
    this_state = p_base_state->this_state;

    status = lifecycle_get_const_base_data(&p_const_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* If this is the first crank after a state change we need to do condition presets. */
    if (p_inst_base->attr & FBE_LIFECYCLE_INST_BASE_ATTR_STATE_CHANGE) {
        status = lifecycle_do_all_cond_presets(p_leaf_class_const, p_inst_base, this_state);
        if (status == FBE_STATUS_OK) {
            p_inst_base->attr &= (~FBE_LIFECYCLE_INST_BASE_ATTR_STATE_CHANGE);
        }
        if (status != FBE_STATUS_OK) {
            return status;
        }
    }

    /* Here we traverse rotaries in the class hierarcy looking for a set condition.
     * If the state changes, we are done. */
    p_this_class_const = (fbe_lifecycle_const_t*)p_const_base;
    while (p_inst_base->state == this_state) {
        /* Does this class have a rotary for this state? */
        status = lifecycle_get_class_rotary(p_this_class_const, this_state, &p_rotary);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        /* If we have a rotary, does it have any conditions that are set? */
        if (p_rotary != NULL) {
            rotary_cond_max = p_rotary->rotary_cond_max;
            /* Loop through the conditions in this rotary. */
            for (ii = 0; ii < rotary_cond_max; ii++) {
                p_rotary_cond = &p_rotary->p_rotary_cond[ii];
                p_const_cond = p_rotary_cond->p_const_cond;
                cond_id = p_const_cond->cond_id;
                status = lifecycle_is_cond_set(p_this_class_const,
                                               p_inst_base,
                                               cond_id,
                                               &cond_is_set,
                                               &p_cond_inst);
                if (status != FBE_STATUS_OK) {
                    return status;
                }
                if (cond_is_set == FBE_TRUE) {
                    break;
                }
            }
        }
        /* Did we find a set condition? */
        if (cond_is_set == FBE_TRUE) {
            break;
        }
        /* Are we at the bottom of the class hierarchy? */
        if (p_this_class_const == p_leaf_class_const) {
            /* We've cranked all of the rotaries, so we are done. */
            break;
        }
        /* Move down the class hierarchy. */
        status = lifecycle_get_derived_class_const(p_leaf_class_const, p_this_class_const, &p_derived_class_const);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        p_this_class_const = p_derived_class_const;
    }

    /* If we find a set condition, we need to process the condition. */
    if ((p_inst_base->state == this_state) && (cond_is_set == FBE_TRUE)) {
        /* Find the most derived const condition. */
        status = lifecycle_get_rotary_cond_data(p_leaf_class_const, this_state, cond_id, NULL, NULL, &p_const_cond);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        if (p_const_cond == NULL) {
            lifecycle_error(p_inst_base,
                "%s: Constant condition not found, id: 0x%X\n",
                __FUNCTION__, cond_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Get a pointer to the constant data for the base condition. */
        status = lifecycle_get_base_cond_data(p_leaf_class_const, cond_id, NULL, &p_const_base_cond);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        if (p_const_base_cond == NULL) {
            lifecycle_error(p_inst_base,
                "%s: Constant base condition not found, id: 0x%X\n",
                __FUNCTION__, cond_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (p_const_cond->p_cond_func != NULL) {
            /* Call the condition function. */
            return lifecycle_run_cond_func(p_inst_base, p_const_base_cond, p_cond_inst, p_const_cond->p_cond_func);
        }
        /* A null condition function is ok, we want to ignore the condition, but we also need to clear it. */
        status = lifecycle_clear_a_cond(p_leaf_class_const, p_inst_base, cond_id);
        p_inst_base->reschedule_msecs = 0;
        if (status != FBE_STATUS_OK) {
            return status;
        }
    }

    /* If we did not find a set condition... */
    if ((p_inst_base->state == this_state) && (cond_is_set == FBE_FALSE)) {
        /* If we are in the DESTROY state this object is ready to be destroyed. */
        if (p_inst_base->state == FBE_LIFECYCLE_STATE_DESTROY) {
            return FBE_STATUS_NO_OBJECT;
        }
        /* We may need to make a state change. */
        if ((p_base_state->type == FBE_LIFECYCLE_STATE_TYPE_TRANSITIONAL) &&
            (p_inst_base->state != p_base_state->next_state)) {
            status = lifecycle_change_state(p_inst_base,
                                            p_base_state,
                                            p_base_state->next_state,
                                            FBE_LIFECYCLE_TRACE_TYPE_STATE_CHANGE);
            if (status != FBE_STATUS_OK) {
                return status;
            }
            /* Reschedule the monitor. */
            p_inst_base->reschedule_msecs = 0;
        }
    }

    return FBE_STATUS_OK;
}
