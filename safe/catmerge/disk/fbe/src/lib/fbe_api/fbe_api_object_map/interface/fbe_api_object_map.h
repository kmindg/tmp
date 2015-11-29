#ifndef fbe_api_OBJECT_MAP_H
#define fbe_api_OBJECT_MAP_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_object_map.h
 ***************************************************************************
 *
 *  Description
 *
 *      APIs for getting information on the object id's of devices
 *
 *  History:
 *
 *      06/22/07 sharel    Created
 *      
 ***************************************************************************/
 
#include "fbe/fbe_types.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_object_map_interface.h"

fbe_status_t fbe_api_object_map_init (void);
fbe_status_t fbe_api_object_map_destroy (void);
fbe_status_t fbe_api_object_map_get_port_obj_id(fbe_u32_t port_num,fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_get_enclosure_obj_id(fbe_u32_t port_num, fbe_u32_t encl_num, fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_get_component_obj_id(fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t component_num, fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_get_logical_drive_obj_id(fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t disk_num, fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_get_physical_drive_obj_id(fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t disk_num, fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_add_board_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state);
fbe_status_t fbe_api_object_map_add_port_object (fbe_object_id_t object_id, fbe_port_number_t port_number, fbe_lifecycle_state_t lifecycle_state);

fbe_status_t fbe_api_object_map_add_enclosure_object (fbe_object_id_t object_id, fbe_port_number_t port_number, fbe_enclosure_number_t lcc_number, fbe_lifecycle_state_t lifecycle_state);
fbe_status_t fbe_api_object_map_add_lcc_object (fbe_object_id_t new_object_id, fbe_port_number_t port_number, fbe_enclosure_number_t encl_number, fbe_component_id_t component_id, fbe_lifecycle_state_t lifecycle_state);

fbe_status_t fbe_api_object_map_add_physical_drive_object (fbe_object_id_t object_id,
                                                   fbe_port_number_t port_number,
                                                   fbe_enclosure_number_t lcc_number,
                                                   fbe_enclosure_slot_number_t disk_number,
                                                   fbe_lifecycle_state_t lifecycle_state);

fbe_status_t fbe_api_object_map_add_logical_drive_object (fbe_object_id_t new_object_id,
                                                           fbe_port_number_t port_number,
                                                           fbe_enclosure_number_t lcc_number,
                                                           fbe_enclosure_slot_number_t disk_number,
                                                           fbe_lifecycle_state_t lifecycle_state);

fbe_bool_t fbe_api_object_map_object_id_exists (fbe_object_id_t object_id);
fbe_status_t fbe_api_object_map_reset_all_tables (void);
fbe_status_t fbe_api_object_map_get_object_port_index (fbe_object_id_t object_id, fbe_s32_t *index);
fbe_status_t fbe_api_object_map_get_object_encl_index (fbe_object_id_t object_id,  fbe_s32_t *index);
fbe_status_t  fbe_api_object_map_get_object_drive_index (fbe_object_id_t object_id,  fbe_s32_t *index);
fbe_status_t  fbe_api_object_map_get_object_encl_address (fbe_object_id_t object_id,  fbe_s32_t *index);
fbe_status_t object_map_get_object_lifecycle_state (fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state);
fbe_status_t object_map_increment_queued_io_count (fbe_object_id_t object_id);
fbe_status_t object_map_decrement_queued_io_count (fbe_object_id_t object_id);
fbe_status_t object_map_drive_has_queued_io(fbe_object_id_t object_id, fbe_bool_t *queued_io);
fbe_status_t object_map_set_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state);
fbe_status_t fbe_api_object_map_get_board_obj_id(fbe_object_id_t *obj_id_ptr);
fbe_status_t object_map_get_object_type (fbe_object_id_t object_id, fbe_topology_object_type_t *object_type);
fbe_bool_t  fbe_api_object_map_object_discovered(fbe_object_id_t object_id);
fbe_status_t fbe_api_object_map_get_total_logical_drives (fbe_u32_t port_number, fbe_u32_t *total_drives);
fbe_api_object_map_handle fbe_api_object_map_get_object_handle (fbe_object_id_t object_id);
fbe_status_t fbe_api_object_map_get_state_by_handle (fbe_api_object_map_handle object_handle, fbe_lifecycle_state_t *state);
fbe_status_t fbe_api_object_map_get_logical_drive_object_handle (fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t disk_num, fbe_api_object_map_handle *handle);
fbe_status_t fbe_api_object_map_get_id_by_handle (fbe_api_object_map_handle object_handle, fbe_object_id_t *object_id);
fbe_status_t fbe_api_object_map_get_obj_info_by_handle (fbe_api_object_map_handle object_handle, fbe_api_object_map_obj_info_t *object_info);
fbe_status_t fbe_api_object_map_set_proactive_sparing_recommanded(fbe_object_id_t object_id, fbe_bool_t recommanded);
fbe_status_t fbe_api_object_map_is_proactive_sparing_recommanded(fbe_object_id_t object_id, fbe_bool_t *recommanded);

fbe_status_t fbe_api_object_map_set_encl_addr(fbe_u32_t port, fbe_u32_t encl_pos, fbe_u32_t encl_addr);
fbe_status_t fbe_api_object_map_get_encl_addr(fbe_u32_t port, fbe_u32_t encl_pos, fbe_u32_t *encl_addr);
fbe_status_t fbe_api_object_map_set_encl_physical_pos(fbe_u32_t port, fbe_u32_t encl_pos, fbe_u32_t encl_addr);
fbe_status_t fbe_api_object_map_get_encl_physical_pos(fbe_u32_t *port, fbe_u32_t *encl_pos, fbe_u32_t encl_addr);

#endif /* fbe_api_OBJECT_MAP_H */

