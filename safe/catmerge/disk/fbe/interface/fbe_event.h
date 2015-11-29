#ifndef FBE_EVENT_H
#define FBE_EVENT_H

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_event_interface.h"
#include "fbe_medic.h"
#include "fbe/fbe_xor_api.h"  // for fbe_raid_verify_error_counts_t

enum fbe_event_flags_e {
    FBE_EVENT_FLAG_DENY         = 0x00000001,
    FBE_EVENT_FLAG_USER_DATA    = 0x00000002
};

enum fbe_event_stack_size_e {
    FBE_EVENT_STACK_SIZE = 8
};

typedef enum fbe_event_status_e{
    FBE_EVENT_STATUS_INVALID,
    FBE_EVENT_STATUS_OK,
    FBE_EVENT_STATUS_NO_USER_DATA,
    FBE_EVENT_STATUS_GENERIC_FAILURE,
    FBE_EVENT_STATUS_INVALID_EVENT,
    FBE_EVENT_STATUS_BUSY
}fbe_event_status_t;

struct fbe_event_s;

typedef fbe_s32_t fbe_event_level_t;

typedef void * fbe_event_completion_context_t;
typedef fbe_status_t (* fbe_event_completion_function_t) (struct fbe_event_s * event, fbe_event_completion_context_t context);

typedef struct fbe_event_stack_s
{
    fbe_event_completion_function_t completion_function;
    fbe_event_completion_context_t  completion_context;
    fbe_lba_t          current_offset;   /* block transport will use this value to choose next edge */
    fbe_lba_t          previous_offset;
    fbe_lba_t          lba;
    fbe_block_count_t  block_count;
    /* priority */
}fbe_event_stack_t;

typedef struct fbe_event_permit_request_s
{
    fbe_permit_event_type_t                 permit_event_type;
    fbe_object_id_t                         object_id; 
    fbe_lun_index_t                         object_index; /*!< Given by upper levels like MCC. */ 
    fbe_bool_t                              is_start_b;             
    fbe_bool_t                              is_end_b;              
    fbe_bool_t                              is_beyond_capacity_b; /* If event spans into paged. */  
    fbe_lba_t                               top_lba;    /*!< Top level object's lba. */
    /*! @note The following fields are set to indicate to the originator that 
     *        the start of the request was consumed but the end of the
     *        request was not.  The `unconsumed_block_count' indicates how
     *        many blocks the originator should reduce the request by.
     */
    fbe_block_count_t                       unconsumed_block_count; /*!< Start of request consumed. This many end blocks not consumed. */

} fbe_event_permit_request_t;

typedef struct fbe_event_data_request_s
{
   fbe_data_event_type_t    data_event_type;
   // add other fields specific to data request
}fbe_event_data_request_t;

typedef struct fbe_event_verify_report_s
{
    fbe_bool_t                      pass_completed_b;   // TRUE if pass completed
    fbe_raid_verify_error_counts_t  error_counts;       // The counts from the current pass
    fbe_u16_t                       data_disks;         // Needed for RAID10 verify report

}fbe_event_verify_report_t;

typedef struct fbe_event_drive_location_s
{
    fbe_u16_t                       slot_number;
    fbe_u8_t                        bus_number;
    fbe_u8_t                        enclosure_number;
}fbe_event_drive_location_t;

typedef enum fbe_event_drive_location_value_e 
{
    FBE_EVENT_DRIVE_LOCATION_MAX_SLOT_VALUE = 0xffff,
    FBE_EVENT_DRIVE_LOCATION_MAX_PORT_VALUE = 0x00ff,
    FBE_EVENT_DRIVE_LOCATION_MAX_ENCL_VALUE = 0x00ff,

}fbe_event_drive_location_value_t;

typedef struct fbe_event_event_log_request_s
{
    fbe_event_log_event_type_t      event_log_type;             // type of the event we need to log
    fbe_raid_verify_flags_t         verify_flags;               // verify flags
    fbe_object_id_t                 source_pvd_object_id;       // object id of source PVD
    fbe_event_drive_location_t      source_pvd_location;        // Source pvd location
    fbe_object_id_t                 destination_pvd_object_id;  // object id of destination PVD 
    fbe_event_drive_location_t      destination_pvd_location;   // Destination pvd location
    fbe_u32_t                       adjusted_position;          // disk position adjusted for RAID-10 if needed
}fbe_event_event_log_request_t;

typedef union fbe_event_data_u
{
    
    fbe_event_permit_request_t        permit_request;
    fbe_event_data_request_t          data_request;
    fbe_event_verify_report_t         verify_report;
    fbe_event_event_log_request_t     event_log_request;

}fbe_event_data_t;

typedef enum fbe_event_state_e {
    FBE_EVENT_STATE_INVALID,
    FBE_EVENT_STATE_IN_PROGRESS,
    FBE_EVENT_STATE_QUEUED,
    FBE_EVENT_STATE_CANCELED,

    FBE_EVENT_STATE_LAST
} fbe_event_state_t;

typedef struct fbe_event_s {
    fbe_u64_t                           magic_number; /* Must be first */

    /* The events will arrive to the event entry.
        The object needs to know from what edge event arrived.
    */
    fbe_base_edge_t                 * base_edge;
    
    fbe_queue_element_t                 queue_element; /* used to queue the event */
    fbe_queue_context_t                 queue_context; /* Sometimes events queued outside of the object scope
                                                       metadata service is a good example. In this case we need 
                                                       to keep context inside the event*/

    fbe_event_status_t                  event_status;
    fbe_atomic_t                        event_state; /* fbe_event_state_t */

    fbe_packet_t                        * master_packet;

    fbe_event_type_t                    event_type;
    fbe_u32_t                           event_flags;
    fbe_medic_action_priority_t         medic_action_priority;
	fbe_bool_t                          event_in_use;

    struct fbe_block_transport_server_s     * block_transport_server;

    fbe_event_data_t                    event_data;   /* This is the data that is sent
                                                        It can be data event, permit event
                                                        or verify report event */               

    /* Completion stack area */
    fbe_event_level_t                   current_level;
    fbe_event_stack_t                   stack[FBE_EVENT_STACK_SIZE];

} fbe_event_t;


fbe_event_t * fbe_event_allocate(void);
fbe_status_t fbe_event_release(fbe_event_t * event);

fbe_status_t fbe_event_init(fbe_event_t * event);
fbe_status_t fbe_event_destroy(fbe_event_t * event);

fbe_event_state_t fbe_event_exchange_state(fbe_event_t * event, fbe_event_state_t new_state);
fbe_status_t fbe_event_set_status(fbe_event_t * event, fbe_event_status_t event_status);
fbe_status_t fbe_event_get_status(fbe_event_t * event, fbe_event_status_t * event_status);
fbe_event_t * fbe_event_queue_element_to_event(fbe_queue_element_t * queue_element);


fbe_status_t fbe_event_set_medic_action_priority(fbe_event_t * event, fbe_medic_action_priority_t medic_action_priority);
fbe_status_t fbe_event_get_medic_action_priority(fbe_event_t * event, fbe_medic_action_priority_t * medic_action_priority);

fbe_status_t fbe_event_complete(fbe_event_t * event);
fbe_status_t fbe_event_set_master_packet(fbe_event_t * event, fbe_packet_t * master_packet);
fbe_status_t fbe_event_get_master_packet(fbe_event_t * event, fbe_packet_t ** master_packet);

fbe_status_t fbe_event_get_type(fbe_event_t * event, fbe_event_type_t * event_type);
fbe_status_t fbe_event_set_type(fbe_event_t * event, fbe_event_type_t event_type);
fbe_status_t fbe_event_get_edge(fbe_event_t * event, fbe_base_edge_t ** base_edge);
fbe_status_t fbe_event_set_edge(fbe_event_t * event, fbe_base_edge_t * base_edge);

fbe_status_t fbe_event_get_flags(fbe_event_t * event, fbe_u32_t * event_flags);
fbe_status_t fbe_event_set_flags(fbe_event_t * event, fbe_u32_t  event_flags);

fbe_status_t fbe_event_enqueue_event(fbe_event_t * event, fbe_queue_head_t * queue_head);
fbe_event_t * fbe_event_dequeue_event(fbe_queue_head_t * queue_head);
fbe_status_t fbe_event_remove_event_from_queue(fbe_event_t * event, fbe_queue_head_t * queue_head);

fbe_queue_element_t * fbe_event_get_queue_element(fbe_event_t * event);

void fbe_event_trace(fbe_trace_level_t trace_level,
                         fbe_u32_t message_id,
                         const fbe_char_t * fmt, ...)
                         __attribute__((format(__printf_func__,3,4)));


/* Event stack related functions */
fbe_event_stack_t * fbe_event_allocate_stack(fbe_event_t * event);
fbe_status_t fbe_event_release_stack(fbe_event_t * event, fbe_event_stack_t *event_stack);
fbe_event_stack_t * fbe_event_get_current_stack(fbe_event_t * event);
fbe_status_t fbe_event_stack_set_completion_function(fbe_event_stack_t * event_stack,
                                                    fbe_event_completion_function_t completion_function,
                                                    fbe_event_completion_context_t  completion_context);

fbe_status_t fbe_event_stack_set_extent(fbe_event_stack_t * event_stack, fbe_lba_t lba, fbe_block_count_t block_count);
fbe_status_t fbe_event_init_permit_request_data(fbe_event_t * event, fbe_event_permit_request_t *permit_request_p,
                                                fbe_permit_event_type_t permit_event_type);
fbe_status_t fbe_event_set_permit_request_data(fbe_event_t * event, fbe_event_permit_request_t * permit_request_p);
fbe_status_t fbe_event_set_data_request_data(fbe_event_t * event, fbe_event_data_request_t * data_request_p);
fbe_status_t fbe_event_set_verify_report_data(fbe_event_t * event, fbe_event_verify_report_t* verify_report_p);

fbe_status_t fbe_event_get_permit_request_data(fbe_event_t * event, fbe_event_permit_request_t * permit_request_p);
fbe_status_t fbe_event_get_data_request_data(fbe_event_t * event, fbe_event_data_request_t* data_request_p);
fbe_status_t fbe_event_get_verify_report_data(fbe_event_t * event, fbe_event_verify_report_t* verify_report_p);

fbe_status_t fbe_event_stack_get_extent(fbe_event_stack_t * event_stack, fbe_lba_t * lba, fbe_block_count_t * block_count);

fbe_status_t fbe_event_set_block_transport_server(fbe_event_t * event, struct fbe_block_transport_server_s * block_transport_server);
fbe_status_t fbe_event_get_block_transport_server(fbe_event_t * event, struct fbe_block_transport_server_s ** block_transport_server);

fbe_status_t fbe_event_stack_set_current_offset(fbe_event_stack_t * event_stack, fbe_lba_t current_offset);
fbe_status_t fbe_event_stack_get_current_offset(fbe_event_stack_t * event_stack, fbe_lba_t * current_offset);


fbe_status_t fbe_event_stack_set_previous_offset(fbe_event_stack_t * event_stack, fbe_lba_t previous_offset);
fbe_status_t fbe_event_stack_get_previous_offset(fbe_event_stack_t * event_stack, fbe_lba_t * previous_offset);
fbe_status_t fbe_event_service_get_block_transport_event_count(fbe_u32_t *event_count,
                                                                                        struct  fbe_block_transport_server_s *block_transport_server);
fbe_status_t fbe_event_service_client_is_event_in_fly( fbe_object_id_t object_id, bool *is_event_in_fly);
void fbe_event_service_in_fly_event_save_object_id(fbe_object_id_t object_id);
void fbe_event_service_in_fly_event_clear_object_id(void);
fbe_status_t fbe_event_get_event_log_request_data(fbe_event_t * event, fbe_event_event_log_request_t* event_log_p);
fbe_status_t fbe_event_log_set_source_data(fbe_event_event_log_request_t *event_log_p, 
                                           fbe_u32_t source_bus, fbe_u32_t source_enclosure, fbe_u32_t source_slot);
fbe_status_t fbe_event_log_set_destination_data(fbe_event_event_log_request_t *event_log_p, 
                                                fbe_u32_t destination_bus, fbe_u32_t destination_enclosure, fbe_u32_t destination_slot);
fbe_status_t fbe_event_set_event_log_request_data(fbe_event_t * event, fbe_event_event_log_request_t* event_log_p);


#endif /* FBE_EVENT_H */
