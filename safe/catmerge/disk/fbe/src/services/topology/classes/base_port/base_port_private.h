#ifndef BASE_PORT_PRIVATE_H
#define BASE_PORT_PRIVATE_H

#include "fbe_base_port.h"
#include "base_discovering_private.h"
#include "fbe/fbe_stat.h"
#include "fbe/fbe_port.h"
#include "fbe_cpd_shim.h"
#include "fbe/fbe_encryption.h"

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_port);

/* These are the lifecycle condition ids for a base port class. */
typedef enum fbe_base_port_lifecycle_cond_id_e {
	FBE_BASE_PORT_LIFECYCLE_COND_UPDATE_PORT_NUMBER = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BASE_PORT),
	FBE_BASE_PORT_LIFECYCLE_COND_INIT_MINIPORT_SHIM,
    FBE_BASE_PORT_LIFECYCLE_COND_GET_HARDWARE_INFO,
	FBE_BASE_PORT_LIFECYCLE_COND_GET_CONFIG,
    FBE_BASE_PORT_LIFECYCLE_COND_SFP_INFORMATION_CHANGE,
	FBE_BASE_PORT_LIFECYCLE_COND_GET_CAPABILITIES, /* Protocol specific*/    
    /*FBE_BASE_PORT_LIFECYCLE_COND_GET_PORT_NAME,  Protocol specific*/    
    FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CALLBACKS,
    FBE_BASE_PORT_LIFECYCLE_COND_GET_PORT_DATA, /* Protocol specific*/
    FBE_BASE_PORT_LIFECYCLE_COND_UNREGISTER_CALLBACKS,
	FBE_BASE_PORT_LIFECYCLE_COND_DESTROY_MINIPORT_SHIM,
    FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CONFIG,
	FBE_BASE_PORT_LIFECYCLE_COND_UNREGISTER_CONFIG,
	FBE_BASE_PORT_LIFECYCLE_COND_LAST /* must be last */
} fbe_base_port_lifecycle_cond_id_t;

typedef struct fbe_base_port_s{
	fbe_base_discovering_t base_discovering;
    fbe_port_connect_class_t port_connect_class;
    fbe_port_role_t   port_role;	
	fbe_port_number_t port_number;
	fbe_u32_t io_port_number;
	fbe_u32_t io_portal_number;
	fbe_u32_t assigned_bus_number;

    fbe_port_hardware_info_t  hardware_info;
    fbe_port_sfp_info_t       port_sfp_info; /* TODO: Query and fill this as part of ACTIVATE rotary.*/

	fbe_number_of_enclosures_t number_of_enclosures;
	fbe_drive_configuration_handle_t port_configuration_handle;
    fbe_u32_t                 maximum_transfer_length;
    fbe_u32_t                 maximum_sg_entries;

    fbe_port_link_information_t port_link_info;

    fbe_bool_t       multi_core_affinity_enabled;
    fbe_u64_t        active_proc_mask;
    fbe_bool_t       debug_remove;
    fbe_key_handle_t	kek_handle;
    fbe_key_handle_t	kek_kek_handle;
    fbe_u8_t kek_key[FBE_ENCRYPTION_WRAPPED_KEK_SIZE];

    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BASE_PORT_LIFECYCLE_COND_LAST));
}fbe_base_port_t;

/* Methods */
fbe_status_t fbe_base_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_port_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_base_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_port_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_base_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_base_port_monitor_load_verify(void);

fbe_status_t fbe_base_port_set_port_info(fbe_base_port_t * p_base_port);
fbe_status_t fbe_base_port_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);


fbe_status_t fbe_base_port_set_port_number(fbe_base_port_t * base_port, fbe_port_number_t port_number);
fbe_status_t fbe_base_port_get_port_number(fbe_base_port_t * base_port, fbe_port_number_t * port_number);
fbe_status_t fbe_base_port_get_io_port_number(fbe_base_port_t * p_base_port, fbe_u32_t * p_io_port_number);
fbe_status_t fbe_base_port_get_io_portal_number(fbe_base_port_t * p_base_port, fbe_u32_t * p_io_portal_number);
fbe_status_t fbe_base_port_set_assigned_bus_number(fbe_base_port_t * base_port, fbe_u32_t assigned_bus_number);
fbe_status_t fbe_base_port_get_assigned_bus_number(fbe_base_port_t * base_port, fbe_u32_t * assigned_bus_number);
/* fbe_status_t fbe_base_port_get_port_type(fbe_base_port_t * base_port, fbe_port_type_t   *port_type);*/
fbe_status_t fbe_base_port_set_port_connect_class(fbe_base_port_t * base_port, fbe_port_connect_class_t   connect_class);
fbe_status_t fbe_base_port_get_port_connect_class(fbe_base_port_t * base_port, fbe_port_connect_class_t   *connect_class);
fbe_status_t fbe_base_port_set_port_role(fbe_base_port_t * base_port, fbe_port_role_t  port_role);
fbe_status_t fbe_base_port_get_port_role(fbe_base_port_t * base_port, fbe_port_role_t   *port_role);
fbe_status_t fbe_base_port_get_port_vendor_id(fbe_base_port_t * base_port, fbe_u32_t   *vendor_id);
fbe_status_t fbe_base_port_get_port_device_id(fbe_base_port_t * base_port, fbe_u32_t   *device_id);
fbe_status_t fbe_base_port_set_hardware_info(fbe_base_port_t * base_port, fbe_port_hardware_info_t *port_hardware_info);
fbe_status_t fbe_base_port_get_hardware_info(fbe_base_port_t * base_port, fbe_port_hardware_info_t *port_hardware_info);
fbe_status_t fbe_base_port_set_sfp_info(fbe_base_port_t * base_port, fbe_port_sfp_info_t *port_sfp_info);
fbe_status_t fbe_base_port_get_sfp_info(fbe_base_port_t * base_port, fbe_port_sfp_info_t *port_sfp_info);
fbe_status_t fbe_base_port_set_maximum_transfer_length(fbe_base_port_t * base_port, fbe_u32_t maximum_transfer_length);
fbe_status_t fbe_base_port_get_maximum_transfer_length(fbe_base_port_t * base_port, fbe_u32_t * maximum_transfer_length);
fbe_status_t fbe_base_port_set_maximum_sg_entries(fbe_base_port_t * base_port, fbe_u32_t maximum_sg_entries);
fbe_status_t fbe_base_port_get_maximum_sg_entries(fbe_base_port_t * base_port, fbe_u32_t * maximum_sg_entries);
fbe_status_t fbe_base_port_set_link_info(fbe_base_port_t * base_port, fbe_port_link_information_t *port_link_info_p);
fbe_status_t fbe_base_port_get_link_info(fbe_base_port_t * base_port, fbe_port_link_information_t *port_link_info_p);
fbe_status_t fbe_base_port_set_core_affinity_info(fbe_base_port_t * base_port, fbe_bool_t affinity_enabled, fbe_u64_t active_proc_mask);
fbe_status_t fbe_base_port_get_core_affinity_info(fbe_base_port_t * base_port, fbe_bool_t *affinity_enabled, fbe_u64_t *active_proc_mask);
fbe_status_t fbe_base_port_get_debug_remove(fbe_base_port_t * base_port, fbe_bool_t *debug_remove_p);
fbe_status_t fbe_base_port_set_debug_remove(fbe_base_port_t * base_port, fbe_bool_t debug_remove);
fbe_status_t fbe_base_port_get_kek_handle(fbe_base_port_t * base_port, fbe_key_handle_t *kek_handle);
fbe_status_t fbe_base_port_get_kek_kek_handle(fbe_base_port_t * base_port, fbe_key_handle_t *kek_kek_handle);
fbe_status_t fbe_base_port_set_kek_handle(fbe_base_port_t * base_port, fbe_key_handle_t kek_handle);

#endif /* FBE_BASE_PORT_PRIVATE_H */
