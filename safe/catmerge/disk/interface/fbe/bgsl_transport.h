#ifndef BGSL_TRANSPORT_H
#define BGSL_TRANSPORT_H

#include "csx_ext.h"
#include "fbe/bgsl_winddk.h"
#include "fbe/bgsl_types.h"
#include "fbe/bgsl_memory.h"
#include "fbe/bgsl_queue.h"
#include "fbe/bgsl_atomic.h"
#include "fbe/bgsl_package.h"
#include "fbe/bgsl_class.h"

#include "fbe/bgsl_payload.h"

#include "fbe/bgsl_service.h"
#include "fbe/bgsl_time.h"

/* #include "fbe/bgsl_topology_interface.h" */
#include "fbe/bgsl_trace_interface.h"

enum bgsl_packet_stack_size_e {
    BGSL_PACKET_STACK_SIZE = 10
};

typedef struct bgsl_packet_status_s{
    bgsl_status_t code;
    bgsl_u32_t qualifier;
}bgsl_packet_status_t;

enum bgsl_transport_defaults_e {
    BGSL_TRANSPORT_DEFAULT_LEVEL = -1
};

typedef bgsl_s32_t bgsl_packet_level_t;

typedef void * bgsl_packet_cancel_context_t;
typedef bgsl_status_t (* bgsl_packet_cancel_function_t) (bgsl_packet_cancel_context_t context);

struct bgsl_packet_s;
typedef void * bgsl_packet_completion_context_t;
typedef bgsl_status_t (* bgsl_packet_completion_function_t) (void * packet, bgsl_packet_completion_context_t context);

typedef struct bgsl_packet_stack_s {
    bgsl_packet_completion_function_t completion_function;
    bgsl_packet_completion_context_t  completion_context;
}bgsl_packet_stack_t;

typedef enum bgsl_packet_state_e {
    BGSL_PACKET_STATE_INVALID,
    BGSL_PACKET_STATE_IN_PROGRESS,
    BGSL_PACKET_STATE_QUEUED,
    // State: Cancel by teh caller is registered. It is yet to be processed by the owning object.
    BGSL_PACKET_STATE_CANCEL_PENDING,
    // State: Owning object is in the process of canceling the packet.
    BGSL_PACKET_STATE_CANCEL_BEING_PROCESSED,
    // Packet cancellation is complete by the owning object.
    BGSL_PACKET_STATE_CANCELED,
    BGSL_PACKET_STATE_LAST
} bgsl_packet_state_t;

enum bgsl_packet_flags_e{
    BGSL_PACKET_FLAG_NO_ATTRIB = 0x0,
    BGSL_PACKET_FLAG_TRAVERSE = 0x00000001,
    BGSL_PACKET_FLAG_PASS_THRU = 0x0000002,/* WARNING - TO BE USED BY UPPER DH ONLY !!!!!!!!!!!!*/
};

typedef bgsl_u32_t bgsl_packet_attr_t;

typedef enum bgsl_packet_priority_e {
    BGSL_PACKET_PRIORITY_INVALID,

    BGSL_PACKET_PRIORITY_OPTIONAL,
    BGSL_PACKET_PRIORITY_LOW,
    BGSL_PACKET_PRIORITY_NORMAL,
    BGSL_PACKET_PRIORITY_URGENT,

    BGSL_PACKET_PRIORITY_LAST
}bgsl_packet_priority_t;

typedef void* bgsl_event_context_t;

/* Transport is responsible for event delivery */
typedef enum bgsl_event_type_e {
    BGSL_EVENT_TYPE_INVALID,
    BGSL_EVENT_TYPE_EDGE_STATE_CHANGE,
    BGSL_EVENT_TYPE_LAST
} bgsl_event_type_t;

/* The edge index is a unique edge identifier within the client or server scope */
typedef bgsl_u16_t bgsl_edge_index_t;

#define BGSL_EDGE_INDEX_INVALID 0xFFFF

/* bgsl_path_state_t is common for all edges */
typedef enum bgsl_path_state_e {
    BGSL_PATH_STATE_INVALID, /* not connected to the server object */
    BGSL_PATH_STATE_ENABLED,  /* The server object is ready to serve the client */
    BGSL_PATH_STATE_DISABLED, /* The server object is temporarily not ready to serve the client */
    BGSL_PATH_STATE_BROKEN,
    BGSL_PATH_STATE_SLUMBER,
    BGSL_PATH_STATE_GONE     /* The server object is about to be destroyed */
}bgsl_path_state_t;

typedef enum bgsl_transport_id_e {
    BGSL_TRANSPORT_ID_INVALID,
    BGSL_TRANSPORT_ID_BASE,  /* I am not shure if we realy need it */
    BGSL_TRANSPORT_ID_DISCOVERY,
    BGSL_TRANSPORT_ID_SSP, /*!< SAS SCSI transport */
    BGSL_TRANSPORT_ID_BLOCK,
    BGSL_TRANSPORT_ID_SMP, /*!< SAS SMP transport for SMP port edge between port and enclosure */
    BGSL_TRANSPORT_ID_STP, /*!< SAS SASA transport */

    BGSL_TRANSPORT_ID_LAST
}bgsl_transport_id_t;

enum bgsl_transport_tag_e{
    BGSL_TRANSPORT_TAG_INVALID = 0xFF
};

/* bgsl_path_attr_t - bgsl_path attrubutes is a transport specific set of flags */
typedef bgsl_u32_t bgsl_path_attr_t;

/* Hook function for Edge tap */
/* TODO: probabily should pass in a tap context, so the state of the tap can be preserved and carried to the tap */
typedef bgsl_status_t (* bgsl_edge_hook_function_t) (struct bgsl_packet_s * packet); 

typedef struct bgsl_transport_control_set_edge_tap_hook_s {
    bgsl_edge_hook_function_t edge_tap_hook;
}bgsl_transport_control_set_edge_tap_hook_t;

struct bgsl_base_edge_s;

typedef struct bgsl_base_edge_s{
    struct bgsl_base_edge_s * next_edge;
    bgsl_edge_hook_function_t hook;

    bgsl_object_id_t     client_id;
    bgsl_object_id_t     server_id;

    bgsl_edge_index_t    client_index;
    bgsl_edge_index_t    server_index;

    bgsl_path_state_t    path_state;
    bgsl_path_attr_t     path_attr;
    bgsl_transport_id_t  transport_id;
}bgsl_base_edge_t;



static __forceinline bgsl_status_t
bgsl_base_transport_get_path_state(bgsl_base_edge_t * base_edge, bgsl_path_state_t * path_state)
{
    *path_state =  base_edge->path_state;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_get_path_attributes(bgsl_base_edge_t * base_edge, bgsl_path_attr_t * path_attr)
{
    *path_attr =  base_edge->path_attr;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_set_path_attributes(bgsl_base_edge_t * base_edge, bgsl_path_attr_t  path_attr)
{
    base_edge->path_attr |= path_attr;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_set_server_id(bgsl_base_edge_t * base_edge, bgsl_object_id_t server_id)
{
    base_edge->server_id = server_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_get_server_id(bgsl_base_edge_t * base_edge, bgsl_object_id_t * server_id)
{
    *server_id = base_edge->server_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_set_server_index(bgsl_base_edge_t * base_edge, bgsl_edge_index_t server_index)
{
    base_edge->server_index = server_index;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_get_server_index(bgsl_base_edge_t * base_edge, bgsl_edge_index_t * server_index)
{
    *server_index = base_edge->server_index;
    return BGSL_STATUS_OK;
}


static __forceinline bgsl_status_t
bgsl_base_transport_set_client_id(bgsl_base_edge_t * base_edge, bgsl_object_id_t client_id)
{
    base_edge->client_id = client_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_get_client_id(bgsl_base_edge_t * base_edge, bgsl_object_id_t * client_id)
{
    *client_id = base_edge->client_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_set_client_index(bgsl_base_edge_t * base_edge, bgsl_edge_index_t client_index)
{
    base_edge->client_index = client_index;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_get_client_index(bgsl_base_edge_t * base_edge, bgsl_edge_index_t * client_index)
{
    *client_index = base_edge->client_index;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_set_transport_id(bgsl_base_edge_t * base_edge, bgsl_transport_id_t transport_id)
{
    base_edge->transport_id = transport_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_get_transport_id(bgsl_base_edge_t * base_edge, bgsl_transport_id_t * transport_id)
{
    * transport_id = base_edge->transport_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_set_hook_function(bgsl_base_edge_t * base_edge, bgsl_edge_hook_function_t hook)
{
    base_edge->hook = hook;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_base_transport_get_hook_function(bgsl_base_edge_t * base_edge, bgsl_edge_hook_function_t * hook)
{
    * hook = base_edge->hook;
    return BGSL_STATUS_OK;
}


typedef void * bgsl_queue_context_t;

typedef struct bgsl_packet_s {
    bgsl_u64_t                           magic_number;

    /* The address header */
    bgsl_package_id_t                    package_id; /* should always be valid */
    bgsl_service_id_t                    service_id; 
    bgsl_class_id_t                      class_id;
    bgsl_object_id_t                     object_id;
    /* End of address */

    /* The packets will arrive to the control entry of the usurper or
        to the io_entry of the executor.
        The executor needs to know from what edge packet arrived in order to redirect it 
        to the appropriate bouncer. 
    */
    bgsl_base_edge_t                 * base_edge;
    
    bgsl_memory_request_t                memory_request; /* Used for async memory allocations */
    bgsl_queue_element_t                 queue_element; /* used to queue the packet with object */
    bgsl_queue_context_t                 queue_context; /* Sometimes packets queued outside of the object scope
                                                       transport server is a good example. In this case we need 
                                                       to keep context inside the packet*/

    bgsl_u32_t                           age_threshold; /* This will be used if packet enqueued on age queue */
    bgsl_u8_t                            tag;            /*!< Block transport service will assign unique tag for each I/O if instructed to */
    bgsl_packet_status_t                 packet_status;
    bgsl_atomic_t                        packet_state; /* bgsl_packet_state_t */

    bgsl_packet_attr_t                   packet_attr;
    bgsl_packet_priority_t               packet_priority;

    bgsl_packet_cancel_function_t   packet_cancel_function;
    bgsl_packet_cancel_context_t    packet_cancel_context;

    /* expiration_time is number of milliseconds since the system booted after which the packet should be timed out */
    /* It is strongly recommended to maintain this value properly to avoid "lost" packets */
    /* bgsl_time_t                           expiration_time; */

    /* Subpacket area */
    struct bgsl_packet_s         * master_packet;
    bgsl_queue_element_t                 subpacket_queue_element;
    bgsl_queue_head_t                    subpacket_queue_head;
    bgsl_spinlock_t                      subpacket_queue_lock;

    /* Completion stack area */
    bgsl_packet_level_t                  current_level;
    bgsl_packet_stack_t                  stack[BGSL_PACKET_STACK_SIZE];

    bgsl_payload_t           payload;
} bgsl_packet_t;
/* Typedef the bgsl_packet_t structure to "bgsl_mgmt_packet_t", later we will change caller to use bgsl_packet_t structure
 * DP: Typedef bgsl_packet to bgsl_mgmt_packet, remove this comment before check-in.
 */
typedef bgsl_packet_t bgsl_mgmt_packet_t;

/* Initialization related functions */
bgsl_status_t bgsl_transport_initialize_packet(bgsl_packet_t * packet);
bgsl_status_t bgsl_transport_reuse_packet(bgsl_packet_t * packet);

/* This function does some cleanup, but do NOT release memory */
bgsl_status_t bgsl_transport_destroy_packet(bgsl_packet_t * packet);

static __forceinline bgsl_status_t 
bgsl_transport_set_magic_number(bgsl_packet_t * packet, bgsl_u64_t magic_number)
{
    packet->magic_number = magic_number;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t 
bgsl_transport_set_address(bgsl_packet_t * packet,
                            bgsl_package_id_t package_id,
                            bgsl_service_id_t service_id,
                            bgsl_class_id_t   class_id,
                            bgsl_object_id_t object_id)
{
    packet->package_id = package_id;
    packet->service_id = service_id;
    packet->class_id    = class_id;
    packet->object_id   = object_id;
    return BGSL_STATUS_OK;
}

/* State related functions */
static __forceinline bgsl_packet_state_t 
bgsl_transport_exchange_state(bgsl_packet_t * packet, bgsl_packet_state_t new_state)
{
    return (bgsl_packet_state_t)bgsl_atomic_exchange(&packet->packet_state, new_state);
}

static __forceinline bgsl_packet_state_t
bgsl_transport_get_state(bgsl_packet_t * packet)
{
    return packet->packet_state;
}

static __forceinline void 
bgsl_transport_set_status(bgsl_packet_t * packet, bgsl_status_t code, bgsl_u32_t qualifier)
{
    packet->packet_status.code = code;
    packet->packet_status.qualifier = qualifier;
}

static __forceinline bgsl_status_t 
bgsl_transport_set_cancel_function(  bgsl_packet_t * packet, 
                                    bgsl_packet_cancel_function_t packet_cancel_function,
                                    bgsl_packet_cancel_context_t  packet_cancel_context)
{
    packet->packet_cancel_function = packet_cancel_function;
    packet->packet_cancel_context = packet_cancel_context;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t 
bgsl_transport_cancel_packet(bgsl_packet_t * packet)
{
    bgsl_packet_state_t old_state;

    old_state = bgsl_transport_exchange_state(packet, BGSL_PACKET_STATE_CANCELED);

    switch(old_state) {
    case BGSL_PACKET_STATE_IN_PROGRESS:
        break;
    case BGSL_PACKET_STATE_QUEUED:
        bgsl_transport_set_status(packet, BGSL_STATUS_CANCEL_PENDING, 0);
        if(packet->packet_cancel_function != NULL){
            packet->packet_cancel_function(packet->packet_cancel_context);
        }
        break;
    default:
        KvTrace("%s: Critical error: Invalid packet state %X \n", __FUNCTION__, old_state);
    }

    return BGSL_STATUS_OK;
    
}

static __forceinline bgsl_status_t 
bgsl_transport_get_status_code(bgsl_packet_t * packet)
{
    return packet->packet_status.code;
}

static __forceinline bgsl_u32_t 
bgsl_transport_get_status_qualifier(bgsl_packet_t * packet)
{
    return packet->packet_status.qualifier;
}

static __forceinline bgsl_status_t 
bgsl_transport_copy_packet_status(bgsl_packet_t * src, bgsl_packet_t * dst)
{
    dst->packet_status.code = src->packet_status.code;
    dst->packet_status.qualifier = src->packet_status.qualifier;

    return BGSL_STATUS_OK;
}

/* Address related functions */
static __forceinline bgsl_status_t 
bgsl_transport_get_package_id(bgsl_packet_t * packet, bgsl_package_id_t * package_id)
{
    *package_id = packet->package_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t 
bgsl_transport_get_service_id(bgsl_packet_t * packet, bgsl_service_id_t * service_id)
{
    *service_id = packet->service_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t 
bgsl_transport_get_class_id(bgsl_packet_t * packet, bgsl_class_id_t * class_id)
{
    *class_id = packet->class_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t 
bgsl_transport_get_object_id(bgsl_packet_t * packet, bgsl_object_id_t * object_id)
{
    *object_id = packet->object_id;
    return BGSL_STATUS_OK;
}

/* Queueing related functions */
static __forceinline bgsl_packet_t * 
bgsl_transport_queue_element_to_packet(bgsl_queue_element_t * queue_element)
{
    bgsl_packet_t * packet;
    if (queue_element == NULL)
    {
        return NULL;
    }
    packet = (bgsl_packet_t  *)((bgsl_u8_t *)queue_element - (bgsl_u8_t *)(&((bgsl_packet_t  *)0)->queue_element));
    return packet;
}

static __forceinline bgsl_packet_t * 
bgsl_transport_memory_request_to_packet(bgsl_memory_request_t * memory_request)
{
    bgsl_packet_t * packet;
    packet = (bgsl_packet_t  *)((bgsl_u8_t *)memory_request - (bgsl_u8_t *)(&((bgsl_packet_t  *)0)->memory_request));
    return packet;
}

static __forceinline bgsl_packet_t * 
bgsl_transport_subpacket_queue_element_to_packet(bgsl_queue_element_t * queue_element)
{
    bgsl_packet_t * packet;
    if (queue_element == NULL)
    {
        return NULL;
    }
    packet = (bgsl_packet_t  *)((bgsl_u8_t *)queue_element - (bgsl_u8_t *)(&((bgsl_packet_t  *)0)->subpacket_queue_element));
    return packet;
}

static __forceinline bgsl_queue_element_t * 
bgsl_transport_get_queue_element(bgsl_packet_t * packet)
{
    return &packet->queue_element;
}

/* This function updates expiration_time field with sum of expiration_time_interval and the current time reading */
#if 0
static __forceinline bgsl_status_t 
bgsl_transport_set_expiration_time(bgsl_packet_t * packet, bgsl_time_t expiration_time_interval)
{
    bgsl_time_t t = 0;
    t = bgsl_get_time();
    packet->expiration_time = t + expiration_time_interval;
    return BGSL_STATUS_OK;
}
#endif

bgsl_status_t bgsl_transport_complete_packet(bgsl_packet_t * packet);

static __forceinline bgsl_status_t 
bgsl_transport_add_subpacket(bgsl_packet_t * packet, bgsl_packet_t * base_subpacket)
{
    bgsl_status_t status;

    bgsl_spinlock_lock(&packet->subpacket_queue_lock);
    status  = bgsl_transport_get_status_code(packet);

    if((status == BGSL_STATUS_CANCELED) || (status == BGSL_STATUS_CANCEL_PENDING)){
        bgsl_spinlock_unlock(&packet->subpacket_queue_lock);
        return status;
    } else {
        bgsl_queue_push(&packet->subpacket_queue_head, &base_subpacket->subpacket_queue_element);
        base_subpacket->master_packet = packet;
        bgsl_spinlock_unlock(&packet->subpacket_queue_lock);
        return BGSL_STATUS_OK;
    }
}

static __forceinline bgsl_status_t 
bgsl_transport_remove_subpacket(bgsl_packet_t * base_subpacket)
{
    bgsl_packet_t * packet = base_subpacket->master_packet;
    
    if(packet == NULL){
        KvTrace("bgsl_transport.h: %s Invalid master_packet pointer\n", __FUNCTION__);
        return BGSL_STATUS_GENERIC_FAILURE;
    }

    bgsl_spinlock_lock(&packet->subpacket_queue_lock);
    bgsl_queue_remove(&base_subpacket->subpacket_queue_element);
    bgsl_spinlock_unlock(&packet->subpacket_queue_lock);

    base_subpacket->master_packet = NULL;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_bool_t 
bgsl_transport_is_subpacket_queue_empty(bgsl_packet_t * packet)
{
    /* It is possible that we will have to remove lock from here */
    bgsl_spinlock_lock(&packet->subpacket_queue_lock);
    if(bgsl_queue_is_empty(&packet->subpacket_queue_head)){
        bgsl_spinlock_unlock(&packet->subpacket_queue_lock);
        return 1;
    } else {
        bgsl_spinlock_unlock(&packet->subpacket_queue_lock);
        return 0;
    }
}

static __forceinline bgsl_status_t 
bgsl_transport_cancel_subpackets(bgsl_packet_t * packet)
{   
    bgsl_queue_element_t * queue_element = NULL;
    bgsl_queue_element_t * next_element = NULL;
    bgsl_packet_t * sub_packet = NULL;

    bgsl_spinlock_lock(&packet->subpacket_queue_lock);
    queue_element = bgsl_queue_front(&packet->subpacket_queue_head);
    while((queue_element != NULL) && (queue_element != &packet->subpacket_queue_head)){
        next_element = (bgsl_queue_element_t *)queue_element->next;
        sub_packet = bgsl_transport_subpacket_queue_element_to_packet(queue_element);
        bgsl_transport_cancel_packet(sub_packet);
        queue_element = next_element;
    }

    bgsl_spinlock_unlock(&packet->subpacket_queue_lock);

    return BGSL_STATUS_OK;
}

static __forceinline bgsl_packet_t * 
bgsl_transport_get_master_packet(bgsl_packet_t * packet)
{
    return packet->master_packet;
}

static __forceinline bgsl_status_t 
bgsl_transport_set_completion_function(  bgsl_packet_t * packet,
                                        bgsl_packet_completion_function_t completion_function,
                                        bgsl_packet_completion_context_t  completion_context)
{
    if(packet->current_level >= BGSL_PACKET_STACK_SIZE -1) {
        return BGSL_STATUS_GENERIC_FAILURE;
    }
    packet->current_level++;
    packet->stack[packet->current_level].completion_function = completion_function;
    packet->stack[packet->current_level].completion_context = completion_context;

    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t 
bgsl_transport_unset_completion_function(bgsl_packet_t * packet,
                                        bgsl_packet_completion_function_t completion_function,
                                        bgsl_packet_completion_context_t  completion_context)
{
    if ((packet->stack[packet->current_level].completion_function == completion_function) &&
        (packet->stack[packet->current_level].completion_context == completion_context)) {
        packet->stack[packet->current_level].completion_function = NULL;
        packet->stack[packet->current_level].completion_context = NULL;
        packet->current_level--;
    }
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_memory_request_t * 
bgsl_transport_get_memory_request(bgsl_packet_t * packet)
{
    return &packet->memory_request;
}

static __forceinline bgsl_packet_t *
bgsl_transport_payload_to_packet(bgsl_payload_t * payload)
{
    bgsl_packet_t * packet;
    packet = (bgsl_packet_t  *)((bgsl_u8_t *)payload - (bgsl_u8_t *)(&((bgsl_packet_t  *)0)->payload));
    return packet;
}

bgsl_status_t bgsl_transport_increment_stack_level(bgsl_packet_t * packet);

static __forceinline bgsl_status_t 
bgsl_transport_get_packet_attr(bgsl_packet_t * packet, bgsl_packet_attr_t * packet_attr)
{
    *packet_attr = packet->packet_attr;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t 
bgsl_transport_set_packet_attr(bgsl_packet_t * packet, bgsl_packet_attr_t packet_attr)
{
    packet->packet_attr |= packet_attr;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_payload_t * 
bgsl_transport_get_payload(bgsl_packet_t * packet)
{
    return (&packet->payload);
}

static __forceinline bgsl_status_t
bgsl_transport_get_transport_id(bgsl_packet_t * packet, bgsl_transport_id_t * transport_id)
{
    *transport_id = packet->base_edge->transport_id;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_transport_get_server_index(bgsl_packet_t * packet, bgsl_edge_index_t* server_index)
{
    *server_index = packet->base_edge->server_index;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_base_edge_t *
bgsl_transport_get_edge(bgsl_packet_t * packet)
{
    return packet->base_edge;
}

static __forceinline bgsl_status_t
bgsl_transport_get_packet_priority(bgsl_packet_t * packet, bgsl_packet_priority_t * packet_priority)
{
    * packet_priority = packet->packet_priority;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_transport_set_packet_priority(bgsl_packet_t * packet, bgsl_packet_priority_t packet_priority)
{
    packet->packet_priority = packet_priority;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_transport_get_age_threshold(bgsl_packet_t * packet, bgsl_u32_t * age_threshold)
{
    * age_threshold = packet->age_threshold;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t
bgsl_transport_set_age_threshold(bgsl_packet_t * packet, bgsl_u32_t age_threshold)
{
    packet->age_threshold = age_threshold;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t 
bgsl_transport_set_tag(bgsl_packet_t * packet, bgsl_u8_t tag)
{
    packet->tag = tag;
    return BGSL_STATUS_OK;
}

static __forceinline bgsl_status_t 
bgsl_transport_get_tag(bgsl_packet_t * packet, bgsl_u8_t * tag)
{
    *tag = packet->tag;
    return BGSL_STATUS_OK;
}

bgsl_status_t
bgsl_transport_build_control_packet(bgsl_packet_t * packet, 
                                    bgsl_payload_control_operation_opcode_t      control_code,
                                    bgsl_payload_control_buffer_t            buffer,
                                    bgsl_payload_control_buffer_length_t in_buffer_length,
                                    bgsl_payload_control_buffer_length_t out_buffer_length,
                                    bgsl_packet_completion_function_t        completion_function,
                                    bgsl_packet_completion_context_t         completion_context);

/* This function is DEPRECATED */
bgsl_payload_control_operation_opcode_t bgsl_transport_get_control_code(bgsl_packet_t * packet);

bgsl_status_t bgsl_transport_enqueue_packet(bgsl_packet_t * packet, bgsl_queue_head_t * queue_head);
bgsl_packet_t * bgsl_transport_dequeue_packet(bgsl_queue_head_t * queue_head);
bgsl_status_t bgsl_transport_remove_packet_from_queue(bgsl_packet_t * packet, bgsl_queue_head_t * queue_head);

void 
bgsl_base_transport_trace(bgsl_trace_level_t trace_level,
                         bgsl_u32_t message_id,
                         const char * fmt, ...)
                         __attribute__((format(__printf_func__,3,4)));

#endif /* BGSL_TRANSPORT_H */

