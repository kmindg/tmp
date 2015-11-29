#include "terminator_base.h"
#include "fbe_terminator_device_registry.h"
#include "terminator_enclosure.h"
#include "terminator_virtual_phy.h"
#include "terminator_drive.h"
#include "fbe_terminator_common.h"

static fbe_u32_t DEFAULT_RESET_DELAY = 2000; /* 2 seconds */

fbe_status_t base_component_init(base_component_t * self)
{
    fbe_queue_element_init(&self->queue_element);
    fbe_queue_init(&self->child_list_head);

    self->component_type  = TERMINATOR_COMPONENT_TYPE_INVALID;
    self->component_state = TERMINATOR_COMPONENT_STATE_INITIALIZED;

    self->parent_ptr = NULL;
    self->attributes = NULL;

    fbe_queue_init(&self->io_queue_head);
    fbe_spinlock_init(&self->io_queue_lock);

    self->last_io_id = DEVICE_HANDLE_INVALID;

    self->reset_function = NULL;
    self->reset_delay    = DEFAULT_RESET_DELAY;

    return FBE_STATUS_OK;
}

/* Free the memory of this node, child list should be empty before calling
 *  this function or we would return failure.
 */
fbe_status_t base_component_destroy(base_component_t *self, fbe_bool_t delete_self)
{
    fbe_status_t status      = FBE_STATUS_GENERIC_FAILURE;
    fbe_u64_t    child_count = 0;

    if ((child_count = base_component_get_child_count(self)) != 0)
    {
        /* We will leak a drive. Generate an error.
         */
        if (self->component_type == TERMINATOR_COMPONENT_TYPE_DRIVE)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: cannot remove drive: %p 0x%llx children\n", 
                             __FUNCTION__,
                             self,
                             (unsigned long long)child_count);
        }

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* This needs to be prior to the destroy of attributes since in the case of a remote
     *  memory drive, the serial number is needed to identify the drive to remove.
     */
    if (self->component_type == TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        drive_free((terminator_drive_t *)self);
    }

    status = base_component_destroy_attributes(self->component_type, self->attributes);
    RETURN_ON_ERROR_STATUS;

    /* HACK: remove from TDR */
    if (fbe_terminator_device_registry_remove_device_by_ptr(self) != TDR_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: could not remove handle %p from TDR\n",
                         __FUNCTION__,
                         self);
    }

    fbe_spinlock_destroy(&self->io_queue_lock);

    if (delete_self)
    {
        fbe_terminator_free_memory(self);
    }

    return status;
}

fbe_status_t base_component_destroy_attributes(terminator_component_type_t component_type, void *attributes)
{
    fbe_status_t status = FBE_STATUS_OK;

	if(attributes == NULL){
		return status;
	}
    switch(component_type)
    {
        case TERMINATOR_COMPONENT_TYPE_BOARD:
        case TERMINATOR_COMPONENT_TYPE_PORT:
        case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
			fbe_terminator_free_memory(attributes);
			break;
        case TERMINATOR_COMPONENT_TYPE_DRIVE:
            // In future have seperate destroy functions for each of these just
            // like Virtual phy maybe....
            fbe_terminator_free_memory(attributes);
            break;
        case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
            status = sas_virtual_phy_destroy((terminator_sas_virtual_phy_info_t *)attributes);
            break;
    }
    return(status);
}


fbe_status_t base_component_add_parent(base_component_t * child, base_component_t * parent)
{
    child->parent_ptr = parent;
    return FBE_STATUS_OK;
}

fbe_status_t base_component_remove_parent(base_component_t * child)
{
    child->parent_ptr = NULL;
    return FBE_STATUS_OK;
}
fbe_status_t base_component_get_parent(base_component_t * self, base_component_t ** parent)
{
    *parent = self->parent_ptr;
    return FBE_STATUS_OK;
}

fbe_status_t base_component_add_child(base_component_t * self, fbe_queue_element_t * element)
{
    base_component_add_parent((base_component_t *)element, self);
    fbe_queue_push(&self->child_list_head, element);
    return FBE_STATUS_OK;
}

fbe_status_t base_component_add_child_to_front(base_component_t * self, fbe_queue_element_t * element)
{
    base_component_add_parent((base_component_t *)element, self);
    fbe_queue_push_front(&self->child_list_head, element);
    return FBE_STATUS_OK;
}

fbe_status_t base_component_remove_child(fbe_queue_element_t * element)
{
    base_component_remove_parent((base_component_t *)element);
    fbe_queue_remove(element);
    return FBE_STATUS_OK;
}

void base_component_assign_attributes(base_component_t * self, void * atrributes_p)
{
    self->attributes = atrributes_p;
}

void * base_component_get_attributes(base_component_t * self)
{
    return (self->attributes);
}

fbe_u64_t base_component_get_child_count(base_component_t * self)
{
    fbe_u64_t count = 0;
    fbe_queue_head_t * head = NULL;
    fbe_queue_element_t * next_p = NULL;

    head = &self->child_list_head;

    if ( fbe_queue_is_empty(head))
    {
        return count;
    }

    next_p = fbe_queue_front(head);
    
    while(next_p != NULL)
    {
        count++;
        next_p = fbe_queue_next(head, next_p);
        
    }
    return count;
}

fbe_queue_element_t * base_component_get_queue_element(base_component_t * self)
{
    return &self->queue_element;
}
base_component_t * base_component_get_child_by_type(base_component_t * self,
                                                    terminator_component_type_t component_type)
{
    base_component_t * child = NULL;
    base_component_t * matching_child = NULL;
    child = (base_component_t *)fbe_queue_front(&self->child_list_head);
    while (child != NULL)
    {
        if (child->component_type == component_type)
            matching_child = child;
        child = (base_component_t *)fbe_queue_next(&self->child_list_head, &child->queue_element);
    }
    return matching_child;
}

void base_component_set_component_type(base_component_t * self, terminator_component_type_t component_type)
{
    self->component_type = component_type;
}

terminator_component_type_t base_component_get_component_type(base_component_t * self)
{
    return (self->component_type);
}

base_component_t * base_component_get_next_child(base_component_t * self, base_component_t * child)
{
    base_component_t * matching_child = NULL;
    fbe_queue_element_t * queue_element = NULL;

    queue_element = base_component_get_queue_element(child);

    if (fbe_queue_is_element_on_queue(queue_element))
        matching_child = (base_component_t *)fbe_queue_next(&self->child_list_head, &child->queue_element);
    return matching_child;
}

base_component_t * base_component_get_first_child(base_component_t * self)
{
    base_component_t * matching_child = NULL;

    if (fbe_queue_is_empty(&self->child_list_head))
        return matching_child;
    matching_child = (base_component_t *)fbe_queue_front(&self->child_list_head);
    return matching_child;
}

void base_component_set_login_pending(base_component_t * self, fbe_bool_t pending)
{
    //self->login_pending = pending;

    if(pending == FBE_FALSE)
    {   //Clearing the bit indicates we completed generating login
       base_component_set_state(self, TERMINATOR_COMPONENT_STATE_LOGIN_COMPLETE);
    }
    else
    {
       base_component_set_state(self, TERMINATOR_COMPONENT_STATE_LOGIN_PENDING);
    }
}

fbe_bool_t base_component_get_login_pending(base_component_t * self)
{
    return (TERMINATOR_COMPONENT_STATE_LOGIN_PENDING == base_component_get_state(self));
}

/* Use recursive to find and remove all children */
fbe_status_t base_component_remove_all_children(base_component_t * self)
{
    fbe_status_t status = FBE_STATUS_OK;
    base_component_t * to_be_removed = NULL;
    fbe_queue_element_t * q_element = NULL;

    to_be_removed = base_component_get_first_child(self);
    while(to_be_removed != NULL)
    {
        if (base_component_get_child_count(to_be_removed) != 0)
        {
            status = base_component_remove_all_children(to_be_removed);
        }
        /* to_be_removed should not have any child at this point, now remove and destroy it */
        q_element = base_component_get_queue_element(to_be_removed);
        status = base_component_remove_child(q_element);
        status = base_component_destroy(to_be_removed, FBE_TRUE);
        /* get the next one */
        to_be_removed = base_component_get_first_child(self);
    }
    return status;
}
/* add children to logout queue, but do not remove them from parents queue*/
/* when miniport_api logout the device, it should mark the logout_complete flag*/
fbe_status_t base_component_add_all_children_to_logout_queue(base_component_t * self, fbe_queue_head_t *logout_queue_head)
{
    fbe_status_t status = FBE_STATUS_OK;
    base_component_t * to_be_removed = NULL;
    logout_queue_element_t * logout_q_element = NULL;

    to_be_removed = base_component_get_first_child(self);
    while(to_be_removed != NULL)
    {
        if (base_component_get_child_count(to_be_removed) != 0)
        {
            status = base_component_add_all_children_to_logout_queue(to_be_removed, logout_queue_head);
        }

        /* allocate a logout_queue_element, fill the device id and add it to logout q */
        logout_q_element = (logout_queue_element_t *)fbe_terminator_allocate_memory(sizeof(logout_queue_element_t));
        if (logout_q_element == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for logout queue element at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* initialize the logout queue element */
        fbe_queue_element_init(&logout_q_element->queue_element);
        logout_q_element->device_id = to_be_removed;

        /* add it to logout queue */
        fbe_queue_push(logout_queue_head, &logout_q_element->queue_element);
        // set state to logout pending
        base_component_set_state(to_be_removed, TERMINATOR_COMPONENT_STATE_LOGOUT_PENDING);
        /* get the next one */
        to_be_removed = base_component_get_next_child(self, to_be_removed);
    }
    return status;
}

fbe_status_t get_device_login_data(base_component_t * self, fbe_cpd_shim_callback_login_t * login_data)
{
    void * attributes = NULL;
    fbe_cpd_shim_callback_login_t * data = NULL;

    /* the device can be an enclosure, a drive, or a virtual phy.  They may have different attributes */
    switch (self->component_type){
        case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
            attributes = base_component_get_attributes(self);
            data = sas_enclosure_info_get_login_data(attributes);
            fbe_move_memory(login_data, data, sizeof(fbe_cpd_shim_callback_login_t));
            break;
        case TERMINATOR_COMPONENT_TYPE_DRIVE:
            attributes = base_component_get_attributes(self);
            data = sas_drive_info_get_login_data(attributes);
            fbe_move_memory(login_data, data, sizeof(fbe_cpd_shim_callback_login_t));
            break;
        case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
            attributes = base_component_get_attributes(self);
            data = sas_virtual_phy_info_get_login_data(attributes);
            fbe_move_memory(login_data, data, sizeof(fbe_cpd_shim_callback_login_t));
            break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

fbe_status_t base_component_io_queue_push(base_component_t * self, fbe_queue_element_t * element)
{
    fbe_queue_push(&self->io_queue_head, element);
    return FBE_STATUS_OK;
}
fbe_queue_element_t * base_component_io_queue_pop(base_component_t * self)
{
    return fbe_queue_pop(&self->io_queue_head);
}
fbe_bool_t base_component_io_queue_is_empty(base_component_t * self)
{
    return fbe_queue_is_empty(&self->io_queue_head);
}
base_component_t * base_component_get_first_device_with_io(base_component_t * self)
{
    base_component_t * child = NULL;

    /* Check if we should start looking for I/O at something other than the beginning.
     */
    if (self->last_io_id != DEVICE_HANDLE_INVALID)
    {
        /* Find this id on our child list.
         */
        child = self->last_io_id;
        if (child != NULL)
        {
            /* Get the next object that we should check for I/O on.
             * By always getting the "next" object we ensure that on each pass
             * a different object is allowed to process I/O.  This allows us to ensure
             * some amount of fairness.
             */
            child = base_component_get_next_child(self, child);
            if (child != NULL)
            {
                /* Save this as that last object we processed I/O on.
                 */
                self->last_io_id = child;
            }
            /* Continue checking until we find an object with I/O.
             */
            while ((child != NULL) && (base_component_io_queue_is_empty(child)))
            {
                if (child != NULL)
                {
                    /* Save this as that last object we processed I/O on.
                     */
                    self->last_io_id = child;
                }

                if (base_component_get_child_count(child) != 0)
                {
                    child = base_component_get_first_device_with_io(child);
                }
                else
                {
                    /* get the next one */
                    child = base_component_get_next_child(self, child);
                }
            } /* end while child not null and the child is empty */
        } /* end if child not null */
    } /* end if last_io_id is not invalid */

    if (child == NULL)
    {
        child = base_component_get_first_child(self);
        self->last_io_id = child;

        while ((child!=NULL)&&(base_component_io_queue_is_empty(child)))
        {
            self->last_io_id = child;

            if (base_component_get_child_count(child) != 0)
            {
                child = base_component_get_first_device_with_io(child);
            }
            else
            {
                /* get the next one */
                child = base_component_get_next_child(self, child);
            }
        }
    }

    if (child == NULL)
    {
        /* Nothing was found, so simply set the last to invalid.
         */
        self->last_io_id = DEVICE_HANDLE_INVALID;
    }
    return child;
}

void base_component_set_logout_complete(base_component_t * self, fbe_bool_t complete)
{
//  self->logut_complete = complete;

    if(complete == FBE_TRUE)
    {
        base_component_set_state(self, TERMINATOR_COMPONENT_STATE_LOGOUT_COMPLETE);
    }
}

fbe_bool_t base_component_get_logout_complete(base_component_t * self)
{
    return (TERMINATOR_COMPONENT_STATE_LOGOUT_COMPLETE == base_component_get_state(self));
}
fbe_status_t base_component_set_all_children_logout_complete(base_component_t * self, fbe_bool_t logut_complete)
{
    fbe_status_t status = FBE_STATUS_OK;
    base_component_t * target_device = NULL;

    target_device = base_component_get_first_child(self);
    while(target_device != NULL)
    {
        if (base_component_get_child_count(target_device) != 0)
        {
            status = base_component_set_all_children_logout_complete(target_device, logut_complete);
        }
        base_component_set_logout_complete(target_device, logut_complete);
        /* get the next one */
        target_device = base_component_get_next_child(self, target_device);
    }
    return status;
}
fbe_status_t base_component_set_all_children_login_pending(base_component_t * self, fbe_bool_t pending)
{
    fbe_status_t status = FBE_STATUS_OK;
    base_component_t * target_device = NULL;

    target_device = base_component_get_first_child(self);
    while(target_device != NULL)
    {
        if (base_component_get_child_count(target_device) != 0)
        {
            status = base_component_set_all_children_login_pending(target_device, pending);
        }
        base_component_set_login_pending(target_device, pending);
        /* get the next one */
        target_device = base_component_get_next_child(self, target_device);
    }
    return status;
}

/**************************************************************************
 *  base_component_set_state()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets the LOGIN/LOGOUT state of the given device.
 *
 *  PARAMETERS:
 *      base object pointer & state to be set.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *      Dont set the LOGOUT_COMPLETE state if the state is LOGIN_PENDING
 *          as any how after logout is generated once again login is
 *          generated. (There may be a narrow timing sequence in which
 *          this may not be always true)
 *
 *  HISTORY:
 *      Oct-27-2008: Rajesh V created.
 **************************************************************************/
void base_component_set_state(base_component_t * self, terminator_component_state_t state)
{
    switch(state)
    {
        case TERMINATOR_COMPONENT_STATE_INITIALIZED:
        case TERMINATOR_COMPONENT_STATE_LOGIN_PENDING:
        case TERMINATOR_COMPONENT_STATE_LOGIN_COMPLETE:
        case TERMINATOR_COMPONENT_STATE_LOGOUT_PENDING:
            self->component_state = state;
            break;
        case TERMINATOR_COMPONENT_STATE_LOGOUT_COMPLETE:
            if(self->component_state != TERMINATOR_COMPONENT_STATE_LOGIN_PENDING)
            {
                self->component_state = state;
            }
            break;
        default:
            self->component_state = TERMINATOR_COMPONENT_STATE_INITIALIZED;
    }
}

/**************************************************************************
 *  base_component_get_state()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets the LOGIN/LOGOUT state of the given device.
 *
 *  PARAMETERS:
 *      base object pointer
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-27-2008: Rajesh V created.
 **************************************************************************/
terminator_component_state_t base_component_get_state(base_component_t *self)
{
    return(self->component_state);
}

/* Device Reset */
void base_component_set_reset_function(base_component_t * self, fbe_terminator_device_reset_function_t reset_function)
{
    self->reset_function = reset_function;
}

fbe_terminator_device_reset_function_t base_component_get_reset_function(base_component_t * self)
{
    return (self->reset_function);
}

void base_component_set_reset_delay(base_component_t * self, fbe_u32_t delay_in_ms)
{
    self->reset_delay = delay_in_ms;
}

fbe_u32_t base_component_get_reset_delay(base_component_t * self)
{
    return (self->reset_delay);
}

void base_component_lock_io_queue(base_component_t * self)
{
    fbe_spinlock_lock(&self->io_queue_lock);
}
void base_component_unlock_io_queue(base_component_t * self)
{
    fbe_spinlock_unlock(&self->io_queue_lock);
}

fbe_status_t base_component_remove_all_children_from_miniport_api_device_table(base_component_t * self, fbe_u32_t port_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    base_component_t * target_device = NULL;

    target_device = base_component_get_first_child(self);
    while(target_device != NULL)
    {
        if (base_component_get_child_count(target_device) != 0)
        {
            status = base_component_remove_all_children_from_miniport_api_device_table(target_device, port_number);
        }
        terminator_remove_device_from_miniport_api_device_table(port_number, target_device);
        /* get the next one */
        target_device = base_component_get_next_child(self, target_device);
    }
    return status;
}
