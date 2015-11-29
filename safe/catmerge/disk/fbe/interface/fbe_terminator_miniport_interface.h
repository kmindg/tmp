#ifndef FBE_TERMINATOR_MINIPORT_INTERFACE_H
#define FBE_TERMINATOR_MINIPORT_INTERFACE_H

/*APIs to get/set information in the miniport*/
#include "fbe/fbe_types.h"

#include "fbe_cpd_shim.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_payload_ex.h"

#include "fbe/fbe_queue.h"

#define MAX_DEVICES_PER_SAS_PORT    0x113
#define INVALID_TMSDT_INDEX         MAX_DEVICES_PER_SAS_PORT
#define INDEX_BIT_MASK  0xFFFF

/*! @note The following defines are the per-drive `transfer limits'.  This is 
 *        required so that a single request doesn't starve other drives.  These
 *        defines mimic the current register values on hardware:
 *        \SYSTEM\CurrentControlSet\Services\cpdisql\Parameters\Device\FE_T0=>
 *        B(512,8)M(2048)
 *                                  "                                 \MaximumSGList=>
 *        (130)
 */
#define FBE_CPD_USER_SHIM_MAX_TRANSFER_LENGTH   ((512 + 8) * (2048))
#define FBE_CPD_USER_SHIM_MAX_SG_ENTRIES        (130)

typedef struct fbe_terminator_io_zero_s{
    fbe_bool_t do_unmap;  /* blocks are being unmapped */
}fbe_terminator_io_zero_t;

typedef struct fbe_terminator_io_read_s{
    fbe_bool_t is_unmapped;   /* is any block read unmapped */
    fbe_bool_t is_unmapped_allowed;
}fbe_terminator_io_read_t;

typedef struct fbe_terminator_io_s{
	fbe_queue_element_t		queue_element;
	fbe_payload_ex_t *			payload; /* If payload is not NULL we should proccess it instead of io_block */
	fbe_u32_t        miniport_port_index; /* Async call to fbe_terminator_miniport_api_complete_io (self->miniport_port_index, terminator_io); */
	fbe_bool_t		is_pending; /* Set to 1 to indicate async. completion */
	void *			memory_ptr; /* Pointer  to cont. memory to be released at async. completion */
	void *			xfer_data; /* Used in read completion has to be released */
    fbe_u32_t       opcode;
    fbe_lba_t       lba;
    fbe_block_count_t block_count;
	fbe_block_size_t block_size;
	fbe_u32_t       collision_found;
	void *			device_ptr; /* pointer to the device (terminator_drive_t *) */
    fbe_bool_t b_key_valid;
    fbe_u8_t  keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u32_t return_size;
    fbe_bool_t is_compressed;
    /* based on 'opcode'.  Specific stuff we need to track */
    union
    {
        fbe_terminator_io_zero_t    zero;
        fbe_terminator_io_read_t    read;
    }u;
}fbe_terminator_io_t;


fbe_status_t fbe_terminator_miniport_api_init(void);
fbe_status_t fbe_terminator_miniport_api_destroy(void);

fbe_status_t fbe_terminator_miniport_api_port_init(fbe_u32_t io_port_number, fbe_u32_t io_portal_number, fbe_u32_t *port_handle);
fbe_status_t fbe_terminator_miniport_api_port_destroy(fbe_u32_t port_handle);

typedef fbe_status_t (*fbe_terminator_miniport_api_callback_function_t)(void * callback_info, void * context);
fbe_status_t fbe_terminator_miniport_api_register_callback(fbe_u32_t port_handle,
                                                           fbe_terminator_miniport_api_callback_function_t callback_function,
														   void * callback_context);

fbe_status_t fbe_terminator_miniport_api_unregister_callback(fbe_u32_t port_handle);

fbe_status_t fbe_terminator_miniport_api_register_sfp_event_callback(fbe_u32_t port_handle,
                                                           fbe_terminator_miniport_api_callback_function_t callback_function,
                                                           void * callback_context);

fbe_status_t fbe_terminator_miniport_api_unregister_sfp_event_callback(fbe_u32_t port_handle);

fbe_status_t fbe_terminator_miniport_api_register_payload_completion(fbe_u32_t port_index,
																fbe_payload_ex_completion_function_t completion_function,
																fbe_payload_ex_completion_context_t  completion_context);

fbe_status_t fbe_terminator_miniport_api_unregister_payload_completion(fbe_u32_t port_index);

fbe_status_t fbe_terminator_miniport_api_enumerate_cpd_ports(fbe_cpd_shim_enumerate_backend_ports_t * cpd_shim_enumerate_backend_ports);

fbe_status_t fbe_terminator_miniport_api_get_port_type (fbe_u32_t port_index,fbe_port_type_t *port_type);
fbe_status_t fbe_terminator_miniport_api_remove_port (fbe_u32_t port_index);
fbe_status_t fbe_terminator_miniport_api_port_inserted (fbe_u32_t port_index);
fbe_status_t fbe_terminator_miniport_api_set_speed (fbe_u32_t port_index, fbe_port_speed_t speed);
fbe_status_t fbe_terminator_miniport_api_get_port_info (fbe_u32_t port_index, fbe_port_info_t * port_info);
fbe_status_t fbe_terminator_miniport_api_send_payload (fbe_u32_t port_index, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);
fbe_status_t fbe_terminator_miniport_api_send_fis (fbe_u32_t port_index, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);
fbe_status_t fbe_terminator_miniport_api_get_port_address (fbe_u32_t port_index, fbe_address_t *port_sas_address);
fbe_status_t fbe_terminator_miniport_api_get_hardware_info (fbe_u32_t port_index, fbe_cpd_shim_hardware_info_t * hdw_info);
fbe_status_t fbe_terminator_miniport_api_get_sfp_media_interface_info (fbe_u32_t port_index, fbe_cpd_shim_sfp_media_interface_info_t * sfp_media_interface_info);
fbe_status_t fbe_terminator_miniport_api_port_configuration (fbe_u32_t port_index, fbe_cpd_shim_port_configuration_t *port_config_info);
fbe_status_t fbe_terminator_miniport_api_get_port_link_info (fbe_u32_t port_index, fbe_cpd_shim_port_lane_info_t * port_link_info);
fbe_status_t fbe_terminator_miniport_api_register_keys (fbe_u32_t port_index, fbe_base_port_mgmt_register_keys_t * port_register_keys_p);
fbe_status_t fbe_terminator_miniport_api_reestablish_key_handle(fbe_u32_t port_index, 
                                                                fbe_base_port_mgmt_reestablish_key_handle_t * port_reestablish_key_handle_p);
fbe_status_t fbe_terminator_miniport_api_unregister_keys (fbe_u32_t port_index, fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys_p);

//fbe_status_t fbe_terminator_miniport_api_complete_io (fbe_u32_t port_index, fbe_payload_ex_t *payload);
fbe_status_t fbe_terminator_miniport_api_start_port_reset (fbe_u32_t port_index);
fbe_status_t fbe_terminator_miniport_api_complete_port_reset (fbe_u32_t port_index);
fbe_status_t fbe_terminator_miniport_api_auto_port_reset(fbe_u32_t port_index);
//fbe_status_t fbe_terminator_miniport_api_device_logged_out (fbe_u32_t port_index, fbe_terminator_api_device_handle_t device_id);
fbe_status_t fbe_terminator_miniport_api_device_state_change_notify (fbe_u32_t port_index);
//fbe_status_t fbe_terminator_miniport_api_all_devices_on_port_logged_in (fbe_terminator_component_ptr_t handle);
//fbe_status_t fbe_terminator_miniport_api_all_devices_on_port_logged_out (fbe_terminator_component_ptr_t handle);
fbe_status_t fbe_terminator_miniport_api_reset_device(fbe_u32_t port_index, fbe_miniport_device_id_t cpd_device_id);

fbe_status_t fbe_terminator_miniport_api_event_notify(fbe_terminator_device_ptr_t port_handle, fbe_cpd_shim_sfp_condition_type_t event_type);
fbe_status_t fbe_terminator_miniport_api_link_event_notify(fbe_terminator_device_ptr_t port_ptr, cpd_shim_port_link_state_t  link_state);
/* Sets global completion delay in miliseconds */
fbe_status_t fbe_terminator_miniport_api_set_global_completion_delay(fbe_u32_t global_completion_delay);

typedef fbe_status_t (* fbe_terminator_miniport_api_port_init_function_t)(fbe_u32_t io_port_number, fbe_u32_t io_portal_number, fbe_u32_t *port_handle);
typedef fbe_status_t (* fbe_terminator_miniport_api_port_destroy_function_t)(fbe_u32_t port_handle);

typedef fbe_status_t (* fbe_terminator_miniport_api_register_callback_function_t)(fbe_u32_t port_handle,
                                                           fbe_terminator_miniport_api_callback_function_t callback_function,
														   void * callback_context);

typedef fbe_status_t (* fbe_terminator_miniport_api_unregister_callback_function_t)(fbe_u32_t port_index);

typedef fbe_status_t (* fbe_terminator_miniport_api_register_sfp_event_callback_function_t)(fbe_u32_t port_handle,
                                                           fbe_terminator_miniport_api_callback_function_t callback_function,
                                                           void * callback_context);

typedef fbe_status_t (* fbe_terminator_miniport_api_unregister_sfp_event_callback_function_t)(fbe_u32_t port_index);

typedef fbe_status_t (* fbe_terminator_miniport_api_register_payload_completion_function_t)(fbe_u32_t port_index,
																fbe_payload_ex_completion_function_t completion_function,
																fbe_payload_ex_completion_context_t  completion_context);

typedef fbe_status_t (* fbe_terminator_miniport_api_unregister_payload_completion_function_t)(fbe_u32_t port_index);

typedef fbe_status_t (* fbe_terminator_miniport_api_enumerate_cpd_ports_function_t)(fbe_cpd_shim_enumerate_backend_ports_t * cpd_shim_enumerate_backend_ports);



typedef fbe_status_t (* fbe_terminator_miniport_api_get_port_type_function_t) (fbe_u32_t port_index,fbe_port_type_t *port_type);
typedef fbe_status_t (* fbe_terminator_miniport_api_remove_port_function_t) (fbe_u32_t port_index);
typedef fbe_status_t (* fbe_terminator_miniport_api_port_inserted_function_t) (fbe_u32_t port_index);
typedef fbe_status_t (* fbe_terminator_miniport_api_set_speed_function_t) (fbe_u32_t port_index, fbe_port_speed_t speed);
typedef fbe_status_t (* fbe_terminator_miniport_api_get_port_info_function_t) (fbe_u32_t port_index, fbe_port_info_t * port_info);
typedef fbe_status_t (* fbe_terminator_miniport_api_send_payload_function_t) (fbe_u32_t port_index, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);
typedef fbe_status_t (* fbe_terminator_miniport_api_send_fis_function_t) (fbe_u32_t port_index, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);
typedef fbe_status_t (* fbe_terminator_miniport_api_reset_device_function_t)(fbe_u32_t port_index, fbe_miniport_device_id_t cpd_device_id);
typedef fbe_status_t (* fbe_terminator_miniport_api_get_port_address_t)(fbe_u32_t port_index, fbe_address_t *port_sas_address);
typedef fbe_status_t (* fbe_terminator_miniport_api_get_hardware_info_function_t) (fbe_u32_t port_index, fbe_cpd_shim_hardware_info_t * hdw_info);
typedef fbe_status_t (* fbe_terminator_miniport_api_get_sfp_media_interface_info_function_t) (fbe_u32_t port_index, fbe_cpd_shim_sfp_media_interface_info_t * sfp_media_interface_info);
typedef fbe_status_t (* fbe_terminator_miniport_api_port_configuration_function_t) (fbe_u32_t port_index, fbe_cpd_shim_port_configuration_t *port_config_info);
typedef fbe_status_t (* fbe_terminator_miniport_api_get_port_link_info_function_t) (fbe_u32_t port_index, fbe_cpd_shim_port_lane_info_t * port_link_info);
typedef fbe_status_t (* fbe_terminator_miniport_api_register_keys_function_t) (fbe_u32_t port_index, fbe_base_port_mgmt_register_keys_t * port_register_keys_p);
typedef fbe_status_t (* fbe_terminator_miniport_api_unregister_keys_function_t) (fbe_u32_t port_index, fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys_p);
typedef fbe_status_t (* fbe_terminator_miniport_api_reestablish_key_handle_function_t) (fbe_u32_t port_index, fbe_base_port_mgmt_reestablish_key_handle_t * port_reestablish_key_handle_p);

typedef struct fbe_terminator_miniport_interface_port_shim_sim_pointers_s{
	fbe_terminator_miniport_api_port_init_function_t		terminator_miniport_api_port_init_function;
	fbe_terminator_miniport_api_port_destroy_function_t		terminator_miniport_api_port_destroy_function;
	fbe_terminator_miniport_api_register_callback_function_t terminator_miniport_api_register_callback_function;
	fbe_terminator_miniport_api_unregister_callback_function_t terminator_miniport_api_unregister_callback_function;
	fbe_terminator_miniport_api_register_payload_completion_function_t terminator_miniport_api_register_payload_completion_function;
	fbe_terminator_miniport_api_unregister_payload_completion_function_t terminator_miniport_api_unregister_payload_completion_function;
	fbe_terminator_miniport_api_enumerate_cpd_ports_function_t terminator_miniport_api_enumerate_cpd_ports_function;
	fbe_terminator_miniport_api_register_sfp_event_callback_function_t terminator_miniport_api_register_sfp_event_callback_function;
	fbe_terminator_miniport_api_unregister_sfp_event_callback_function_t terminator_miniport_api_unregister_sfp_event_callback_function;

    fbe_terminator_miniport_api_get_port_type_function_t	terminator_miniport_api_get_port_type_function;
    fbe_terminator_miniport_api_remove_port_function_t		terminator_miniport_api_remove_port_function;
    fbe_terminator_miniport_api_port_inserted_function_t	terminator_miniport_api_port_inserted_function;
    fbe_terminator_miniport_api_set_speed_function_t		terminator_miniport_api_set_speed_function;
    fbe_terminator_miniport_api_get_port_info_function_t	terminator_miniport_api_get_port_info_function;
    fbe_terminator_miniport_api_send_payload_function_t		terminator_miniport_api_send_payload_function;
    fbe_terminator_miniport_api_send_fis_function_t			terminator_miniport_api_send_fis_function;
    fbe_terminator_miniport_api_reset_device_function_t		terminator_miniport_api_reset_device_function;
	fbe_terminator_miniport_api_get_port_address_t			terminator_miniport_api_get_port_address_function;
	fbe_terminator_miniport_api_get_hardware_info_function_t terminator_miniport_api_get_hardware_info_function;
	fbe_terminator_miniport_api_get_sfp_media_interface_info_function_t terminator_miniport_api_get_sfp_media_interface_info_function;
    fbe_terminator_miniport_api_port_configuration_function_t terminator_miniport_api_port_configuration_function;
    fbe_terminator_miniport_api_get_port_link_info_function_t terminator_miniport_api_get_port_link_info_function;
    fbe_terminator_miniport_api_register_keys_function_t      terminator_miniport_api_register_keys_function;
    fbe_terminator_miniport_api_unregister_keys_function_t      terminator_miniport_api_unregister_keys_function;
    fbe_terminator_miniport_api_reestablish_key_handle_function_t    terminator_miniport_api_reestablish_key_handle_function;
}fbe_terminator_miniport_interface_port_shim_sim_pointers_t;


#endif /*FBE_TERMINATOR_MINIPORT_INTERFACE_H*/
