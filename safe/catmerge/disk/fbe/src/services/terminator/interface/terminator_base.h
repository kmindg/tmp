#ifndef TERMINATOR_BASE_H
#define TERMINATOR_BASE_H

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_queue.h"
#include "fbe_sas.h"
#include "fbe_terminator.h"

#define PORT_DEVICE_ID 1

typedef struct base_component_s
{
    fbe_queue_element_t queue_element;
    fbe_queue_head_t    child_list_head;

    terminator_component_type_t  component_type;
    terminator_component_state_t component_state;

    void *parent_ptr;
    void *attributes;

    fbe_queue_head_t io_queue_head;
    fbe_spinlock_t   io_queue_lock;

    fbe_terminator_device_ptr_t last_io_id; /* last child that processed I/O. */

    fbe_terminator_device_reset_function_t reset_function;
    fbe_u32_t                              reset_delay;
} base_component_t;

typedef struct logout_queue_element_s{
	fbe_queue_element_t	queue_element;
	fbe_terminator_device_ptr_t device_id;
}logout_queue_element_t;

fbe_status_t base_component_init(base_component_t * self);
fbe_status_t base_component_destroy(base_component_t * self, fbe_bool_t delete_self);
fbe_status_t base_component_add_parent(base_component_t * child, base_component_t * parent);
fbe_status_t base_component_remove_parent(base_component_t * child);
fbe_status_t base_component_get_parent(base_component_t * self, base_component_t **parent);
fbe_status_t base_component_add_child(base_component_t * self, fbe_queue_element_t * element);
fbe_status_t base_component_add_child_to_front(base_component_t * self, fbe_queue_element_t * element);
fbe_status_t base_component_remove_child(fbe_queue_element_t * element);
void base_component_assign_attributes(base_component_t * self, void * atrributes_p);
void * base_component_get_attributes(base_component_t * self);
fbe_u64_t base_component_get_child_count(base_component_t * self);
fbe_queue_element_t	* base_component_get_queue_element(base_component_t * self);
//void base_component_assign_id(base_component_t * self, fbe_terminator_api_device_handle_t id);
//fbe_terminator_api_device_handle_t base_component_get_id(base_component_t * self);
//base_component_t * base_component_get_child_by_id(base_component_t * self, fbe_u64_t id);
//base_component_t * base_component_get_child_by_id_and_type(base_component_t * self,
//                                                           fbe_u64_t id,
//                                                           terminator_component_type_t component_type);
base_component_t * base_component_get_child_by_type(base_component_t * self, 
                                                    terminator_component_type_t component_type);
void base_component_set_component_type(base_component_t * self, terminator_component_type_t component_type);
terminator_component_type_t base_component_get_component_type(base_component_t * self);
//fbe_terminator_api_device_handle_t base_generate_device_id(void);
//fbe_terminator_api_device_handle_t base_get_current_device_id(void);
//void base_reset_device_id(void);
base_component_t * base_component_get_next_child(base_component_t * self, base_component_t * child);
base_component_t * base_component_get_first_child(base_component_t * self);
void base_component_set_login_pending(base_component_t * self, fbe_bool_t pending);
fbe_bool_t base_component_get_login_pending(base_component_t * self);
fbe_status_t base_component_remove_all_children(base_component_t * self);
fbe_status_t get_device_login_data(base_component_t * self, fbe_cpd_shim_callback_login_t * login_data);
fbe_status_t base_component_add_all_children_to_logout_queue(base_component_t * self, fbe_queue_head_t *logout_queue_head);
fbe_status_t base_component_io_queue_push(base_component_t * self, fbe_queue_element_t * element);
fbe_queue_element_t * base_component_io_queue_pop(base_component_t * self);
fbe_bool_t base_component_io_queue_is_empty(base_component_t * self);
base_component_t * base_component_get_first_device_with_io(base_component_t * self);
void base_component_set_logout_complete(base_component_t * self, fbe_bool_t complete);
fbe_bool_t base_component_get_logout_complete(base_component_t * self);
fbe_status_t base_component_set_all_children_logout_complete(base_component_t * self, fbe_bool_t logut_complete);
fbe_status_t base_component_set_all_children_login_pending(base_component_t * self, fbe_bool_t pending);
fbe_status_t base_component_destroy_attributes(terminator_component_type_t component_type, void *attributes);
void base_component_set_state(base_component_t * self, terminator_component_state_t state);
terminator_component_state_t base_component_get_state(base_component_t *self);

void base_component_set_reset_function(base_component_t * self, fbe_terminator_device_reset_function_t reset_function);
fbe_terminator_device_reset_function_t base_component_get_reset_function(base_component_t * self);
void base_component_set_reset_delay(base_component_t * self, fbe_u32_t delay_in_ms);
fbe_u32_t base_component_get_reset_delay(base_component_t * self);

void base_component_lock_io_queue(base_component_t * self);
void base_component_unlock_io_queue(base_component_t * self);

fbe_status_t base_component_remove_all_children_from_miniport_api_device_table(base_component_t * self, fbe_u32_t port_number);

#endif  /* TERMINATOR_BASE_H */
