/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#ifndef TERMINATOR_PORT_H
#define TERMINATOR_PORT_H

#include "terminator_base.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_port.h"
#include "fbe_sas.h"
#include "terminator_enclosure.h"
#include "terminator_sas_port.h"
#include "terminator_fc_port.h"

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_encryption.h"

#define SAS_PORT_DEVICE_ID                  0x1
#define TERMINATOR_PORT_IO_DEFAULT_THREAD_COUNT 0x01 /* defines the default number of io threads per port. */
#define TERMINATOR_PORT_IO_MAX_THREAD_COUNT     0x10 /* defines the max number of io threads per port. */

extern fbe_u32_t terminator_port_io_thread_count;
extern fbe_u32_t terminator_port_count;

struct terminator_port_s;

typedef enum io_thread_flag_e{
    IO_THREAD_RUN,
    IO_THREAD_STOP,
    IO_THREAD_DONE
}io_thread_flag_t;

typedef struct port_io_thread_s{
    fbe_thread_t     io_thread_handle; /* handle of the io thread for this port.  Can add more in the future */
    io_thread_flag_t io_thread_flag;
}port_io_thread_t;

typedef struct io_queue_element_s{
    fbe_queue_element_t         queue_element;
    fbe_terminator_device_ptr_t device;
}io_queue_element_t;

typedef struct terminator_port_io_thread_context_s{
    fbe_u32_t io_thread_id;
    struct terminator_port_s * port_p;
}terminator_port_io_thread_context_t;

/* These are the common stuff for all port type */
typedef struct terminator_port_s{
    base_component_t base;
    //fbe_spinlock_t   update_lock;
	fbe_mutex_t		update_mutex;	
    fbe_port_type_t  port_type;
    fbe_port_speed_t port_speed;
    fbe_u32_t        maximum_transfer_bytes;
    fbe_u32_t        maximum_sg_entries;
    fbe_queue_head_t logout_queue_head;
    fbe_spinlock_t   logout_queue_lock;
    fbe_semaphore_t  io_update_semaphore;
    port_io_thread_t io_thread[TERMINATOR_PORT_IO_MAX_THREAD_COUNT];
    terminator_port_io_thread_context_t io_thread_context[TERMINATOR_PORT_IO_MAX_THREAD_COUNT];
    // port reset is also applicable for FC ports. So put the flags here but may what we do
    // when these flags are set may be different for different kinds of ports
    fbe_bool_t       reset_begin;
    fbe_bool_t       reset_completed;
    fbe_u32_t        miniport_port_index;
    fbe_queue_head_t io_queue_head; /* this queue is used to remember the sequence of the io when it comes in */
    fbe_queue_head_t keys_queue_head;
    fbe_spinlock_t   keys_queue_spinlock;
}terminator_port_t;

typedef struct terminator_key_info_s{
    fbe_queue_element_t queue_element;
    fbe_u8_t  keys[FBE_ENCRYPTION_KEY_SIZE];
}terminator_key_info_t;


/* These are the port attributes */
typedef struct terminator_port_info_s{
    union
    {
        terminator_sas_port_info_t sas_info;
        terminator_fc_port_info_t fc_info;
    }type_specific_info;
    fbe_port_connect_class_t connect_class;
    fbe_port_role_t     port_role;
    fbe_cpd_shim_hardware_info_t hdw_info;
    fbe_cpd_shim_sfp_media_interface_info_t sfp_media_interface_info;
    fbe_cpd_shim_port_lane_info_t  port_lane_info;
}terminator_port_info_t;

/* Constructor of a port object */
terminator_port_t * port_new(void);
/* desctructor */
fbe_status_t     port_destroy(terminator_port_t * self);
/* port object interfaces */
fbe_status_t     port_set_type(terminator_port_t * self, fbe_port_type_t type);
fbe_port_type_t  port_get_type(terminator_port_t * self);
fbe_status_t     port_set_speed(terminator_port_t * self, fbe_port_speed_t port_speed);
fbe_port_speed_t port_get_speed(terminator_port_t * self);
fbe_status_t     port_set_maximum_transfer_bytes(terminator_port_t * self, fbe_u32_t maximum_transfer_bytes);
fbe_u32_t        port_get_maximum_transfer_bytes(terminator_port_t * self);
fbe_status_t     port_set_maximum_sg_entries(terminator_port_t * self, fbe_u32_t maximum_sg_entries);
fbe_u32_t        port_get_maximum_sg_entries(terminator_port_t * self);
fbe_status_t     port_enumerate_devices(terminator_port_t * self, fbe_terminator_device_ptr_t device_list[], fbe_u32_t *total_devices);
fbe_status_t port_add_sas_enclosure(terminator_port_t * self, terminator_enclosure_t * new_enclosure, fbe_bool_t new_virtual_phy);
fbe_status_t port_destroy_device(terminator_port_t * self, fbe_terminator_device_ptr_t component_handle);
fbe_status_t port_unmount_device(terminator_port_t * self, fbe_terminator_device_ptr_t component_handle);
fbe_status_t port_logout_device(terminator_port_t * self, fbe_terminator_device_ptr_t component_handle);
fbe_status_t port_io_thread_init(port_io_thread_t * self);
fbe_status_t port_io_thread_start(port_io_thread_t * io_thread, IN fbe_thread_user_root_t  StartRoutine, IN PVOID  StartContext);
fbe_status_t port_io_thread_destroy(terminator_port_t * self);
fbe_status_t port_login_device(terminator_port_t * self, fbe_terminator_device_ptr_t component_handle);
fbe_status_t port_pop_logout_device(terminator_port_t * self, fbe_terminator_device_ptr_t *device_id);
fbe_status_t port_get_next_logout_device(terminator_port_t * self, const fbe_terminator_device_ptr_t device_id, fbe_terminator_device_ptr_t *next_device_id);
fbe_status_t port_front_logout_device(terminator_port_t * self, fbe_terminator_device_ptr_t *device_id);
fbe_status_t port_get_device_logout_complete(terminator_port_t * self, fbe_terminator_api_device_handle_t device_id, fbe_bool_t *flag);
fbe_status_t port_logout(terminator_port_t * self);
fbe_status_t port_is_logout_complete(terminator_port_t * self, fbe_bool_t *flag);
fbe_status_t port_abort_device_io(terminator_port_t * self, fbe_terminator_device_ptr_t device_ptr);
fbe_status_t port_is_device_logout_pending(terminator_port_t * self, fbe_terminator_device_ptr_t device_ptr, fbe_bool_t *pending);
fbe_status_t port_is_logout_queue_empty(terminator_port_t * self, fbe_bool_t *is_empty);
fbe_status_t port_get_sas_port_attributes(terminator_port_t * self, fbe_terminator_port_shim_backend_port_info_t * port_info);
fbe_status_t port_get_fc_port_attributes(terminator_port_t * self, fbe_terminator_port_shim_backend_port_info_t * port_info);


fbe_u32_t port_get_enclosure_count(terminator_port_t * self);
base_component_t * port_get_enclosure_by_chain_depth(terminator_port_t * self, fbe_u8_t enclosure_chain_depth);
terminator_enclosure_t * port_get_enclosure_by_enclosure_number(terminator_port_t * self, fbe_u32_t enclosure_number);
terminator_enclosure_t *port_get_last_enclosure(terminator_port_t * self);
void port_lock_update_lock(terminator_port_t * self);
void port_unlock_update_lock(terminator_port_t * self);

fbe_port_connect_class_t port_get_connect_class(terminator_port_t * self);
fbe_status_t port_set_connect_class(terminator_port_t * self, fbe_port_connect_class_t connect_class);
fbe_port_role_t port_get_role(terminator_port_t * self);
fbe_status_t port_set_role(terminator_port_t * self, fbe_port_role_t port_role);
fbe_status_t port_get_hardware_info(terminator_port_t * self, fbe_cpd_shim_hardware_info_t *hdw_info);
fbe_status_t port_register_keys (terminator_port_t * self, fbe_base_port_mgmt_register_keys_t * port_register_keys_p);
fbe_status_t port_unregister_keys (terminator_port_t * self, fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys_p);
fbe_status_t port_reestablish_key_handle(terminator_port_t * self, fbe_base_port_mgmt_reestablish_key_handle_t * port_reestablish_key_handle_p);
fbe_status_t port_set_hardware_info(terminator_port_t * self, fbe_cpd_shim_hardware_info_t *hdw_info);
fbe_status_t port_get_sfp_media_interface_info(terminator_port_t * self, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info);
fbe_status_t port_set_sfp_media_interface_info(terminator_port_t * self, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info);
fbe_status_t port_get_port_configuration(terminator_port_t * self, fbe_cpd_shim_port_configuration_t *port_config_info);
fbe_status_t port_set_port_configuration(terminator_port_t * self, fbe_cpd_shim_port_configuration_t *port_config_info);
fbe_status_t port_get_port_link_info(terminator_port_t * self, fbe_cpd_shim_port_lane_info_t *port_lane_info);
fbe_status_t port_set_port_link_info(terminator_port_t * self, fbe_cpd_shim_port_lane_info_t *port_lane_info);
fbe_status_t port_get_encryption_key(terminator_port_t *self, fbe_key_handle_t key_handle, fbe_u8_t *key_buffer);

/* sas port */
terminator_sas_port_info_t *    sas_port_info_new(fbe_terminator_sas_port_info_t *user_data);
terminator_sas_port_info_t *    allocate_sas_port_info(void);
fbe_sas_address_t               sas_port_get_address(terminator_port_t * self);
void                            sas_port_set_address(terminator_port_t * self, fbe_sas_address_t sas_address);
fbe_u32_t                       sas_port_get_backend_number(terminator_port_t * self);
void                            sas_port_set_backend_number(terminator_port_t * self, fbe_u32_t port_number);
fbe_u32_t                       sas_port_get_io_port_number(terminator_port_t * self);
void                            sas_port_set_io_port_number(terminator_port_t * self, fbe_u32_t io_port_number);
fbe_u32_t                       sas_port_get_portal_number(terminator_port_t * self);
void                            sas_port_set_portal_number(terminator_port_t * self, fbe_u32_t portal_number);

/* fc port */
fbe_status_t fc_port_call_io_api(terminator_port_t * self,fbe_terminator_io_t * terminator_io);
void fc_port_process_io_block(terminator_port_t * self);
fbe_status_t port_add_fc_enclosure(terminator_port_t * self, terminator_enclosure_t * new_enclosure);
fbe_u32_t  fc_port_get_backend_number(terminator_port_t * self);
fbe_diplex_address_t fc_port_get_address(terminator_port_t * self);
fbe_u32_t fc_port_get_io_port_number(terminator_port_t * self);
fbe_u32_t fc_port_get_portal_number(terminator_port_t * self);
void  fc_port_set_io_port_number(terminator_port_t * self, fbe_u32_t io_port_number);
void fc_port_set_portal_number(terminator_port_t * self, fbe_u32_t portal_number);
void fc_port_set_backend_number(terminator_port_t * self, fbe_u32_t port_number);
void fc_port_set_address(terminator_port_t * self, fbe_diplex_address_t diplex_address);
fbe_status_t terminator_diplex_poll_handler(terminator_port_t * self, fbe_payload_ex_t * payload);
fbe_status_t terminator_diplex_read_handler(terminator_port_t * self, fbe_payload_ex_t * payload);
fbe_status_t terminator_diplex_write_handler(terminator_port_t * self, fbe_payload_ex_t * payload);

fbe_bool_t port_is_matching(terminator_port_t * self, fbe_u32_t io_port_number, fbe_u32_t io_portal_number);

fbe_bool_t port_get_reset_begin(terminator_port_t * self);
fbe_bool_t port_get_reset_completed(terminator_port_t * self);
void port_set_reset_begin(terminator_port_t * self, fbe_bool_t state);
void port_set_reset_completed(terminator_port_t * self, fbe_bool_t state);
fbe_status_t port_activate(terminator_port_t * self);

fbe_status_t port_set_miniport_port_index(terminator_port_t * self, fbe_u32_t port_index);
fbe_status_t port_get_miniport_port_index(terminator_port_t * self, fbe_u32_t *port_index);
fbe_status_t port_enqueue_io(terminator_port_t * self, 
                             fbe_terminator_device_ptr_t handle, 
                             fbe_queue_element_t * element_to_enqueue);

fbe_status_t terminator_port_initialize_protocol_specific_data(terminator_port_t * port_handle);

#endif /* TERMINATOR_PORT_H */
