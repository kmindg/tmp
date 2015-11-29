#include "fbe_lifecycle_private.h"
#include "fbe_notification.h"
#include "fbe_topology.h"
#include "fbe/fbe_notification_lib.h"

/*
 * This is a utility function for changing the lifecycle state (in a consistent way).
 *
 * NOTE!  If the state changed (before we take the state lock) then we won't change the state.
 *        This is not an error and caller's must expect this behavior!
 */

fbe_status_t
lifecycle_change_state(fbe_lifecycle_inst_base_t * p_inst_base,
                       fbe_lifecycle_const_base_state_t * p_old_base_state,
                       fbe_lifecycle_state_t requested_state,
                       fbe_lifecycle_trace_type_t trace_type)
{
    fbe_lifecycle_const_base_state_t * p_new_base_state;
    fbe_lifecycle_state_t new_state;
    fbe_object_id_t object_id;
    fbe_status_t status;
    fbe_u32_t ii;
	fbe_notification_info_t notification_info;
	fbe_class_id_t class_id;
	fbe_handle_t object_handle;
	

    /* Is this a permitted state change? */
    for (ii = 0; ii < p_old_base_state->transition_max; ii++) {
        if (requested_state == p_old_base_state->p_transition_states[ii]) {
            break;
        }
    }
    if ((ii >= p_old_base_state->transition_max) && (trace_type != FBE_LIFECYCLE_TRACE_TYPE_FORCED_STATE_CHANGE)) {
        /* This is not a permitted state change... */
        if (p_inst_base->p_object != NULL) {
            fbe_base_object_get_object_id(p_inst_base->p_object, &object_id);
            lifecycle_error(NULL,
                "%s: State change not permitted: 0x%X => 0x%X for object_id: 0x%X\n",
                __FUNCTION__, p_old_base_state->this_state, requested_state, object_id);
        }
        else{
            lifecycle_error(NULL,
                "%s: State change not permitted: 0x%X => 0x%X\n",
                __FUNCTION__, p_old_base_state->this_state, requested_state);
        }
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the base state data for the new state. */
    status = lifecycle_get_base_state_data(p_inst_base, requested_state, &p_new_base_state);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    if (p_new_base_state == NULL) {
        lifecycle_error(p_inst_base,
            "%s: Base entry not found for state: 0x%X\n",
            __FUNCTION__, p_inst_base->state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Does this state change require a pending state transition? */
    if (p_old_base_state->type == FBE_LIFECYCLE_STATE_TYPE_PENDING) {
        /* If the new state requires a pending transition, but the current state
         * is already the specified pending state, then we already did the pending
         * transition -- so we now go to the requested state. */
        if (p_new_base_state->pending_state == p_old_base_state->this_state) {
            new_state = p_new_base_state->this_state;
        }
        else {
            new_state = p_new_base_state->pending_state;
        }
    }
    else if (p_new_base_state->pending_state != FBE_LIFECYCLE_STATE_INVALID) {
        /* If the current state is not a pending state and the requested state
         * requires a pending transition, then we go to the pending state. */
        new_state = p_new_base_state->pending_state;
    }
    else {
        /* If the current state is not a pending state and the requested state
         * does not specify a pending transition, then we go to the requested state. */
        new_state = p_new_base_state->this_state;
    }

    lifecycle_lock_state(p_inst_base);
    if (p_inst_base->state == p_old_base_state->this_state) {
        /* Change the state. */
        p_inst_base->state = new_state;
        p_inst_base->attr |= FBE_LIFECYCLE_INST_BASE_ATTR_STATE_CHANGE;
        /* Trace this? */
        if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
            fbe_lifecycle_trace_entry_t trace_entry;
            trace_entry.type = trace_type;
            trace_entry.u.state_change.old_state = p_old_base_state->this_state;
            trace_entry.u.state_change.new_state = p_inst_base->state;
            lifecycle_trace(p_inst_base, &trace_entry);
        }
        lifecycle_unlock_state(p_inst_base);

        /* Send out a notification for this state change. */
        if (p_inst_base->p_object != NULL) {
			status  = fbe_notification_convert_state_to_notification_type(new_state, &notification_info.notification_type);
			if (status != FBE_STATUS_OK) {
				lifecycle_error(p_inst_base,
                    "%s: can't map new state, new state: 0x%X\n, status: 0x%X",
                    __FUNCTION__, new_state, status);

				return FBE_STATUS_OK;
			}

            fbe_base_object_get_object_id(p_inst_base->p_object, &object_id);
			object_handle = fbe_base_pointer_to_handle((fbe_base_t *) p_inst_base->p_object);
			class_id = fbe_base_object_get_class_id(object_handle);
			notification_info.class_id = class_id;
			fbe_topology_class_id_to_object_type(class_id, &notification_info.object_type);
            if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
                lifecycle_log_info(p_inst_base,
                                   "LIFE: send notification class: %d object_type: 0x%llx type: 0x%llx \n",
                                   class_id, (unsigned long long)notification_info.object_type, (unsigned long long)notification_info.notification_type);
            }
            status = fbe_notification_send(object_id, notification_info);
            if (status != FBE_STATUS_OK) {
                lifecycle_error(p_inst_base,
                    "%s: State change notification failed, new state: 0x%X\n, status: 0x%X",
                    __FUNCTION__, new_state, status);
            }
        }

    }
    else {
        lifecycle_unlock_state(p_inst_base);
    }

    return FBE_STATUS_OK;
}
