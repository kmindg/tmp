#ifndef FBE_API_OBJECT_MAP_INTERFACE_H
#define FBE_API_OBJECT_MAP_INTERFACE_H


// #error "This file should not be used anymore: use fbe_api_XXX_get_xxx_by_location"
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_object_map_interface.h
 ***************************************************************************
 *
 *  Description
 *
 *      APIs for getting information on the object id's of devices
 *
 *  History:
 *
 *      10/14/07 sharel    Created
 *      
 ***************************************************************************/
 
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_notification_interface.h"


typedef fbe_u64_t fbe_api_object_map_handle;

typedef struct fbe_api_object_map_obj_info_s{
    fbe_object_id_t         object_id;
    fbe_lifecycle_state_t   object_state;
}fbe_api_object_map_obj_info_t;

typedef struct object_map_base_object_s{
    fbe_object_id_t                 object_id;
    fbe_lifecycle_state_t           lifecycle_state;
    //fbe_topology_object_type_t      object_type;
	fbe_u32_t						object_type;
    fbe_bool_t                      discovered;
}object_map_base_object_t;

typedef struct logical_disk_object_map_s{
    object_map_base_object_t        logical_disk_object;/*must be first*/
    fbe_u32_t                       port;
    fbe_u32_t                       encl;
    fbe_u32_t                       encl_addr;
    fbe_u32_t                       slot;
    fbe_bool_t                      near_end_of_life;
}logical_disk_object_map_t;

typedef struct disk_object_map_s{
    object_map_base_object_t    physical_disk_object;/*must be first*/
    logical_disk_object_map_t   logical_disk;
    fbe_u32_t                   port;
    fbe_u32_t                   encl;
    fbe_u32_t                   encl_addr;
    fbe_u32_t                   slot;
}disk_object_map_t;

typedef struct component_object_map_s{
    object_map_base_object_t component_object;/*must be first*/
    fbe_u32_t          port;
    fbe_u32_t          encl;
    fbe_component_id_t component_id;
}component_object_map_t;

typedef struct enclosure_object_map_s{
    object_map_base_object_t enclosure_object;/*must be first*/
    disk_object_map_t disk_fbe_object_list[FBE_API_ENCLOSURE_SLOTS];
    fbe_u32_t         port;
    fbe_u32_t         encl;
    fbe_u32_t         encl_addr;
    fbe_u32_t         expected_component_list_count;
    component_object_map_t component_object_list[FBE_API_MAX_ENCL_COMPONENTS];  //indexed by component_id.
}enclosure_object_map_t;

typedef struct port_obj_map_s{
    object_map_base_object_t port_object;/*must be first*/
    enclosure_object_map_t enclosure_object_list[FBE_API_MAX_ALLOC_ENCL_PER_BUS];
    fbe_u32_t                       port;
}port_object_map_t;

typedef struct board_object_map_s{
    object_map_base_object_t board_object;/*must be first*/
    port_object_map_t port_object_list[FBE_API_PHYSICAL_BUS_COUNT];
}board_object_map_t;

typedef struct encl_physical_position_s
{
    fbe_u32_t   port;
    fbe_u32_t   encl_pos;
} encl_physical_position_t;

fbe_status_t fbe_api_object_map_interface_init (void);
fbe_status_t fbe_api_object_map_interface_destroy (void);
fbe_status_t fbe_api_object_map_interface_get_port_obj_id(fbe_u32_t port_num,fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_interface_get_enclosure_obj_id(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_interface_get_component_obj_id(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t component_num, fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_interface_get_logical_drive_obj_id(fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t disk_num, fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_interface_get_physical_drive_obj_id(fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t disk_num, fbe_object_id_t *obj_id_ptr);
fbe_bool_t fbe_api_object_map_interface_object_exists (fbe_object_id_t object_id);
fbe_status_t fbe_api_object_map_interface_refresh_map (void);
fbe_status_t fbe_api_object_map_interface_register_notification(fbe_notification_type_t state_mask, commmand_response_callback_function_t callback_func);

fbe_status_t fbe_api_object_map_interface_get_object_lifecycle_state (fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state);
fbe_status_t fbe_api_object_map_interface_get_board_obj_id(fbe_object_id_t *obj_id_ptr);
fbe_status_t fbe_api_object_map_interface_get_object_type(fbe_object_id_t object_id, fbe_topology_object_type_t *object_type);
fbe_status_t fbe_api_object_map_interface_get_total_logical_drives(fbe_u32_t port_number, fbe_u32_t *total_drives);
fbe_api_object_map_handle fbe_api_object_map_interface_get_object_handle (fbe_object_id_t object_id);
fbe_status_t fbe_api_object_map_interface_get_logical_drive_object_handle (fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t disk_num, fbe_api_object_map_handle *handle);
fbe_status_t fbe_api_object_map_interface_get_obj_info_by_handle (fbe_api_object_map_handle object_handle, fbe_api_object_map_obj_info_t *object_info);
fbe_status_t fbe_api_object_map_interface_unregister_notification(commmand_response_callback_function_t callback_func);
fbe_status_t fbe_api_object_map_interface_set_proactive_sparing_recommanded(fbe_object_id_t object_id, fbe_bool_t recommanded);
fbe_status_t fbe_api_object_map_interface_is_proactive_sparing_recommanded(fbe_object_id_t object_id, fbe_bool_t *recommanded);
fbe_status_t fbe_api_object_map_interface_set_encl_addr(fbe_u32_t port, fbe_u32_t encl_pos, fbe_u32_t encl_addr);
fbe_status_t fbe_api_object_map_interface_get_encl_addr(fbe_u32_t port, fbe_u32_t encl_pos, fbe_u32_t *encl_addr);
fbe_status_t fbe_api_object_map_interface_set_encl_physical_pos(fbe_u32_t port, fbe_u32_t encl_pos, fbe_u32_t encl_addr);
fbe_status_t fbe_api_object_map_interface_get_encl_physical_pos(fbe_u32_t *port, fbe_u32_t *encl_pos, fbe_u32_t encl_addr);

#endif /* FBE_API_OBJECT_MAP_INTERFACE_H */
