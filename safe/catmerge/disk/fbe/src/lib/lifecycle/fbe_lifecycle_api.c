#include "fbe_lifecycle_private.h"

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 *\fn fbe_status_t fbe_lifecycle_get_cond(fbe_lifecycle_const_t * p_class_const,
 *                                        struct fbe_base_object_s * p_object,
 *                                        fbe_lifecycle_state_t * p_state)
 *
 * \brief This function gets the current state of an object.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param p_state
 *        A return pointer for receiving the state value
 * \return
 *        Return FBE_STATUS_OK if the state was returned, otherwise an error code.
 */

fbe_status_t
fbe_lifecycle_get_state(fbe_lifecycle_const_t * p_class_const,
                        struct fbe_base_object_s * p_object,
                        fbe_lifecycle_state_t * p_state)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_status_t status;

    *p_state = 0;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    *p_state = p_inst_base->state;

    return FBE_STATUS_OK;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 *\fn fbe_status_t fbe_lifecycle_set_cond(fbe_lifecycle_const_t * p_class_const,
 *                                        struct fbe_base_object_s * p_object,
 *                                        fbe_lifecycle_cond_id_t cond_id)
 *
 * \brief This function forcibly sets the state of an object, the new state must be a supported transition
 *        from the current state.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param cond_id
 *        The new state.
 * \return
 *        Return FBE_STATUS_OK if the condition was set, otherwise an error code is returned.
 */

fbe_status_t fbe_lifecycle_set_state(fbe_lifecycle_const_t * p_class_const,
                                     struct fbe_base_object_s * p_object,
                                     fbe_lifecycle_state_t new_state)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_lifecycle_const_base_state_t * p_base_state;
    fbe_status_t status;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
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

    /* Change the state. */
    if (p_base_state->this_state != new_state) {
        status = lifecycle_change_state(p_inst_base,
                                        p_base_state,
                                        new_state,
                                        FBE_LIFECYCLE_TRACE_TYPE_FORCED_STATE_CHANGE);
        if (status != FBE_STATUS_OK) {
            return status;
        }
    }

    /* Reschedule the monitor. */
    status = lifecycle_reschedule(p_object, p_inst_base, 0);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return FBE_STATUS_OK;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 *\fn fbe_status_t fbe_lifecycle_set_cond(fbe_lifecycle_const_t * p_class_const,
 *                                        struct fbe_base_object_s * p_object,
 *                                        fbe_lifecycle_cond_id_t cond_id)
 *
 * \brief This function sets a condition.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param cond_id
 *        The id of the condition.
 * \return
 *        Return FBE_STATUS_OK if the condition was set, otherwise an error code is returned.
 */

fbe_status_t
fbe_lifecycle_set_cond(fbe_lifecycle_const_t * p_class_const,
                       struct fbe_base_object_s * p_object,
                       fbe_lifecycle_cond_id_t cond_id)
{
    fbe_status_t status;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = lifecycle_set_a_cond(p_class_const, p_object, cond_id);
    return status;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 *\fn fbe_status_t fbe_lifecycle_clear_current_cond(struct fbe_base_object_s * p_object)
 *
 * \brief This function requests that the current condition be cleared once the current condition
 *        function completes.  This only makes sense in the context of a condition function or its
 *        associated completion functions.  If called from other contexts this functions silently
 *        accomplishes nothing.
 *
 * \param p_object
 *        A pointer to the object.
 *
 * \return
 *        Return FBE_STATUS_OK if the successful.
 */

fbe_status_t fbe_lifecycle_clear_current_cond(struct fbe_base_object_s * p_object)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_status_t status;

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    if (p_inst_base->p_cond_inst != NULL) {
        p_inst_base->attr |= FBE_LIFECYCLE_INST_BASE_ATTR_CLEAR_CUR_COND;
    }

    return FBE_STATUS_OK;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 *\fn fbe_status_t fbe_lifecycle_force_clear_cond(fbe_lifecycle_const_t * p_class_const,
 *                                                struct fbe_base_object_s * p_object,
 *                                                fbe_lifecycle_cond_id_t cond_id)
 *
 * \brief This function forcibly clears a condition.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param cond_id
 *        The id of the condition.
 *
 * \return
 *        Return FBE_STATUS_OK if the condition was set, otherwise an error code is returned.
 */

fbe_status_t
fbe_lifecycle_force_clear_cond(fbe_lifecycle_const_t * p_class_const,
                               struct fbe_base_object_s * p_object,
                               fbe_lifecycle_cond_id_t cond_id)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_bool_t cond_is_set;
    fbe_lifecycle_inst_base_attr_t attr;
    fbe_status_t status;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_const_t * p_local_class_const;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Check whether the condition is set. */
    status = lifecycle_is_cond_set(p_class_const, p_inst_base, cond_id, &cond_is_set, NULL);
    if (status != FBE_STATUS_OK) {
        return status;
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

    if ((cond_is_set == FBE_TRUE) || /* Peter Puhov. It is important to be able to "reset" interval of timer conditions (It may be achived by separate API as well) */
        (p_const_base_cond->const_cond.cond_type == FBE_LIFECYCLE_COND_TYPE_BASE_TIMER) ||
        (p_const_base_cond->const_cond.cond_type == FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER)) {
        /* Clear the condition. */
        status = lifecycle_clear_a_cond(p_class_const, p_inst_base, cond_id);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        /* We want to reschedule the object's monitor, if it is currently running,
         * we just update the msecs. */
        lifecycle_lock_state(p_inst_base);
        attr = p_inst_base->attr;
        if ((attr & FBE_LIFECYCLE_INST_BASE_ATTR_IN_CRANK) != 0) {
            p_inst_base->reschedule_msecs = 0;
        }
        lifecycle_unlock_state(p_inst_base);
        /* If the object's monitor is not running we reschedule it to start up. */
        if ((attr & FBE_LIFECYCLE_INST_BASE_ATTR_IN_CRANK) == 0) {
            status = lifecycle_reschedule(p_object, p_inst_base, 0);
            if (status != FBE_STATUS_OK) {
                return status;
            }
        }
    }

    return FBE_STATUS_OK;
}

/*
 * XXX FIXME XXX
 */

fbe_status_t
fbe_lifecycle_get_super_cond_func(fbe_lifecycle_const_t * p_class_const,
                                  struct fbe_base_object_s * p_object,
                                  fbe_lifecycle_state_t state,
                                  fbe_lifecycle_cond_id_t cond_id,
                                  fbe_lifecycle_func_cond_t * pp_cond_func)
{
    fbe_lifecycle_const_t * p_super_const;
    fbe_lifecycle_const_rotary_t * p_rotary;
    fbe_lifecycle_const_cond_t * p_const_cond;
    fbe_status_t status;

    *pp_cond_func = NULL;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    p_super_const = p_class_const->p_super;
    while (p_super_const != NULL) {
        /* Does this class have a rotary for this state? */
        status = lifecycle_get_class_rotary(p_super_const, state, &p_rotary);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        if (p_rotary != NULL) {
            /* Does the rotary have this condition? */
            status = lifecycle_get_cond_data(p_rotary, cond_id, NULL, &p_const_cond);
            if (status != FBE_STATUS_OK) {
                return status;
            }
            /* Does this condition have a pointer to a condition function? */
            if ((p_const_cond != NULL) && (p_const_cond->p_cond_func != NULL)) {
                *pp_cond_func = p_const_cond->p_cond_func;
                break;
            }
        }
        p_super_const = p_super_const->p_super;
    }

    return FBE_STATUS_OK;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 * \fn fbe_status_t fbe_lifecycle_class_const_verify(fbe_lifecycle_const_t * p_class_const)
 *
 * \brief This function cranks the rotary of the current lifecycle state.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \return
 *        FBE_STATUS_OK if the verify was successful, otherwise an error status code is returned.   
 */

fbe_status_t
fbe_lifecycle_class_const_verify(fbe_lifecycle_const_t * p_class_const)
{
    fbe_status_t status;

    if (p_class_const == NULL) {
        lifecycle_error(NULL,
            "%s: Null lifecycle class constant pointer!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = lifecycle_verify_class_const_data(p_class_const);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return FBE_STATUS_OK;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 * \fn fbe_status_t fbe_lifecycle_class_const_hierarchy(fbe_lifecycle_const_t * p_class_const,
 *                                                      fbe_class_id_t * p_class_ids,
 *                                                      fbe_u32_t id_count)
 *
 * \brief This function returns an array of class ids ordered by class hierarcy,
 *        from the base class to the derived class.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_class_ids
 *        A pointer to an array for receiving the class ids.
 * \param id_count
 *        The number of class ids in the array.
 * \return
 *        FBE_STATUS_OK if the verify was successful, otherwise an error status code is returned.   
 */

fbe_status_t
fbe_lifecycle_class_const_hierarchy(fbe_lifecycle_const_t * p_class_const,
                                    fbe_class_id_t * p_class_ids,
                                    fbe_u32_t id_count)
{
    fbe_lifecycle_const_t * p_this_const;
    fbe_class_id_t class_id;
    fbe_u32_t ii, jj;

    if (p_class_const == NULL) {
        lifecycle_error(NULL,
            "%s: Null lifecycle class constant pointer!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (ii = 0; ii < id_count; ii++) {
        p_class_ids[ii] = FBE_CLASS_ID_INVALID;
    }

    /* Build an array of class ids, ordered from derived to base. */
    p_this_const = p_class_const;
    ii = 0;
    while (p_this_const != NULL) {
        p_class_ids[ii++] = p_this_const->class_id;
        if (ii >= id_count) {
            break;
        }
        p_this_const = p_this_const->p_super;
    }

    /* Reverse the order, from base to derived. */
    jj = ii - 1;
    ii = 0;
    while (ii < jj) {
        class_id = p_class_ids[ii];
        p_class_ids[ii] = p_class_ids[jj];
        p_class_ids[jj] = class_id;
        ii++;
        jj--;
    }

    return FBE_STATUS_OK;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 * \fn fbe_status_t fbe_lifecycle_crank_object(fbe_lifecycle_const_t * p_class_const,
 *                                             struct fbe_base_object_s * p_object,
 *                                             struct fbe_packet_s * p_packet)
 *
 * \brief This function cranks the rotary of the current lifecycle state.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param p_packet
 *        A pointer to monitor control packet.
 * \return
 *        FBE_STATUS_OK if the object was cranked successfully, otherwise an error status code is returned. 
 */

fbe_status_t
fbe_lifecycle_crank_object(fbe_lifecycle_const_t * p_class_const,
                           fbe_base_object_t * p_object,
                           fbe_packet_t * p_packet)
{
        fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_lifecycle_const_base_state_t * p_base_state;
    fbe_status_t status;
    fbe_u32_t crank_interval = 0;
    p_inst_base = NULL;

    do {
        status = lifecycle_verify(p_class_const, p_object);
        if (status != FBE_STATUS_OK) {
            break;
        }

        /* Get a pointer to the instance data of the base class for this object. */
        status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
        if (status != FBE_STATUS_OK) {
            break;
        }

        crank_interval = lifecycle_crank_interval(p_inst_base);

        /* Trace this? */
        if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
            fbe_lifecycle_trace_entry_t trace_entry;
            trace_entry.type = FBE_LIFECYCLE_TRACE_TYPE_CRANK_BEGIN;
            trace_entry.u.crank_begin.current_state = p_inst_base->state;
            trace_entry.u.crank_begin.crank_interval = crank_interval;
            lifecycle_trace(p_inst_base, &trace_entry);
        }

        /* Check if the crank interval is less than 3 Sec. */
        if(crank_interval < FBE_LIFECYCLE_DEFAULT_TIMER)
        {
            /* Increment reschedule count each time when the interval is LESS than 3 second */
            p_object->reschedule_count++;

            /* Log a message once every 50000 times */
            if(p_object->reschedule_count % 50000 == 0)
            {
                lifecycle_log_info(p_inst_base, "OBJ: %4X rescheduled for %d times!\n", 
                                   p_object->object_id, 
                                   p_object->reschedule_count);
            }
        }
        else
        {
            /* Clear out the reschedule_count */
            p_object->reschedule_count = 0;
        }

        /* Remember that the monitor is cranking. */
        lifecycle_lock_state(p_inst_base);
        p_inst_base->attr |= FBE_LIFECYCLE_INST_BASE_ATTR_IN_CRANK;
        lifecycle_unlock_state(p_inst_base);

        /* Get a pointer to the base state data for the current state of this object. */
        status = lifecycle_get_base_state_data(p_inst_base, p_inst_base->state, &p_base_state);
        if (status != FBE_STATUS_OK) {
            break;
        }
        if (p_base_state == NULL) {/* I think it was a bug to use &p_base_state */
            lifecycle_error(p_inst_base,
                "%s: Base entry not found for state: 0x%X\n",
                __FUNCTION__, p_inst_base->state);
            break;
        }

        /* Remember this control packet. */
        p_inst_base->p_packet = p_packet;

        /* Set the reschedule interval for the next monitor cycle. */
        p_inst_base->reschedule_msecs = p_base_state->reschedule_msecs;

        /* Decrement any timer conditions, in the object. */
        status = lifecycle_decrement_all_timer_cond(p_class_const, p_inst_base);
        if (status != FBE_STATUS_OK) {
            break;
        }

        /* Is this object in an NON-PENDING state? */
        if (p_base_state->this_state < FBE_LIFECYCLE_STATE_NON_PENDING_MAX) {
            status = lifecycle_crank_non_pending_object(p_class_const, p_inst_base, p_base_state);
        }
        /* Or, is this object in an PENDING state? */
        else if (p_base_state->this_state < FBE_LIFECYCLE_STATE_PENDING_MAX) {
            status = lifecycle_crank_pending_object(p_class_const, p_inst_base, p_base_state);
        }
        else {
            lifecycle_error(p_inst_base,
                "%s: Invalid state: 0x%X\n",
                __FUNCTION__, p_base_state->this_state);
        }

    } while(0);

    /* If we are not pending on a functional packet sent downstream... */
    if (status != FBE_STATUS_PENDING) {
        if (p_inst_base != NULL) {
            /* Forget about this control packet. */
            p_inst_base->p_packet = NULL;
            /* If the object was not destroyed, reschedule the monitor? */
            if (status != FBE_STATUS_NO_OBJECT) {
                (void)lifecycle_reschedule(p_object, p_inst_base, p_inst_base->reschedule_msecs);
            }
        
            lifecycle_lock_state(p_inst_base);
            p_inst_base->attr &= (~FBE_LIFECYCLE_INST_BASE_ATTR_IN_CRANK);
            lifecycle_unlock_state(p_inst_base);

            /* Set our last crank timestamp. */
            p_inst_base->last_crank_time = fbe_get_time();
        }

        /* Set the packet status. */
        fbe_transport_set_status(p_packet, status, 0);
        /* Complete the monitor control packet. */
        fbe_transport_complete_packet(p_packet);
        /* Is the object destroyed? */
        if (status == FBE_STATUS_NO_OBJECT) {
            fbe_base_object_destroy_request(p_object);
        }
    }

    return status;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 * \fn fbe_status_t fbe_lifecycle_reschedule(fbe_lifecycle_const_t * p_class_const,
 *                                           struct fbe_base_object_s * p_object,
 *                                           fbe_u32_t msecs)
 *
 * \brief This function changes the reschedule interval for the next monitor cycle.
 *
 *        There are some restrictions on this, in particular, this change will take effect only while
 *        currently in a monitor cycle. As a result, this operation is only meaningful when called from
 *        a condition function (or a completion function set up by a condition function).
 *
 *        A further restriction is that the reschedule interval can only be shortened.  That is, if the
 *        value of msecs is greater than the current reschedule interval, the reschedule interval is
 *        not changed.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param msecs
 *        The new reschedule interval.
 * \return
 *        FBE_STATUS_OK if the object was cranked successfully, otherwise an error status code is returned. 
 */

fbe_status_t
fbe_lifecycle_reschedule(fbe_lifecycle_const_t * p_class_const,
                         struct fbe_base_object_s * p_object,
                         fbe_u32_t msecs)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_lifecycle_inst_base_attr_t attr;
    fbe_status_t status;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* If the object's monitor is running we just update the msecs. */
    lifecycle_lock_state(p_inst_base);
    attr = p_inst_base->attr;
    if (((attr & FBE_LIFECYCLE_INST_BASE_ATTR_IN_CRANK) != 0) &&
        (msecs < p_inst_base->reschedule_msecs)) {
        p_inst_base->reschedule_msecs = msecs;
    }
    lifecycle_unlock_state(p_inst_base);

    /* If the object's monitor is not running we reschedule it to start up now.  Note, in
     * this case we override the msecs parameter. */
    if ((attr & FBE_LIFECYCLE_INST_BASE_ATTR_IN_CRANK) == 0) {
        status = lifecycle_reschedule(p_object, p_inst_base, 0);
        if (status != FBE_STATUS_OK) {
            return status;
        }
    }

    return FBE_STATUS_OK;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 * \fn fbe_status_t fbe_lifecycle_set_trace_flags(fbe_lifecycle_const_t * p_class_const,
 *                                                struct fbe_base_object_s * p_object,
 *                                                fbe_lifecycle_trace_flags_t flags)
 *
 * \brief This function sets the trace flags for an object.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param flags
 *        The trace flags.
 * \return
 *        Return FBE_STATUS_OK if the flags were set, otherwise an error code is returned.
 */

fbe_status_t
fbe_lifecycle_set_trace_flags(fbe_lifecycle_const_t * p_class_const,
                              struct fbe_base_object_s * p_object,
                              fbe_lifecycle_trace_flags_t flags)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_status_t status;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    p_inst_base->trace_flags = flags;

    return FBE_STATUS_OK;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 * \fn fbe_status_t fbe_lifecycle_set_trace_buffer(fbe_lifecycle_const_t * p_class_const,
 *                                                 struct fbe_base_object_s * p_object,
 *                                                 void * p_buffer,
 *                                                 fbe_u32_t buffer_size)
 *
 * \brief This function sets the trace buffer for the object.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param p_buffer
 *        A pointer to the trace buffer.  If this pointer is null, then trace is disabled.
 * \param buffer_size
 *        The size (in bytes) of the trace buffer.
 * \return
 *        Return FBE_STATUS_OK if the buffer was set, otherwise an error code is returned.
 */

fbe_status_t
fbe_lifecycle_set_trace_buffer(fbe_lifecycle_const_t * p_class_const,
                               struct fbe_base_object_s * p_object,
                               void * p_buffer,
                               fbe_u32_t buffer_size)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_u16_t trace_max;
    fbe_status_t status;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* If the buffer pointer is NULL, we want to turn off tracing. */
    if (p_buffer == NULL) {
        if (lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE) {
            lifecycle_lock_trace(p_inst_base);
            p_inst_base->trace_max = 0;
            p_inst_base->first_trace_index = 0;
            p_inst_base->next_trace_index = 0;
            p_inst_base->p_trace = NULL;
            lifecycle_unlock_trace(p_inst_base);
            fbe_spinlock_destroy(&p_inst_base->trace_lock);
        }
    }
    /* Otherwise, we want to turn on tracing. */
    else {
        trace_max = (fbe_u16_t)(buffer_size / sizeof(fbe_lifecycle_trace_entry_t));
        if (trace_max == 0) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (p_inst_base->p_trace == NULL) {
            fbe_spinlock_init(&p_inst_base->trace_lock);
            lifecycle_lock_trace(p_inst_base);
        }
        else {
            lifecycle_lock_trace(p_inst_base);
        }
        p_inst_base->trace_max = trace_max;
        p_inst_base->first_trace_index = trace_max;
        p_inst_base->next_trace_index = 0;
        p_inst_base->p_trace = (fbe_lifecycle_trace_entry_t*)p_buffer;
        lifecycle_unlock_trace(p_inst_base);
    }
    return FBE_STATUS_OK;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 *\fn fbe_status_t fbe_lifecycle_log_trace(fbe_lifecycle_const_t * p_class_const,
 *                                         struct fbe_base_object_s * p_object,
 *                                         fbe_u32_t entry_count)
 *
 * \brief This function writes trace entries to the log.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param entry_count
 *        The number of entries to log (if zero all entries are logged).
 * \return
 *        Return FBE_STATUS_OK if entries were logged, otherwise an error code is returned.
 */

fbe_status_t
fbe_lifecycle_log_trace(fbe_lifecycle_const_t * p_class_const,
                        struct fbe_base_object_s * p_object,
                        fbe_u16_t entry_count)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_status_t status;
    fbe_u16_t ii;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Is trace turned on? */
    if ((p_inst_base->p_trace == NULL) || (p_inst_base->trace_max == 0)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Decide which entry we will start with. */
    if ((entry_count == 0) || (entry_count >= p_inst_base->trace_max)) {
        entry_count = p_inst_base->trace_max;
        ii = p_inst_base->next_trace_index;
    }
    else if (p_inst_base->next_trace_index > entry_count) {
        ii = p_inst_base->next_trace_index - (entry_count + 1);
    }
    else {
        ii = p_inst_base->trace_max - (entry_count - p_inst_base->next_trace_index);
    }

    /* Log entries from our starting point to the end of the buffer. */
    while ((entry_count > 0) && (ii < p_inst_base->trace_max)) {
        if (p_inst_base->p_trace[ii].type != FBE_LIFECYCLE_TRACE_TYPE_NULL) {
            lifecycle_log_trace_entry(p_inst_base, &p_inst_base->p_trace[ii]);
            entry_count--;
        }
        ii++;
    }

    /* Log entries from the beginning of our buffer up to our starting point. */
    ii = 0;
    while ((entry_count > 0) && (ii < p_inst_base->next_trace_index)) {
        if (p_inst_base->p_trace[ii].type != FBE_LIFECYCLE_TRACE_TYPE_NULL) {
            lifecycle_log_trace_entry(p_inst_base, &p_inst_base->p_trace[ii]);
            entry_count--;
        }
        ii++;
    }

    return FBE_STATUS_OK;
}


/*************************************************************************************************************************************/
/*PAY ATTENTION  - fbe_lifecycle_stop_timer will not work if you used fbe_lifecycle_clear_current_cond before or after it            */
/* so always make sure your logic is written in a way that once we call fbe_lifecycle_stop_timer, no fbe_lifecycle_clear_current_cond*/
/* was called before or after it                                                                                                     */
/*************************************************************************************************************************************/
fbe_status_t
fbe_lifecycle_stop_timer(fbe_lifecycle_const_t * p_class_const,
                         struct fbe_base_object_s * p_object,
                         fbe_lifecycle_cond_id_t cond_id)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_bool_t cond_is_set;
    fbe_status_t status;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_const_t * p_local_class_const;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Check whether the condition is set. */
    status = lifecycle_is_cond_set(p_class_const, p_inst_base, cond_id, &cond_is_set, NULL);
    if (status != FBE_STATUS_OK) {
        return status;
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

    if ((cond_is_set == FBE_TRUE) && 
        (p_const_base_cond->const_cond.cond_type == FBE_LIFECYCLE_COND_TYPE_BASE_TIMER) ||
        (p_const_base_cond->const_cond.cond_type == FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER)) {

        /* stop the timer */
        status = lifecycle_stop_timer(p_class_const, p_inst_base, cond_id);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_lifecycle_verify(fbe_lifecycle_const_t * p_class_const,
                        struct fbe_base_object_s * p_object)
{
    fbe_status_t status;

    status = lifecycle_verify(p_class_const, p_object);

    return status;
}

/*! \ingroup FBE_LIFECYCLE_STATE
 *
 *\fn fbe_status_t fbe_lifecycle_do_all_cond_presets(fbe_lifecycle_const_t * p_class_const,
 *                                                   struct fbe_base_object_s * p_object,
 *                                                   fbe_lifecycle_state_t this_state)
 *
 * \brief This function sets all preset conditions for the current state of an object.
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param this_state
 *        the state for all preset conditions
 * \return
 *        Return FBE_STATUS_OK if succeeds, otherwise an error code.
 */

fbe_status_t fbe_lifecycle_do_all_cond_presets(fbe_lifecycle_const_t * p_class_const, 
                                                fbe_base_object_t * p_object,
                                                fbe_lifecycle_state_t this_state)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_status_t status;

    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = lifecycle_do_all_cond_presets(p_class_const, p_inst_base, this_state);

    return status;
}


/*! \ingroup FBE_LIFECYCLE_STATE
 *
 *\fn fbe_status_t fbe_lifecycle_set_interval_timer_cond(fbe_lifecycle_const_t * p_class_const,
 *                                                       struct fbe_base_object_s * p_object,
 *                                                       fbe_lifecycle_cond_id_t cond_id)
 *
 * \brief This function sets the interval of the timer condition
 *
 * \param p_class_const
 *        A pointer to the lifecycle class constant data of the object.
 * \param p_object
 *        A pointer to the object.
 * \param cond_id
 *        The id of the condition.
 *
 * \return
 *        Return FBE_STATUS_OK if the condition was set, otherwise an error code is returned.
 */

fbe_status_t
fbe_lifecycle_set_interval_timer_cond(fbe_lifecycle_const_t * p_class_const,
                                      struct fbe_base_object_s * p_object,
                                      fbe_lifecycle_cond_id_t cond_id, 
                                      fbe_u32_t interval)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_bool_t cond_is_set;
    fbe_status_t status;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_lifecycle_const_t * p_local_class_const;
    fbe_lifecycle_inst_cond_t * p_cond_inst;

    status = lifecycle_verify(p_class_const, p_object);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data of the base class for this object. */
    status = lifecycle_get_inst_base_data(p_object, &p_inst_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Check whether the condition is set. */
    status = lifecycle_is_cond_set(p_class_const, p_inst_base, cond_id, &cond_is_set, NULL);
    if (status != FBE_STATUS_OK) {
        return status;
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

    /* Get a pointer to the class data of the class that has the base definition for the condition. */
    status = lifecycle_get_inst_cond_data(p_class_const, p_object, cond_id, &p_cond_inst);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    if (((p_const_base_cond->const_cond.cond_type == FBE_LIFECYCLE_COND_TYPE_BASE_TIMER) ||
        (p_const_base_cond->const_cond.cond_type == FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER))) 
    {

        lifecycle_lock_cond(p_inst_base);
        /* set the timer */
        lifecycle_set_timer_cond(p_const_base_cond,
                                 p_cond_inst,
                                 interval);
        lifecycle_unlock_cond(p_inst_base);
        return FBE_STATUS_OK;
    }

    lifecycle_error(NULL,
            "%s: cond_id: 0x%X is_set: %d timer: 0x%x set_count: %d\n",
            __FUNCTION__, cond_id, cond_is_set, p_cond_inst->u.timer.interval, p_cond_inst->u.simple.set_count);

    return FBE_STATUS_GENERIC_FAILURE;
}
