#ifndef DELL_BOARD_PRIVATE_H
#define DELL_BOARD_PRIVATE_H

#include "fbe/fbe_port.h"
#include "fbe_base_board.h"
#include "base_board_private.h"
#include "fbe_fibre_cpd_shim.h"

typedef fbe_u32_t fbe_dell_board_state_t;
typedef enum fbe_dell_board_state_bit_e {
	FBE_DELL_BOARD_UPDATE_HARDWARE_PORTS = 0x00000001
}fbe_dell_board_state_bit_t;

enum fbe_dell_board_number_of_slots_e{
	FBE_DELL_BOARD_NUMBER_OF_IO_PORTS = 4
};

enum fbe_dell_board_first_slot_index_e{
	FBE_DELL_BOARD_FIRST_IO_PORT_INDEX = 0
};

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(dell_board);

typedef struct fbe_dell_board_s{
	fbe_base_board_t			base_board;
	fbe_dell_board_state_t		dell_board_state;
	fbe_u32_t					io_port_array[FBE_DELL_BOARD_NUMBER_OF_IO_PORTS];
	fbe_port_type_t				io_port_type[FBE_DELL_BOARD_NUMBER_OF_IO_PORTS];

	FBE_LIFECYCLE_DEF_INST_DATA;

}fbe_dell_board_t;

fbe_status_t fbe_dell_board_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_dell_board_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_dell_board_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_dell_board_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_dell_board_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_dell_board_monitor_load_verify(void);

fbe_status_t fbe_dell_board_update_hardware_ports(fbe_dell_board_t * dell_board, fbe_packet_t * packet);

#endif /* DELL_BOARD_PRIVATE_H */
