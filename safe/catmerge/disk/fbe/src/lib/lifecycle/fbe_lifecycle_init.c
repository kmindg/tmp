#include "fbe_lifecycle_private.h"
#include "fbe_notification.h"
#include "fbe_topology.h"
/*
 * This function initializes the class instance data of an object.
 */

fbe_status_t
lifecycle_init_inst_data(fbe_lifecycle_const_t * p_class_const,
                         fbe_lifecycle_inst_t * p_class_inst,
                         fbe_base_object_t * p_object)
{
    fbe_lifecycle_inst_t * p_super_inst;
    fbe_lifecycle_const_base_t * p_base_const;
    fbe_lifecycle_inst_base_t * p_base_inst;
    fbe_lifecycle_inst_cond_t * p_cond_inst;
    fbe_lifecycle_const_rotary_t * p_rotary;
    fbe_u32_t ii;
    fbe_status_t status;
	fbe_object_id_t object_id;
	fbe_notification_info_t notification_info;
	fbe_class_id_t class_id;
	fbe_handle_t object_handle;
	fbe_package_id_t package_id;

    /* If there is super class constant data, we want to get the super instance data. */
    p_super_inst = (p_class_const->p_super != NULL)
                   ? p_class_const->p_super->p_callback->get_inst_data(p_object) : NULL;

    /* If the class constant data indicates base conditions, then these must be initialized. */
    p_cond_inst = NULL;
    if (p_class_const->p_base_cond_array->base_cond_max > 0) {
        p_cond_inst = (*p_class_const->p_callback->get_inst_cond)(p_object);
        if (p_cond_inst == NULL) {
            lifecycle_error(NULL,
                "%s: Null lifecycle condition pointer!\n",
                __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        for (ii = 0; ii < p_class_const->p_base_cond_array->base_cond_max; ii++) {
            lifecycle_initialize_a_cond(p_class_const->p_base_cond_array->pp_base_cond[ii], &p_cond_inst[ii]);
        }
    }

    /* Get the base class const and inst pointers. */
    status = lifecycle_get_const_base_data(&p_base_const);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    p_base_inst = (fbe_lifecycle_inst_base_t*)(*p_base_const->class_const.p_callback->get_inst_data)(p_object);

    /* If there is no super class then this should be the base class. */
    if (p_class_const->p_super == NULL) {
        if ((fbe_lifecycle_const_base_t*)p_class_const != p_base_const) {
            lifecycle_error(NULL,
                "%s: Invalid base class constant data!\n",
                __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if ((fbe_lifecycle_inst_base_t*)p_class_inst != p_base_inst) {
            lifecycle_error(NULL,
                "%s: Invalid base class instance data!\n",
                __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Initialize the base class instance data. */
        p_base_inst->canary = FBE_LIFECYCLE_CANARY_INST_BASE;
        p_base_inst->state = FBE_LIFECYCLE_STATE_SPECIALIZE;
        p_base_inst->attr = 0;
        p_base_inst->reschedule_msecs = 0;
        p_base_inst->trace_flags = 0; /* FBE_LIFECYCLE_TRACE_FLAGS_ALL */
        p_base_inst->trace_max = 0;
        p_base_inst->first_trace_index = 0;
        p_base_inst->next_trace_index = 0;
        fbe_spinlock_init(&p_base_inst->state_lock);
        //csx_p_dump_native_string("%s: %p\n", __FUNCTION__ , &p_base_inst->state_lock.lock);
        //csx_p_spl_create_nid_always(CSX_MY_MODULE_CONTEXT(), &p_base_inst->state_lock.lock, CSX_NULL);
        
        fbe_spinlock_init(&p_base_inst->cond_lock);
        //csx_p_dump_native_string("%s: %p\n", __FUNCTION__ , &p_base_inst->cond_lock.lock);
        //csx_p_spl_create_nid_always(CSX_MY_MODULE_CONTEXT(), &p_base_inst->cond_lock.lock, CSX_NULL);
        
        p_base_inst->last_crank_time = fbe_get_time();
        p_base_inst->p_cond_inst = NULL;
        p_base_inst->p_object = p_object;
        p_base_inst->p_packet = NULL;
        p_base_inst->p_trace = NULL;

		 /* Send out a notification for this state change. */
        if (p_object != NULL) {
            fbe_base_object_get_object_id(p_object, &object_id);
			notification_info.notification_type = FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE;
            
			object_handle = fbe_base_pointer_to_handle((fbe_base_t *) p_object);
			class_id = fbe_base_object_get_class_id(object_handle);
			notification_info.class_id = class_id;
			notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;/*in specialized state we don't have type, but we may want to get notification*/

			fbe_get_package_id(&package_id);
			if(package_id != FBE_PACKAGE_ID_SEP_0){	/* base_config will handle it */						
				status = fbe_notification_send(object_id, notification_info);
				if (status != FBE_STATUS_OK) {
					lifecycle_error(p_base_inst,
						"%s: State change notification failed, new state: 0x%X\n, status: 0x%X",
						__FUNCTION__, FBE_LIFECYCLE_STATE_SPECIALIZE, status);
				}
			}/* if(package_id != FBE_PACKAGE_ID_SEP_0) */
        }
    }

    /* Initialize the class instance data. */
    p_class_inst->canary = FBE_LIFECYCLE_CANARY_INST;

    /* Set the leaf class const data pointer to this derrived class. */
    p_base_inst->p_leaf_class_const = p_class_const;

    /* Does this class have a rotary for the SPECIALIZE state? */
    status = lifecycle_get_class_rotary(p_class_const, FBE_LIFECYCLE_STATE_SPECIALIZE, &p_rotary);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    /* If we have a specialize rotary, does it have any conditions to preset? */
    if (p_rotary != NULL) {
        status = lifecycle_do_cond_presets(p_class_const, p_rotary, p_base_inst);
        if (status != FBE_STATUS_OK) {
            return status;
        }
    }

    return FBE_STATUS_OK;
}

/*
 * This function "destroys" the lifecycle structures of a class.
 */

void
fbe_lifecycle_destroy_object(fbe_lifecycle_inst_t * p_class_inst,
                             struct fbe_base_object_s * p_object)
{
    fbe_lifecycle_inst_base_t * p_inst_base;
    fbe_lifecycle_const_base_t * p_base_const;
    
    lifecycle_get_const_base_data(&p_base_const);
    
    if (p_class_inst != NULL) {
        p_inst_base = (fbe_lifecycle_inst_base_t*)(*p_base_const->class_const.p_callback->get_inst_data)(p_object);
        //if (p_class_inst->canary == FBE_LIFECYCLE_CANARY_INST) {
        if (p_class_inst->canary == FBE_LIFECYCLE_CANARY_CONST) {        
            //p_inst_base = (fbe_lifecycle_inst_base_t*)p_class_inst;
            if (p_inst_base->canary == FBE_LIFECYCLE_CANARY_INST_BASE) {
                p_inst_base->canary = FBE_LIFECYCLE_CANARY_DEAD;
                if ((lifecycle_is_trace_enabled(p_inst_base) == FBE_TRUE)&&(p_inst_base->p_trace != NULL))
				{
                    lifecycle_lock_trace(p_inst_base);
                    p_inst_base->p_trace = NULL;
                    p_inst_base->trace_max = 0;
                    p_inst_base->first_trace_index = 0;
                    p_inst_base->next_trace_index = 0;
                    lifecycle_unlock_trace(p_inst_base);
                    fbe_spinlock_destroy(&p_inst_base->trace_lock);
                }
                fbe_spinlock_destroy(&p_inst_base->state_lock);
                fbe_spinlock_destroy(&p_inst_base->cond_lock);
				
            }
            //p_class_inst->canary = FBE_LIFECYCLE_CANARY_DEAD;
        }
    }
}
