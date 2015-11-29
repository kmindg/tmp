#include "fbe_lifecycle_private.h"

/*
 * This is a tracing function that will write an ERR log message.
 */

void
lifecycle_error(fbe_lifecycle_inst_base_t * p_inst_base, const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t trace_level;
    fbe_object_id_t object_id;

    va_list args;

    default_trace_level = fbe_trace_get_lib_default_trace_level(FBE_LIBRARY_ID_LIFECYCLE);
    object_id = 0;
    if ((p_inst_base != NULL) && (p_inst_base->p_object != NULL)) {
        fbe_base_object_get_object_id(p_inst_base->p_object, &object_id);
        trace_level = fbe_base_object_get_trace_level(p_inst_base->p_object);
        if (trace_level < default_trace_level) {
            trace_level = default_trace_level;
        }
    }
    else {
        trace_level = default_trace_level;
    }
    if (trace_level < FBE_TRACE_LEVEL_ERROR) {
        return;
    }
    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                     FBE_LIBRARY_ID_LIFECYCLE,
                     FBE_TRACE_LEVEL_ERROR,
                     object_id,
                     fmt, 
                     args);
    va_end(args);
}

void
lifecycle_warning(fbe_lifecycle_inst_base_t * p_inst_base, const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t trace_level;
    fbe_object_id_t object_id;

    va_list args;

    default_trace_level = fbe_trace_get_lib_default_trace_level(FBE_LIBRARY_ID_LIFECYCLE);
    object_id = 0;
    if ((p_inst_base != NULL) && (p_inst_base->p_object != NULL)) {
        fbe_base_object_get_object_id(p_inst_base->p_object, &object_id);
        trace_level = fbe_base_object_get_trace_level(p_inst_base->p_object);
        if (trace_level < default_trace_level) {
            trace_level = default_trace_level;
        }
    }
    else {
        trace_level = default_trace_level;
    }
    if (trace_level < FBE_TRACE_LEVEL_WARNING) {
        return;
    }
    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                     FBE_LIBRARY_ID_LIFECYCLE,
                     FBE_TRACE_LEVEL_WARNING,
                     object_id,
                     fmt, 
                     args);
    va_end(args);
}

/*
 * This is a tracing function that will write an DBG_LOW ktrace message.
 */

void
lifecycle_log_info(fbe_lifecycle_inst_base_t * p_inst_base, const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t trace_level;
    fbe_object_id_t object_id;

    va_list args;

    object_id = 0;
    default_trace_level = fbe_trace_get_lib_default_trace_level(FBE_LIBRARY_ID_LIFECYCLE);
    if ((p_inst_base != NULL) && (p_inst_base->p_object != NULL)) {
        fbe_base_object_get_object_id(p_inst_base->p_object, &object_id);
        trace_level = fbe_base_object_get_trace_level(p_inst_base->p_object);
        if (trace_level < default_trace_level) {
            trace_level = default_trace_level;
        }
    }
    else {
        trace_level = default_trace_level;
    }
    if (trace_level < FBE_TRACE_LEVEL_INFO) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                     FBE_LIBRARY_ID_LIFECYCLE,
                     FBE_TRACE_LEVEL_INFO,
                     object_id,
                     fmt, 
                     args);
    va_end(args);
}

/*
 * This is a tracing function that will write a log entry for a trace element.
 */

void
lifecycle_log_trace_entry(fbe_lifecycle_inst_base_t * p_inst_base,
                          fbe_lifecycle_trace_entry_t * p_entry)
{
    fbe_lifecycle_const_t * p_class_const;
    const char * p_str1, * p_str2;
    fbe_u32_t val1, val2;

    p_class_const = p_inst_base->p_leaf_class_const;
    val1 = val2 = 0;
    switch(p_entry->type) {
        case FBE_LIFECYCLE_TRACE_TYPE_CRANK_BEGIN:
            val1 = p_entry->u.crank_begin.current_state;
            val2 = p_entry->u.crank_begin.crank_interval;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_CLEAR:
            val1 = p_entry->u.condition_clear.current_state;
            val2 = p_entry->u.condition_clear.cond_id;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_PRESET:
            val1 = p_entry->u.condition_preset.current_state;
            val2 = p_entry->u.condition_preset.cond_id;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_SET:
            val1 = p_entry->u.condition_set.current_state;
            val2 = p_entry->u.condition_set.cond_id;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_BEFORE:
            val2 = p_entry->u.condition_run_before.cond_id;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_AFTER:
            val1 = p_entry->u.condition_run_after.status;
            val2 = p_entry->u.condition_run_after.cond_id;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_STATE_CHANGE:
            val1 = p_entry->u.state_change.old_state;
            val2 = p_entry->u.state_change.new_state;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_STATE_CHANGE:
            val1 = p_entry->u.cond_state_change.old_state;
            val2 = p_entry->u.cond_state_change.new_state;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_FORCED_STATE_CHANGE:
            val1 = p_entry->u.forced_state_change.old_state;
            val2 = p_entry->u.forced_state_change.new_state;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_RESCHEDULE_MONITOR:
            val1 = p_entry->u.reschedule_monitor.msecs;
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_CRANK_END:
            val1 = p_entry->u.crank_end.ending_state;
            break;
        default:
            lifecycle_log_info(p_inst_base, "LIFECYCLE entry error, invalid entry type: 0x%X\n", p_entry->type);
            return;
    }

    p_str1 = p_str2 = "<error>";
    switch(p_entry->type) {
        case FBE_LIFECYCLE_TRACE_TYPE_CRANK_BEGIN:
        case FBE_LIFECYCLE_TRACE_TYPE_CRANK_END:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_CLEAR:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_PRESET:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_SET:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_BEFORE:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_AFTER:
        {
            switch(p_entry->type) {
                case FBE_LIFECYCLE_TRACE_TYPE_CRANK_BEGIN:
                case FBE_LIFECYCLE_TRACE_TYPE_CRANK_END:
                case FBE_LIFECYCLE_TRACE_TYPE_COND_CLEAR:
                case FBE_LIFECYCLE_TRACE_TYPE_COND_PRESET:
                case FBE_LIFECYCLE_TRACE_TYPE_COND_SET:
                {
                    fbe_lifecycle_const_base_state_t * p_base_state = NULL;
                    lifecycle_get_base_state_data(p_inst_base, val1, &p_base_state);
                    if (p_base_state != NULL) {
                        p_str1 = p_base_state->p_name;
                    }
                    break;
                }
                case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_AFTER:
                    switch(val1) {
                        case FBE_LIFECYCLE_STATUS_DONE:
                            p_str1 = "DONE";
                            break;
                        case FBE_LIFECYCLE_STATUS_PENDING:
                            p_str1 = "PENDING";
                            break;
                        case FBE_LIFECYCLE_STATUS_RESCHEDULE:
                            p_str1 = "RESCHEDULE";
                            break;
                    }
                    break;
            }
            switch(p_entry->type) {
                case FBE_LIFECYCLE_TRACE_TYPE_COND_CLEAR:
                case FBE_LIFECYCLE_TRACE_TYPE_COND_PRESET:
                case FBE_LIFECYCLE_TRACE_TYPE_COND_SET:
                case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_BEFORE:
                case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_AFTER:
                {
                    fbe_lifecycle_const_base_cond_t * p_const_base_cond = NULL;
                    lifecycle_get_base_cond_data(p_class_const, val2, NULL, &p_const_base_cond);
                    if (p_const_base_cond != NULL) {
                        p_str2 = p_const_base_cond->p_cond_name;
                    }
                    break;
                }
            }
            break;
        }
        case FBE_LIFECYCLE_TRACE_TYPE_STATE_CHANGE:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_STATE_CHANGE:
        case FBE_LIFECYCLE_TRACE_TYPE_FORCED_STATE_CHANGE:
        {
            fbe_lifecycle_const_base_state_t * p_base_state = NULL;
            lifecycle_get_base_state_data(p_inst_base, val1, &p_base_state);
            if (p_base_state != NULL) {
                p_str1 = p_base_state->p_name;
            }
            lifecycle_get_base_state_data(p_inst_base, val2, &p_base_state);
            if (p_base_state != NULL) {
                p_str2 = p_base_state->p_name;
            }
        }
        case FBE_LIFECYCLE_TRACE_TYPE_RESCHEDULE_MONITOR:
            break;
        default: lifecycle_log_info(p_inst_base, "LIFECYCLE internal error, entry type: 0x%X\n", p_entry->type);
            return;
    }

    switch(p_entry->type) {
        case FBE_LIFECYCLE_TRACE_TYPE_CRANK_BEGIN:
            lifecycle_log_info(p_inst_base, "LIFECYCLE crank-begin, state: 0x%X/%s, interval: %u\n",
                               val1, p_str1, val2);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_CLEAR:
            lifecycle_log_info(p_inst_base, "LIFECYCLE clear-condition, state: 0x%X/%s, id: 0x%X/%s\n",
                               val1, p_str1, val2, p_str2);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_PRESET:
            lifecycle_log_info(p_inst_base, "LIFECYCLE preset-condition, state: 0x%X/%s, id: 0x%X/%s\n",
                               val1, p_str1, val2, p_str2);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_SET:
            lifecycle_log_info(p_inst_base, "LIFECYCLE set-condition, state: 0x%X/%s, id: 0x%X/%s\n",
                               val1, p_str1, val2, p_str2);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_BEFORE:
            lifecycle_log_info(p_inst_base, "LIFECYCLE before-run-condition, id: 0x%X/%s\n",
                               val2, p_str2);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_AFTER:
            lifecycle_log_info(p_inst_base, "LIFECYCLE after-run-condition, status: 0x%X/%s, id: 0x%X/%s\n",
                               val1, p_str1, val2, p_str2);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_STATE_CHANGE:
            lifecycle_log_info(p_inst_base, "LIFECYCLE state-change, old state 0x%X/%s, new state: 0x%X/%s\n",
                               val1, p_str1, val2, p_str2);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_STATE_CHANGE:
            lifecycle_log_info(p_inst_base, "LIFECYCLE cond-state-change, old state 0x%X/%s, new state: 0x%X/%s\n",
                               val1, p_str1, val2, p_str2);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_FORCED_STATE_CHANGE:
            lifecycle_log_info(p_inst_base, "LIFECYCLE forced-state-change, old state 0x%X/%s, new state: 0x%X/%s\n",
                               val1, p_str1, val2, p_str2);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_RESCHEDULE_MONITOR:
            lifecycle_log_info(p_inst_base, "LIFECYCLE reschedule_monitor, msecs: 0x%X\n",
                               val1);
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_CRANK_END:
            lifecycle_log_info(p_inst_base, "LIFECYCLE crank-done, state: 0x%X/%s\n",
                               val1, p_str1);
            break;
        default:
            lifecycle_log_info(p_inst_base, "LIFECYCLE internal error, entry type: 0x%X",
                               p_entry->type);
            return;
    }
}

/*
 * This is a tracing function for dispatching a trace entry.
 */

void
lifecycle_trace(fbe_lifecycle_inst_base_t * p_inst_base,
                fbe_lifecycle_trace_entry_t * p_trace_entry)
{
    /* Check whether this type of trace is currently permitted. */
    switch(p_trace_entry->type) {
        case FBE_LIFECYCLE_TRACE_TYPE_CRANK_BEGIN:
        case FBE_LIFECYCLE_TRACE_TYPE_RESCHEDULE_MONITOR:
        case FBE_LIFECYCLE_TRACE_TYPE_CRANK_END:
            if ((p_inst_base->trace_flags & FBE_LIFECYCLE_TRACE_FLAGS_CRANKING) == 0) {
                return;
            }
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_STATE_CHANGE:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_STATE_CHANGE:
        case FBE_LIFECYCLE_TRACE_TYPE_FORCED_STATE_CHANGE:
            if ((p_inst_base->trace_flags & FBE_LIFECYCLE_TRACE_FLAGS_STATE_CHANGE) == 0) {
                return;
            }
            break;
        case FBE_LIFECYCLE_TRACE_TYPE_COND_CLEAR:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_PRESET:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_SET:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_BEFORE:
        case FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_AFTER:
            if ((p_inst_base->trace_flags & FBE_LIFECYCLE_TRACE_FLAGS_CONDITIONS) == 0) {
                return;
            }
            break;
        default:
            return;
    }

    /* If we don't have a trace buffer, we just log the trace entries. */
    if (p_inst_base->p_trace == NULL) {
        lifecycle_log_trace_entry(p_inst_base, p_trace_entry);
        return;
    }

    /* Lock the trace buffer. */
    lifecycle_lock_trace(p_inst_base);

    /* Copy in the trace entry. */
    fbe_copy_memory(&p_inst_base->p_trace[p_inst_base->next_trace_index],
                  p_trace_entry, sizeof(fbe_lifecycle_trace_entry_t));

    if (p_inst_base->first_trace_index == p_inst_base->trace_max) {
        /* This is the first trace element put in the buffer,
         * we need to initialize the first element index. */
        p_inst_base->first_trace_index = p_inst_base->next_trace_index;
    }
    else if (p_inst_base->first_trace_index == p_inst_base->next_trace_index) {
        /* The new trace element has overlayed the first trace element,
         * we need to bump the first element index. */
        p_inst_base->first_trace_index++;
        if (p_inst_base->first_trace_index >= p_inst_base->trace_max) {
            /* The first element index needs to roll around to the beginning of the buffer. */
            p_inst_base->first_trace_index = 0;
        }
    }

    /* Bump the next element index. */
    p_inst_base->next_trace_index++;
    if (p_inst_base->next_trace_index >= p_inst_base->trace_max) {
        /* The next element index needs to roll around to the beginning of the buffer. */
        p_inst_base->next_trace_index = 0;
    }

    lifecycle_unlock_trace(p_inst_base);
}
