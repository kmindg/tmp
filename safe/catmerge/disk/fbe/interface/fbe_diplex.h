#ifndef FBE_DIPLEX_H
#define FBE_DIPLEX_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"

#define FBE_DIPLEX_MAX_PORTS 8
#define FBE_DIPLEX_MAX_ENCLOSURES 8

#define FBE_DIPLEX_BUFFER_LEN 125

#define ENCLOSURE_ADDRESS_MASK 0x07

fbe_status_t fbe_diplex_init(void);
fbe_status_t fbe_diplex_destroy(void);

fbe_status_t fbe_diplex_init_port(fbe_u32_t port_number, fbe_u32_t diplex_port_number);
fbe_status_t fbe_diplex_destroy_port(fbe_u32_t port_number);

typedef enum fbe_diplex_opcode_e {
	FBE_DIPLEX_OPCODE_LCC_POLL        = 0x11,
	FBE_DIPLEX_OPCODE_LCC_READ        = 0x12,
	FBE_DIPLEX_OPCODE_LCC_WRITE       = 0x13,
	FBE_DIPLEX_OPCODE_LCC_SERIAL_READ = 0x14,
	FBE_DIPLEX_OPCODE_LCC_EXT_STATUS  = 0x2B
} fbe_diplex_opcode_t;

typedef struct fbe_diplex_command_s {
	fbe_u8_t in_buffer[FBE_DIPLEX_BUFFER_LEN];
	fbe_u8_t out_buffer[FBE_DIPLEX_BUFFER_LEN];
}fbe_diplex_command_t;

typedef int (*fbe_diplex_callback_func_t)(fbe_u32_t port, fbe_diplex_command_t * command);

fbe_status_t fbe_diplex_set_callback(fbe_diplex_opcode_t opcode, fbe_diplex_callback_func_t callback);

/* This type is result of the poll command */
enum fbe_diplex_lcc_info_e{
	FBE_DIPLEX_LCC_INFO_INVALID = 0xFF
};

typedef fbe_u8_t fbe_diplex_lcc_info_t;

fbe_status_t fbe_diplex_send_cmd(fbe_u32_t port_number, fbe_diplex_command_t * diplex_command, fbe_u32_t in_len, fbe_u32_t * out_len);


fbe_status_t fbe_diplex_build_poll_cmd(fbe_u8_t * buffer, fbe_u32_t len);
fbe_status_t fbe_diplex_poll_response_get_enclosures_number(fbe_u8_t * buffer, fbe_u32_t * enclosures_number);

fbe_status_t fbe_diplex_poll_response_get_enclosure_address(fbe_u8_t * buffer, 
															fbe_u32_t enclosure_index, 
															fbe_diplex_address_t * enclosure_address);


fbe_status_t fbe_diplex_build_broadcast_enable_all_drives_cmd(fbe_u8_t * buffer, fbe_u32_t len);
fbe_status_t fbe_diplex_build_read_cmd(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_address_t enclosure_address);
fbe_status_t fbe_diplex_validate_checksum(fbe_u8_t * buffer, fbe_u32_t len);
fbe_status_t fbe_diplex_build_serial_read_cmd(	fbe_u8_t * buffer,
												fbe_u32_t len,
												fbe_diplex_address_t enclosure_address,
												fbe_u8_t fru_prom_id,
												fbe_u8_t read_size,
												fbe_u32_t start_addr);
fbe_status_t fbe_diplex_get_serial_read_cmd_addr(fbe_u8_t * buffer, fbe_u32_t len, fbe_u32_t * read_addr);
fbe_status_t fbe_diplex_get_serial_read_cmd_read_size(fbe_u8_t * buffer, fbe_u32_t len, fbe_u8_t * read_size);
fbe_status_t fbe_diplex_get_serial_read_cmd_enclosure_addr(	fbe_u8_t * buffer,
															fbe_u32_t len,
															fbe_diplex_address_t * enclosure_addr);
fbe_status_t fbe_diplex_get_serial_read_rsp_length(fbe_u8_t * buffer, fbe_u32_t len, fbe_u8_t * rsp_length);
fbe_status_t fbe_diplex_set_serial_read_rsp_length(fbe_u8_t * buffer, fbe_u32_t len, fbe_u8_t rsp_length);
fbe_status_t fbe_diplex_get_serial_read_rsp(fbe_u8_t * buffer, fbe_u32_t len, fbe_u8_t ** rsp);

typedef enum fbe_diplex_drive_state_bit_e {
	FBE_DIPLEX_DRIVE_STATE_REMOVED			= 0x0001,
	FBE_DIPLEX_DRIVE_STATE_INSERTED			= 0x0002,
	FBE_DIPLEX_DRIVE_STATE_INTERNAL_FAULT	= 0x0004,
	FBE_DIPLEX_DRIVE_STATE_REQUEST_BYPASS	= 0x0008,
	FBE_DIPLEX_DRIVE_STATE_BYPASSED			= 0x0010
}fbe_diplex_drive_state_bit_t;

typedef fbe_u32_t fbe_diplex_drive_state_t;
typedef fbe_u32_t fbe_diplex_slot_t;

fbe_status_t fbe_diplex_get_drive_state(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_slot_t slot, fbe_diplex_drive_state_t * state);

/* READ power regulator status A and B. */
typedef fbe_u16_t fbe_diplex_power_status_t;

/* READ cable port status. */
typedef fbe_u8_t fbe_diplex_cable_port_status_t;

/* READ misc status registers 1 and 2. */
typedef fbe_u16_t fbe_diplex_misc_status_register_t;
typedef enum fbe_diplex_misc_status_bit_e {
	FBE_DIPLEX_EXTENDED_STATE_CHANGE     = 0x8000, /* R1/B7: when 1, extended state change. */
	FBE_DIPLEX_PRIMARY_IS_BYPASSED       = 0x4000, /* R1/B6: when 0, primary port is bypassed. */
	FBE_DIPLEX_EXPANSION_IS_BYPASSED     = 0x2000, /* R1/B5: when 0, expansion port is bypassed. */
	FBE_DIPLEX_DPE_OR_DAE_INDICATOR      = 0x1000, /* R1/B4: when 0, enclosure is a DPE, when 1, a DAE. */
	FBE_DIPLEX_OTHER_LCC_FAULT           = 0x0800, /* R1/B3: when 0, the other LCC in the enclosure had a fault. */
	FBE_DIPLEX_OTHER_LCC_PRESENT         = 0x0400, /* R1/B2: when 0, the other LCC in the enclosure is present. */
	FBE_DIPLEX_DESK_OR_RACK_INDICATOR    = 0x0200, /* R1/B1: when 0, desk-side enclosure, when 1, a rack-mount. */
	FBE_DIPLEX_SLOT_INDICATOR            = 0x0100, /* R1/B0: when 0, LCC is in enclosure slot B, when 1, in slot A. */
	FBE_DIPLEX_ENCLOSURE_FAULT_IS_OFF    = 0x0080, /* R2/B7: when 0, enclosure fault LED illuminated. */
	FBE_DIPLEX_ENCLOSURE_ID_MASK         = 0x0070, /* R2/B6-4: enclosure identifier (range from 0 to 7). */
	FBE_DIPLEX_LOOP_ID_VALID             = 0x0008, /* R2/B3: when zero, loop ID value is indeterminate. */
	FBE_DIPLEX_LOOP_ID_MASK              = 0x0007  /* R2/B2-0: loop ID (range from 0 to 7).. */
} fbe_diplex_misc_status_bit_t;

typedef enum fbe_diplex_expansion_port_state_bit_e {
	FBE_DIPLEX_EXPANSION_PORT_STATE_INVALID		= 0x0000, 
	FBE_DIPLEX_EXPANSION_PORT_STATE_REMOVED		= 0x0001,
	FBE_DIPLEX_EXPANSION_PORT_STATE_INSERTED	= 0x0002,
	FBE_DIPLEX_EXPANSION_PORT_STATE_BYPASSED	= 0x0004
}fbe_diplex_expansion_port_state_bit_t;

typedef fbe_u32_t fbe_diplex_expansion_port_state_t;
typedef fbe_u32_t fbe_diplex_expansion_port_t;

fbe_status_t fbe_diplex_get_expansion_port_state(fbe_diplex_misc_status_register_t misc_status, fbe_diplex_lcc_info_t lcc_info);

/* READ additional status. */
typedef fbe_u8_t fbe_diplex_additional_status_t;
typedef enum fbe_diplex_additional_status_bit_e {
	FBE_DIPLEX_LOOP_ID_MISMATCH          = 0x80, /* B7: when 1, loop id mismatch between LCCs. */
	FBE_DIPLEX_PEER_IN_LOCAL             = 0x40, /* B6: when 1, peer has not received its loop id from the host. */
	FBE_DIPLEX_HARDWARE_TYPE_CODE        = 0x38, /* B5-3: LCC hardware type. */
	FBE_DIPLEX_ADDITIONAL_STATUS_B2      = 0x04, /* B4: reserved. */
	FBE_DIPLEX_ADDITIONAL_STATUS_B1      = 0x02, /* B2: reserved. */
	FBE_DIPLEX_ADDITIONAL_STATUS_B0      = 0x01, /* B1: reserved. */
} fbe_diplex_additional_status_bit_t;

/* READ additional status bits 5,4,3 (shifted down) are the hardware type. */
typedef enum fbe_diplex_hardware_type_code_e {
	FBE_DIPLEX_HARDWARE_TYPE_CODE_INVALID       = 0xFF,
	FBE_DIPLEX_HARDWARE_TYPE_CODE_KOTO          = 0x00,
	FBE_DIPLEX_HARDWARE_TYPE_CODE_YUKON         = 0x01,
	FBE_DIPLEX_HARDWARE_TYPE_CODE_KOTO_PLUS     = 0x02,
	FBE_DIPLEX_HARDWARE_TYPE_CODE_STILETTO_2GIG = 0x04,
	FBE_DIPLEX_HARDWARE_TYPE_CODE_STILETTO_4GIG = 0x05,
	FBE_DIPLEX_HARDWARE_TYPE_CODE_LAST
} fbe_diplex_hardware_type_code_t;

fbe_diplex_hardware_type_code_t fbe_diplex_get_hardware_type_code(fbe_diplex_additional_status_t additional_status);

/* WRITE misc control registers 1 and 2. */
typedef fbe_u16_t fbe_diplex_misc_control_register_t;
typedef enum fbe_diplex_misc_control_bit_e {
	FBE_DIPLEX_SIGNAL_PEER_LCC           = 0x8000, /* R1/B7: signal peer LCC (unused on Stiletto). */
	FBE_DIPLEX_PSA_OK_TO_SHUTDOWN        = 0x4000, /* R1/B6: when 0, allow power supply A to shutdown. */
	FBE_DIPLEX_ENABLE_ODIS_PRIMARY       = 0x2000, /* R1/B5: when 0, disable fibre primary port. */
	FBE_DIPLEX_ENABLE_ODIS_EXPANSION     = 0x1000, /* R1/B4: when 0, disable fibre expansion port. */
	FBE_DIPLEX_ENABLE_DIPLEX_PRIMARY     = 0x0800, /* R1/B3: when 0, bypass diplex primary port. */
	FBE_DIPLEX_ENABLE_DIPLEX_EXPANSION   = 0x0400, /* R1/B2: when 0, bypass diplex expansion port. */
	FBE_DIPLEX_MISC_CONTROL_R1_B1        = 0x0200, /* R1/B1: not used. */
	FBE_DIPLEX_MISC_CONTROL_R1_B0        = 0x0100, /* R1/B0: not used. */
	FBE_DIPLEX_MISC_CONTROL_R1_B7        = 0x0080, /* R2/B7: reserved. */
	FBE_DIPLEX_PSAB_OK_TO_SHUTDOWN       = 0x0040, /* R2/B6: when 0, allow power supply B to shutdown. */
	FBE_DIPLEX_MISC_CONTROL_R1_B5        = 0x0020, /* R2/B5: reserved. */
	FBE_DIPLEX_MISC_CONTROL_R1_B4        = 0x0010, /* R2/B4: reserved. */
	FBE_DIPLEX_DISABLE_MC_ON_PRIMARY     = 0x0008, /* R2/B3: when 1, disables media converter on primary port. */
	FBE_DIPLEX_DISABLE_MC_ON_EXPANSION   = 0x0004, /* R2/B2: when 1, disables media converter on expansion port. */
	FBE_DIPLEX_FAULT_LED_OFF             = 0x0002, /* R2/B1: when 0, illuminate LCC fault LED. */
	FBE_DIPLEX_ENCLOSURE_FAULT_LED_ON    = 0x0001  /* R2/B0: when 1, illuminate enclosure fault LED. */
} fbe_diplex_misc_control_bit_t;

/* LCC READ/WRITE drive registers */
typedef fbe_u16_t fbe_diplex_drive_register_t; /* Presence, bypass rqst and bypass status, etc. */
typedef enum fbe_diplex_drive_register_bit_e {
	FBE_DIPLEX_NO_DRIVES  = ((fbe_diplex_drive_register_t)0x80000), /* None of the drives in a drive register. */
	FBE_DIPLEX_ALL_DRIVES = ((fbe_diplex_drive_register_t)0x7ffff)  /* All of the drives in a drive register. */
} fbe_diplex_drive_register_bit_t;

fbe_status_t fbe_diplex_build_read_rsp(	fbe_u8_t * buffer,
										fbe_u32_t len,
										fbe_diplex_address_t enclosure_address,
										fbe_diplex_power_status_t power_status,
										fbe_diplex_drive_register_t drive_presence,
										fbe_diplex_drive_register_t drive_fault_sensor,
										fbe_diplex_drive_register_t drive_bypass_request,
										fbe_diplex_drive_register_t drive_bypass_status,
										fbe_diplex_cable_port_status_t cable_port_status,
										fbe_diplex_misc_status_register_t misc_status,
										fbe_diplex_additional_status_t additional_status);
fbe_status_t fbe_diplex_get_read_rsp_power_status(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_power_status_t * power_status);
fbe_status_t fbe_diplex_get_read_rsp_drive_presence(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_drive_register_t * drive_presence);
fbe_status_t fbe_diplex_get_read_rsp_drive_fault_sensor(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_drive_register_t * drive_fault_sensor);
fbe_status_t fbe_diplex_get_read_rsp_drive_bypass_request(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_drive_register_t * drive_bypass_request);
fbe_status_t fbe_diplex_get_read_rsp_drive_bypass_status(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_drive_register_t * drive_bypass_status);
fbe_status_t fbe_diplex_get_read_rsp_cable_port_status(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_cable_port_status_t * cable_port_status);
fbe_status_t fbe_diplex_get_read_rsp_misc_status(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_misc_status_register_t * misc_status);
fbe_status_t fbe_diplex_get_read_rsp_additional_status(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_additional_status_t * additional_status);
fbe_bool_t fbe_diplex_get_read_extended_status(fbe_diplex_misc_status_register_t misc_status);

void fbe_diplex_set_write_cmd_defaults(fbe_u8_t * buffer, fbe_diplex_address_t enclosure_address);
void fbe_diplex_set_write_cmd_drive_fault_led(fbe_u8_t * buffer, fbe_diplex_drive_register_t drive_bits);
void fbe_diplex_set_write_cmd_drive_bypass(fbe_u8_t * buffer, fbe_diplex_drive_register_t drive_bits);
void fbe_diplex_set_write_cmd_drive_enable(fbe_u8_t * buffer, fbe_diplex_drive_register_t drive_bits);
void fbe_diplex_set_write_cmd_misc_control(fbe_u8_t * buffer, fbe_diplex_misc_control_register_t control_bits);
fbe_status_t fbe_diplex_get_write_cmd_length(fbe_u8_t * buffer, fbe_u32_t len, fbe_u8_t * length);
fbe_status_t fbe_diplex_get_write_cmd_enclosure_addr(	fbe_u8_t * buffer,
														fbe_u32_t len,
														fbe_diplex_address_t * enclosure_addr);
fbe_status_t fbe_diplex_get_write_cmd_drive_bypass(	fbe_u8_t * buffer,
													fbe_u32_t len,
													fbe_diplex_drive_register_t * bypass_bits);

fbe_status_t fbe_diplex_build_extended_status(	fbe_u8_t * buffer,
												fbe_u32_t len,
												fbe_diplex_address_t enclosure_address);
fbe_status_t fbe_diplex_get_extended_status_encl_fault(	fbe_u8_t * buffer, fbe_u32_t len, fbe_u8_t * enclosure_fault);
fbe_status_t fbe_diplex_get_extended_status_encl_speed(	fbe_u8_t * buffer, fbe_u32_t len, fbe_u8_t * enclosure_speed);
fbe_status_t fbe_diplex_get_extended_status_drive_init_error(	fbe_u8_t * buffer,
																fbe_u32_t len,
																fbe_diplex_drive_register_t * drive_init_error);
fbe_status_t fbe_diplex_get_extended_status_drive_speed_error(	fbe_u8_t * buffer,
																fbe_u32_t len,
																fbe_diplex_drive_register_t * drive_speed_error);

/* Enclosure map definitions */
typedef fbe_u8_t fbe_diplex_enclosure_map_entry_t;
typedef struct fbe_diplex_enclosure_map_s{
	fbe_diplex_enclosure_map_entry_t enclosure_map_entry[FBE_DIPLEX_MAX_ENCLOSURES];
	fbe_u8_t alpa_of_hba;
}fbe_diplex_enclosure_map_t;

void fbe_diplex_set_enclosure_map_defaults(fbe_diplex_enclosure_map_t * enclosure_map);
void fbe_diplex_set_enclosure_map_entry(fbe_diplex_enclosure_map_t * enclosure_map,
										fbe_u8_t enclosure_index,
										fbe_bool_t stiletto_type,
										fbe_diplex_address_t enclosure_address);

void fbe_diplex_set_enclosure_map_alpa(fbe_diplex_enclosure_map_t * enclosure_map, fbe_u8_t alpa);

fbe_status_t fbe_diplex_build_enclosure_map_cmd(fbe_u8_t * buffer, 
												fbe_u32_t len, 
												fbe_diplex_address_t enclosure_address,
												fbe_diplex_enclosure_map_t * enclosure_map);

fbe_status_t fbe_diplex_get_enclosure_address(fbe_u8_t * buffer, fbe_u32_t len, fbe_diplex_address_t * diplex_address);

fbe_status_t fbe_diplex_set_write_cmd_expansion_port_bypass(fbe_u8_t * buffer, fbe_u32_t len);
fbe_status_t fbe_diplex_set_write_cmd_expansion_port_unbypass(fbe_u8_t * buffer, fbe_u32_t len);

fbe_bool_t fbe_diplex_is_broadcast_command(fbe_u8_t * buffer, fbe_u32_t len);
fbe_status_t fbe_diplex_build_broadcast_cmd(fbe_u8_t * buffer, fbe_u32_t len, fbe_u8_t loop);

fbe_status_t fbe_diplex_purge(fbe_u32_t port_number);

#endif /* FBE_DIPLEX_H */
