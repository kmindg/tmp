#include "fbe_lifecycle_private.h"

/*
 * This is a local utility for verifying the class hierarchy of lifecycle instance data of a class.
 */

static fbe_status_t
lifecycle_verify_class_inst_data(fbe_lifecycle_const_t * p_leaf_class_const,
                                 fbe_base_object_t * p_object)
{
    fbe_lifecycle_const_t * p_class_const;
    fbe_lifecycle_inst_t * p_class_inst;
    fbe_lifecycle_const_t * p_invalid_class_const;
    fbe_lifecycle_inst_t * p_invalid_class_inst;
    fbe_status_t status;

    /* Traverse the class hierarchy from the leaf class to the super class,
     * looking for classes that are not initialized. On each pass, find
     * the class highest in the hierarchy that is not initialized. */
    do {
        p_invalid_class_const = NULL;
        p_invalid_class_inst = NULL;
        p_class_const = p_leaf_class_const;
        while (p_class_const != NULL) {
            p_class_inst = (*p_class_const->p_callback->get_inst_data)(p_object);
            if (p_class_inst == NULL) {
                lifecycle_error(NULL,
                    "%s: Null lifecycle class instance pointer, class_id: 0x%X\n",
                    __FUNCTION__, p_class_const->class_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if (p_class_inst->canary != FBE_LIFECYCLE_CANARY_INST) {
                p_invalid_class_const = p_class_const;
                p_invalid_class_inst = p_class_inst;
            }
            else {
                /* If we find an initialized class, then all of its super classes
                 * should also be initialized. */
                break;
            }
            p_class_const = p_class_const->p_super;
        }
        /* If we find an uninitalized class, then initialize it. */
        if (p_invalid_class_const != NULL) {
            status = lifecycle_init_inst_data(p_invalid_class_const, p_invalid_class_inst, p_object);
            if (status != FBE_STATUS_OK ) {
                return status;
            }
        }
    } while (p_invalid_class_const != NULL);

    return FBE_STATUS_OK;
}

/*
 * This is utility function that does a trivial verification that the caller is suppling a
 * reasonable lifecycle instance data.  If we find that both the class constant data and
 * class instance data have valid canaries, we assume that all is well. If we find a valid
 * canary in the class constant data but the canary in the instance data is invalid then
 * we assume that the instance data has not yet been initialized (so we initialize the
 * instance data).
 */

fbe_status_t
lifecycle_verify(fbe_lifecycle_const_t * p_class_const,
                 fbe_base_object_t * p_object)
{
    fbe_lifecycle_inst_t * p_class_inst;
    fbe_status_t status;

    if (p_class_const == NULL) {
        lifecycle_error(NULL,
            "%s: Null lifecycle class constant pointer!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (p_class_const->canary != FBE_LIFECYCLE_CANARY_CONST) {
        lifecycle_error(NULL,
            "%s: Invalid canary in the class constant data!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    p_class_inst = (*p_class_const->p_callback->get_inst_data)(p_object);
    if (p_class_inst->canary != FBE_LIFECYCLE_CANARY_INST) {
        status = lifecycle_verify_class_inst_data(p_class_const, p_object);
        if (status != FBE_STATUS_OK ) {
            return status;
        }
    }
    return FBE_STATUS_OK;
}

/*
 * This is a local utility for verifying the base conditions in class constant data.
 */

static fbe_status_t
lifecycle_verify_base_conditions(fbe_lifecycle_const_t * p_class_const)
{
    fbe_lifecycle_const_base_cond_t * p_base_cond;
    fbe_class_id_t class_id;
    fbe_u32_t class_num;
    fbe_u32_t ii;

    if (p_class_const->p_callback->get_inst_cond == NULL) {
        lifecycle_error(NULL,
            "%s: Null get-instance-condition function in class constant data!\n",
            __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    for (ii = 0; ii < p_class_const->p_base_cond_array->base_cond_max; ii++) {
        p_base_cond = p_class_const->p_base_cond_array->pp_base_cond[ii];
        if (p_base_cond->const_cond.canary != FBE_LIFECYCLE_CANARY_CONST_COND) {
            lifecycle_error(NULL,
                "%s: Invalid canary in condition constant data!\n",
                __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if ((p_base_cond->canary != FBE_LIFECYCLE_CANARY_CONST_BASE_COND) &&
            (p_base_cond->canary != FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND)) {
            lifecycle_error(NULL,
                "%s: Invalid canary in base condition constant data!\n",
                __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        class_id = fbe_lifecycle_get_cond_class_id(p_base_cond->const_cond.cond_id);
        if (class_id != p_class_const->class_id) {
            lifecycle_error(NULL,
                "%s: Invalid id in condition constant data, class_id: 0x%X, cond_id: 0x%X\n",
                __FUNCTION__, p_class_const->class_id, p_base_cond->const_cond.cond_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        class_num = fbe_lifecycle_get_cond_class_num(p_base_cond->const_cond.cond_id);
        if (class_num != ii) {
            lifecycle_error(NULL,
                "%s: Invalid sequence number in base condition constant data, class_id: 0x%X, cond_id: 0x%X, ii: 0x%X\n",
                __FUNCTION__, p_class_const->class_id, p_base_cond->const_cond.cond_id, ii);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (((p_base_cond->const_cond.cond_attr & FBE_LIFECYCLE_CONST_COND_ATTR_NOSET) != 0) &&
            (p_base_cond->const_cond.p_cond_func != FBE_LIFECYCLE_NULL_FUNC)) {
            lifecycle_error(NULL,
                "%s: condition function not allowed with a NOSET attribute, class_id: 0x%X, cond_id: 0x%X, ii: 0x%X\n",
                __FUNCTION__, p_class_const->class_id, p_base_cond->const_cond.cond_id, ii);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;
}

/*
 * This is a local utility for verifying the base conditions of a rotary in class constant data.
 */

static fbe_status_t
lifecycle_verify_rotary_base_condition(fbe_lifecycle_const_t * p_class_const,
                                       fbe_lifecycle_const_rotary_t * p_rotary,
                                       fbe_lifecycle_const_cond_t * p_const_cond)
{
    fbe_class_id_t class_id;

    class_id = fbe_lifecycle_get_cond_class_id(p_const_cond->cond_id);
    if (class_id != p_class_const->class_id) {
        lifecycle_error(NULL,
            "%s: Cannot include super class base conditions in class rotaries, class_id: 0x%X, cond_id: 0x%X\n",
            __FUNCTION__, p_class_const->class_id, p_const_cond->cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*
 * This is a local utility for verifying the derived conditions of a rotary in class constant data.
 */

static fbe_status_t
lifecycle_verify_rotary_derived_condition(fbe_lifecycle_const_t * p_class_const,
                                          fbe_lifecycle_const_rotary_t * p_rotary,
                                          fbe_lifecycle_const_cond_t * p_const_cond)
{
    fbe_const_class_info_t * p_class_info;
    const char * p_class_name;
    fbe_status_t status;

#if 0 /* FIXME, this needs to be enabled! But first the base_discovering condition,
       * FBE_BASE_DISCOVERED_LIFECYCLE_COND_GET_PORT_OBJECT_ID, be be deleted, and
       * all of its derived conditions.
       */
    if (p_const_cond->p_cond_func == FBE_LIFECYCLE_NULL_FUNC) {
        status = fbe_get_class_by_id(p_class_const->class_id, &p_class_info);
        p_class_name = (status == FBE_STATUS_OK) ? p_class_info->class_name : "?";
        lifecycle_error(NULL,
            "%s: A null function is not allowed in a derived condition, class_id: 0x%X:%s, cond_id: 0x%X\n",
            __FUNCTION__, p_class_const->class_id, p_class_name, p_const_cond->cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
#endif
    if ((p_const_cond->cond_attr & FBE_LIFECYCLE_CONST_COND_ATTR_NOSET) != 0) {
        status = fbe_get_class_by_id(p_class_const->class_id, &p_class_info);
        p_class_name = (status == FBE_STATUS_OK) ? p_class_info->class_name : "?";
        lifecycle_error(NULL,
            "%s: The NOSET attribute is not allowed in a derived condition, class_id: 0x%X:%s, cond_id: 0x%X\n",
            __FUNCTION__, p_class_const->class_id, p_class_name, p_const_cond->cond_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*
 * This is a local utility for verifying the rotaries in class constant data.
 */

static fbe_status_t
lifecycle_verify_rotaries(fbe_lifecycle_const_t * p_class_const)
{
    fbe_lifecycle_const_rotary_t * p_rotary;
    fbe_lifecycle_const_rotary_cond_t * p_rotary_cond;
    fbe_lifecycle_const_cond_t * p_const_cond;
    fbe_u32_t rotary_ii, rotary_max;
    fbe_u32_t cond_ii, cond_max;
    fbe_status_t status;

    /* Loop over the rotaries in a class. */
    rotary_max = p_class_const->p_rotary_array->rotary_max;
    for (rotary_ii = 0; rotary_ii < rotary_max; rotary_ii++) {
        p_rotary = &p_class_const->p_rotary_array->p_rotary[rotary_ii];
        /* Loop over the conditions in each rotary. */
        p_rotary_cond = p_rotary->p_rotary_cond;
        cond_max = p_rotary->rotary_cond_max;
        for (cond_ii = 0; cond_ii < cond_max; cond_ii++) {
            p_const_cond = p_rotary_cond[cond_ii].p_const_cond;
            if (p_const_cond->canary != FBE_LIFECYCLE_CANARY_CONST_COND) {
                lifecycle_error(NULL,
                    "%s: Invalid canary in condition constant data, class_id: 0x%X, cond_id: 0x%X\n",
                    __FUNCTION__, p_class_const->class_id, p_const_cond->cond_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Is this a base condition? */
            if ((p_const_cond->cond_type == FBE_LIFECYCLE_COND_TYPE_BASE) ||
                (p_const_cond->cond_type == FBE_LIFECYCLE_COND_TYPE_BASE_TIMER)) {
                status = lifecycle_verify_rotary_base_condition(p_class_const, p_rotary, p_const_cond);
                if (status != FBE_STATUS_OK) {
                    return status;
                }
            }
            else if ((p_const_cond->cond_type == FBE_LIFECYCLE_COND_TYPE_DERIVED) ||
                     (p_const_cond->cond_type == FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER)) {
                status = lifecycle_verify_rotary_derived_condition(p_class_const, p_rotary, p_const_cond);
                if (status != FBE_STATUS_OK) {
                    return status;
                }
            }
            else {
                lifecycle_error(NULL,
                    "%s: Invalid type in condition constant data, class_id: 0x%X, cond_id: 0x%X, cond_type: 0x%X\n",
                __FUNCTION__, p_class_const->class_id, p_const_cond->cond_id, p_const_cond->cond_type);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    return FBE_STATUS_OK;
}

/*
 * This is a utility function for verifying the class hierarchy of lifecycle constant data structures.
 */

fbe_status_t
lifecycle_verify_class_const_data(fbe_lifecycle_const_t * p_leaf_class_const)
{
    fbe_lifecycle_const_t * p_class_const;
    fbe_lifecycle_const_base_t * p_base_const;
    fbe_lifecycle_const_base_t * p_base_const_verify;
    fbe_status_t status;

    /* Traverse the class hierarchy from the leaf class to the super class, verify the constant
     * data for each class in the hierarchy. */
    p_class_const = p_leaf_class_const;
    while (p_class_const != NULL) {

        /* Is this a valid constant data structure? */
        if (p_class_const->canary != FBE_LIFECYCLE_CANARY_CONST) {
            lifecycle_error(NULL,
                "%s: Invalid canary in the class constant data!\n",
                __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Are there some callbacks specified? */
        if (p_class_const->p_callback == NULL) {
            lifecycle_error(NULL,
                "%s: Null callback pointer in class constant data, class_id: 0x%X\n",
                __FUNCTION__, p_class_const->class_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Is there a callback for getting the objects instance data pointer? */
        if (p_class_const->p_callback->get_inst_data == NULL) {
            lifecycle_error(NULL,
                "%s: Null get-instance-data function in class constant data, class_id: 0x%X\n",
                __FUNCTION__, p_class_const->class_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Is this the base class? */
        if (p_class_const->p_super == NULL) {
            p_base_const = (fbe_lifecycle_const_base_t*)p_class_const;
            /* Is this valid base class constant data? */
            if (p_base_const->canary != FBE_LIFECYCLE_CANARY_CONST_BASE) {
                lifecycle_error(NULL,
                    "%s: Invalid canary in the base class constant data, leaf_class_id: 0x%X\n",
                    __FUNCTION__, p_leaf_class_const->class_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Verify that the inheritance chain does indeed end with the base class. */
            status = lifecycle_get_const_base_data(&p_base_const_verify);
            if (status != FBE_STATUS_OK) {
                return status;
            }
            if (p_base_const != p_base_const_verify) {
                lifecycle_error(NULL,
                    "%s: Invalid hierarchy in class constant data, leaf_class_id: 0x%X\n",
                    __FUNCTION__, p_leaf_class_const->class_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        /* Verify the base conditions of this class. */
        if (p_class_const->p_base_cond_array->base_cond_max > 0) {
            status = lifecycle_verify_base_conditions(p_class_const);
            if (status != FBE_STATUS_OK) {
                return status;
            }
        }

        /* Verify the rotaries of this class. */
        status = lifecycle_verify_rotaries(p_class_const);
        if (status != FBE_STATUS_OK) {
            return status;
        }

#if 0
        /* Trace this. */
        lifecycle_log_info(NULL, "LIFECYCLE class_const_verify, class_id: 0x%X\n", p_class_const->class_id);
#endif

        /* Move up the class hierarchy. */
        p_class_const = p_class_const->p_super;
    }

    return FBE_STATUS_OK;
}
