#ifndef FBE_BASE_BOARD_H
#define FBE_BASE_BOARD_H

#include "fbe/fbe_types.h"
#include "fbe_base_discovering.h"
#include "fbe_base_object.h"
#include "fbe/fbe_board_types.h"
#include "fbe/fbe_terminator_api.h"
#include "spid_types.h"
#include "specl_types.h"

enum fbe_base_board_invalid_be_port_e{
	FBE_BASE_BOARD_BE_PORT_INVALID = 0xFFFFFFFF
};

enum fbe_base_board_invalid_io_port_e{
	FBE_BASE_BOARD_IO_PORT_INVALID = 0xFFFFFFFF
};

enum fbe_base_board_invalid_io_portal_e{
	FBE_BASE_BOARD_IO_PORTAL_INVALID = 0xFFFFFFFF
};

fbe_status_t fbe_base_board_set_mgmt_attributes(fbe_object_mgmt_attributes_t mgmt_attributes);
fbe_status_t fbe_base_board_get_mgmt_attributes(fbe_object_mgmt_attributes_t * mgmt_attributes);

fbe_status_t fbe_base_board_sim_set_terminator_api_get_board_info_function(fbe_terminator_api_get_board_info_function_t function);
fbe_status_t fbe_base_board_sim_set_terminator_api_get_sp_id_function(fbe_terminator_api_get_sp_id_function_t function);
fbe_status_t fbe_base_board_sim_set_terminator_api_is_single_sp_system_function(fbe_terminator_api_is_single_sp_system_function_t function);
fbe_status_t fbe_base_board_sim_set_terminator_api_process_specl_sfi_mask_data_queue_function(fbe_terminator_api_process_specl_sfi_mask_data_queue_function_t function);

fbe_status_t fbe_base_board_pe_xlate_smb_status(fbe_u32_t smbusStatus, 
                                                fbe_resume_prom_status_t *resumePromStatus,
                                                fbe_bool_t retries_xhausted);

char * fbe_base_board_decode_device_type(fbe_u64_t deviceType);

#endif /* FBE_BASE_BOARD_H */
