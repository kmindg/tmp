#ifndef FBE_CPD_SHIM_H
#define FBE_CPD_SHIM_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_atomic.h"
#include "fbe_fibre.h"

#include "fbe/fbe_payload_ex.h"
#include "fbe/fbe_port.h"
#include "fbe_sas.h"

#ifdef C4_INTEGRATED
#define FBE_CPD_SHIM_MAX_PORTS 64
#else
#define FBE_CPD_SHIM_MAX_PORTS 64
#endif /* C4_INTEGRATED - C4ARCH - sizing */
#define FBE_CPD_SHIM_INVALID_PORT_HANDLE FBE_CPD_SHIM_MAX_PORTS
#define FBE_CPD_SHIM_MAX_EXPANDERS 10

/* From cpd_interface.h */
/* This macro returns the device index portion of the miniport_login_context.
 * The index is a unique, zero-based, consecutive numbering of devices.
 */
#define FBE_CPD_GET_INDEX_FROM_CONTEXT(m_mlc)                           \
            ((fbe_u16_t)((UINT_PTR)(m_mlc) & 0xFFFF))

typedef struct fbe_cpd_shim_hardware_info_s{
    fbe_u32_t   vendor;
    fbe_u32_t   device;
    fbe_u32_t   bus;
    fbe_u32_t   slot;
    fbe_u32_t   function;
    fbe_u32_t   hw_major_rev;
    fbe_u32_t   hw_minor_rev;
    fbe_u32_t   firmware_rev_1;
    fbe_u32_t   firmware_rev_2;
    fbe_u32_t   firmware_rev_3;
    fbe_u32_t   firmware_rev_4;
} fbe_cpd_shim_hardware_info_t;

typedef enum fbe_cpd_shim_port_role_e{
    FBE_CPD_SHIM_PORT_ROLE_INVALID,
    FBE_CPD_SHIM_PORT_ROLE_FE,    /* Front End Port */
    FBE_CPD_SHIM_PORT_ROLE_BE,    /* Back End Port */
    FBE_CPD_SHIM_PORT_ROLE_UNC,   /* Uncommitted Port */
    FBE_CPD_SHIM_PORT_ROLE_BOOT,  /* BE Boot Port */
    FBE_CPD_SHIM_PORT_ROLE_SPECIAL, /* Special Port (MV Port) */
    FBE_CPD_SHIM_PORT_ROLE_MAX
} fbe_cpd_shim_port_role_t;

typedef enum fbe_cpd_shim_connect_class_e {
	FBE_CPD_SHIM_CONNECT_CLASS_INVALID,

	FBE_CPD_SHIM_CONNECT_CLASS_SAS,
	FBE_CPD_SHIM_CONNECT_CLASS_FC,
    FBE_CPD_SHIM_CONNECT_CLASS_ISCSI,
    FBE_CPD_SHIM_CONNECT_CLASS_FCOE,

	FBE_CPD_SHIM_IO_CONNECT_CLASS_LAST
}fbe_cpd_shim_connect_class_t;

typedef struct fbe_cpd_shim_port_configuration_s{
    fbe_cpd_shim_connect_class_t connect_class;
    fbe_cpd_shim_port_role_t     port_role;
    fbe_u32_t                    flare_bus_num;
    fbe_bool_t                   multi_core_affinity_enabled;
    fbe_u64_t                    core_affinity_proc_mask;
}fbe_cpd_shim_port_configuration_t;

typedef enum fbe_cpd_io_module_type_e {
	FBE_CPD_SHIM_IO_MODULE_TYPE_INVALID,

	FBE_CPD_SHIM_IO_MODULE_TYPE_SAS,
	FBE_CPD_SHIM_IO_MODULE_TYPE_FC,
    FBE_CPD_SHIM_IO_MODULE_TYPE_ISCSI,
    FBE_CPD_SHIM_IO_MODULE_TYPE_PROTOCOL_AGNOSTIC,
    FBE_CPD_SHIM_IO_MODULE_TYPE_UNKNOWN,

	FBE_CPD_SHIM_IO_MODULE_TYPE_LAST
}fbe_cpd_io_module_type_t;

typedef struct fbe_cpd_shim_backend_port_info_s {
    fbe_port_type_t	         port_type;/* TODO: Delete*/
    fbe_u32_t                port_number;
    fbe_u32_t                portal_number;
    fbe_u32_t                assigned_bus_number;
    fbe_cpd_shim_connect_class_t connect_class;
    fbe_cpd_shim_port_role_t     port_role;
    fbe_cpd_shim_hardware_info_t hdw_info;
}fbe_cpd_shim_backend_port_info_t;

typedef struct fbe_cpd_shim_enumerate_backend_ports_s {
	fbe_bool_t							rescan_required;
    fbe_u32_t                           number_of_io_ports;
	fbe_u32_t                           total_discovered_io_ports;
    fbe_cpd_shim_backend_port_info_t    io_port_array[FBE_CPD_SHIM_MAX_PORTS];
}fbe_cpd_shim_enumerate_backend_ports_t;

typedef struct fbe_cpd_shim_expander_info_s {
    union {
        struct {
            fbe_sas_address_t sas_address;
            fbe_ses_address_t ses_address;
        } sas;
    } u;
    fbe_u8_t bus;
    fbe_u8_t target_id;
    fbe_u8_t enclosure_num;
    fbe_u8_t cabling_position;
    fbe_bool_t new_entry;
}fbe_cpd_shim_expander_info_t;

typedef struct fbe_cpd_shim_expander_list_s {
    fbe_u8_t num_expanders;
    fbe_cpd_shim_expander_info_t expander_info[FBE_CPD_SHIM_MAX_EXPANDERS];
}fbe_cpd_shim_expander_list_t;

#define FBE_CPD_SHIM_MAX_IO_MODULES     32

typedef struct fbe_cpd_io_module_info_s {
    fbe_cpd_io_module_type_t io_module_type;    
	fbe_bool_t               inserted;
	fbe_bool_t               power_good;
	fbe_u32_t	             port_count;	
}fbe_cpd_io_module_info_t;

typedef struct fbe_cpd_shim_enumerate_io_modules_s {
	fbe_bool_t						rescan_required;
	fbe_u32_t                       number_of_io_modules; /* Number of SAS IO SLICs. (includes mezzanine controllers)*/
    fbe_u32_t                       total_enumerated_io_ports; /* Number of SAS IO portals (each SLIC could have multiple portals)*/
	fbe_cpd_io_module_info_t        io_module_array[FBE_CPD_SHIM_MAX_IO_MODULES];
}fbe_cpd_shim_enumerate_io_modules_t;

typedef struct fbe_cpd_shim_port_name_info_s{
    fbe_sas_address_t sas_address;
}fbe_cpd_shim_port_name_info_t;

typedef enum fbe_cpd_shim_callback_type_e {
    FBE_CPD_SHIM_CALLBACK_TYPE_INVALID,
    FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN,
    FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN_FAILED,
    FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT,
    FBE_CPD_SHIM_CALLBACK_TYPE_DEVICE_FAILED,

    FBE_CPD_SHIM_CALLBACK_TYPE_DISCOVERY,
    FBE_CPD_SHIM_CALLBACK_TYPE_LINK_UP,
    FBE_CPD_SHIM_CALLBACK_TYPE_LINK_DOWN,
    FBE_CPD_SHIM_CALLBACK_TYPE_LANE_STATUS,

    FBE_CPD_SHIM_CALLBACK_TYPE_DRIVER_RESET,

    FBE_CPD_SHIM_CALLBACK_TYPE_CTRL_RESET,

    /* Port notifications are obsolete and will be removed.*/
    FBE_CPD_SHIM_CALLBACK_TYPE_PORT_UP,
    FBE_CPD_SHIM_CALLBACK_TYPE_PORT_DOWN,
    FBE_CPD_SHIM_CALLBACK_TYPE_DEVICE_TABLE_UPDATE,
    FBE_CPD_SHIM_CALLBACK_TYPE_PORT_STATUS_UPDATE,

    FBE_CPD_SHIM_CALLBACK_TYPE_SFP,

    FBE_CPD_SHIM_CALLBACK_TYPE_ENCRYPTION,
    FBE_CPD_SHIM_CALLBACK_TYPE_LAST
}fbe_cpd_shim_callback_type_t;


#define FBE_MAX_EXPANDERS 10

typedef enum fbe_cpd_shim_discovery_device_type_e{
    FBE_CPD_SHIM_DEVICE_TYPE_INVALID,
    FBE_CPD_SHIM_DEVICE_TYPE_SSP,
    FBE_CPD_SHIM_DEVICE_TYPE_STP,
    FBE_CPD_SHIM_DEVICE_TYPE_ENCLOSURE,
    FBE_CPD_SHIM_DEVICE_TYPE_VIRTUAL,    
    FBE_CPD_SHIM_DEVICE_TYPE_LAST
}fbe_cpd_shim_discovery_device_type_t;

typedef struct fbe_cpd_shim_device_locator_s{
    fbe_u8_t            enclosure_chain_depth;
    fbe_u8_t            enclosure_chain_width;
    fbe_u8_t            phy_number;
    fbe_u8_t            padding;
}fbe_cpd_shim_device_locator_t;

typedef struct fbe_cpd_shim_callback_login_s {
    fbe_cpd_shim_discovery_device_type_t device_type;    
    fbe_miniport_device_id_t             device_id;
    fbe_sas_address_t                    device_address;
    fbe_cpd_shim_device_locator_t        device_locator;
    fbe_miniport_device_id_t             parent_device_id; 
    fbe_sas_address_t                    parent_device_address;
}fbe_cpd_shim_callback_login_t;

typedef enum fbe_cpd_shim_discovery_event_type_e {
    FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_INVALID,

    FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_START,
    FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_COMPLETE,

    FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_LAST
}fbe_cpd_shim_discovery_event_type_t;

typedef struct fbe_cpd_shim_callback_discovery_s {
    fbe_cpd_shim_discovery_event_type_t discovery_event_type;
}fbe_cpd_shim_callback_discovery_t;

typedef enum fbe_cpd_shim_driver_reset_event_type_e {
    FBE_CPD_SHIM_DRIVER_RESET_EVENT_TYPE_INVALID,

    FBE_CPD_SHIM_DRIVER_RESET_EVENT_TYPE_BEGIN,
    FBE_CPD_SHIM_DRIVER_RESET_EVENT_TYPE_COMPLETED,

    FBE_CPD_SHIM_CTRL_RESET_EVENT_TYPE_BEGIN,
    FBE_CPD_SHIM_CTRL_RESET_EVENT_TYPE_COMPLETED,
    FBE_CPD_SHIM_DRIVER_RESET_EVENT_TYPE_LAST
}fbe_cpd_shim_driver_reset_event_type_t;

typedef struct fbe_cpd_shim_callback_driver_reset_s {
    fbe_cpd_shim_driver_reset_event_type_t driver_reset_event_type;
    fbe_u32_t                               additional_information;/* For future use.*/
}fbe_cpd_shim_callback_driver_reset_t;


typedef enum cpd_shim_port_link_state_e {
    CPD_SHIM_PORT_LINK_STATE_INVALID,
    CPD_SHIM_PORT_LINK_STATE_UP,
    CPD_SHIM_PORT_LINK_STATE_DOWN,
    CPD_SHIM_PORT_LINK_STATE_DEGRADED,
    CPD_SHIM_PORT_LINK_STATE_NOT_INITIALIZED,
    CPD_SHIM_PORT_LINK_STATE_MAX
}cpd_shim_port_link_state_t;

typedef struct fbe_cpd_shim_port_lane_info_s {
    fbe_u32_t                   portal_number;     /* Port number assigned by the miniport.*/
    fbe_u32_t                   link_speed;
    cpd_shim_port_link_state_t  link_state;

    fbe_u32_t   nr_phys;         /* Number of phys that make up this port.*/
    fbe_u32_t   phy_map;         /* Map of the phys that ake up this port.*/
    fbe_u32_t   nr_phys_enabled; /* Number of phys that are currently enabled.*/
    fbe_u32_t   nr_phys_up;     /* Number of phys that are currently up.*/
}fbe_cpd_shim_port_lane_info_t;


typedef enum cpd_shim_physical_port_state_e {
    CPD_SHIM_PHYSICAL_PORT_STATE_INVALID,
    CPD_SHIM_PHYSICAL_PORT_STATE_LINK_DOWN,
    CPD_SHIM_PHYSICAL_PORT_STATE_LINK_UP,
    CPD_SHIM_PHYSICAL_PORT_STATE_LINK_DEGRADED,
    CPD_SHIM_PHYSICAL_PORT_STATE_DISCOVERY_BEGIN,
    CPD_SHIM_PHYSICAL_PORT_STATE_DISCOVERY_COMPLETE,
    CPD_SHIM_PHYSICAL_PORT_STATE_RESET_BEGIN,
    CPD_SHIM_PHYSICAL_PORT_STATE_RESET_COMPLETE,
    CPD_SHIM_PHYSICAL_PORT_STATE_CTRL_RESET_START,
    CPD_SHIM_PHYSICAL_PORT_STATE_CTRL_RESET_COMPLETE,
    CPD_SHIM_PHYSICAL_PORT_STATE_LAST
}cpd_shim_physical_port_state_t;

typedef enum fbe_cpd_shim_device_login_reason_e{
    CPD_SHIM_DEVICE_LOGIN_REASON_NORMAL,
    CPD_SHIM_DEVICE_LOGIN_REASON_UNSUPPORTED_EXPANDER,
    CPD_SHIM_DEVICE_LOGIN_REASON_UNSUPPORTED_TOPOLOGY,
    CPD_SHIM_DEVICE_LOGIN_REASON_EXPANDER_MIXED_COMPLIANCE,
    CPD_SHIM_DEVICE_LOGIN_REASON_TOO_MANY_END_DEVICES,
    CPD_SHIM_DEVICE_LOGIN_REASON_INVALID,
    CPD_SHIM_DEVICE_LOGIN_REASON_LAST
}fbe_cpd_shim_device_login_reason_t;

typedef struct fbe_cpd_shim_port_info_s{
    fbe_atomic_t             port_update_generation_code;
    cpd_shim_physical_port_state_t port_state;
	fbe_cpd_shim_port_lane_info_t  port_lane_info;
}fbe_cpd_shim_port_info_t;

/* TODO: Additional protocol specific capabilities are bubbled up by
         miniport driver. Add additional protocol specific structires and
         functions to retrieve this as required by clients/consumers.*/

typedef struct fbe_cpd_shim_port_capabilities_s{
    fbe_u32_t                maximum_transfer_length;
    fbe_u32_t                maximum_sg_entries;
    fbe_u32_t                sg_length;
    fbe_u32_t                maximum_portals;
    fbe_u32_t                portal_number;
    fbe_u32_t                link_speed;
}fbe_cpd_shim_port_capabilities_t;

typedef struct fbe_cpd_shim_device_table_entry_s{
    fbe_atomic_t                            current_gen_number;
    fbe_bool_t                           log_out_received;
    fbe_cpd_shim_discovery_device_type_t device_type;    
    fbe_miniport_device_id_t             device_id;
    fbe_sas_address_t                    device_address;
    fbe_cpd_shim_device_locator_t        device_locator;
    fbe_miniport_device_id_t             parent_device_id; 
    fbe_cpd_shim_device_login_reason_t   device_login_reason;
}fbe_cpd_shim_device_table_entry_t;

typedef struct fbe_cpd_shim_device_table_s{
    fbe_u32_t                             device_table_size;
    fbe_cpd_shim_device_table_entry_t     device_entry[1]; /* Variable length array.*/
}fbe_cpd_shim_device_table_t;

typedef enum fbe_cpd_shim_media_interface_information_type_e{
    FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_INVALID,
    FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_ALL,
    FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_HIGHEST,
    FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_LAST,
    FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_CACHED,
    FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_MAX
}fbe_cpd_shim_media_interface_information_type_t;

typedef enum fbe_cpd_shim_sfp_condition_type_e{
    FBE_CPD_SHIM_SFP_CONDITION_INVALID,
    FBE_CPD_SHIM_SFP_CONDITION_GOOD,
    FBE_CPD_SHIM_SFP_CONDITION_INSERTED,
    FBE_CPD_SHIM_SFP_CONDITION_REMOVED,
    FBE_CPD_SHIM_SFP_CONDITION_FAULT,
    FBE_CPD_SHIM_SFP_CONDITION_WARNING,
    FBE_CPD_SHIM_SFP_CONDITION_INFO,
    FBE_CPD_SHIM_SFP_CONDITION_MAX,
}fbe_cpd_shim_sfp_condition_type_t;

typedef enum fbe_cpd_shim_sfp_subcondition_type_e{
    FBE_CPD_SHIM_SFP_NONE                   = 0x00000000,

    /*
     * INFO
     */
    FBE_CPD_SHIM_SFP_INFO_SPD_LEN_AVAIL,
    FBE_CPD_SHIM_SFP_INFO_RECHECK_MII,
    FBE_CPD_SHIM_SFP_INFO_BAD_EMC_CHKSUM,

    /*
     * FBE_CPD_SHIM_SFP HW Faults
     */
    FBE_CPD_SHIM_SFP_BAD_CHKSUM,
    FBE_CPD_SHIM_SFP_BAD_I2C,
    FBE_CPD_SHIM_SFP_DEV_ERR, //relates to STATUS_SMB_DEV_ERR

    /*
     * FBE_CPD_SHIM_SFP_CONDITION_BAD_DIAGS Warnings
     */
    FBE_CPD_SHIM_SFP_DIAG_TXFAULT           = 0x00001000,
    FBE_CPD_SHIM_SFP_DIAG_TEMP_HI_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_TEMP_LO_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_VCC_HI_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_VCC_LO_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_HI_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_LO_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_TX_POWER_HI_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_TX_POWER_LO_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_RX_POWER_HI_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_RX_POWER_LO_ALARM,
    FBE_CPD_SHIM_SFP_DIAG_TEMP_HI_WARNING,
    FBE_CPD_SHIM_SFP_DIAG_TEMP_LO_WARNING,
    FBE_CPD_SHIM_SFP_DIAG_VCC_HI_WARNING,
    FBE_CPD_SHIM_SFP_DIAG_VCC_LO_WARNING,
    FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_HI_WARNING,
    FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_LO_WARNING,
    FBE_CPD_SHIM_SFP_DIAG_TX_POWER_HI_WARNING,
    FBE_CPD_SHIM_SFP_DIAG_TX_POWER_LO_WARNING,
    FBE_CPD_SHIM_SFP_DIAG_RX_POWER_HI_WARNING,
    FBE_CPD_SHIM_SFP_DIAG_RX_POWER_LO_WARNING,

    /*
     * FBE_CPD_SHIM_SFP_CONDITION_UNQUALIFIED Faults
     */
    FBE_CPD_SHIM_SFP_UNQUAL_OPT_NOT_4G     = 0x00002000,
    FBE_CPD_SHIM_SFP_UNQUAL_COP_AUTO,
    FBE_CPD_SHIM_SFP_UNQUAL_COP_SFP_SPEED,
    FBE_CPD_SHIM_SFP_UNQUAL_SPEED_EXCEED_SFP,
    FBE_CPD_SHIM_SFP_UNQUAL_PART,
    FBE_CPD_SHIM_SFP_UNKNOWN_TYPE,
    FBE_CPD_SHIM_SFP_SPEED_MISMATCH,
    FBE_CPD_SHIM_SFP_EDC_MODE_MISMATCH,
    FBE_CPD_SHIM_SFP_SAS_SPECL_ERROR
}fbe_cpd_shim_sfp_subcondition_type_t;

typedef enum fbe_cpd_shim_sfp_media_type_e{
    FBE_CPD_SHIM_SFP_MEDIA_TYPE_INVALID,
    FBE_CPD_SHIM_SFP_MEDIA_TYPE_COPPER,
    FBE_CPD_SHIM_SFP_MEDIA_TYPE_OPTICAL,
    FBE_CPD_SHIM_SFP_MEDIA_TYPE_NAS_COPPER,
    FBE_CPD_SHIM_SFP_MEDIA_TYPE_UNKNOWN,
    FBE_CPD_SHIM_SFP_MEDIA_TYPE_MINI_SAS_HD
}fbe_cpd_shim_sfp_media_type_t;

#define FBE_CPD_SHIM_SFP_VENDOR_DATA_LENGTH     32
#define FBE_CPD_SHIM_SFP_EMC_DATA_LENGTH        32

typedef struct fbe_cpd_shim_sfp_media_interface_info_s{
    fbe_cpd_shim_sfp_condition_type_t condition_type;
    fbe_cpd_shim_sfp_subcondition_type_t condition_additional_info;
    fbe_u32_t                speeds;
    fbe_cpd_shim_sfp_media_type_t     media_type;
    fbe_u32_t                cable_length;
    fbe_u32_t                hw_type;

    fbe_u8_t                 emc_part_number[FBE_PORT_SFP_EMC_DATA_LENGTH];
    fbe_u8_t                 emc_part_revision[FBE_PORT_SFP_EMC_DATA_LENGTH];
    fbe_u8_t                 emc_serial_number[FBE_PORT_SFP_EMC_DATA_LENGTH];

    fbe_u8_t                 vendor_part_number[FBE_PORT_SFP_VENDOR_DATA_LENGTH];
    fbe_u8_t                 vendor_part_revision[FBE_PORT_SFP_VENDOR_DATA_LENGTH];
    fbe_u8_t                 vendor_serial_number[FBE_PORT_SFP_VENDOR_DATA_LENGTH];

}fbe_cpd_shim_sfp_media_interface_info_t;

typedef struct fbe_cpd_shim_encryption_context_s{
    void *context;
    fbe_status_t status;
}fbe_cpd_shim_encryption_context_t;

typedef struct fbe_cpd_shim_callback_info_s {
    fbe_cpd_shim_callback_type_t callback_type;
    union {
        fbe_cpd_shim_callback_discovery_t     discovery;
        fbe_cpd_shim_callback_login_t         login;
        fbe_cpd_shim_callback_login_t         logout;
        fbe_cpd_shim_port_lane_info_t         port_lane_info;
        fbe_cpd_shim_callback_driver_reset_t  driver_reset;
        fbe_cpd_shim_sfp_media_interface_info_t sfp_info;
        fbe_cpd_shim_port_info_t             *port_info;
        fbe_cpd_shim_device_table_t          *topology_device_information_table;
        fbe_cpd_shim_encryption_context_t  encrypt_context;
    }info;
}fbe_cpd_shim_callback_info_t;



#define FBE_CPD_SHIM_REGISTER_CALLBACKS_INITIATOR                0x000000001
#define FBE_CPD_SHIM_REGISTER_CALLBACKS_TARGET                   0x000000002
#define FBE_CPD_SHIM_REGISTER_CALLBACKS_NOTIFY_EXISTING_LOGINS   0x000000004
#define FBE_CPD_SHIM_REGISTER_CALLBACKS_SFP_EVENTS               0x000000008
#define FBE_CPD_SHIM_REGISTER_CALLBACKS_DO_NOT_INIT              0x000000010
#define FBE_CPD_SHIM_REGISTER_CALLBACKS_ENCRYPTION               0x000000020
#define FBE_CPD_SHIM_REGISTER_FILTER_INTERCEPT                   0x100000000 /* PPFD related*/



typedef void * fbe_cpd_shim_callback_context_t;
typedef fbe_status_t (* fbe_cpd_shim_callback_function_t)(fbe_cpd_shim_callback_info_t * callback_info,
                                                          fbe_cpd_shim_callback_context_t context);

fbe_status_t fbe_cpd_shim_init(void);
fbe_status_t fbe_cpd_shim_destroy(void);

fbe_status_t 
fbe_cpd_shim_port_init(fbe_u32_t port_number, fbe_u32_t io_port_number,fbe_u32_t * port_handle);

fbe_status_t fbe_cpd_shim_port_destroy(fbe_u32_t port_number);

fbe_status_t fbe_cpd_shim_port_register_payload_completion(fbe_u32_t port_number,
                                                           fbe_payload_ex_completion_function_t completion_function,
                                                           fbe_payload_ex_completion_context_t  completion_context);

fbe_status_t fbe_cpd_shim_port_unregister_payload_completion(    fbe_u32_t port_number);

fbe_status_t fbe_cpd_shim_port_register_callback(fbe_u32_t port_handle,
                                                 fbe_u32_t registration_flags,
                                                 fbe_cpd_shim_callback_function_t callback_function,
                                                 fbe_cpd_shim_callback_context_t callback_context);

fbe_status_t fbe_cpd_shim_port_unregister_callback(fbe_u32_t port_handle);

fbe_status_t fbe_cpd_shim_enumerate_backend_io_modules(fbe_cpd_shim_enumerate_io_modules_t * cpd_shim_enumerate_io_modules);
fbe_status_t fbe_cpd_shim_enumerate_backend_ports(fbe_cpd_shim_enumerate_backend_ports_t * cpd_shim_enumerate_backend_ports);

fbe_status_t fbe_cpd_shim_send_payload(fbe_u32_t port_number, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);

fbe_status_t fbe_cpd_shim_send_fis(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);

fbe_status_t fbe_cpd_shim_reset_expander_phy(fbe_u32_t port_handle, fbe_cpd_device_id_t smp_port_device_id,fbe_u8_t phy_id);
fbe_status_t fbe_cpd_shim_reset_device(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id);
fbe_status_t fbe_cpd_shim_change_speed(fbe_u32_t port_handle, fbe_port_speed_t speed);
fbe_status_t fbe_cpd_shim_get_port_info(fbe_u32_t port_handle, fbe_port_info_t * port_info);
fbe_status_t fbe_cpd_shim_get_port_name(fbe_u32_t port_handle, fbe_port_name_t * port_name);
fbe_status_t fbe_cpd_shim_get_port_config_info(fbe_u32_t port_handle, fbe_cpd_shim_port_configuration_t *fbe_config_info);
fbe_status_t fbe_cpd_shim_get_port_capabilities(fbe_u32_t port_handle, fbe_cpd_shim_port_capabilities_t *port_capabilities);

fbe_status_t fbe_cpd_shim_port_get_device_table_max_index(fbe_u32_t port_handle,fbe_u32_t *device_table_max_index);
fbe_status_t fbe_cpd_shim_port_get_device_table_ptr(fbe_u32_t port_handle,fbe_cpd_shim_device_table_t **device_table_ptr);
fbe_status_t fbe_cpd_shim_api_initilize_config1_data(void);
fbe_status_t fbe_cpd_shim_sim_start_internal_test(fbe_u32_t port_number);

fbe_status_t fbe_cpd_shim_enumerate_backend_ports(fbe_cpd_shim_enumerate_backend_ports_t * backend_ports);
fbe_status_t fbe_cpd_shim_get_expander_list(fbe_u32_t portNumber, fbe_cpd_shim_expander_list_t * expander_list);

struct fbe_terminator_miniport_interface_port_shim_sim_pointers_s;
fbe_status_t fbe_cpd_shim_sim_set_terminator_miniport_pointers(struct fbe_terminator_miniport_interface_port_shim_sim_pointers_s * miniport_pointers);
fbe_status_t fbe_cpd_shim_get_media_inteface_info(fbe_u32_t  port_handle, fbe_cpd_shim_media_interface_information_type_t mii_type,
                                 fbe_cpd_shim_sfp_media_interface_info_t *media_interface_info);
fbe_status_t fbe_cpd_shim_get_hardware_info(fbe_u32_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info);
fbe_status_t fbe_cpd_shim_register_data_encryption_keys(fbe_u32_t port_handle, 
                                                        fbe_base_port_mgmt_register_keys_t * port_register_keys_p);
fbe_status_t fbe_cpd_shim_unregister_data_encryption_keys(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info);
fbe_status_t fbe_cpd_shim_register_kek(fbe_u32_t port_handle, 
                                       fbe_base_port_mgmt_register_keys_t * port_register_keys_p,
                                       void *port_kek_context);
fbe_status_t fbe_cpd_shim_rebase_all_keys(fbe_u32_t port_handle, 
                                          fbe_key_handle_t key_handle,
                                          void *port_kek_context);
fbe_status_t fbe_cpd_shim_set_encryption_mode(fbe_u32_t port_handle, fbe_port_encryption_mode_t mode);
fbe_status_t fbe_cpd_shim_unregister_kek(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info);
fbe_status_t fbe_cpd_shim_register_kek_kek(fbe_u32_t port_handle, 
                                           fbe_base_port_mgmt_register_keys_t * port_register_keys_p,
                                           void *port_kek_kek_context);
fbe_status_t fbe_cpd_shim_unregister_kek_kek(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t * unregister_info);
fbe_status_t fbe_cpd_shim_reestablish_key_handle(fbe_u32_t port_handle, fbe_base_port_mgmt_reestablish_key_handle_t * key_handle_info);
fbe_status_t fbe_cpd_shim_get_port_lane_info(fbe_u32_t  port_handle, fbe_cpd_shim_connect_class_t connect_class, 
                                             fbe_cpd_shim_port_lane_info_t *port_lane_info);

fbe_status_t fbe_cpd_shim_set_async_io(fbe_bool_t sync_io);
fbe_status_t fbe_cpd_shim_set_dmrb_zeroing(fbe_bool_t dmrb_zeroing);
fbe_status_t fbe_cpd_shim_get_async_io(fbe_bool_t * async_io);
fbe_status_t fbe_cpd_shim_get_port_name_info(fbe_u32_t port_handle, fbe_cpd_shim_port_name_info_t *port_name_info);

#endif /* FBE_CPD_SHIM_H */

