#ifndef FBE_SHIM_GENERIC_INTERFCAE_H
#define FBE_SHIM_GENERIC_INTERFCAE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_common.h"


#define FBE_NTSTATUS_PENDING                   ((DWORD   )0x00000103L)  
#define FBE_NTSTATUS_FAILURE                   ((DWORD   )0xFFFFFFFFL)  
#define FBE_NTSTATUS_SUCCESS                   ((DWORD   )0x00000000L) 

typedef struct raid_object_state_change_message_s{
	struct raid_object_state_change_message_s *	next_data;
	update_object_msg_t							state_change_message;
}raid_object_state_change_message_t;

fbe_status_t fbe_shim_flare_interface_init (void);
fbe_status_t fbe_shim_flare_interface_destroy (void);
fbe_status_t fbe_shim_register_raid_io_complete_callback(commmand_response_callback_function_t rg_io_callback_function);
INT_32 fbe_shim_change_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t new_state);
INT_32 fbe_shim_get_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t *state);
fbe_status_t fbe_shim_flare_interface_get_board_object_id (fbe_object_id_t *object_id);
INT_32 fbe_shim_flare_interface_get_object_type (fbe_object_id_t object_id, fbe_topology_object_type_t *object_type);
fbe_status_t fbe_shim_flare_interface_register_for_event (fbe_notification_type_t state_mask, commmand_response_callback_function_t callback_func);
fbe_status_t fbe_shim_flare_interface_unregister_from_events (commmand_response_callback_function_t callback_func);

#endif /*FBE_SHIM_GENERIC_INTERFCAE_H*/
