#ifndef FBE_PMC_SHIM_H
#define FBE_PMC_SHIM_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_atomic.h"
#include "fbe_fibre.h"

#include "fbe/fbe_payload_ex.h"
#include "fbe/fbe_port.h"
#include "fbe_sas.h"
#include "fbe_cpd_shim.h"

#define FBE_PMC_SHIM_MAX_PORTS FBE_CPD_SHIM_MAX_PORTS


/* From cpd_interface.h */
/* This macro returns the device index portion of the miniport_login_context.
 * The index is a unique, zero-based, consecutive numbering of devices.
 */
#define FBE_CPD_GET_INDEX_FROM_CONTEXT(m_mlc)                           \
            ((fbe_u16_t)((UINT_PTR)(m_mlc) & 0xFFFF))


typedef enum fbe_pmc_shim_interface_type_e {
	FBE_PMC_SHIM_INTERFACE_TYPE_INVALID,

	FBE_PMC_SHIM_INTERFACE_TYPE_FCDMTL,
	FBE_PMC_SHIM_INTERFACE_TYPE_SASBELSI,
    FBE_PMC_SHIM_INTERFACE_TYPE_DMRB,

	FBE_PMC_SHIM_INTERFACE_TYPE_LAST
}fbe_pmc_shim_interface_type_t;

typedef struct fbe_pmc_shim_backend_port_info_s {
	fbe_u32_t	port_number;
	fbe_u32_t	portal_number;
	fbe_u32_t	assigned_bus_number;
	fbe_port_type_t	port_type;
}fbe_pmc_shim_backend_port_info_t;

typedef struct fbe_pmc_shim_enumerate_backend_ports_s {
	fbe_bool_t							rescan_required;
	fbe_u32_t                           number_of_io_ports; /* Number of backend SAS IO ports*/
    fbe_u32_t                           total_discovered_io_ports; /* Number of SAS IO portals - Includes BE,FE & UNC*/
	fbe_pmc_shim_backend_port_info_t    io_port_array[FBE_PMC_SHIM_MAX_PORTS];
}fbe_pmc_shim_enumerate_backend_ports_t;


#define FBE_PMC_SHIM_MAX_IO_MODULES     32

typedef enum fbe_pmc_io_module_type_e {
	FBE_PMC_SHIM_IO_MODULE_TYPE_INVALID,

	FBE_PMC_SHIM_IO_MODULE_TYPE_SAS,
	FBE_PMC_SHIM_IO_MODULE_TYPE_FC,
    FBE_PMC_SHIM_IO_MODULE_TYPE_ISCSI,
    FBE_PMC_SHIM_IO_MODULE_TYPE_UNKNOWN,

	FBE_PMC_SHIM_IO_MODULE_TYPE_LAST
}fbe_pmc_io_module_type_t;

typedef struct fbe_pmc_io_module_info_s {
    fbe_pmc_io_module_type_t io_module_type;
	fbe_bool_t               inserted;
	fbe_bool_t               power_good;
	fbe_u32_t	             port_count;	
}fbe_pmc_io_module_info_t;

typedef struct fbe_pmc_shim_enumerate_io_modules_s {
	fbe_bool_t						rescan_required;
	fbe_u32_t                       number_of_io_modules; /* Number of SAS IO SLICs. (includes mezzanine controllers)*/
    fbe_u32_t                       total_enumerated_io_ports; /* Number of SAS IO portals (each SLIC could have multiple portals)*/
	fbe_pmc_io_module_info_t        io_module_array[FBE_PMC_SHIM_MAX_IO_MODULES];
}fbe_pmc_shim_enumerate_io_modules_t;

typedef enum fbe_pmc_shim_callback_type_e {
	FBE_PMC_SHIM_CALLBACK_TYPE_INVALID,
	FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN,
	FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN_FAILED,
	FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT,
	FBE_PMC_SHIM_CALLBACK_TYPE_DEVICE_FAILED,

    FBE_PMC_SHIM_CALLBACK_TYPE_DISCOVERY,
    FBE_PMC_SHIM_CALLBACK_TYPE_LINK_UP,
    FBE_PMC_SHIM_CALLBACK_TYPE_LINK_DOWN,
    FBE_PMC_SHIM_CALLBACK_TYPE_LINK_DEGRADED,

	FBE_PMC_SHIM_CALLBACK_TYPE_DRIVER_RESET,

	/* Port notifications are obsolete and will be removed.*/
    FBE_PMC_SHIM_CALLBACK_TYPE_PORT_UP,
    FBE_PMC_SHIM_CALLBACK_TYPE_PORT_DOWN,
	FBE_PMC_SHIM_CALLBACK_TYPE_DEVICE_TABLE_UPDATE,
	FBE_PMC_SHIM_CALLBACK_TYPE_PORT_STATUS_UPDATE,

	FBE_PMC_SHIM_CALLBACK_TYPE_LAST
}fbe_pmc_shim_callback_type_t;


#define FBE_MAX_EXPANDERS 10

typedef enum fbe_pmc_shim_discovery_device_type_e{
    FBE_PMC_SHIM_DEVICE_TYPE_INVALID,
    FBE_PMC_SHIM_DEVICE_TYPE_SSP,
    FBE_PMC_SHIM_DEVICE_TYPE_STP,
    FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE,
    FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL,    
    FBE_PMC_SHIM_DEVICE_TYPE_LAST
}fbe_pmc_shim_discovery_device_type_t;

typedef struct fbe_pmc_shim_device_locator_s{
    fbe_u8_t            enclosure_chain_depth;
    fbe_u8_t            enclosure_chain_width;
    fbe_u8_t            phy_number;
    fbe_u8_t            padding;
}fbe_pmc_shim_device_locator_t;

typedef struct fbe_pmc_shim_callback_login_s {
    fbe_pmc_shim_discovery_device_type_t device_type;    
	fbe_miniport_device_id_t             device_id;
    fbe_sas_address_t                    device_address;
    fbe_pmc_shim_device_locator_t        device_locator;
    fbe_miniport_device_id_t             parent_device_id; 
    fbe_sas_address_t                    parent_device_address;
}fbe_pmc_shim_callback_login_t;

typedef enum fbe_pmc_shim_discovery_event_type_e {
	FBE_PMC_SHIM_DISCOVERY_EVENT_TYPE_INVALID,

	FBE_PMC_SHIM_DISCOVERY_EVENT_TYPE_START,
	FBE_PMC_SHIM_DISCOVERY_EVENT_TYPE_COMPLETE,

	FBE_PMC_SHIM_DISCOVERY_EVENT_TYPE_LAST
}fbe_pmc_shim_discovery_event_type_t;

typedef struct fbe_pmc_shim_callback_discovery_s {
	fbe_pmc_shim_discovery_event_type_t discovery_event_type;
}fbe_pmc_shim_callback_discovery_t;

typedef enum fbe_pmc_shim_driver_reset_event_type_e {
	FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_INVALID,

	FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_BEGIN,
	FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_COMPLETED,

	FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_LAST
}fbe_pmc_shim_driver_reset_event_type_t;

typedef struct fbe_pmc_shim_callback_driver_reset_s {
	fbe_pmc_shim_driver_reset_event_type_t driver_reset_event_type;
	fbe_u32_t							   additional_information;/* For future use.*/
}fbe_pmc_shim_callback_driver_reset_t;

typedef struct fbe_pmc_shim_callback_port_up_s {
	fbe_u32_t   portal_number;     /* Port number assigned by the miniport.*/
    fbe_u32_t   nr_phys;         /* Number of phys that make up this port.*/
    fbe_u32_t   phy_map;         /* Map of the phys that ake up this port.*/
    fbe_u32_t   nr_phys_enabled; /* Number of phys that are currently enabled.*/
    fbe_u32_t   nr_phys_up;     /* Number of phys that are currently up.*/
}fbe_pmc_shim_callback_port_up_t;


typedef enum pmc_shim_physical_port_state_e {
	PMC_SHIM_PHYSICAL_PORT_STATE_INVALID,
	PMC_SHIM_PHYSICAL_PORT_STATE_LINK_DOWN,
	PMC_SHIM_PHYSICAL_PORT_STATE_LINK_UP,
	PMC_SHIM_PHYSICAL_PORT_STATE_LINK_UP_DEGRADED,
	PMC_SHIM_PHYSICAL_PORT_STATE_DISCOVERY_BEGIN,
	PMC_SHIM_PHYSICAL_PORT_STATE_DISCOVERY_COMPLETE,
	PMC_SHIM_PHYSICAL_PORT_STATE_RESET_BEGIN,
	PMC_SHIM_PHYSICAL_PORT_STATE_RESET_COMPLETE,
	PMC_SHIM_PHYSICAL_PORT_STATE_LAST
}pmc_shim_physical_port_state_t;

typedef struct fbe_pmc_shim_port_info_s{	
	fbe_atomic_t		    port_update_generation_code;	
	pmc_shim_physical_port_state_t   port_state;
	fbe_u32_t				port_number;     /* Port number assigned by the miniport.*/
    fbe_u32_t				nr_phys;         /* Number of phys that make up this port.*/
    fbe_u32_t				phy_map;         /* Map of the phys that ake up this port.*/
    fbe_u32_t				nr_phys_enabled; /* Number of phys that are currently enabled.*/
    fbe_u32_t				nr_phys_up;     /* Number of phys that are currently up.*/
}fbe_pmc_shim_port_info_t;

typedef enum fbe_pmc_shim_device_login_reason_e{
    PMC_SHIM_DEVICE_LOGIN_REASON_NORMAL,
    PMC_SHIM_DEVICE_LOGIN_REASON_UNSUPPORTED_EXPANDER,
    PMC_SHIM_DEVICE_LOGIN_REASON_UNSUPPORTED_TOPOLOGY,
    PMC_SHIM_DEVICE_LOGIN_REASON_EXPANDER_MIXED_COMPLIANCE,
    PMC_SHIM_DEVICE_LOGIN_REASON_TOO_MANY_END_DEVICES,
    PMC_SHIM_DEVICE_LOGIN_REASON_INVALID,
    PMC_SHIM_DEVICE_LOGIN_REASON_LAST
}fbe_pmc_shim_device_login_reason_t;

typedef struct fbe_pmc_shim_device_table_entry_s{
	fbe_atomic_t						 current_gen_number;
	fbe_bool_t							 log_out_received;	
    fbe_pmc_shim_discovery_device_type_t device_type;    
	fbe_miniport_device_id_t             device_id;
    fbe_sas_address_t                    device_address;
    fbe_pmc_shim_device_locator_t        device_locator;
    fbe_miniport_device_id_t             parent_device_id;     
    fbe_pmc_shim_device_login_reason_t   device_login_reason;
}fbe_pmc_shim_device_table_entry_t;

typedef struct fbe_pmc_shim_device_table_s{	
	fbe_u32_t							 device_table_size;
	fbe_pmc_shim_device_table_entry_t	 device_entry[1]; /* Variable length array.*/
}fbe_pmc_shim_device_table_t;

typedef struct fbe_pmc_shim_callback_info_s {
	fbe_pmc_shim_callback_type_t callback_type;
	union {
		fbe_pmc_shim_callback_discovery_t	discovery;
		fbe_pmc_shim_callback_login_t		login;
        fbe_pmc_shim_callback_login_t		logout;
        fbe_pmc_shim_callback_port_up_t     port_up;
        fbe_pmc_shim_callback_port_up_t     port_down;
        fbe_pmc_shim_callback_port_up_t     link_up;
        fbe_pmc_shim_callback_port_up_t     link_down;
		fbe_pmc_shim_callback_driver_reset_t driver_reset;
		fbe_pmc_shim_port_info_t		   *port_info;		
		fbe_pmc_shim_device_table_t   	   *topology_device_information_table;
	}info;
}fbe_pmc_shim_callback_info_t;


typedef void * fbe_pmc_shim_callback_context_t;
typedef fbe_status_t (* fbe_pmc_shim_callback_function_t)(fbe_pmc_shim_callback_info_t * callback_info, fbe_pmc_shim_callback_context_t context);

fbe_status_t fbe_pmc_shim_init(void);
fbe_status_t fbe_pmc_shim_destroy(void);

fbe_status_t 
fbe_pmc_shim_port_init(fbe_u32_t port_number, fbe_u32_t io_port_number,fbe_u32_t * port_handle);

fbe_status_t fbe_pmc_shim_port_destroy(fbe_u32_t port_number);

fbe_status_t fbe_pmc_shim_port_register_payload_completion(	fbe_u32_t port_number,
														fbe_payload_ex_completion_function_t completion_function,
														fbe_payload_ex_completion_context_t  completion_context);

fbe_status_t fbe_pmc_shim_port_unregister_payload_completion(	fbe_u32_t port_number);

fbe_status_t fbe_pmc_shim_port_register_callback(	fbe_u32_t port_handle,												    
													fbe_pmc_shim_callback_function_t callback_function,
													fbe_pmc_shim_callback_context_t callback_context);

fbe_status_t fbe_pmc_shim_port_unregister_callback(	fbe_u32_t port_handle);

fbe_status_t fbe_pmc_shim_enumerate_backend_io_modules(fbe_pmc_shim_enumerate_io_modules_t * pmc_shim_enumerate_io_modules);

fbe_status_t fbe_pmc_shim_enumerate_backend_ports(fbe_pmc_shim_enumerate_backend_ports_t * pmc_shim_enumerate_backend_ports);

fbe_status_t fbe_pmc_shim_send_payload(fbe_u32_t port_number, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);

fbe_status_t fbe_pmc_shim_send_fis(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);

fbe_status_t fbe_pmc_shim_reset_device(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id);
fbe_status_t fbe_pmc_shim_reset_expander_phy(fbe_u32_t port_handle, fbe_cpd_device_id_t smp_port_device_id,fbe_u8_t phy_id);
fbe_status_t fbe_pmc_shim_change_speed(fbe_u32_t port_handle, fbe_port_speed_t speed);
fbe_status_t fbe_pmc_shim_get_port_info(fbe_u32_t port_handle, fbe_port_info_t * port_info);
fbe_status_t fbe_pmc_shim_get_port_name(fbe_u32_t port_handle, fbe_port_name_t * port_name);

fbe_status_t fbe_pmc_shim_port_get_device_table_max_index(fbe_u32_t port_handle,fbe_u32_t *device_table_max_index);
fbe_status_t fbe_pmc_shim_port_get_device_table_ptr(fbe_u32_t port_handle,fbe_pmc_shim_device_table_t **device_table_ptr);

struct fbe_terminator_miniport_interface_port_shim_sim_pointers_s;
fbe_status_t fbe_pmc_shim_sim_set_terminator_miniport_pointers(struct fbe_terminator_miniport_interface_port_shim_sim_pointers_s * miniport_pointers);
#endif /*FBE_PMC_SHIM_H */
