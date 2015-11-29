#ifndef FBE_PORT_H
#define FBE_PORT_H

#include "fbe/fbe_object.h"

#define FBE_PORT_NUMBER_INVALID FBE_INVALID_PORT_NUM
#define FBE_PORT_ENCL_SLOT_NA 0xFFFFFFFE
#define FBE_PORT_SERIAL_NUM_SIZE 16

#define FBE_PORT_MAX_KEYS_PER_REQUEST 2

typedef fbe_u64_t fbe_miniport_device_id_t;

typedef enum fbe_port_request_status_e { 
	FBE_PORT_REQUEST_STATUS_SUCCESS,				/* CPD_STATUS_GOOD					 */
	FBE_PORT_REQUEST_STATUS_INVALID_REQUEST,		/* CPD_STATUS_INVALID_REQUEST		 */
	FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES, /* CPD_STATUS_INSUFFICIENT_RESOURCES */
	FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN,	/* CPD_STATUS_DEVICE_NOT_LOGGED_IN   */
	FBE_PORT_REQUEST_STATUS_BUSY,					/* CPD_STATUS_DEVICE_BUSY			 */
	FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR,			/* CPD_STATUS_BAD_REPLY				 */
	FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT,			/* CPD_STATUS_TIMEOUT				 */
	FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT,		/* CPD_STATUS_DEVICE_NOT_RESPONDING  */
	FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR,		/* CPD_STATUS_LINK_FAIL				 */
	FBE_PORT_REQUEST_STATUS_DATA_OVERRUN,			/* CPD_STATUS_DATA_OVERRUN			 */
	FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN,			/* CPD_STATUS_DATA_UNDERRUN			 */
	FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE,	/* CPD_STATUS_ABORTED_BY_UPPER_LEVEL */
	FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE,		/* CPD_STATUS_ABORTED_BY_DEVICE		 */
	FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR,			/* CPD_STATUS_SATA_NCQ_ERROR         */
	FBE_PORT_REQUEST_STATUS_REJECTED_NCQ_MODE,		/* CPD_STATUS_SATA_REJECTED_NCQ_MODE */
	FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT,		/* CPD_STATUS_INCIDENTAL_ABORT       */
        FBE_PORT_REQUEST_STATUS_ENCRYPTION_NOT_ENABLED,         /* CPD_STATUS_ENCRYPTION_NOT_ENABLED */
        FBE_PORT_REQUEST_STATUS_ENCRYPTION_BAD_HANDLE,          /* CPD_STATUS_ENCRYPTION_KEY_NOT_FOUND */
        FBE_PORT_REQUEST_STATUS_ENCRYPTION_KEY_WRAP_ERROR,      /* CPD_STATUS_ENCRYPTION_KEY_ERROR */

	FBE_PORT_REQUEST_STATUS_ERROR /* This status should be removed ASAP */ 
}fbe_port_request_status_t;

typedef enum fbe_port_recovery_status_e{
    FBE_PORT_RECOVERY_STATUS_NO_RECOVERY_PERFORMED,
    FBE_PORT_RECOVERY_STATUS_SUCCESS,
    FBE_PORT_RECOVERY_STATUS_TIMED_OUT,
    FBE_PORT_RECOVERY_STATUS_MINIPORT_ERROR,
    FBE_PORT_RECOVERY_STATUS_COMPLETED_WITH_ERROR,
    FBE_PORT_RECOVERY_STATUS_ERROR,

}fbe_port_recovery_status_t;

/* TODO: Remove this and use connect type and hwd info for specialization.*/
typedef enum fbe_port_type_e {
	FBE_PORT_TYPE_INVALID,
	FBE_PORT_TYPE_FIBRE,
    FBE_PORT_TYPE_SAS_LSI,
	FBE_PORT_TYPE_SAS_PMC,
	FBE_PORT_TYPE_FC_PMC,
    FBE_PORT_TYPE_ISCSI,
    FBE_PORT_TYPE_FCOE,
	FBE_PORT_TYPE_LAST
}fbe_port_type_t;

typedef enum fbe_base_port_control_code_e {
	FBE_BASE_PORT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_PORT),

	FBE_BASE_PORT_CONTROL_CODE_GET_NUMBER_OF_ENCLOSURES,
	FBE_BASE_PORT_CONTROL_CODE_GET_PORT_NUMBER,
	FBE_BASE_PORT_CONTROL_CODE_GET_PORT_ROLE,
	FBE_BASE_PORT_CONTROL_CODE_LOGIN_DRIVE,
	FBE_BASE_PORT_CONTROL_CODE_LOGOUT_DRIVE,
	FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO,
	FBE_BASE_PORT_CONTROL_CODE_SET_LINK_SPEED,
        FBE_BASE_PORT_CONTROL_CODE_GET_HARDWARE_INFORMATION,
        FBE_BASE_PORT_CONTROL_CODE_GET_SFP_INFORMATION,
        FBE_BASE_PORT_CONTROL_CODE_GET_PORT_LINK_INFO,
        FBE_BASE_PORT_CONTROL_CODE_SET_DEBUG_CONTROL,
        FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEYS,
        FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEYS,
        FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEK,
        FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEK,
        FBE_BASE_PORT_CONTROL_CODE_GET_KEK_HANDLE,
        FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEK_KEK,
        FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEK_KEK,
        FBE_BASE_PORT_CONTROL_CODE_REESTABLISH_KEY_HANDLE,
        FBE_BASE_PORT_CONTROL_CODE_SET_ENCRYPTION_MODE,
        FBE_BASE_PORT_CONTROL_CODE_DBEUG_REGISTER_DEK,
	FBE_BASE_PORT_CONTROL_CODE_LAST
}fbe_base_port_control_code_t;

typedef fbe_u32_t fbe_number_of_enclosures_t;

typedef enum fbe_port_encryption_mode_e{
    FBE_PORT_ENCRYPTION_MODE_INVALID,
    FBE_PORT_ENCRYPTION_MODE_DISABLED,
    FBE_PORT_ENCRYPTION_MODE_PLAINTEXT_KEYS,
    FBE_PORT_ENCRYPTION_MODE_WRAPPED_DEKS,
    FBE_PORT_ENCRYPTION_MODE_WRAPPED_KEKS_DEKS,
    FBE_PORT_ENCRYPTION_MODE_NOT_SUPPORTED,
    FBE_PORT_ENCRYPTION_MODE_UNCOMMITTED,
    FBE_PORT_ENCRYPTION_MODE_LAST,
}fbe_port_encryption_mode_t;

typedef enum fbe_port_speed_e {
	FBE_PORT_SPEED_INVALID,

	FBE_PORT_SPEED_FIBRE_FIRST,
	FBE_PORT_SPEED_FIBRE_1_GBPS,
	FBE_PORT_SPEED_FIBRE_2_GBPS,
	FBE_PORT_SPEED_FIBRE_4_GBPS,
	FBE_PORT_SPEED_FIBRE_LAST,

	FBE_PORT_SPEED_SAS_FIRST,
	FBE_PORT_SPEED_SAS_1_5_GBPS,
	FBE_PORT_SPEED_SAS_3_GBPS,
	FBE_PORT_SPEED_SAS_6_GBPS,
	FBE_PORT_SPEED_SAS_12_GBPS,
	FBE_PORT_SPEED_SAS_LAST,

	FBE_PORT_SPEED_LAST
}fbe_port_speed_t;

typedef enum fbe_port_role_e{
    FBE_PORT_ROLE_INVALID,
    FBE_PORT_ROLE_FE,    /* Front End Port */
    FBE_PORT_ROLE_BE,    /* Back End Port */
    FBE_PORT_ROLE_UNC,   /* Uncommitted Port */
    FBE_PORT_ROLE_BOOT,  /* BE Boot Port */
    FBE_PORT_ROLE_SPECIAL, /* Special Port (MV Port) */
    FBE_PORT_ROLE_MAX
} fbe_port_role_t;

typedef enum fbe_port_connect_class_e {
	FBE_PORT_CONNECT_CLASS_INVALID,

	FBE_PORT_CONNECT_CLASS_SAS,
	FBE_PORT_CONNECT_CLASS_FC,
    FBE_PORT_CONNECT_CLASS_ISCSI,
    FBE_PORT_CONNECT_CLASS_FCOE,

	FBE_PORT_CONNECT_CLASS_LAST
}fbe_port_connect_class_t;

/* FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO */
typedef enum fbe_port_link_state_e{
    FBE_PORT_LINK_STATE_DOWN,
    FBE_PORT_LINK_STATE_UP,
    FBE_PORT_LINK_STATE_NOT_INITIALIZED,
    FBE_PORT_LINK_STATE_INVALID,
    FBE_PORT_LINK_STATE_DEGRADED, /* Not used for now. */
    FBE_PORT_LINK_STATE_MAX
}fbe_port_link_state_t;

typedef struct fbe_port_sas_link_information_s {
    fbe_u32_t               portal_number;     /* Port number assigned by the miniport.*/    
    fbe_u32_t               nr_phys;         /* Number of phys that make up this port.*/
    fbe_u32_t               phy_map;         /* Map of the phys that ake up this port.*/
    fbe_u32_t               nr_phys_enabled; /* Number of phys that are currently enabled.*/
    fbe_u32_t               nr_phys_up;     /* Number of phys that are currently up.*/
}fbe_port_sas_link_information_t;

typedef struct fbe_port_fc_link_information_s {
    fbe_u32_t       unused;
}fbe_port_fc_link_information_t;

typedef struct fbe_port_iscsi_link_information_s {
    fbe_u32_t       unused;
}fbe_port_iscsi_link_information_t;


typedef struct fbe_port_link_information_s{
    fbe_u32_t                   portal_number;
    fbe_port_connect_class_t    port_connect_class;
    fbe_port_link_state_t       link_state;
    fbe_u32_t                   link_speed;
    union{
        fbe_port_sas_link_information_t     sas_port;
        fbe_port_iscsi_link_information_t   iscsi;
        fbe_port_fc_link_information_t      fc;
    }u;
}fbe_port_link_information_t;

typedef struct fbe_port_info_s {
    fbe_port_type_t type; /* Obsolete. Remove and use connect class. */
    fbe_port_role_t port_role;
	fbe_u32_t       io_port_number;
	fbe_u32_t       io_portal_number;
	fbe_u32_t       assigned_bus_number;
    fbe_u32_t       maximum_transfer_bytes;
    fbe_u32_t       maximum_sg_entries;
	fbe_port_speed_t link_speed; /* Obsolete. Remove and use generic SPEED_*. */
    fbe_port_link_information_t  port_link_info;	

    fbe_bool_t       multi_core_affinity_enabled; 
	fbe_u64_t        active_proc_mask;
        fbe_port_encryption_mode_t enc_mode;
} fbe_port_info_t;

/* FBE_BASE_PORT_CONTROL_CODE_GET_NUMBER_OF_ENCLOSURES */
typedef struct fbe_base_port_mgmt_get_number_of_enclosures_s{
	fbe_number_of_enclosures_t number_of_enclosures;
}fbe_base_port_mgmt_get_number_of_enclosures_t;

/* FBE_BASE_PORT_CONTROL_CODE_GET_PORT_NUMBER */
typedef struct fbe_base_port_mgmt_get_port_number_s{
	fbe_port_number_t port_number;
}fbe_base_port_mgmt_get_port_number_t;

/* FBE_BASE_PORT_CONTROL_CODE_GET_PORT_ROLE */
typedef struct fbe_base_port_mgmt_get_port_role_s{
	fbe_port_role_t port_role;
}fbe_base_port_mgmt_get_port_role_t;

/* FBE_BASE_PORT_CONTROL_CODE_LOGIN_DRIVE */
typedef struct fbe_base_port_mgmt_login_drive_s{
	fbe_address_t drive_address;
}fbe_base_port_mgmt_login_drive_t;

/* FBE_BASE_PORT_CONTROL_CODE_LOGOUT_DRIVE */
typedef struct fbe_base_port_mgmt_logout_drive_s{
	fbe_address_t drive_address;
}fbe_base_port_mgmt_logout_drive_t;

/* FBE_BASE_PORT_CONTROL_CODE_SET_LINK_SPEED */
typedef struct fbe_port_mgmt_set_link_speed_s {
	fbe_port_speed_t link_speed;
} fbe_port_mgmt_set_link_speed_t;

/* FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEYS */
typedef struct fbe_base_port_mgmt_register_keys_s{
    fbe_u32_t num_of_keys;
    fbe_u32_t key_size;

    /* Pointer to the key to be provided as input to the miniport*/
    fbe_key_handle_t *key_ptr; 

    /* Unwrap key handle used ONLY for Wrapped DEKs. This will
     * be filled by the port object before sent to miniport */
    fbe_key_handle_t kek_handle;

    /* Handle to the keys stored by the miniport. Need to 
     * provide this handle back to the miniport on a per 
     * IO basis if that IO needs to be encrypted 
     */
    fbe_key_handle_t mp_key_handle[FBE_PORT_MAX_KEYS_PER_REQUEST]; 
}fbe_base_port_mgmt_register_keys_t;

/* FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEY_ENCRYPTION_KEY */
typedef struct fbe_base_port_mgmt_register_kek_s{

    /* Pointer to the key to be provided as input to the miniport*/
    fbe_key_handle_t *kek_ptr; 
    fbe_u32_t   key_size;
    fbe_key_handle_t kek_kek_handle; /* We need the kek kek handle to unwrap this KEK */
    fbe_key_handle_t kek_handle; /* OUTPUT */
}fbe_base_port_mgmt_register_kek_t;

/* FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEY_FOR_KEY_ENCRYPTION_KEY */
typedef struct fbe_base_port_mgmt_register_kek_kek_s{
    /* Pointer to the key to be provided as input to the miniport*/
    fbe_key_handle_t *kek_ptr; 
    fbe_u32_t   key_size;
    fbe_key_handle_t mp_key_handle;
}fbe_base_port_mgmt_register_kek_kek_t;

/* FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEYS */
typedef struct fbe_base_port_mgmt_unregister_keys_s{
    fbe_u32_t num_of_keys;
    fbe_key_handle_t mp_key_handle[FBE_PORT_MAX_KEYS_PER_REQUEST]; 
} fbe_base_port_mgmt_unregister_keys_t;

/* FBE_BASE_PORT_CONTROL_CODE_GET_KEY_ENCRYPTION_KEY_HANDLE */
typedef struct fbe_base_port_mgmt_get_kek_handle_s{
    /* Pointer to the key to be provided as input to the miniport*/
    fbe_key_handle_t kek_ptr; 
}fbe_base_port_mgmt_get_kek_handle_t;

/* FBE_BASE_PORT_CONTROL_CODE_REESTABLISH_KEY_HANDLE */
typedef struct fbe_base_port_mgmt_reestablish_key_handle_s{
    fbe_key_handle_t mp_key_handle; 
} fbe_base_port_mgmt_reestablish_key_handle_t;

/* FBE_BASE_PORT_CONTROL_CODE_DBEUG_REGISTER_DEK */
typedef struct fbe_base_port_mgmt_debug_register_dek_s{
    fbe_u32_t key_size;
    fbe_u8_t key[72];
    fbe_key_handle_t key_handle;
}fbe_base_port_mgmt_debug_register_dek_t;

/* CPD_IOCTL_GET_NAME */
typedef struct fbe_port_name_s {	
	fbe_address_t port_name;
} fbe_port_name_t;

typedef struct fbe_port_hardware_info_s{
    fbe_u32_t   pci_bus;
    fbe_u32_t   pci_slot;
    fbe_u32_t   pci_function;

    fbe_u32_t   vendor_id;
    fbe_u32_t   device_id;

    fbe_u32_t   hw_major_rev;       // hardware major rev
    fbe_u32_t   hw_minor_rev;       // hardware minor rev
    fbe_u32_t   firmware_rev_1;     // Firmware rev - 4 levels
    fbe_u32_t   firmware_rev_2;
    fbe_u32_t   firmware_rev_3;
    fbe_u32_t   firmware_rev_4;
}fbe_port_hardware_info_t;

#define FBE_PORT_SFP_VENDOR_DATA_LENGTH     32
#define FBE_PORT_SFP_EMC_DATA_LENGTH        32

typedef enum fbe_port_sfp_condition_type_e{
    FBE_PORT_SFP_CONDITION_INVALID,
    FBE_PORT_SFP_CONDITION_GOOD,
    FBE_PORT_SFP_CONDITION_INSERTED,
    FBE_PORT_SFP_CONDITION_REMOVED,
    FBE_PORT_SFP_CONDITION_FAULT,
    FBE_PORT_SFP_CONDITION_WARNING,
    FBE_PORT_SFP_CONDITION_INFO,
    FBE_PORT_SFP_CONDITION_MAX,
}fbe_port_sfp_condition_type_t;


typedef enum fbe_port_sfp_sub_condition_type_e{
    FBE_PORT_SFP_SUBCONDITION_NONE,
    FBE_PORT_SFP_SUBCONDITION_GOOD,
    FBE_PORT_SFP_SUBCONDITION_DEVICE_ERROR,
    FBE_PORT_SFP_SUBCONDITION_CHECKSUM_PENDING,
    FBE_PORT_SFP_SUBCONDITION_UNSUPPORTED
}fbe_port_sfp_sub_condition_type_t;

typedef enum fbe_port_sfp_media_type_e{
    FBE_PORT_SFP_MEDIA_TYPE_INVALID,
    FBE_PORT_SFP_MEDIA_TYPE_COPPER,
    FBE_PORT_SFP_MEDIA_TYPE_OPTICAL,
    FBE_PORT_SFP_MEDIA_TYPE_NAS_COPPER,
    FBE_PORT_SFP_MEDIA_TYPE_UNKNOWN,
    FBE_PORT_SFP_MEDIA_TYPE_MINI_SAS_HD
}fbe_port_sfp_media_type_t;

typedef enum
{
    FBE_SFP_PROTOCOL_UNKNOWN = 0x0000,
    FBE_SFP_PROTOCOL_FC = 0x0001,
    FBE_SFP_PROTOCOL_ISCSI = 0x0002,
    FBE_SFP_PROTOCOL_FCOE = 0x0004
}fbe_sfp_protocol_t;

typedef struct fbe_port_sfp_info_s{
    fbe_port_sfp_condition_type_t condition_type;
    fbe_port_sfp_sub_condition_type_t   sub_condition_type;
    fbe_u32_t                condition_additional_info;
    fbe_u32_t                speeds;
    fbe_port_sfp_media_type_t     media_type;
    fbe_u32_t                cable_length;
    fbe_u32_t                hw_type;

    fbe_u8_t                 emc_part_number[FBE_PORT_SFP_EMC_DATA_LENGTH];
    fbe_u8_t                 emc_part_revision[FBE_PORT_SFP_EMC_DATA_LENGTH];
    fbe_u8_t                 emc_serial_number[FBE_PORT_SFP_EMC_DATA_LENGTH];

    fbe_u8_t                 vendor_part_number[FBE_PORT_SFP_VENDOR_DATA_LENGTH];
    fbe_u8_t                 vendor_part_revision[FBE_PORT_SFP_VENDOR_DATA_LENGTH];
    fbe_u8_t                 vendor_serial_number[FBE_PORT_SFP_VENDOR_DATA_LENGTH];
    fbe_sfp_protocol_t       supported_protocols;
}fbe_port_sfp_info_t;

typedef struct fbe_port_dbg_ctrl_s{
    fbe_bool_t insert_masked;
}fbe_port_dbg_ctrl_t;

typedef struct fbe_port_set_encryption_mode_s{
    fbe_port_encryption_mode_t encryption_mode;
}fbe_base_port_mgmt_set_encryption_mode_t;
#endif /* FBE_PORT_H */

