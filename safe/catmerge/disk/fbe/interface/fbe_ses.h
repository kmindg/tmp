#ifndef FBE_SES_H
#define FBE_SES_H

#include "fbe/fbe_types.h"

#define FBE_SES_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE 6
#define FBE_SES_SEND_DIAGNOSTIC_RESULTS_CDB_SIZE 6

enum fbe_ses_page_sizes_e {
	FBE_SES_ENCLOSURE_CONFIGURATION_PAGE_SIZE	= 261,
	FBE_SES_ENCLOSURE_STATUS_PAGE_SIZE			= 272,
	FBE_SES_ADDITIONAL_STATUS_PAGE_SIZE			= 488,

	FBE_SES_ENCLOSURE_CONTROL_PAGE_SIZE			= 272,
	FBE_SES_ENCLOSURE_CONTROL_DATA_SIZE			= 268
};

/* SES-2 Device Slot status element */
typedef enum fbe_ses_element_type_e {
	FBE_SES_ELEMENT_TYPE_UNSPECIFIED								= 0x00,
	FBE_SES_ELEMENT_TYPE_DEVICE_SLOT								= 0x01,
	FBE_SES_ELEMENT_TYPE_POWER_SUPPLY								= 0x02,
	FBE_SES_ELEMENT_TYPE_COOLING									= 0x03,
	FBE_SES_ELEMENT_TYPE_TEMPERATURE_SENSOR							= 0x04,
	FBE_SES_ELEMENT_TYPE_DOOR_LOCK									= 0x05,
	FBE_SES_ELEMENT_TYPE_AUDIBLE_ALARM								= 0x06,
	FBE_SES_ELEMENT_TYPE_ENCLOSURE_SERVICES_CONTROLLER_ELECTRONICS	= 0x07,
	FBE_SES_ELEMENT_TYPE_SCC_CONTROLLER_ELECTRONICS					= 0x08,
	FBE_SES_ELEMENT_TYPE_NONVOLATILE_CACHE							= 0x09,
	FBE_SES_ELEMENT_TYPE_INVALID_OPERATION_REASON					= 0x0A,
	FBE_SES_ELEMENT_TYPE_UNINTERRUPTIBLE_POWER_SUPPLY				= 0x0B,
	FBE_SES_ELEMENT_TYPE_DISPLAY									= 0x0C,
	FBE_SES_ELEMENT_TYPE_KEY_PAD_ENTRY								= 0x0D,
	FBE_SES_ELEMENT_TYPE_ENCLOSURE									= 0x0E,
	FBE_SES_ELEMENT_TYPE_SCSI_PORT_TRANSCEIVER						= 0x0F,
	FBE_SES_ELEMENT_TYPE_LANGUAGE									= 0x10,
	FBE_SES_ELEMENT_TYPE_COMMUNICATION_PORT							= 0x11,
	FBE_SES_ELEMENT_TYPE_VOLTAGE_SENSOR								= 0x12,
	FBE_SES_ELEMENT_TYPE_CURRENT_SENSOR								= 0x13,
	FBE_SES_ELEMENT_TYPE_SCSI_TARGET_PORT							= 0x14,
	FBE_SES_ELEMENT_TYPE_SCSI_INITIATOR_PORT						= 0x15,
	FBE_SES_ELEMENT_TYPE_SIMPLE_SUBENCLOSURE						= 0x16,
	FBE_SES_ELEMENT_TYPE_ARRAY_DEVICE_SLOT							= 0x17,
	FBE_SES_ELEMENT_TYPE_SAS_EXPANDER								= 0x18,
	FBE_SES_ELEMENT_TYPE_SAS_CONNECTOR								= 0x19
}fbe_ses_element_type_t;

#pragma pack(1) /* Go back to default alignment.*/

typedef struct fbe_ses_status_element_s {
	/* Byte 0 */
    fbe_u8_t status_code :4;
    fbe_u8_t swap : 1;
    fbe_u8_t disabled : 1;
    fbe_u8_t prdfail : 1;
    fbe_u8_t reserved : 1;
	/* Bytes 1 - 3 element specific*/
    fbe_u8_t element_specific[3];
} fbe_ses_status_element_t;

typedef struct fbe_ses_slot_status_element_s {
	/* Byte 0 */
    fbe_u8_t status_code :4;
    fbe_u8_t swap : 1;
    fbe_u8_t disabled : 1;
    fbe_u8_t prdfail : 1;
    fbe_u8_t reserved : 1;
	/* Byte 1 */
    fbe_u8_t slot_address;
	/* Byte 2 */
    fbe_u8_t report : 1;
    fbe_u8_t ident : 1;
    fbe_u8_t rmv : 1;
    fbe_u8_t ready_insert : 1;
    fbe_u8_t encl_byp_b : 1;
	fbe_u8_t encl_byp_a : 1;
    fbe_u8_t do_not_remove : 1;
    fbe_u8_t app_client_byp_a : 1;
	/* Byte 3 */
	fbe_u8_t dev_byp_b : 1;
	fbe_u8_t dev_byp_a : 1;
	fbe_u8_t byp_b : 1;
	fbe_u8_t byp_a : 1;
    fbe_u8_t device_off : 1;	
    fbe_u8_t fault_reqstd : 1;
	fbe_u8_t fault_sensed : 1;
    fbe_u8_t app_client_byp_b : 1;
} fbe_ses_slot_status_element_t;

/* SES-2 Enclosure descriptor */
typedef struct fbe_ses_enclosure_descriptor_s {
	/* Byte 0 */
    fbe_u8_t number_of_encl_services_processes :3;
    fbe_u8_t reserved1 : 1;
    fbe_u8_t relative_encl_services_process_identifier : 3;
    fbe_u8_t reserved2 : 1;
	/* Byte 1 */
	fbe_u8_t subenclosure_identifier;
	/* Byte 2 */
	fbe_u8_t number_of_type_descriptor_headers;
	/* Byte 3 */
	fbe_u8_t encl_descriptor_length;
	/* Bytes 4 - 11 */
	fbe_u64_t encl_logical_identifier;
	/* Bytes 12 -19 */
	fbe_u64_t encl_vendor_identifier;
	/* Bytes 20 - 35 */
	fbe_u8_t product_identification[16];
	/* Bytes 36 - 39 */
	fbe_u32_t product_revision_level;
	/* Starting form byte 40 vendor specific enclosure info*/
}fbe_ses_enclosure_descriptor_t;

typedef struct fbe_ses_type_descriptor_s {
	/* Byte 0 */
    fbe_u8_t element_type;
	/* Byte 1 */
    fbe_u8_t number_of_possible_elements;
	/* Byte 2 */
    fbe_u8_t subenclosure_identifier;
	/* Byte 3 */
    fbe_u8_t type_descriptor_text_lenght;
}fbe_ses_type_descriptor_t;

typedef struct fbe_ses_config_page_s {
	/* Byte 0 */
    fbe_u8_t page_code;
	/* Byte 1 */
    fbe_u8_t number_of_secondary_subenclosures;
	/* Bytes 2 - 3 */
	fbe_u16_t page_lenght;
	/* Bytes 3 - 7 */
	fbe_u32_t generation_code;
	/* Starting descriptor header list from byte 8 */

	/* Starting descriptor text list */
}fbe_ses_config_page_t;

#pragma pack() /* Go back to default alignment.*/

fbe_status_t fbe_ses_build_encl_config_page(fbe_u8_t *cdb, 
                                            fbe_u32_t cdb_buffer_size, 
                                            fbe_u32_t response_buffer_size);

fbe_status_t fbe_ses_build_additional_status_page(fbe_u8_t *cdb, 
                                                  fbe_u32_t cdb_buffer_size, 
                                                  fbe_u32_t response_buffer_size);

fbe_status_t fbe_ses_build_encl_status_page(fbe_u8_t *cdb, 
                                            fbe_u32_t cdb_buffer_size, 
                                            fbe_u32_t response_buffer_size);

fbe_status_t fbe_ses_build_encl_control_page(fbe_u8_t *cdb, 
                                             fbe_u32_t cdb_buffer_size, 
                                             fbe_u32_t cmd_buffer_size);

fbe_status_t fbe_ses_drive_control_fault_led_onoff(fbe_u8_t slot_id,
                                                   fbe_u8_t *cmd_buffer,
                                                   fbe_u32_t cmd_buffer_size,
                                                   fbe_bool_t fault_led_on);

fbe_status_t fbe_ses_drive_control_fault_led_flash_onoff(fbe_u8_t slot_id,
                                                         fbe_u8_t *cmd_buffer,
                                                         fbe_u32_t cmd_buffer_size,
                                                         fbe_bool_t flash_on);

fbe_status_t fbe_ses_build_encl_control_cmd(fbe_u8_t *cmd, fbe_u32_t cmd_buffer_size);

fbe_status_t fbe_ses_get_drive_status(fbe_u8_t *resp_buffer, fbe_u32_t slot_id, fbe_u32_t *drive_status);
fbe_status_t fbe_ses_get_drive_slot_address(fbe_u8_t *resp_buffer, fbe_u32_t device_num, fbe_u32_t *slot_address);
fbe_status_t fbe_ses_get_drive_swap_info(fbe_u8_t *resp_buffer, fbe_u32_t slot_id, fbe_u32_t *swap);
fbe_status_t fbe_ses_get_drive_sas_address(fbe_u8_t *resp_buffer, fbe_u32_t slot_id, fbe_sas_address_t *sas_address);

fbe_status_t fbe_ses_get_status_element(fbe_u8_t *resp_buffer, fbe_u32_t index, fbe_ses_status_element_t * status_element);

#endif /* FBE_SES_H */