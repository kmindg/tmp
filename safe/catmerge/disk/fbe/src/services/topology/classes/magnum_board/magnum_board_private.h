#ifndef MAGNUM_BOARD_PRIVATE_H
#define MAGNUM_BOARD_PRIVATE_H

#include "fbe/fbe_port.h"
#include "fbe_base_board.h"
#include "base_board_private.h"
#include "fbe_cpd_shim.h"


enum fbe_magnum_board_number_of_slots_e{
    FBE_MAGNUM_BOARD_NUMBER_OF_IO_PORTS = FBE_CPD_SHIM_MAX_PORTS
};

enum fbe_magnum_board_first_slot_index_e{
    FBE_MAGNUM_BOARD_FIRST_IO_PORT_INDEX = 0
};

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(magnum_board);

typedef struct fbe_magnum_board_io_port_s {
    fbe_u32_t io_port_type;
    fbe_u32_t io_portal_number;
    fbe_u32_t io_port_number;
    fbe_u32_t be_port_number;
} fbe_magnum_board_io_port_t;

typedef struct fbe_magnum_board_s {
    fbe_base_board_t base_board;
    fbe_bool_t       rescan_required;
    fbe_magnum_board_io_port_t io_port_array[FBE_MAGNUM_BOARD_NUMBER_OF_IO_PORTS];
    FBE_LIFECYCLE_DEF_INST_DATA;
} fbe_magnum_board_t;

fbe_status_t fbe_magnum_board_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_magnum_board_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_magnum_board_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_magnum_board_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_magnum_board_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_magnum_board_monitor_load_verify(void);

fbe_status_t fbe_magnum_board_init(fbe_magnum_board_t * magnum_board);

fbe_status_t fbe_magnum_board_update_hardware_ports(fbe_magnum_board_t * magnum_board, fbe_bool_t *rescan_required);

#endif /* MAGNUM_BOARD_PRIVATE_H */
