#ifndef SAS_PORT_PRIVATE_H
#define SAS_PORT_PRIVATE_H

#include "fbe/fbe_winddk.h"
#include "fbe_base_port.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "base_port_private.h"

#include "fbe_cpd_shim.h"

#define SAS_PORT_MAX_ENCLOSURES 10

#define SAS_PORT_VENDOR_ID_PMC  0x11F8
/* These are the lifecycle condition ids for a discovering class. */
typedef enum fbe_fc_port_lifecycle_cond_id_e {
	FBE_SAS_PORT_LIFECYCLE_COND_UPDATE_DISCOVERY_TABLE = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_PORT),    
	FBE_SAS_PORT_LIFECYCLE_COND_DRIVER_RESET_BEGIN_RECEIVED,
	FBE_SAS_PORT_LIFECYCLE_COND_WAIT_FOR_DRIVER_RESET_COMPLETE,
	FBE_SAS_PORT_LIFECYCLE_COND_LINK_UP,
	FBE_SAS_PORT_LIFECYCLE_COND_DISCOVERY_START,
	FBE_SAS_PORT_LIFECYCLE_COND_DISCOVERY_COMPLETE,
	FBE_SAS_PORT_LIFECYCLE_COND_LAST /* must be last */
} fbe_fc_port_lifecycle_cond_id_t;

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(fc_port);

typedef struct fbe_fc_port_s{
	fbe_base_port_t base_port;

    //fbe_sas_address_t           port_address;
	fbe_u32_t                   enclosures_number;

	FBE_LIFECYCLE_DEF_INST_DATA;
	FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_PORT_LIFECYCLE_COND_LAST));
}fbe_fc_port_t;

/* Methods */
fbe_status_t fbe_fc_port_init(fbe_fc_port_t * fc_port);
fbe_status_t fbe_fc_port_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);


fbe_status_t fbe_fc_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_fc_port_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_fc_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_fc_port_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_fc_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_fc_port_monitor_load_verify(void);

/* fbe_status_t fc_port_send_io_completion(fbe_io_block_t * io_block, fbe_io_block_completion_context_t context); */

fbe_status_t fbe_fc_port_monitor(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_fc_port_create_edge(fbe_fc_port_t * fc_port, fbe_packet_t * packet);
fbe_status_t fbe_fc_port_get_expander_list(fbe_fc_port_t * fc_port, fbe_packet_t * packet);


fbe_status_t fbe_fc_port_get_port_address(fbe_fc_port_t * fc_port, fbe_sas_address_t * port_address);
fbe_status_t fbe_fc_port_set_port_address(fbe_fc_port_t * fc_port, fbe_sas_address_t port_address);


#endif /* SAS_PORT_PRIVATE_H */
