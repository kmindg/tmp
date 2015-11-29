#ifndef FBE_PAYLOAD_SMP_OPERATION_H
#define FBE_PAYLOAD_SMP_OPERATION_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_payload_sg_descriptor.h"

typedef enum fbe_payload_smp_element_type_e{
    FBE_PAYLOAD_SMP_ELEMENT_TYPE_INVALID,
    FBE_PAYLOAD_SMP_ELEMENT_TYPE_SSP,
    FBE_PAYLOAD_SMP_ELEMENT_TYPE_STP,
    FBE_PAYLOAD_SMP_ELEMENT_TYPE_EXPANDER,
    FBE_PAYLOAD_SMP_ELEMENT_TYPE_VIRTUAL,

    FBE_PAYLOAD_SMP_ELEMENT_TYPE_LAST
} fbe_payload_smp_element_type_t;

typedef enum fbe_payload_smp_opcode_e {
    FBE_PAYLOAD_SMP_OPCODE_INVALID,
    FBE_PAYLOAD_SMP_OPCODE_GET_ELEMENT_LIST,
    FBE_PAYLOAD_SMP_OPCODE_RESET_END_DEVICE,
    FBE_PAYLOAD_SMP_OPCODE_LAST
} fbe_payload_smp_opcode_t;

/* FBE_PAYLOAD_SMP_OPCODE_GET_ELEMENT_LIST */
typedef struct fbe_payload_smp_get_element_list_command_s {
    fbe_address_t address;
    fbe_generation_code_t generation_code;
}fbe_payload_smp_get_element_list_command_t;

enum fbe_payload_smp_element_list_size_e {
   /* This is calculated based on FBE_MEMORY_CHUNK_SIZE and sizeof(fbe_payload_smp_element_t)
    * Increase to 70 to support Cayenne and Naga enclosure.
    * Cayenne IOSXP elements (0 drives + 5 ports(4 external and 1 internal) + 1 virtual phy = 6; 
    * Cayenne DRVSXP elements (60 drives + 1 incoming port + 1 virtual phy = 62;
    * Naga IOSXP elements (0 drives + 6 ports(4 external and 2 internal) + 1 virtual phy = 7; 
    * Naga DRVSXP elements (60 drives + 1 incoming port + 1 virtual phy = 62; 
    * SAS driver sets 69 as the maximum number of elements in the registry. We just use 70 here. 
    */
    FBE_PAYLOAD_SMP_ELEMENT_LIST_SIZE = 70,  
};

typedef enum fbe_payload_smp_element_extended_status_e{
    FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_NORMAL,
    FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_UNSUPPORTED_ENCL,
    FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_UNSUPPORTED_TOPOLOGY,
    FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_MIXED_COMPLIANCE,
    FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_TOO_MANY_END_DEVICES,
    FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_NO_DEVICE,
    FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_LAST
}fbe_payload_smp_element_extended_status_t;

typedef struct fbe_payload_smp_element_s{
    fbe_payload_smp_element_type_t element_type;
    fbe_address_t address;
    fbe_generation_code_t generation_code;
    fbe_u8_t phy_number;
    fbe_u8_t enclosure_chain_depth;
    fbe_payload_smp_element_extended_status_t   element_status;
    fbe_u8_t reserved2;
    fbe_u8_t reserved3;
} fbe_payload_smp_element_t;

typedef struct fbe_payload_smp_get_element_list_data_s {
    fbe_u32_t number_of_elements;
    fbe_payload_smp_element_t element_list[FBE_PAYLOAD_SMP_ELEMENT_LIST_SIZE];
} fbe_payload_smp_get_element_list_data_t;

typedef enum fbe_payload_smp_status_e{
	FBE_PAYLOAD_SMP_STATUS_OK,
	FBE_PAYLOAD_SMP_STATUS_FAILURE,	
	FBE_PAYLOAD_SMP_STATUS_LAST
}fbe_payload_smp_status_t;

/* FBE_PAYLOAD_SMP_OPCODE_RESET_END_DEVICE */
typedef struct fbe_payload_smp_reset_end_device_command_s {
    fbe_u8_t              phy_id;/* Phy ID of the device to be reset.*/
}fbe_payload_smp_reset_end_device_command_t;

typedef fbe_u32_t fbe_payload_smp_status_qualifier_t;

typedef struct fbe_payload_smp_operation_s{
    fbe_payload_operation_header_t operation_header; /* Must be first */
    fbe_payload_smp_opcode_t smp_opcode;
    union {
        fbe_payload_smp_get_element_list_command_t get_element_list_command;
        fbe_payload_smp_reset_end_device_command_t reset_end_device_command;
    }command;
    fbe_payload_sg_descriptor_t payload_sg_descriptor;
    fbe_payload_smp_status_t                status;
    fbe_payload_smp_status_qualifier_t      status_qualifier;
} fbe_payload_smp_operation_t;

fbe_status_t fbe_payload_smp_get_opcode(fbe_payload_smp_operation_t * smp_operation,
                                        fbe_payload_smp_opcode_t * smp_opcode);

fbe_status_t fbe_payload_smp_build_get_element_list(fbe_payload_smp_operation_t * smp_operation,
                                                    fbe_address_t address,
                                                    fbe_generation_code_t generation_code);

fbe_status_t fbe_payload_smp_get_children_list_command_sas_address(fbe_payload_smp_operation_t * smp_operation,
                                                                   fbe_sas_address_t * sas_address);

fbe_status_t fbe_payload_smp_build_reset_end_device(fbe_payload_smp_operation_t * smp_operation,
                                                    fbe_u8_t phy_id);
fbe_status_t fbe_payload_smp_get_status(fbe_payload_smp_operation_t * smp_operation, fbe_payload_smp_status_t * status);
fbe_status_t fbe_payload_smp_set_status(fbe_payload_smp_operation_t * smp_operation, fbe_payload_smp_status_t status);

fbe_status_t fbe_payload_smp_get_status_qualifier(fbe_payload_smp_operation_t * smp_operation, fbe_payload_smp_status_qualifier_t * status_qualifier);
fbe_status_t fbe_payload_smp_set_status_qualifier(fbe_payload_smp_operation_t * smp_operation, fbe_payload_smp_status_qualifier_t status_qualifier);

#endif /* FBE_PAYLOAD_SMP_OPERATION_H */
