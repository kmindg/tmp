#include "fbe_lifecycle_private.h"

/*
 * This is a utility function for getting the base class instance data,
 * starting with a pointer to class constant data.
 */

fbe_status_t
lifecycle_get_const_base_data(fbe_lifecycle_const_base_t ** pp_const_base)
{
    fbe_lifecycle_const_base_t * p_const_base;

    *pp_const_base = NULL;

    p_const_base = &fbe_base_object_lifecycle_const;

    if (p_const_base->canary != FBE_LIFECYCLE_CANARY_CONST_BASE) {
        lifecycle_error(NULL,
            "%s: Invalid canary in the base_object's base constant data!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (p_const_base->class_const.canary != FBE_LIFECYCLE_CANARY_CONST) {
        lifecycle_error(NULL,
            "%s: Invalid canary in the base_object's constant data!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pp_const_base = p_const_base;

    return FBE_STATUS_OK;
}

/*
 * This is a utility function for getting the base class instance data,
 * starting with a pointer to class constant data.
 */

fbe_status_t
lifecycle_get_inst_base_data(fbe_base_object_t * p_object,
                             fbe_lifecycle_inst_base_t ** pp_inst_base)
{
    fbe_lifecycle_const_base_t * p_const_base;
    fbe_lifecycle_inst_t * p_class_inst;
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_status_t status;

    *pp_inst_base = NULL;

    status = lifecycle_get_const_base_data(&p_const_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    p_class_inst = (*p_const_base->class_const.p_callback->get_inst_data)(p_object);
    if (p_class_inst == NULL) {
        lifecycle_error(NULL,
            "%s: Null lifecycle class instance pointer!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    p_inst_base = (fbe_lifecycle_inst_base_t*)p_class_inst;
    if (p_inst_base->canary != FBE_LIFECYCLE_CANARY_INST_BASE) {
		/*object may be in specialized, try again*/
        lifecycle_warning(NULL,
            "%s: Invalid canary in base_object base instance data!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (p_inst_base->p_object != p_object) {
		/*object may be in specialized, try again*/
        lifecycle_warning(NULL,
            "%s: Invalid object pointer in base class instance data!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (p_inst_base->class_inst.canary != FBE_LIFECYCLE_CANARY_INST) {
		/*object may be in specialized, try again*/
        lifecycle_warning(NULL,
            "%s: Invalid canary in the base_object's instance data!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pp_inst_base = p_inst_base;

    return FBE_STATUS_OK;
}

/*
 * This is a utility function for getting the base class state data for a given state.
 */

fbe_status_t
lifecycle_get_base_state_data(fbe_lifecycle_inst_base_t * p_inst_base,
                              fbe_lifecycle_state_t state,
                              fbe_lifecycle_const_base_state_t ** pp_base_state)
{
    fbe_lifecycle_const_base_t * p_const_base;
    fbe_lifecycle_const_base_state_t * p_base_state;
    fbe_status_t status;
    fbe_u32_t ii;

    status = lifecycle_get_const_base_data(&p_const_base);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    p_base_state = p_const_base->p_base_state;

    *pp_base_state = NULL;
    for (ii = 0; ii <  p_const_base->state_max; ii++) {
        if (p_base_state[ii].this_state == state) {
            *pp_base_state = &p_base_state[ii];
            break;
        }
    }

    return FBE_STATUS_OK;
}

/*
 * This is a utility function for getting the class constant data of a specific class,
 * starting with a class constant pointer for any class lower in the inheritance sequence.
 */

fbe_status_t
lifecycle_get_class_const_data(fbe_lifecycle_const_t * p_class_const,
                              fbe_class_id_t class_id,
                              fbe_lifecycle_const_t ** pp_class_const)
{
    *pp_class_const = NULL;

    while (p_class_const != NULL) {
        if (p_class_const->canary != FBE_LIFECYCLE_CANARY_CONST) {
            lifecycle_error(NULL,
                "%s: Invalid lifecycle canary !\n",
                __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (p_class_const->class_id == class_id) {
            break;
        }
        p_class_const = p_class_const->p_super;
    }

    if (p_class_const == NULL) {
        lifecycle_error(NULL,
            "%s: Class %u not found!\n",
            __FUNCTION__, class_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pp_class_const = p_class_const;

    return FBE_STATUS_OK;
}

/*
 * This is a utility function for getting the class instance condition data for a
 * specific condition id.
 */

fbe_status_t
lifecycle_get_inst_cond_data(fbe_lifecycle_const_t * p_lower_class_const,
                             fbe_base_object_t * p_object,
                             fbe_lifecycle_cond_id_t cond_id,
                             fbe_lifecycle_inst_cond_t ** pp_cond_inst)
{
    fbe_lifecycle_const_t * p_higher_class_const;
    fbe_lifecycle_inst_cond_t * p_class_cond_inst;
    fbe_class_id_t class_id;
    fbe_u32_t class_num;
    fbe_status_t status;

    *pp_cond_inst = NULL;

    /* Dig the class id out of the condition id. */
    class_id = fbe_lifecycle_get_cond_class_id(cond_id);

    /* Get a pointer to the constant data of the class that has the base definition for the condition. */
    status = lifecycle_get_class_const_data(p_lower_class_const, class_id, &p_higher_class_const);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Get a pointer to the instance data  of the class that has the base definition for the condition. */
    p_class_cond_inst = (*p_higher_class_const->p_callback->get_inst_cond)(p_object);
    if (p_class_cond_inst == NULL) {
        lifecycle_error(NULL,
            "%s: Null lifecycle class condition instance pointer!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Dig the class sequence number out of the condition id. */
    class_num = fbe_lifecycle_get_cond_class_num(cond_id);

    /* Range check the sequence number. */
    if (class_num >= p_higher_class_const->p_base_cond_array->base_cond_max) {
        lifecycle_error(NULL,
            "%s: Condition range error, cond_id: 0x%X, index: 0x%X, max: 0x%X\n",
            __FUNCTION__, cond_id, class_num, p_higher_class_const->p_base_cond_array->base_cond_max);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pp_cond_inst = &p_class_cond_inst[class_num];

    return FBE_STATUS_OK;
}

/*
 * This is a utility function for getting the rotary condition data for a specific
 * conditions id in a specific rotary.
 */

fbe_status_t
lifecycle_get_cond_data(fbe_lifecycle_const_rotary_t * p_rotary,
                        fbe_lifecycle_cond_id_t cond_id,
                        fbe_lifecycle_const_rotary_cond_t ** pp_const_rotary_cond,
                        fbe_lifecycle_const_cond_t ** pp_const_cond)
{
    fbe_lifecycle_const_rotary_cond_t * p_const_rotary_cond;
    fbe_lifecycle_const_rotary_cond_t * p_rotary_cond;
    fbe_lifecycle_const_cond_t * p_const_cond;
    fbe_u32_t ii, cond_max;

    if (pp_const_rotary_cond != NULL) *pp_const_rotary_cond = NULL;
    if (pp_const_cond != NULL) *pp_const_cond = NULL;

    p_rotary_cond = p_rotary->p_rotary_cond;
    cond_max = p_rotary->rotary_cond_max;

    for (ii = 0; ii < cond_max; ii++) {
        p_const_rotary_cond = &p_rotary_cond[ii];
        p_const_cond = p_const_rotary_cond->p_const_cond;
        if (p_const_cond->cond_id == cond_id) {
            if (pp_const_rotary_cond != NULL) *pp_const_rotary_cond = p_const_rotary_cond; 
            if (pp_const_cond != NULL) *pp_const_cond = p_const_cond;
            break;
        }
    }
    return FBE_STATUS_OK;
}

/*
 * This is a local utility function for getting the rotary condition data for a specific
 * condition id in a specific state rotary.  It starts with a class constatnt pointer and
 * searches up the class inheritance sequence for the first occurance of the condition in
 * a rotary for the specified state.  I.e., it will discover the "most derived" occurance
 * of the condition in the rotary conditions of the class hierarchy.
 *
 * It will stop searching the class hierarchy upon reaching the class constant data that
 * has a class id equal to the specified class id.
 */

fbe_status_t
lifecycle_get_rotary_cond_data(fbe_lifecycle_const_t * p_class_const,
                               fbe_lifecycle_state_t state,
                               fbe_lifecycle_cond_id_t cond_id,
                               fbe_lifecycle_const_t ** pp_class_const,
                               fbe_lifecycle_const_rotary_cond_t ** pp_const_rotary_cond,
                               fbe_lifecycle_const_cond_t ** pp_const_cond)
{
    fbe_class_id_t cond_class_id;
    fbe_lifecycle_const_rotary_cond_t * p_const_rotary_cond;
    fbe_lifecycle_const_cond_t * p_const_cond;
    fbe_u32_t ii, rotary_max;
    static fbe_status_t status;

    if (pp_class_const != NULL) *pp_class_const = NULL;
    if (pp_const_rotary_cond != NULL) *pp_const_rotary_cond = NULL;
    if (pp_const_cond != NULL) *pp_const_cond = NULL;

    p_const_cond = NULL;
    cond_class_id = fbe_lifecycle_get_cond_class_id(cond_id);

    /* Loop up the class hierarchy. */
    while ((p_const_cond == NULL) && (p_class_const != NULL)) {
        if (p_class_const->canary != FBE_LIFECYCLE_CANARY_CONST) {
            lifecycle_error(NULL,
                "%s: Invalid lifecycle canary !\n",
                __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        rotary_max = p_class_const->p_rotary_array->rotary_max;
        /* Loop over the rotaries in a class. */
        for (ii = 0; ii < rotary_max; ii++) {
            if (p_class_const->p_rotary_array->p_rotary[ii].this_state == state) {
                status = lifecycle_get_cond_data(&p_class_const->p_rotary_array->p_rotary[ii],
                                                 cond_id,
                                                 &p_const_rotary_cond,
                                                 &p_const_cond);
                if (status != FBE_STATUS_OK) {
                    return status;
                }
            }
        }
        if (p_const_cond == NULL) {
            p_class_const = (p_class_const->class_id == cond_class_id) ? NULL : p_class_const->p_super;
        }
    }

    /* Did we find the condition? */
    if (p_const_cond != NULL) {
        if (pp_class_const != NULL) *pp_class_const = p_class_const;
        if (pp_const_rotary_cond != NULL) *pp_const_rotary_cond = p_const_rotary_cond;
        if (pp_const_cond != NULL) *pp_const_cond = p_const_cond;
    }

    return FBE_STATUS_OK;
}

/*
 * This is a utility function for finding the base condition constant data for a specific
 * condition id.  It starts with a class instance pointer and searches up the class inheritance
 * sequence for the base condition definition.  It will stop searching the class hierarchy upon
 * reaching the class constant data that has a class id equal to the specified class id.
 */

fbe_status_t
lifecycle_get_base_cond_data(fbe_lifecycle_const_t * p_leaf_class_const,
                             fbe_lifecycle_cond_id_t cond_id,
                             fbe_lifecycle_const_t ** pp_class_const,
                             fbe_lifecycle_const_base_cond_t ** pp_const_base_cond)
{
    fbe_lifecycle_const_t * p_class_const;
    fbe_class_id_t cond_class_id;
    fbe_lifecycle_const_base_cond_t * p_const_base_cond;
    fbe_u32_t ii;

    if (pp_class_const != NULL) *pp_class_const = NULL;
    if (pp_const_base_cond != NULL) *pp_const_base_cond = NULL;

    p_const_base_cond = NULL;
    cond_class_id = fbe_lifecycle_get_cond_class_id(cond_id);
    p_class_const = p_leaf_class_const;

    while (p_class_const != NULL) {
        if (p_class_const->class_id == cond_class_id) {
            for (ii = 0; ii < p_class_const->p_base_cond_array->base_cond_max; ii++) {
                p_const_base_cond = p_class_const->p_base_cond_array->pp_base_cond[ii];
                if (p_const_base_cond == NULL) {
                    lifecycle_error(NULL,
                        "%s: null condition constant data pointer!\n",
                        __FUNCTION__);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                if (p_const_base_cond->canary != FBE_LIFECYCLE_CANARY_CONST_BASE_COND) {
                    lifecycle_error(NULL,
                        "%s: Invalid canary in base condition constant data!\n",
                        __FUNCTION__);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                if (p_const_base_cond->const_cond.cond_id == cond_id) {
                    if (pp_class_const != NULL) *pp_class_const = p_class_const;
                    if (pp_const_base_cond != NULL) *pp_const_base_cond = p_const_base_cond;
                    return FBE_STATUS_OK;
                }
            }
            return FBE_STATUS_OK;
        }
        else {
            p_class_const = p_class_const->p_super;
        }
    }
    return FBE_STATUS_OK;
}

/*
 * This is a utility function the find the prior class instance data
 * in a class hierarchy.
 */

fbe_status_t
lifecycle_get_derived_class_const(fbe_lifecycle_const_t * p_leaf_class_const,
                                  fbe_lifecycle_const_t * p_super_class_const,
                                  fbe_lifecycle_const_t ** pp_derived_class_const)
{
    fbe_lifecycle_const_t * p_derived_class_const;

    *pp_derived_class_const = NULL;

    if ((p_leaf_class_const == NULL) || (p_super_class_const == NULL) || (p_leaf_class_const == p_super_class_const)) {
        lifecycle_error(NULL,
            "%s: Invalid class hierarchy, leaf: %p, super: %p\n",
            __FUNCTION__, p_leaf_class_const, p_super_class_const);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    p_derived_class_const = p_leaf_class_const;
    while (p_derived_class_const != NULL) {
        if (p_derived_class_const->p_super == p_super_class_const) {
            break;
        }
        p_derived_class_const = p_derived_class_const->p_super;
    }
    if (p_derived_class_const == NULL) {
        lifecycle_error(NULL,
            "%s: Broken class hierarchy, leaf: %p, super: %p\n",
            __FUNCTION__, p_leaf_class_const, p_super_class_const);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pp_derived_class_const = p_derived_class_const;

    return FBE_STATUS_OK;
}

/*
 * This is a utility function to find a rotary for a given state in a given class.
 */

fbe_status_t
lifecycle_get_class_rotary(fbe_lifecycle_const_t * p_class_const,
                           fbe_lifecycle_state_t state,
                           fbe_lifecycle_const_rotary_t ** pp_rotary)
{
    fbe_u32_t ii, rotary_max;
    fbe_lifecycle_const_rotary_t * p_rotary;

    *pp_rotary = NULL;

    rotary_max = p_class_const->p_rotary_array->rotary_max;

    for (ii = 0; ii < rotary_max; ii++) {
        p_rotary = &p_class_const->p_rotary_array->p_rotary[ii];
        if (p_rotary->this_state == state) {
            *pp_rotary = p_rotary;
            break;
        }
    }
    return FBE_STATUS_OK;
}
