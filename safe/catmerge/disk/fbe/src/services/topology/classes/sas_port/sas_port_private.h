#ifndef SAS_PORT_PRIVATE_H
#define SAS_PORT_PRIVATE_H

#include "fbe/fbe_winddk.h"
#include "fbe_base_port.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "base_port_private.h"

#include "fbe_cpd_shim.h"
#include "fbe_ssp_transport.h"
#include "fbe_smp_transport.h"
#include "fbe_stp_transport.h"

#define SAS_PORT_MAX_ENCLOSURES 10

#define SAS_PORT_VENDOR_ID_PMC  0x11F8
/* These are the lifecycle condition ids for a discovering class. */
typedef enum fbe_sas_port_lifecycle_cond_id_e {
	FBE_SAS_PORT_LIFECYCLE_COND_UPDATE_DISCOVERY_TABLE = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_PORT),    
	FBE_SAS_PORT_LIFECYCLE_COND_DRIVER_RESET_BEGIN_RECEIVED,
	FBE_SAS_PORT_LIFECYCLE_COND_WAIT_FOR_DRIVER_RESET_COMPLETE,
	FBE_SAS_PORT_LIFECYCLE_COND_LINK_UP,
    FBE_SAS_PORT_LIFECYCLE_COND_LAST /* must be last */
} fbe_sas_port_lifecycle_cond_id_t;

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_port);

typedef struct fbe_sas_port_s{
	fbe_base_port_t base_port;
	fbe_ssp_transport_server_t	ssp_transport_server;
	fbe_smp_transport_server_t	smp_transport_server;
	fbe_stp_transport_server_t	stp_transport_server;

    fbe_sas_address_t           port_address;
	fbe_u32_t                   enclosures_number;

    fbe_bool_t                  power_failure_detected;

	FBE_LIFECYCLE_DEF_INST_DATA;
	FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_PORT_LIFECYCLE_COND_LAST));
}fbe_sas_port_t;


typedef struct fbe_sas_port_device_table_entry_s{

    fbe_bool_t                              entry_valid;
    fbe_sas_element_type_t                  device_type;
	fbe_miniport_device_id_t                device_id;
    fbe_cpd_shim_device_locator_t           device_locator;

    fbe_sas_address_t                       device_address;
    fbe_generation_code_t                   generation_code;
    fbe_miniport_device_id_t                parent_device_id; 
    fbe_sas_address_t                       parent_device_address;   

}fbe_sas_port_device_table_entry_t;

/* Methods */
fbe_status_t fbe_sas_port_init(fbe_sas_port_t * sas_port);
fbe_status_t fbe_sas_port_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);


fbe_status_t fbe_sas_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_port_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_port_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_port_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sas_port_monitor_load_verify(void);

/* fbe_status_t sas_port_send_io_completion(fbe_io_block_t * io_block, fbe_io_block_completion_context_t context); */

fbe_status_t fbe_sas_port_monitor(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_port_create_edge(fbe_sas_port_t * sas_port, fbe_packet_t * packet);
fbe_status_t fbe_sas_port_get_expander_list(fbe_sas_port_t * sas_port, fbe_packet_t * packet);


fbe_status_t fbe_sas_port_ssp_transport_entry(fbe_sas_port_t * sas_port,
												fbe_transport_entry_t transport_entry,
												fbe_packet_t * packet);

fbe_status_t fbe_sas_port_stp_transport_entry(fbe_sas_port_t * sas_port,
												fbe_transport_entry_t transport_entry,
												fbe_packet_t * packet);

fbe_status_t fbe_sas_port_set_ssp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr);
fbe_status_t fbe_sas_port_clear_ssp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr);

fbe_status_t
fbe_sas_port_smp_transport_entry(fbe_sas_port_t * sas_port,
                                 fbe_transport_entry_t transport_entry,
                                 fbe_packet_t * packet);

fbe_status_t fbe_sas_port_set_smp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr);
fbe_status_t fbe_sas_port_clear_smp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr);

fbe_status_t fbe_sas_port_set_stp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr);
fbe_status_t fbe_sas_port_clear_stp_path_attr(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index, fbe_path_attr_t path_attr);

fbe_smp_edge_t *fbe_sas_port_get_smp_edge_by_server_index(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index);
fbe_ssp_edge_t *fbe_sas_port_get_ssp_edge_by_server_index(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index);
fbe_stp_edge_t *fbe_sas_port_get_stp_edge_by_server_index(fbe_sas_port_t * sas_port, fbe_edge_index_t server_index);

fbe_status_t fbe_sas_port_get_port_address(fbe_sas_port_t * sas_port, fbe_sas_address_t * port_address);
fbe_status_t fbe_sas_port_set_port_address(fbe_sas_port_t * sas_port, fbe_sas_address_t port_address);

fbe_bool_t fbe_sas_port_get_power_failure_detected(fbe_sas_port_t *sas_port);

#endif /* SAS_PORT_PRIVATE_H */
