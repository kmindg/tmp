#ifndef FBE_PAYLOAD_DISCOVERY_OPERATION_H
#define FBE_PAYLOAD_DISCOVERY_OPERATION_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_payload_sg_descriptor.h"
/* #include "fbe/fbe_enclosure.h" */

typedef enum fbe_payload_discovery_element_type_e{
    FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_INVALID,
    FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_SSP,
    FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_STP,
    FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_EXPANDER,
    FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_VIRTUAL,

    FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_LAST
}fbe_payload_discovery_element_type_t;

typedef enum fbe_payload_discovery_opcode_e {
    FBE_PAYLOAD_DISCOVERY_OPCODE_INVALID,
    FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PORT_OBJECT_ID,
    FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PROTOCOL_ADDRESS,
    FBE_PAYLOAD_DISCOVERY_OPCODE_GET_ELEMENT_LIST,
    FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SLOT_INFO,
    FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SPINUP_CREDIT,
    FBE_PAYLOAD_DISCOVERY_OPCODE_RELEASE_SPINUP_CREDIT,
    FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_ON,
    FBE_PAYLOAD_DISCOVERY_OPCODE_UNBYPASS,
    FBE_PAYLOAD_DISCOVERY_OPCODE_SPIN_UP,
    FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_ON,
    FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_ON_PASSIVE,
    FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_SAVE_OFF,
    FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_CYCLE,
    FBE_PAYLOAD_DISCOVERY_OPCODE_CHECK_DUP_ENCL_SN,
    FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SERVER_INFO,
    FBE_PAYLOAD_DISCOVERY_OPCODE_GET_DRIVE_LOCATION_INFO,
    FBE_PAYLOAD_DISCOVERY_OPCODE_NOTIFY_FW_ACTIVATION_STATUS,
	FBE_PAYLOAD_DISCOVERY_OPCODE_RESET_SLOT,

    FBE_PAYLOAD_DISCOVERY_OPCODE_LAST
}fbe_payload_discovery_opcode_t;

typedef enum fbe_payload_discovery_status_e{
    FBE_PAYLOAD_DISCOVERY_STATUS_OK,
    FBE_PAYLOAD_DISCOVERY_STATUS_FAILURE,
    
    FBE_PAYLOAD_DISCOVERY_STATUS_LAST
}fbe_payload_discovery_status_t;

typedef fbe_u32_t fbe_payload_discovery_status_qualifier_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PORT_OBJECT_ID */
typedef struct fbe_payload_discovery_get_port_object_id_data_s {
    fbe_object_id_t port_object_id;
}fbe_payload_discovery_get_port_object_id_data_t;


/* FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PROTOCOL_ADDRESS */
typedef struct fbe_payload_discovery_get_protocol_address_command_s {
    fbe_object_id_t client_id;
}fbe_payload_discovery_get_protocol_address_command_t;

typedef struct fbe_payload_discovery_get_protocol_address_data_s {
    fbe_address_t address;
    fbe_generation_code_t generation_code;
    fbe_payload_discovery_element_type_t element_type;
    fbe_u8_t chain_depth;
}fbe_payload_discovery_get_protocol_address_data_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_GET_ELEMENT_LIST */
typedef struct fbe_payload_discovery_get_element_list_command_s {
    fbe_address_t address;
    fbe_generation_code_t generation_code;
}fbe_payload_discovery_get_element_list_command_t;

enum fbe_payload_discovery_element_list_size_e {
    FBE_PAYLOAD_DISCOVERY_ELEMENT_LIST_SIZE = 24,
};

enum fbe_payload_discovery_enclosure_serial_number_size_e{
    FBE_PAYLOAD_DISCOVERY_ENCLOSURE_SERIAL_NUMBER_SIZE = 16,
};

typedef struct fbe_payload_discovery_element_s{
    fbe_payload_discovery_element_type_t element_type;
    fbe_address_t address;
    fbe_generation_code_t generation_code;
    fbe_u8_t phy_number;
    fbe_u8_t enclosure_chain_depth;
    fbe_u8_t reserved2;
    fbe_u8_t reserved3;
}fbe_payload_discovery_element_t;

typedef struct fbe_payload_discovery_get_element_list_data_s {
    fbe_u32_t number_of_elements;
    fbe_payload_discovery_element_t element_list[FBE_PAYLOAD_DISCOVERY_ELEMENT_LIST_SIZE];
}fbe_payload_discovery_get_element_list_data_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SLOT_INFO */
typedef struct fbe_payload_discovery_get_slot_info_command_s {
    fbe_object_id_t client_id;
}fbe_payload_discovery_get_slot_info_command_t;

typedef struct fbe_payload_discovery_get_slot_info_data_s {
    fbe_bool_t inserted;
    fbe_bool_t powered_off;
    fbe_bool_t bypassed;
    fbe_u8_t   local_lcc_side_id;
}fbe_payload_discovery_get_slot_info_data_t;


/* FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_ON */
typedef struct fbe_payload_discovery_power_on_command_s {
    fbe_object_id_t client_id;
}fbe_payload_discovery_power_on_command_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_UNBYPASS */
typedef struct fbe_payload_discovery_unbypass_command_s {
    fbe_object_id_t client_id;
}fbe_payload_discovery_unbypass_command_t;

/* The following two structs need to removed. */
/* FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_ON_SLOT */
typedef struct fbe_payload_discovery_power_on_slot_command_s {
    fbe_object_id_t client_id;
}fbe_payload_discovery_power_on_slot_command_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_UNBYPASS_SLOT */
typedef struct fbe_payload_discovery_unbypass_slot_command_s {
    fbe_object_id_t client_id;
}fbe_payload_discovery_unbypass_slot_command_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_SPIN_UP */
typedef struct fbe_payload_discovery_spin_up_command_s {
    fbe_object_id_t client_id;
}fbe_payload_discovery_spin_up_command_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_POWER_CYCLE */
typedef struct fbe_payload_discovery_power_cycle_command_s {
    fbe_object_id_t client_id;
    fbe_bool_t completed;
    fbe_u32_t duration;
}fbe_payload_discovery_power_cycle_command_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_CHECK_DUP_ENCL_SN */
typedef struct fbe_payload_discovery_check_dup_encl_SN_data_s{
    fbe_u8_t  encl_SN[FBE_PAYLOAD_DISCOVERY_ENCLOSURE_SERIAL_NUMBER_SIZE];
    fbe_u32_t serial_number_length;
    fbe_u32_t encl_pos;
}fbe_payload_discovery_check_dup_encl_SN_data_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_NOTIFY_FW_ACTIVATION_STATUS */
typedef struct fbe_payload_discovery_notify_fw_activation_status_s{
	fbe_object_id_t client_id;
    fbe_bool_t activation_in_progress;
}fbe_payload_discovery_notify_fw_activation_status_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SERVER_INFO */
typedef struct fbe_payload_discovery_get_server_info_command_s {
	fbe_object_id_t client_id;
}fbe_payload_discovery_get_server_info_command_t;
typedef struct fbe_payload_discovery_get_server_info_data_s {
    fbe_address_t address;
    fbe_port_number_t port_number;
    fbe_u32_t position;
    fbe_enclosure_component_id_t component_id;
}fbe_payload_discovery_get_server_info_data_t;

/* FBE_PAYLOAD_DISCOVERY_OPCODE_GET_BUS_ENCLOSURE_SLOT */
typedef struct fbe_payload_discovery_get_drive_location_info_s {
    fbe_u8_t  sp_id;
    fbe_port_number_t port_number;
    fbe_u32_t enclosure_number;
    fbe_u16_t slot_number;
    fbe_u32_t bank_width;
}fbe_payload_discovery_get_drive_location_info_t;

/* Default command format */
typedef struct fbe_payload_discovery_commom_command_s {
    fbe_object_id_t client_id;
}fbe_payload_discovery_common_command_t;

/* This is our internal discovery protocol*/
typedef struct fbe_payload_discovery_operation_s{
    fbe_payload_operation_header_t          operation_header; /* Must be first */

    fbe_payload_discovery_opcode_t discovery_opcode;
    union {
        fbe_payload_discovery_common_command_t                  common_command;
        fbe_payload_discovery_get_protocol_address_command_t    get_protocol_address_command;
        fbe_payload_discovery_get_element_list_command_t        get_element_list_command;
        fbe_payload_discovery_get_slot_info_command_t           get_slot_info_command;
        fbe_payload_discovery_power_on_command_t                power_on_command;
        fbe_payload_discovery_unbypass_command_t                unbypass_command;
/* The following two fields need to be removed because we have alread created the generic functions 
 * while are used for all the based discovered objects.
 */
        fbe_payload_discovery_power_on_slot_command_t           power_on_slot_command;
        fbe_payload_discovery_unbypass_slot_command_t           unbypass_slot_command;
        fbe_payload_discovery_spin_up_command_t                 spin_up_command;
        fbe_payload_discovery_power_cycle_command_t             power_cycle_command;
        fbe_payload_discovery_notify_fw_activation_status_t     notify_fw_status_command;
        fbe_payload_discovery_get_server_info_command_t         get_server_info_command;
    }command;
    fbe_payload_sg_descriptor_t payload_sg_descriptor; /* describes sg list passed in. */
    fbe_payload_discovery_status_t                              status;
    fbe_payload_discovery_status_qualifier_t                    status_qualifier;
}fbe_payload_discovery_operation_t;


fbe_status_t fbe_payload_discovery_get_opcode(fbe_payload_discovery_operation_t * discovery_operation,
                                              fbe_payload_discovery_opcode_t * discovery_opcode);

fbe_status_t fbe_payload_discovery_get_status(fbe_payload_discovery_operation_t * discovery_operation, fbe_payload_discovery_status_t * status);
fbe_status_t fbe_payload_discovery_set_status(fbe_payload_discovery_operation_t * discovery_operation, fbe_payload_discovery_status_t status);

fbe_status_t fbe_payload_discovery_get_status_qualifier(fbe_payload_discovery_operation_t * discovery_operation, fbe_payload_discovery_status_qualifier_t * status_qualifier);
fbe_status_t fbe_payload_discovery_set_status_qualifier(fbe_payload_discovery_operation_t * discovery_operation, fbe_payload_discovery_status_qualifier_t status_qualifier);

fbe_status_t fbe_payload_discovery_build_get_protocol_address(fbe_payload_discovery_operation_t * discovery_operation,
                                                              fbe_object_id_t client_id);

fbe_status_t fbe_payload_discovery_build_get_port_object_id(fbe_payload_discovery_operation_t * discovery_operation);

fbe_status_t fbe_payload_discovery_build_get_element_list(fbe_payload_discovery_operation_t * discovery_operation,
                                                          fbe_address_t address, fbe_generation_code_t generation_code);

fbe_status_t fbe_payload_discovery_build_get_slot_info(fbe_payload_discovery_operation_t * discovery_operation,
                                                       fbe_object_id_t client_id);

fbe_status_t fbe_payload_discovery_build_power_on(fbe_payload_discovery_operation_t * discovery_operation, 
                                                  fbe_object_id_t client_id);

fbe_status_t fbe_payload_discovery_build_unbypass(fbe_payload_discovery_operation_t * discovery_operation, 
                                                  fbe_object_id_t client_id);

/* The following two declarations need to be removed. */
fbe_status_t fbe_payload_discovery_build_power_on_slot(fbe_payload_discovery_operation_t * discovery_operation,
                                                       fbe_object_id_t client_id);

fbe_status_t fbe_payload_discovery_build_unbypass_slot(fbe_payload_discovery_operation_t * discovery_operation,
                                                       fbe_object_id_t client_id);

fbe_status_t fbe_payload_discovery_build_spin_up(fbe_payload_discovery_operation_t * discovery_operation,
                                                 fbe_object_id_t client_id);

fbe_status_t fbe_payload_discovery_build_power_cycle(fbe_payload_discovery_operation_t * discovery_operation, 
                                                     fbe_object_id_t client_id,
                                                     fbe_bool_t completed,
                                                     fbe_u32_t duration);

fbe_status_t fbe_payload_discovery_build_common_command(fbe_payload_discovery_operation_t * discovery_operation,
                                                        fbe_payload_discovery_opcode_t opcode,
                                                        fbe_object_id_t client_id);

fbe_status_t fbe_payload_discovery_get_children_list_command_sas_address(fbe_payload_discovery_operation_t * discovery_operation,
                                                                         fbe_sas_address_t * sas_address);

fbe_status_t fbe_payload_discovery_build_get_server_info(fbe_payload_discovery_operation_t * discovery_operation,
                                                         fbe_object_id_t client_id);

fbe_status_t
fbe_payload_discovery_build_notify_fw_activation_status(fbe_payload_discovery_operation_t * discovery_operation,
                                                        fbe_bool_t in_progress,
                                                        fbe_object_id_t client_id);

fbe_status_t
fbe_payload_discovery_build_encl_SN(fbe_payload_discovery_operation_t * discovery_operation,  
                                    fbe_payload_discovery_check_dup_encl_SN_data_t *op_data,
                                    fbe_u8_t *encl_SN,
                                    fbe_u32_t length);

fbe_status_t 
fbe_payload_discovery_build_get_drive_location_info(fbe_payload_discovery_operation_t * discovery_operation,
                                                   fbe_object_id_t client_id);
#endif /* FBE_PAYLOAD_DISCOVERY_OPERATION_H */
