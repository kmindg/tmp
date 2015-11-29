#ifndef BASE_DISCOVERED_PRIVATE_H
#define BASE_DISCOVERED_PRIVATE_H

#include "fbe_base.h"
#include "fbe_trace.h"

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_object.h"
#include "fbe_base_object.h"

#include "base_object_private.h"
#include "fbe_discovery_transport.h"

/* These are the lifecycle condition ids for a base discovered object. */
typedef enum fbe_base_discovered_lifecycle_cond_id_e {
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_INVALID = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BASE_DISCOVERED),
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_NOT_READY, /* BYPASS, POWER OFF and path state handling */
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_PROTOCOL_EDGES_NOT_READY, /* virtual place holder for leaf classes */

    FBE_BASE_DISCOVERED_LIFECYCLE_COND_GET_PORT_OBJECT_ID,    

    FBE_BASE_DISCOVERED_LIFECYCLE_COND_CHECK_QUEUED_IO_ACTIVATE_TIMER,
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_ATTACH_PROTOCOL_EDGE,
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_OPEN_PROTOCOL_EDGE,
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_DETACH_PROTOCOL_EDGE,

    FBE_BASE_DISCOVERED_LIFECYCLE_COND_DETACH_DISCOVERY_EDGE, /* destroy state condition */
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_INVALID_TYPE,

    FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD, /* firmware download */
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD_PEER, 
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_ABORT_FW_DOWNLOAD, 
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_SAVE, /* power save */

    FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVATE_TIMER,
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_CYCLE, /* power cycle */

    FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVE_POWER_SAVE,/*<--this is the new one from Titan and forward*/
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_PASSIVE_POWER_SAVE,/*<--this is the new one from Titan and forward*/
    FBE_BASE_DISCOVERED_LIFECYCLE_COND_EXIT_POWER_SAVE,/*<--this is the new one from Titan and forward*/

    FBE_BASE_DISCOVERED_LIFECYCLE_COND_LAST /* must be last */
} fbe_base_discovered_lifecycle_cond_id_t;

typedef struct fbe_base_discovered_s {
    fbe_base_object_t       base_object;
    fbe_discovery_edge_t    discovery_edge;
    fbe_object_id_t         port_object_id;

    fbe_object_death_reason_t   death_reason;

    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BASE_DISCOVERED_LIFECYCLE_COND_LAST));
}fbe_base_discovered_t;

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_discovered);

/* Methods */
fbe_status_t fbe_base_discovered_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_discovered_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_base_discovered_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_discovered_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_base_discovered_monitor_load_verify(void);

/* fbe_status_t fbe_base_discovered_send_packet(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet); */
fbe_status_t fbe_base_discovered_send_control_packet(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);
fbe_status_t fbe_base_discovered_send_functional_packet(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);

/* Executer methods */
fbe_status_t fbe_base_discovered_send_get_port_object_id_command(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);
fbe_status_t fbe_base_discovered_send_handle_edge_attr_command(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);
fbe_status_t fbe_base_discovered_discovery_transport_entry(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);

/* Basic functionality */
/* Sends remove discovery edge command to the server */
fbe_status_t fbe_base_discovered_detach_edge(fbe_base_discovered_t * base_discovered, fbe_packet_t * packet);

fbe_status_t fbe_base_discovered_get_port_object_id(fbe_base_discovered_t * base_discovered, fbe_object_id_t * port_object_id);
fbe_status_t fbe_base_discovered_get_path_attributes(fbe_base_discovered_t * base_discovered, fbe_path_attr_t * path_attr);
fbe_status_t fbe_base_discovered_get_path_state(fbe_base_discovered_t * base_discovered, fbe_path_state_t * path_state);
fbe_status_t fbe_base_discovered_set_death_reason (fbe_base_discovered_t *base_discovered, fbe_object_death_reason_t death_reason);
fbe_status_t fbe_base_discovered_get_death_reason (fbe_base_discovered_t *base_discovered, fbe_object_death_reason_t *death_reason);

#endif /* BASE_DISCOVERED_PRIVATE_H */
