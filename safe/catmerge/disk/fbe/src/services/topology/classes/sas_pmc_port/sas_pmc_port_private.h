#ifndef SAS_PMC_PORT_PRIVATE_H
#define SAS_PMC_PORT_PRIVATE_H

#include "fbe/fbe_winddk.h"
#include "fbe_base_port.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "fbe_sas.h"
#include "sas_port_private.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_stat.h"


/* These are the lifecycle condition ids for PMC Port class. */
typedef enum fbe_sas_pmc_port_lifecycle_cond_id_e {
	FBE_SAS_PMC_PORT_LIFECYCLE_COND_CTRL_RESET_COMPLETED = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_PMC_PORT),
	FBE_SAS_PMC_PORT_LIFECYCLE_COND_LAST /* must be last */
} fbe_sas_pmc_port_lifecycle_cond_id_t;

#define FBE_SAS_PMC_INVALID_TABLE_INDEX 0xFFFF

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_pmc_port);

#define FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_PROCESS_PENDING			0x00000002
#define FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_DELETE_PENDING				0x00000004

#define FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_SSP_PROTOCOL_EDGE_VALID	0x00000020
#define FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_STP_PROTOCOL_EDGE_VALID	0x00000040
#define FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_SMP_PROTOCOL_EDGE_VALID	0x00000080

#define FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_DEVICE_RESET_OUTSTANDING	0x00000100
#define FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_SMP_CHANGE_EVENT			0x00000200

#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_FLAG(attributes,flag) ((attributes) |= (flag))
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_FLAG(attributes,flag) ((attributes) &= ~(flag))
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_FLAG_SET(attributes,flag) (((attributes)& (flag)) != 0)

#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_SSP_PROTOCOL_EDGE_VALID(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_SSP_PROTOCOL_EDGE_VALID)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_SSP_PROTOCOL_EDGE_VALID(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_SSP_PROTOCOL_EDGE_VALID)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_SSP_PROTOCOL_EDGE_VALID(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_FLAG_SET((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_SSP_PROTOCOL_EDGE_VALID)

#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_STP_PROTOCOL_EDGE_VALID(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_STP_PROTOCOL_EDGE_VALID)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_STP_PROTOCOL_EDGE_VALID(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_STP_PROTOCOL_EDGE_VALID)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_STP_PROTOCOL_EDGE_VALID(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_FLAG_SET((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_STP_PROTOCOL_EDGE_VALID)

#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_SMP_PROTOCOL_EDGE_VALID(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_SMP_PROTOCOL_EDGE_VALID)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_SMP_PROTOCOL_EDGE_VALID(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_SMP_PROTOCOL_EDGE_VALID)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_SMP_PROTOCOL_EDGE_VALID(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_FLAG_SET((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_SMP_PROTOCOL_EDGE_VALID)

#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_PROCESS_PENDING(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_PROCESS_PENDING)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_PROCESS_PENDING(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_PROCESS_PENDING)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_PROCESS_PENDING(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_FLAG_SET((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_PROCESS_PENDING)

#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_DELETE_PENDING(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_SET_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_DELETE_PENDING)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_DELETE_PENDING(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_FLAG((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_DELETE_PENDING)
#define SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_DELETE_PENDING(device_table_ptr) \
	SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_FLAG_SET((device_table_ptr)->device_info_flags,FBE_SAS_PMC_PORT_DEV_TABLE_ENTRY_DELETE_PENDING)

typedef struct fbe_sas_pmc_port_device_table_entry_s{    
	fbe_atomic_t							    current_gen_number;
	fbe_bool_t                              device_logged_in;
	fbe_u32_t								device_info_flags;
    fbe_cpd_shim_discovery_device_type_t    device_type;
	fbe_miniport_device_id_t                device_id;
    fbe_cpd_shim_device_locator_t           device_locator;	
    fbe_sas_address_t                       device_address;
    fbe_generation_code_t                   generation_code;
    fbe_miniport_device_id_t                parent_device_id; 
    fbe_sas_address_t                       parent_device_address;
    fbe_cpd_shim_device_login_reason_t      device_login_reason;
}fbe_sas_pmc_device_table_entry_t;


typedef struct fbe_sas_pmc_port_s{
	fbe_sas_port_t						sas_port;    
	fbe_bool_t							callback_registered;
    fbe_spinlock_t		                list_lock;
	fbe_u32_t							device_table_max_index;
    fbe_sas_pmc_device_table_entry_t   *device_table_ptr;
    fbe_atomic_t                        current_generation_code;
	fbe_bool_t							reset_complete_received;
	fbe_u32_t							da_child_index;    
    fbe_sas_address_t                   port_address;
    fbe_generation_code_t               port_generation_code;
	fbe_bool_t							monitor_da_enclosure_login;
	fbe_bool_t							da_enclosure_activating_fw;
	fbe_u32_t							da_login_delay_poll_count;    

	/* Statistical information */
	fbe_atomic_t	io_counter;
	fbe_u64_t		io_error_tag;
	fbe_stat_t		port_stat;

	FBE_LIFECYCLE_DEF_INST_DATA;
	FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_PMC_PORT_LIFECYCLE_COND_LAST));
}fbe_sas_pmc_port_t;

#define FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port_ptr)	((sas_pmc_port_ptr)->device_table_max_index)
#define FBE_SAS_PMC_IS_DIRECT_ATTACHED_DEVICE(parent_device_id) ((parent_device_id) == -1) /* Temp. */

/* Methods */
fbe_status_t fbe_sas_pmc_port_init(fbe_sas_pmc_port_t * sas_pmc_port);
fbe_status_t fbe_sas_pmc_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_pmc_port_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_pmc_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_pmc_port_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_pmc_port_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_pmc_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_pmc_port_monitor_load_verify(void);
fbe_status_t fbe_sas_pmc_port_create_edge(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
fbe_status_t fbe_sas_pmc_port_get_expander_list(fbe_sas_pmc_port_t * sas_pmc_port, fbe_packet_t * packet);
fbe_status_t sas_pmc_port_send_payload_completion(fbe_payload_ex_t * payload, fbe_payload_ex_completion_context_t context);
fbe_status_t sas_port_pmc_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context);

#endif /* SAS_PMC_PORT_PRIVATE_H */
