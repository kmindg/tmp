#ifndef FBE_TOPOLOGY_DISCOVERY_H
#define FBE_TOPOLOGY_DISCOVERY_H

#error "This file is deprecated"

#include "fbe/fbe_types.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_board_types.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_physical_drive.h"

#define MAX_DISCOVERY_PORTS			8
#define MAX_DISCOVERY_ENCLOSURES		8
#define MAX_DISCOVERY_ENCLOSURE_SLOTS		15

typedef struct topology_discovery_base_info_s{
	fbe_object_id_t				object_id;
	fbe_lifecycle_state_t			lifecycle_state;
}topology_discovery_base_info_t;

typedef struct topology_discovery_drive_info_s{
	topology_discovery_base_info_t	base_info;
	fbe_drive_type_t				drive_type;
	union {
		fbe_terminator_sas_drive_info_t		sas_drive;
	}drive_data;/*protocol specific drive info*/

	topology_discovery_base_info_t	logical_drive_info;
	/*TODO - add here generic drive info(not defined yet), similar to  fbe_port_info_t*/
}topology_discovery_drive_info_t;

typedef struct topology_discovery_enclosure_info_s{
	topology_discovery_base_info_t			base_info;
	fbe_enclosure_type_t					enclosure_type;
	union {
		fbe_terminator_sas_encl_info_t	sas_encl;
	}enclosure_data;/*protocol specific encl info*/
	/*TODO - add here generic enclosure info(not defined yet), similar to  fbe_port_info_t*/
	topology_discovery_drive_info_t			drive_info[MAX_DISCOVERY_ENCLOSURE_SLOTS];
}topology_discovery_enclosure_info_t;

typedef struct topology_discovery_port_info_s{
	topology_discovery_base_info_t			base_info;
	fbe_port_type_t							port_type;
	union {
		fbe_terminator_sas_port_info_t	sas_port;
	}port_data;/*protocol specific port info*/
	fbe_port_info_t							port_info;/*generic port info*/
	topology_discovery_enclosure_info_t		enclosure_info[MAX_DISCOVERY_ENCLOSURES];
}topology_discovery_port_info_t;

typedef struct topology_discovery_map_s{
	fbe_object_id_t					board_object_id;
	fbe_terminator_board_info_t		board_data;
	topology_discovery_port_info_t	port_info[MAX_DISCOVERY_PORTS];
	fbe_u32_t						total_objects;
}topology_discovery_map_t;

fbe_status_t fbe_topology_discovery_init(void);
fbe_status_t fbe_topology_discovery_clean_map(topology_discovery_map_t	*user_map);
fbe_status_t fbe_topology_discovery_destroy(void);
fbe_status_t fbe_topology_discovery_fill_map(topology_discovery_map_t *user_map);
fbe_status_t fbe_topology_discovery_port_exists (fbe_port_number_t port_number, topology_discovery_map_t *user_map, fbe_bool_t *exist);
fbe_status_t fbe_topology_discovery_enclosure_exists (fbe_port_number_t port_number, fbe_enclosure_number_t enclosure_number, topology_discovery_map_t *user_map, fbe_bool_t *exist);
fbe_status_t fbe_topology_discovery_physical_drive_exists (fbe_port_number_t port_number, fbe_enclosure_number_t enclosure_number, fbe_u32_t slot_number,  topology_discovery_map_t *user_map, fbe_bool_t *exist);
fbe_status_t fbe_topology_discovery_logical_drive_exists (fbe_port_number_t port_number, fbe_enclosure_number_t enclosure_number, fbe_u32_t slot_number,  topology_discovery_map_t *user_map, fbe_bool_t *exist);
fbe_status_t fbe_topology_discovery_get_port_lifecycle_state (fbe_port_number_t port_number, topology_discovery_map_t *user_map, fbe_lifecycle_state_t *lifecycle_state);
fbe_status_t fbe_topology_discovery_get_enclosure_lifecycle_state (fbe_port_number_t port_number, fbe_enclosure_number_t enclosure_number, topology_discovery_map_t *user_map, fbe_lifecycle_state_t *lifecycle_state);
fbe_status_t fbe_topology_discovery_get_physical_drive_lifecycle_state (fbe_port_number_t port_number, fbe_enclosure_number_t enclosure_number, fbe_u32_t slot_number,  topology_discovery_map_t *user_map, fbe_lifecycle_state_t *lifecycle_state);
fbe_status_t fbe_topology_discovery_get_logical_drive_lifecycle_state (fbe_port_number_t port_number, fbe_enclosure_number_t enclosure_number, fbe_u32_t slot_number,  topology_discovery_map_t *user_map, fbe_lifecycle_state_t *lifecycle_state);
fbe_status_t fbe_topology_discovery_get_logical_drive_lifecycle_state (fbe_port_number_t port_number, fbe_enclosure_number_t enclosure_number, fbe_u32_t slot_number,  topology_discovery_map_t *user_map, fbe_lifecycle_state_t *lifecycle_state);
fbe_status_t fbe_topology_discovery_get_logical_drive_object_id (fbe_port_number_t port_number, fbe_enclosure_number_t enclosure_number, fbe_u32_t slot_number,  topology_discovery_map_t *user_map, fbe_object_id_t *object_id_p);


#endif /* FBE_TOPOLOGY_DISCOVERY_H */

