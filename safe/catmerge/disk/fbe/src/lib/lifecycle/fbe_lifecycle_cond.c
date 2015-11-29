#include "fbe_lifecycle_private.h"

/*--- Local Condition Primitives --------------------------------------------------*/

static void
lifecycle_init_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                    fbe_lifecycle_inst_cond_t * p_cond_inst)
{
    switch(p_const_base_cond->const_cond.cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED:
            p_cond_inst->u.simple.set_count = p_cond_inst->u.simple.call_set_count = 0;
            break;
        case FBE_LIFECYCLE_COND_TYPE_BASE_TIMER:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER:
            p_cond_inst->u.timer.interval = 0;
            break;
    }
}

static void
lifecycle_set_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                   fbe_lifecycle_inst_cond_t * p_cond_inst)
{
    switch(p_const_base_cond->const_cond.cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED:
            if ((p_cond_inst->u.simple.set_count + 1) != 0) {
                p_cond_inst->u.simple.set_count += 1;
                /* Log a warning if the set_count is incremented
                 * every 500 times for a particular condition. 
                 * This happens only when cond A sets cond B but 
                 * not clearing cond A itself. */
                if(p_cond_inst->u.simple.set_count % 500 == 0)
                {
                    lifecycle_warning(NULL,
                        "%s: Set count is incremented for the cond:0x%X/%s recursively.\n",
                        __FUNCTION__, 
                        p_const_base_cond->const_cond.cond_id, 
                        p_const_base_cond->p_cond_name);
                }
            }
            break;
        case FBE_LIFECYCLE_COND_TYPE_BASE_TIMER:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER:
        {
            fbe_lifecycle_const_base_timer_cond_t * p_const_base_timer_cond;
            p_const_base_timer_cond = (fbe_lifecycle_const_base_timer_cond_t*)p_const_base_cond;
            if (p_const_base_timer_cond->canary == FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND) {
                p_cond_inst->u.timer.interval = FBE_LIFECYCLE_COND_MAX_TIMER;
            }
            else {
                lifecycle_error(NULL,
                    "%s: Corrupt timer condition cond id: 0x%X\n",
                    __FUNCTION__, p_const_base_cond->const_cond.cond_id);
            }
            break;
        }
    }
}

static void
lifecycle_groom_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                     fbe_lifecycle_inst_cond_t * p_cond_inst)
{
    switch(p_const_base_cond->const_cond.cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED:
            p_cond_inst->u.simple.call_set_count = p_cond_inst->u.simple.set_count;
            break;
		default:
            break;
    }
}

static void
lifecycle_clear_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                     fbe_lifecycle_inst_cond_t * p_cond_inst)
{
    switch(p_const_base_cond->const_cond.cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED:
            if (p_cond_inst->u.simple.call_set_count == 0) {
                p_cond_inst->u.simple.set_count = 0;
            }
            else if (p_cond_inst->u.simple.call_set_count >= p_cond_inst->u.simple.set_count) {
                p_cond_inst->u.simple.set_count = p_cond_inst->u.simple.call_set_count = 0;
            }
            else {
                p_cond_inst->u.simple.set_count -= p_cond_inst->u.simple.call_set_count;
            }
            break;
        case FBE_LIFECYCLE_COND_TYPE_BASE_TIMER:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER:
            p_cond_inst->u.timer.interval = 0;
            break;
    }
}

static fbe_bool_t
lifecycle_test_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                    fbe_lifecycle_inst_cond_t * p_cond_inst)
{
    fbe_bool_t return_value = FBE_FALSE;

    switch(p_const_base_cond->const_cond.cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED:
            return_value = (p_cond_inst->u.simple.set_count != 0) ? FBE_TRUE : FBE_FALSE;
            break;
        case FBE_LIFECYCLE_COND_TYPE_BASE_TIMER:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER:
            return_value = (p_cond_inst->u.timer.interval == FBE_LIFECYCLE_COND_MAX_TIMER) ? FBE_TRUE : FBE_FALSE;
            break;
    }
    return return_value;
}

void
lifecycle_set_timer_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                         fbe_lifecycle_inst_cond_t * p_cond_inst,
                         fbe_u32_t interval)
{
    fbe_lifecycle_const_base_timer_cond_t * p_const_base_timer_cond;

    switch(p_const_base_cond->const_cond.cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE_TIMER:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER:
            p_const_base_timer_cond = (fbe_lifecycle_const_base_timer_cond_t*)p_const_base_cond;
            if (p_const_base_timer_cond->canary == FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND) {
                if ((interval != 0) && (interval < p_const_base_timer_cond->interval)) {
                    p_cond_inst->u.timer.interval = interval;
                }
                else {
                    p_cond_inst->u.timer.interval = p_const_base_timer_cond->interval;
                }
            }
            else {
                lifecycle_error(NULL,
                    "%s: Corrupt timer condition, cond id: 0x%X\n",
                    __FUNCTION__, p_const_base_cond->const_cond.cond_id);
            }
            break;
        default:
            lifecycle_error(NULL,
                "%s: Set timer called with a condition that is not a timer, cond id: 0x%X\n",
                __FUNCTION__, p_const_base_cond->const_cond.cond_id);
    }
}

static void
lifecycle_decrement_timer_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                               fbe_lifecycle_inst_cond_t * p_cond_inst,
                               fbe_u32_t decrement_val)
{
    fbe_lifecycle_const_base_timer_cond_t * p_const_base_timer_cond;

    switch(p_const_base_cond->const_cond.cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE_TIMER:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER:
            p_const_base_timer_cond = (fbe_lifecycle_const_base_timer_cond_t*)p_const_base_cond;
            if (p_const_base_timer_cond->canary == FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND) {
                if ((p_cond_inst->u.timer.interval != 0) &&
                    (p_cond_inst->u.timer.interval != FBE_LIFECYCLE_COND_MAX_TIMER)) {
                    if (decrement_val < p_cond_inst->u.timer.interval) {
                        /* The timer has not yet expired, so just decrement by the crank interval. */
                        p_cond_inst->u.timer.interval -= decrement_val;
                    }
                    else {
                        /* The timer has expired. */
                        p_cond_inst->u.timer.interval = FBE_LIFECYCLE_COND_MAX_TIMER;
                    }
                }
            }
            else {
                lifecycle_error(NULL,
                    "%s: Corrupt timer condition, cond id: 0x%X\n",
                    __FUNCTION__, p_const_base_cond->const_cond.cond_id);
            }
            break;
        default:
            lifecycle_error(NULL,
                "%s: Decrement timer called with a condition that is not a timer, cond id: 0x%X\n",
                __FUNCTION__, p_const_base_cond->const_cond.cond_id);
    }
}

/*---------------------------------------------------------------------------------*/

/*
 * This function initializes the instance data of a function.
 */

void
lifecycle_initialize_a_cond(fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                            fbe_lifecycle_inst_cond_t * p_cond_inst)
{
    lifecycle_init_cond(p_const_base_cond, p_cond_inst);
}

/*
 * This function determines whether a condition is set.
 */

fbe_status_t
lifecycle_is_cond_set(fbe_lifecycle_const_t * p_class_const,
                      fbe_lifecycle_inst_base_t * p_inst_base,
                      fbe_lifecycle_cond_id_t cond_id,
                      fbe_bool_t * p_cond_is_set,
                      fbe_lifecycle_inst_cond_t ** pp_cond_inst)
{
    fbe_lifecycle_const_t * p_local_class_const;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_inst_cond_t * p_cond_inst;
    fbe_status_t status;

    *p_cond_is_set = FBE_FALSE;
    if (pp_cond_inst != NULL) {
        *pp_cond_inst = NULL;
    }

    /* Get a pointer to the constant data for the base condition and its local class. */
    status = lifecycle_get_base_cond_data(p_class_const, cond_id, &p_local_class_const, &p_const_base_cond);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    if (p_const_base_cond == NULL) {
        lifecycle_error(NULL,
            "%s: Condition not found in the object, cond_id: 0x%X\n",
            __FUNCTION__, cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (p_local_class_const == NULL) {
        lifecycle_error(NULL,
            "%s: Local class not found for condition, cond_id: 0x%X\n",
            __FUNCTION__, cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the class data of the class that has the base definition for the condition. */
    status = lifecycle_get_inst_cond_data(p_class_const, p_inst_base->p_object, cond_id, &p_cond_inst);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    lifecycle_lock_cond(p_inst_base);
    *p_cond_is_set = lifecycle_test_cond(p_const_base_cond, p_cond_inst);
    lifecycle_unlock_cond(p_inst_base);

    if ((*p_cond_is_set == FBE_TRUE) && (pp_cond_inst != NULL)) {
        *pp_cond_inst = p_cond_inst;
    }

    return FBE_STATUS_OK;
}

/*
 * This function clears a condition.
 */

void
lifecycle_clear_this_cond(fbe_lifecycle_inst_base_t * p_inst_base,
                          fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                          fbe_lifecycle_inst_cond_t * p_cond_inst,
						  fbe_bool_t clear_a_timer)
{
    /* Lock the state while we clear the condition. */
    lifecycle_lock_cond(p_inst_base);
	/*timer clearing (i.e. stopping the timer) was added at a later stage and we did not want to affect any existing logic
	This is why the code here is not perfect*/
	if (!clear_a_timer) {
    switch (p_const_base_cond->const_cond.cond_type) {
        case FBE_LIFECYCLE_COND_TYPE_BASE_TIMER:
        case FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER:
            lifecycle_set_timer_cond(p_const_base_cond, p_cond_inst, 0);
            break;
        default:
            lifecycle_clear_cond(p_const_base_cond, p_cond_inst);
    }
	}else{
		lifecycle_clear_cond(p_const_base_cond, p_cond_inst);
	}

    lifecycle_unlock_cond(p_inst_base);
    /* Trace this? */
    if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
        fbe_lifecycle_trace_entry_t trace_entry;
        trace_entry.type = FBE_LIFECYCLE_TRACE_TYPE_COND_CLEAR;
        trace_entry.u.condition_clear.current_state = p_inst_base->state;
        trace_entry.u.condition_clear.cond_id = p_const_base_cond->const_cond.cond_id;
        lifecycle_trace(p_inst_base, &trace_entry);
    }
}

/*
 * This is another function for clearing a condition.
 */

fbe_status_t
lifecycle_clear_a_cond(fbe_lifecycle_const_t * p_class_const,
                       fbe_lifecycle_inst_base_t * p_inst_base,
                       fbe_lifecycle_cond_id_t cond_id)
{
    fbe_lifecycle_const_t * p_local_class_const;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_inst_cond_t * p_cond_inst;
    fbe_status_t status;

    /* Get a pointer to the constant data for the base condition and its local class. */
    status = lifecycle_get_base_cond_data(p_class_const, cond_id, &p_local_class_const, &p_const_base_cond);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    if (p_const_base_cond == NULL) {
        lifecycle_error(NULL,
            "%s: Condition not found in the object, cond_id: 0x%X\n",
            __FUNCTION__, cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (p_local_class_const == NULL) {
        lifecycle_error(NULL,
            "%s: Local class not found for condition, cond_id: 0x%X\n",
            __FUNCTION__, cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the instance data of the class that has the base definition for the condition. */
    status = lifecycle_get_inst_cond_data(p_class_const, p_inst_base->p_object, cond_id, &p_cond_inst);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Clear the condition. */
    lifecycle_clear_this_cond(p_inst_base, p_const_base_cond, p_cond_inst, FBE_FALSE);

    return FBE_STATUS_OK;
}

/*
 * This function sets a condition.
 */

fbe_status_t
lifecycle_set_a_cond(fbe_lifecycle_const_t * p_class_const,
                     fbe_base_object_t * p_object,
                     fbe_lifecycle_cond_id_t cond_id)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_lifecycle_const_base_state_t * p_base_state;
    fbe_lifecycle_const_base_state_t * p_next_state;
    fbe_lifecycle_const_t * p_local_class_const;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_const_rotary_cond_t * p_const_rotary_cond;
    fbe_lifecycle_inst_cond_t * p_cond_inst;
    fbe_lifecycle_inst_base_attr_t attr;
    fbe_lifecycle_state_t next_state;
    fbe_bool_t redo_presets_check;
    fbe_status_t status;

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the constant data for the base condition and its local class. */
    status = lifecycle_get_base_cond_data(p_class_const, cond_id, &p_local_class_const, &p_const_base_cond);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    if (p_const_base_cond == NULL) {
        lifecycle_error(p_inst_base,
            "%s: Condition not found in the object, cond_id: 0x%X\n",
            __FUNCTION__, cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (p_local_class_const == NULL) {
        lifecycle_error(p_inst_base,
            "%s: Local class not found for condition, cond_id: 0x%X\n",
            __FUNCTION__, cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the base state data for the current state of this object. */
    status = lifecycle_get_base_state_data(p_inst_base, p_inst_base->state, &p_base_state);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    if (p_base_state == NULL) {
        lifecycle_error(p_inst_base,
            "%s: Base entry not found for state: 0x%X\n",
            __FUNCTION__, p_inst_base->state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the state transition for the current state. */
    if (p_base_state->this_state < FBE_LIFECYCLE_STATE_NON_PENDING_MAX) {
        next_state = p_const_base_cond->p_state_transition[p_base_state->this_state];
    }
    else if ((p_base_state->type == FBE_LIFECYCLE_STATE_TYPE_PENDING) &&
             (p_base_state->next_state < FBE_LIFECYCLE_STATE_NON_PENDING_MAX)) {
        /* When we are in a pending state the transition we need is determined based on the
         * next state of the pending state. */
        next_state = p_const_base_cond->p_state_transition[p_base_state->next_state];
    }
    else {
        const char * p_state_name = (p_base_state->p_name != NULL) ? p_base_state->p_name : "<null>";
        const char * p_cond_name = (p_const_base_cond->p_cond_name != NULL) ? p_const_base_cond->p_cond_name : "<null>";
        lifecycle_error(p_inst_base,
            "%s:Invalid cur state find,state:%s cond_id:%s\n",
            __FUNCTION__, p_state_name, p_cond_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Is this a valid state transition? */
    if (next_state == FBE_LIFECYCLE_STATE_INVALID) {
        const char * p_state_name = (p_base_state->p_name != NULL) ? p_base_state->p_name : "<null>";
        const char * p_cond_name = (p_const_base_cond->p_cond_name != NULL) ? p_const_base_cond->p_cond_name : "<null>";
        lifecycle_error(p_inst_base,
            "%s:Can't set cond in the cur state,state:%s,cond_id:%s\n",
            __FUNCTION__, p_state_name, p_cond_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if (next_state >= FBE_LIFECYCLE_STATE_NON_PENDING_MAX) {
        const char * p_state_name = (p_base_state->p_name != NULL) ? p_base_state->p_name : "<null>";
        const char * p_cond_name = (p_const_base_cond->p_cond_name != NULL) ? p_const_base_cond->p_cond_name : "<null>";
        lifecycle_error(p_inst_base,
            "%s:Invalid next state find, "
            "cur_state:%s,next_state:0x%X,cond_id:%s\n",
            __FUNCTION__, p_state_name, next_state, p_cond_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If a state transition is needed, then validate the next state. */
    if (p_base_state->this_state != next_state) {
        /* Get a pointer to the base state data for the next state. */
        status = lifecycle_get_base_state_data(p_inst_base, next_state, &p_next_state);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        if (p_next_state == NULL) {
            lifecycle_error(p_inst_base,
                "%s: Base entry not found for state: 0x%X\n",
                __FUNCTION__, next_state);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* If the current state is a pending state, then the state transition may be circle back
         * to this current state!  */
        if ((p_base_state->type == FBE_LIFECYCLE_STATE_TYPE_PENDING) &&
            (p_next_state->pending_state == p_base_state->this_state)) {
            next_state = p_base_state->this_state;
        }
    }

    /* Get a pointer to the instance data for the condition. */
    status = lifecycle_get_inst_cond_data(p_local_class_const, p_object, cond_id, &p_cond_inst);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    lifecycle_lock_cond(p_inst_base);
    /* If this condition is not currently set, then we may need to check for a REDO_PRESET
     * attribute on the rotary condition (if there is an applicable rotary condition. */
    redo_presets_check = (lifecycle_test_cond(p_const_base_cond, p_cond_inst) == FBE_FALSE) ? FBE_TRUE : FBE_FALSE;
    /* When the base condition has the NOSET attribute, we don't actually set the condition.
     * We just want to do a state transition or possibly redo the presets. */
    if ((p_const_base_cond->const_cond.cond_attr & FBE_LIFECYCLE_CONST_COND_ATTR_NOSET) == 0) {
        /* When the base condition does not have NOSET attribute, we do need to set the condition. */
        lifecycle_set_cond(p_const_base_cond, p_cond_inst);
        /* Trace this? */
    }

    if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
        fbe_lifecycle_trace_entry_t trace_entry;
        trace_entry.type = FBE_LIFECYCLE_TRACE_TYPE_COND_SET;
        trace_entry.u.condition_set.current_state = p_inst_base->state;
        trace_entry.u.condition_set.cond_id = cond_id;
        lifecycle_trace(p_inst_base, &trace_entry);
    }

    lifecycle_unlock_cond(p_inst_base);

    /* We may need to change the state and we need to decide this while holding the lock. */
    if (p_base_state->this_state != next_state) {
        (void)lifecycle_change_state(p_inst_base,
                                     p_base_state,
                                     next_state,
                                     FBE_LIFECYCLE_TRACE_TYPE_COND_STATE_CHANGE);
    }
    else if (redo_presets_check == FBE_TRUE) {
        /* If we don't change the state then we may want to redo the presets for the current state. */
        status = lifecycle_get_rotary_cond_data(p_class_const,
                                                p_base_state->this_state,
                                                cond_id,
                                                NULL,
                                                &p_const_rotary_cond,
                                                NULL);
        /* It is not an error if we don't find a rotary condition in the current rotary. */
        if ((status == FBE_STATUS_OK) && (p_const_rotary_cond != NULL)) {
            if ((p_const_rotary_cond->attr & FBE_LIFECYCLE_ROTARY_COND_ATTR_REDO_PRESETS) != 0) {
                (void)lifecycle_do_all_cond_presets(p_class_const, p_inst_base, p_base_state->this_state);
            }
        }
    }

    /* We want to reschedule the object's monitor. If it is currently running,
     * we just update the msecs. */
    lifecycle_lock_state(p_inst_base);
    attr = p_inst_base->attr;
    /* this object needs attention right away, reschedule after current condition finishes */
    p_inst_base->reschedule_msecs = 0;
   
    lifecycle_unlock_state(p_inst_base);

    /* If the object's monitor is not running we reschedule it to start up. */
    if ((attr & FBE_LIFECYCLE_INST_BASE_ATTR_IN_CRANK) == 0) {
        (void)lifecycle_reschedule(p_object, p_inst_base, 0);
    }

    return FBE_STATUS_OK;
}

/*
 * This function does condition presets in a given rotary
 */

fbe_status_t
lifecycle_do_cond_presets(fbe_lifecycle_const_t * p_class_const,
                          fbe_lifecycle_const_rotary_t * p_rotary,
                          fbe_lifecycle_inst_base_t * p_inst_base)
{
    fbe_u32_t ii, rotary_cond_max;
    fbe_lifecycle_const_rotary_cond_t * p_rotary_cond;
    fbe_lifecycle_const_t * p_local_class_const;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_inst_cond_t * p_cond_inst;
    fbe_lifecycle_cond_id_t cond_id;
    fbe_status_t status;

    /* Loop through the conditions in the rotary. */
    rotary_cond_max = p_rotary->rotary_cond_max;
    for (ii = 0; ii < rotary_cond_max; ii++) {
        p_rotary_cond = &p_rotary->p_rotary_cond[ii];
        /* Is this a preset condition? */
        if (p_rotary_cond->attr & FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET) {
            cond_id = p_rotary_cond->p_const_cond->cond_id;
            /* Get a pointer to the constant data for the base condition and its local class. */
            status = lifecycle_get_base_cond_data(p_class_const,
                                                  cond_id,
                                                  &p_local_class_const,
                                                  &p_const_base_cond);
            if (status != FBE_STATUS_OK) {
                return status;
            }
            if (p_const_base_cond == NULL) {
                lifecycle_error(NULL,
                    "%s: Condition not found in the object, cond_id: 0x%X\n",
                    __FUNCTION__, cond_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if (p_local_class_const == NULL) {
                lifecycle_error(NULL,
                    "%s: Local class not found for condition, cond_id: 0x%X\n",
                    __FUNCTION__, cond_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Get a pointer to the instance data for the condition. */
            status = lifecycle_get_inst_cond_data(p_local_class_const,
                                                  p_inst_base->p_object,
                                                  cond_id,
                                                  &p_cond_inst);
            if (status != FBE_STATUS_OK) {
                return status;
            }
            lifecycle_lock_cond(p_inst_base);
            if (lifecycle_test_cond(p_const_base_cond, p_cond_inst) == FBE_FALSE) {
                lifecycle_set_cond(p_const_base_cond, p_cond_inst);
                /* Trace this? */
                if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
                    fbe_lifecycle_trace_entry_t trace_entry;
                    trace_entry.type = FBE_LIFECYCLE_TRACE_TYPE_COND_PRESET;
                    trace_entry.u.condition_set.current_state = p_inst_base->state;
                    trace_entry.u.condition_set.cond_id = cond_id;
                    lifecycle_trace(p_inst_base, &trace_entry);
                }
            }
            lifecycle_unlock_cond(p_inst_base);
        }
    }
    return FBE_STATUS_OK;
}

/*
 * This is a utility function for presetting conditions when the state changes.
 *
 * We want to traverse the class hierarchy from the leaf class to the base class.
 * For each class instance we look for a rotary for the current offline state.
 * If we find a matching rotary we check to see if any of its conditions are not set,
 * have the preset attribute.  If we find a such a condition, we set it.
 */

fbe_status_t
lifecycle_do_all_cond_presets(fbe_lifecycle_const_t * p_leaf_class_const,
                              fbe_lifecycle_inst_base_t * p_inst_base,
                              fbe_lifecycle_state_t this_state)
{
    fbe_lifecycle_const_t * p_this_class_const;
    fbe_lifecycle_const_rotary_t * p_rotary;
    fbe_status_t status;

    p_this_class_const = p_leaf_class_const;
    while (p_this_class_const != NULL) {
        /* Does this class have a rotary for this state? */
        status = lifecycle_get_class_rotary(p_this_class_const, this_state, &p_rotary);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        /* If we have a rotary, does it have any conditions that are set? */
        if (p_rotary != NULL) {
            status = lifecycle_do_cond_presets(p_this_class_const, p_rotary, p_inst_base);
            if (status != FBE_STATUS_OK) {
                return status;
            }
        }
        p_this_class_const = p_this_class_const->p_super;
    }
    return FBE_STATUS_OK;
}

/*
 * This function decrements timer conditions (before cranking a rotary).
 *
 * We want to traverse the class hierarchy from the leaf class to the base class.
 * For each class instance we look for base timer conditions.  Base timer conditions 
 * are decremented by the interval between monitor cycles.
 */

fbe_status_t
lifecycle_decrement_all_timer_cond(fbe_lifecycle_const_t * p_leaf_class_const,
                                   fbe_lifecycle_inst_base_t * p_inst_base)
{
    fbe_lifecycle_const_t * p_this_class_const;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_inst_cond_t * p_cond_inst;
    fbe_u32_t now_interval;
    fbe_u32_t ii;

    /* Determine how long its been since the last crank. */
    now_interval = lifecycle_crank_interval(p_inst_base);
    if (now_interval == 0) {
        return FBE_STATUS_OK;
    }

    p_this_class_const = p_leaf_class_const;
    while (p_this_class_const != NULL) {
        /* Does this class have any base conditions? */
        if (p_this_class_const->p_base_cond_array->base_cond_max > 0) {
            p_cond_inst = (*p_this_class_const->p_callback->get_inst_cond)(p_inst_base->p_object);
            /* Are any of these base timer conditions? */
            for (ii = 0; ii < p_this_class_const->p_base_cond_array->base_cond_max; ii++) {
                p_const_base_cond = p_this_class_const->p_base_cond_array->pp_base_cond[ii];
                if (p_const_base_cond->const_cond.cond_type == FBE_LIFECYCLE_COND_TYPE_BASE_TIMER) {
                    lifecycle_decrement_timer_cond(p_const_base_cond, &p_cond_inst[ii], now_interval);
                }
            }
        }
        p_this_class_const = p_this_class_const->p_super;
    }
    return FBE_STATUS_OK;
}

/*
 * This is the condition completion function.
 */

static fbe_status_t 
lifecycle_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_object_t * p_object = (fbe_base_object_t*)context;
    fbe_base_object_t * base_object = NULL;
    const char * p_cond_name = NULL;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_inst_cond_t * p_cond_inst;
    fbe_status_t status;

    /* There were cases where the completion context gets corrupted in the packet stack.
     * Reconstruct the object pointer in case of corruption and log critical errors to avoid system panic.
     */
    base_object = (fbe_base_object_t *)((fbe_u8_t *)(packet->stack[0].completion_context) - 
                                        (fbe_u8_t *)(&((fbe_base_object_t *)0)->scheduler_element));

    if(base_object != p_object)
    {
        /* Assign the newly reconstructued pointer to p_object */
        p_object = base_object;

        /* Get a pointer to the instance data of the base class. */
        status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
        if (status != FBE_STATUS_OK) {
            lifecycle_error(NULL,
                "%s: Can't perform pending completion function, status: 0x%X\n",
                __FUNCTION__, status);
            return FBE_STATUS_OK;
        }

        p_const_base_cond = p_inst_base->p_const_base_cond;
        fbe_base_object_get_object_id(base_object, &object_id);

        lifecycle_error(p_inst_base,
            "%s: Reconstructed the Object Pointer. Packet: %p/0x%p\n",
            __FUNCTION__, packet, context);

        if (p_const_base_cond != NULL)
        {    
            p_cond_name = (p_const_base_cond->p_cond_name != NULL) ? p_const_base_cond->p_cond_name : "<null>";            
       
        lifecycle_error(p_inst_base,
            "%s: Obj: %X, cond_id: 0x%X/%s.\n",
            __FUNCTION__, object_id, p_const_base_cond->const_cond.cond_id, p_cond_name);
    }
    else
    {
            lifecycle_error(p_inst_base,
                "%s: Obj: %X, cond_id: UNKNOWN (p_const_base_cond is NULL).\n",
                __FUNCTION__, object_id);
        }
 
    }
    else
    {
        /* Get a pointer to the instance data of the base class. */
        status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
        if (status != FBE_STATUS_OK) {
            lifecycle_error(NULL,
                "%s: Can't perform pending completion function, status: 0x%X\n",
                __FUNCTION__, status);
            return FBE_STATUS_OK;
        }
    }

    /* Should we clear the condition? */
    p_cond_inst = p_inst_base->p_cond_inst;
    p_const_base_cond = p_inst_base->p_const_base_cond;
    p_inst_base->p_cond_inst = NULL;
    p_inst_base->p_const_base_cond = NULL;

       
    if (p_inst_base->attr & FBE_LIFECYCLE_INST_BASE_ATTR_CLEAR_CUR_COND) {
        p_inst_base->attr &= (~FBE_LIFECYCLE_INST_BASE_ATTR_CLEAR_CUR_COND);
        if (p_cond_inst != NULL && p_const_base_cond != NULL)
        {
            lifecycle_clear_this_cond(p_inst_base, p_const_base_cond, p_cond_inst, FBE_FALSE);
            /* When we clear a condition, we reschedule the monitor to run immediately. */
            p_inst_base->reschedule_msecs = 0;
        }
        else
        {
            lifecycle_error(p_inst_base,
                "%s: Obj: %X, Condition was already cleared.  We should not get here.\n",
                __FUNCTION__, object_id);
        }
    }


    /* Reschedule the monitor. */
    (void)lifecycle_reschedule(p_object, p_inst_base, p_inst_base->reschedule_msecs);

    if (p_inst_base->p_packet != NULL) {
        /* Set the packet status. */
        fbe_transport_set_status(p_inst_base->p_packet, FBE_STATUS_OK, 0);
        /* Forget about the current monitor control packet. */
        p_inst_base->p_packet = NULL;
    }

    lifecycle_lock_state(p_inst_base);
    p_inst_base->attr &= (~FBE_LIFECYCLE_INST_BASE_ATTR_IN_CRANK);
    lifecycle_unlock_state(p_inst_base);

    /* Set our last crank timestamp. */
    p_inst_base->last_crank_time = fbe_get_time();

    /* Trace this? */
    if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
        fbe_lifecycle_trace_entry_t trace_entry;
        trace_entry.type = FBE_LIFECYCLE_TRACE_TYPE_CRANK_END;
        trace_entry.u.crank_end.ending_state = p_inst_base->state;
        lifecycle_trace(p_inst_base, &trace_entry);
    }

    return FBE_STATUS_OK;
}

/*
 * This runs a condition function.
 */

fbe_status_t
lifecycle_run_cond_func(fbe_lifecycle_inst_base_t * p_inst_base,
                        fbe_lifecycle_const_base_cond_t * p_const_base_cond,
                        fbe_lifecycle_inst_cond_t * p_cond_inst,
                        const fbe_lifecycle_func_cond_t cond_func)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_status_t status, return_status;

    if ((p_inst_base == NULL) || (p_inst_base->p_packet == NULL)){
        lifecycle_error(p_inst_base,
            "%s: Invalid parameter \n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;

    }
    /* Push our completion function onto the packet's completion stack. */
    status = fbe_transport_set_completion_function(p_inst_base->p_packet,
                                                   lifecycle_cond_completion,
                                                   (fbe_packet_completion_context_t)p_inst_base->p_object);
    if (status != FBE_STATUS_OK) {
        lifecycle_error(p_inst_base,
            "%s: Can't set condition completion function, status: 0x%X\n",
            __FUNCTION__, status);
        return status;
    }

    /* Clear the clear current condition attribute. */
    p_inst_base->attr &= (~FBE_LIFECYCLE_INST_BASE_ATTR_CLEAR_CUR_COND);
    /* Set the current condition pointers. */
    p_inst_base->p_cond_inst = p_cond_inst;
    p_inst_base->p_const_base_cond = p_const_base_cond;

    if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
        fbe_lifecycle_trace_entry_t trace_entry;
        trace_entry.type = FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_BEFORE;
        trace_entry.u.condition_run_before.cond_id = p_const_base_cond->const_cond.cond_id;
        lifecycle_trace(p_inst_base, &trace_entry);
    }

    /* Call the condition function. */
	lifecycle_groom_cond(p_const_base_cond, p_cond_inst);
    lifecycle_status = cond_func(p_inst_base->p_object, p_inst_base->p_packet);

    if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
        fbe_lifecycle_trace_entry_t trace_entry;
        trace_entry.type = FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_AFTER;
        trace_entry.u.condition_run_after.cond_id = p_const_base_cond->const_cond.cond_id;
        trace_entry.u.condition_run_after.status = lifecycle_status;
        lifecycle_trace(p_inst_base, &trace_entry);
    }

    return_status = FBE_STATUS_OK;
    switch(lifecycle_status) {
        case FBE_LIFECYCLE_STATUS_DONE:
            break;
        case FBE_LIFECYCLE_STATUS_PENDING:
            return_status = FBE_STATUS_PENDING;
            break;
        case FBE_LIFECYCLE_STATUS_RESCHEDULE:
            /* Reschedule the monitor. */
            p_inst_base->reschedule_msecs = 0;
            break;
        default:
            lifecycle_error(p_inst_base,
                "%s: Unknown condition function return, status code: 0x%X\n",
                __FUNCTION__, lifecycle_status);
            return_status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* always call the completion function to clean up ... */

    return return_status;
}

fbe_status_t lifecycle_stop_timer(fbe_lifecycle_const_t * p_class_const,
                                    fbe_lifecycle_inst_base_t * p_inst_base,
                                    fbe_lifecycle_cond_id_t cond_id)
{
	fbe_lifecycle_const_t * p_local_class_const;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_inst_cond_t * p_cond_inst;
    fbe_status_t status;

    /* Get a pointer to the constant data for the base condition and its local class. */
    status = lifecycle_get_base_cond_data(p_class_const, cond_id, &p_local_class_const, &p_const_base_cond);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    if (p_const_base_cond == NULL) {
        lifecycle_error(NULL,
            "%s: Condition not found in the object, cond_id: 0x%X\n",
            __FUNCTION__, cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (p_local_class_const == NULL) {
        lifecycle_error(NULL,
            "%s: Local class not found for condition, cond_id: 0x%X\n",
            __FUNCTION__, cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the instance data of the class that has the base definition for the condition. */
    status = lifecycle_get_inst_cond_data(p_class_const, p_inst_base->p_object, cond_id, &p_cond_inst);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Clear the condition. */
    lifecycle_clear_this_cond(p_inst_base, p_const_base_cond, p_cond_inst, FBE_TRUE);/*using FBE_TRUE will stop the timer*/

    return FBE_STATUS_OK;

}
