#ifndef FBE_TRANSPORT_H
#define FBE_TRANSPORT_H

#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_class.h"

#include "fbe/fbe_payload_ex.h"

#include "fbe/fbe_service.h"
#include "fbe/fbe_time.h"

/* #include "fbe/fbe_topology_interface.h" */
#include "fbe/fbe_trace_interface.h"
//#include "fbe/fbe_sep_shim.h"

#define FBE_TRANSPORT_RUN_QUEUE_HIST_DEPTH_SIZE 64

enum fbe_packet_stack_size_e {
    FBE_PACKET_STACK_SIZE = 17
};

typedef struct fbe_packet_status_s{
    fbe_status_t code;
    fbe_u32_t qualifier;
}fbe_packet_status_t;

enum fbe_transport_defaults_e {
    FBE_TRANSPORT_DEFAULT_LEVEL = -1
};

typedef fbe_s32_t fbe_packet_level_t;

typedef void * fbe_packet_cancel_context_t;
typedef fbe_status_t (* fbe_packet_cancel_function_t) (fbe_packet_cancel_context_t context);

struct fbe_packet_s;
typedef void * fbe_packet_completion_context_t;
typedef fbe_status_t (* fbe_packet_completion_function_t) (struct fbe_packet_s * packet, fbe_packet_completion_context_t context);

typedef struct fbe_packet_stack_s {
    fbe_packet_completion_function_t completion_function;
    fbe_packet_completion_context_t  completion_context;
}fbe_packet_stack_t;

typedef enum fbe_packet_state_e {
    FBE_PACKET_STATE_INVALID,
    FBE_PACKET_STATE_IN_PROGRESS,
    FBE_PACKET_STATE_QUEUED,
    FBE_PACKET_STATE_CANCELED,

    FBE_PACKET_STATE_LAST
} fbe_packet_state_t;

enum fbe_packet_flags_e{
    FBE_PACKET_FLAG_NO_ATTRIB                           = 0x00000000,
    FBE_PACKET_FLAG_TRAVERSE                            = 0x00000001,
    //FBE_PACKET_FLAG_PASS_THRU                         = 0x00000002,/* WARNING - TO BE USED BY UPPER DH ONLY !!!!!!!!!!!!*/
    FBE_PACKET_FLAG_SYNC                                = 0x00000004, /* RESERVED FOR INFRASTRUCTURE USE ONLY */
    FBE_PACKET_FLAG_BACKGROUND_SNIFF                    = 0x00000008, /*** packet is marked as being sent from a bakground sniff*/
    FBE_PACKET_FLAG_DO_NOT_CANCEL                       = 0x00000010, /*!< This packet is not allowed to be cancelled. */
    FBE_PACKET_FLAG_EXTERNAL                            = 0x00000020, /* The packet was sent from outside */
    FBE_PACKET_FLAG_DESTROY_ENABLED                     = 0x00000040, /* The packet can be sent to object in destroy state */
    FBE_PACKET_FLAG_ENABLE_QUEUED_COMPLETION            = 0x00000080, /* Can be completed when enqueued */
    FBE_PACKET_FLAG_INTERNAL                            = 0x00000100, /* The packet was sent from inside. */
    FBE_PACKET_FLAG_ASYNC                               = 0x00000200, /* The packet was sent async from user space. RESERVED FOR INFRASTRUCTURE USE ONLY */
    FBE_PACKET_FLAG_NP_LOCK_HELD                        = 0x00000400, /* The packet has the NP lock. */
    FBE_PACKET_FLAG_MONITOR_OP                          = 0x00000800, /* The packet was sent from monitor. */
    FBE_PACKET_FLAG_REINIT_PVD                          = 0x00001000 ,/* TODO (BAD!!! design) Signifies that this packet is a reinit pvd packet (BAD!!! design) */
    FBE_PACKET_FLAG_CONSUMED                            = 0x00002000, /* Caller guarantee that this I/O is for already consumed area */
    FBE_PACKET_FLAG_RDGEN                               = 0x00004000, /* RDGEN use only.. just to make sure we release the packet. */
    FBE_PACKET_FLAG_BVD                                 = 0x00008000, /* BVD use only!!! Run Q may change cpu affinity for this packet */
    FBE_PACKET_FLAG_DO_NOT_HOLD                         = 0x00100000, /* IO was generated from monitor context, do not queue this IO */
    FBE_PACKET_FLAG_COMPLETION_SEMAPHORE_INITIALIZED	= 0x01000000, /* completion_semaphore has been initialized */
    FBE_PACKET_FLAG_COMPLETION_BY_PORT                  = 0x02000000, /* For performance analysis this packet will be completed by port object */
    FBE_PACKET_FLAG_COMPLETION_BY_PORT_WITH_ZEROS    	= 0x04000000, /* For perf analysis port completes packet with zeros in buffer */
    FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED      = 0x08000000, /* We use this flag when we want to pass bufferes to user space but don't need then to be physically contiguos*/
    FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK                  = 0x10000000, /*! Do not get a stripe lock. */
    FBE_PACKET_FLAG_DO_NOT_QUIESCE                      = 0x20000000, /*! Do not get quiesced. */
    FBE_PACKET_FLAG_REDIRECTED                          = 0x40000000, /*! This packet is redirected from peer SP. */
    FBE_PACKET_FLAG_SLF_REQUIRED                        = 0x80000000, /*! This packet needs SLF processing. */
};

typedef fbe_u32_t fbe_packet_attr_t;

typedef enum fbe_packet_priority_e {
    FBE_PACKET_PRIORITY_INVALID,

    FBE_PACKET_PRIORITY_LOW,
    FBE_PACKET_PRIORITY_NORMAL,
    FBE_PACKET_PRIORITY_URGENT,

    FBE_PACKET_PRIORITY_LAST,

    /* 
     * FBE_PACKET_PRIORITY_OPTIONAL is unsupported, but defined
     * so that code to handle it can be left in place. It's put
     * after FBE_PACKET_PRIORITY_LAST so that we don't allocate
     * a queue for it in the block transport.
     */
    FBE_PACKET_PRIORITY_OPTIONAL,
}fbe_packet_priority_t;

typedef fbe_u32_t fbe_packet_resource_priority_t;

typedef fbe_u64_t fbe_packet_io_stamp_t;
#define FBE_PACKET_IO_STAMP_INVALID (0x00FFFFFFFFFFFFFF) /* The first byte is core id */
#define FBE_PACKET_IO_STAMP_MASK    (0xFF00000000000000)
#define FBE_PACKET_IO_STAMP_SHIFT   (56) /* 64 -8 */

/* The edge index is a unique edge identifier within the client or server scope */
typedef fbe_u32_t fbe_edge_index_t;

#define FBE_EDGE_INDEX_INVALID 0xFFFF

/* fbe_path_state_t is common for all edges */
typedef enum fbe_path_state_e {
    FBE_PATH_STATE_INVALID, /* not connected to the server object */
    FBE_PATH_STATE_ENABLED,  /* The server object is ready to serve the client */
    FBE_PATH_STATE_DISABLED, /* The server object is temporarily not ready to serve the client */
    FBE_PATH_STATE_BROKEN,
    FBE_PATH_STATE_SLUMBER,
    FBE_PATH_STATE_GONE     /* The server object is about to be destroyed */
}fbe_path_state_t;

typedef enum fbe_transport_id_e {
    FBE_TRANSPORT_ID_INVALID,
    FBE_TRANSPORT_ID_BASE,  /* I am not shure if we realy need it */
    FBE_TRANSPORT_ID_DISCOVERY,
    FBE_TRANSPORT_ID_SSP, /*!< SAS SCSI transport */
    FBE_TRANSPORT_ID_BLOCK,
    FBE_TRANSPORT_ID_SMP, /*!< SAS SMP transport for SMP port edge between port and enclosure */
    FBE_TRANSPORT_ID_STP, /*!< SAS SASA transport */
    FBE_TRANSPORT_ID_DIPLEX, /*!< FC Diplex transport */

    FBE_TRANSPORT_ID_LAST
}fbe_transport_id_t;

enum fbe_transport_tag_e{
    FBE_TRANSPORT_TAG_INVALID = 0xFF
};

/* RESERVED FOR INFRASTRUCTURE USE ONLY */
typedef enum fbe_transport_completion_type_e{
    FBE_TRANSPORT_COMPLETION_TYPE_INVALID,
    FBE_TRANSPORT_COMPLETION_TYPE_DEFAULT,
    FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED
}fbe_transport_completion_type_t;

/*
 * FBE_TRI_STATE is similar to BOOL, but with an INVALID state in addition
 * to TRUE & FALSE.
 */
typedef enum fbe_tri_state_enum
{
    FBE_TRI_STATE_FALSE   = FALSE,
    FBE_TRI_STATE_TRUE    = TRUE,
    FBE_TRI_STATE_INVALID = 0xffffffff
}
FBE_TRI_STATE;

/* fbe_path_attr_t - fbe_path attrubutes is a transport specific set of flags */
typedef fbe_u32_t fbe_path_attr_t;

/* Hook function for Edge tap */
/* TODO: probabily should pass in a tap context, so the state of the tap can be preserved and carried to the tap */
typedef fbe_status_t (* fbe_edge_hook_function_t) (struct fbe_packet_s * packet); 

typedef struct fbe_transport_control_set_edge_tap_hook_s {
    fbe_edge_hook_function_t edge_tap_hook; /*!< Hook function */
    fbe_u32_t                edge_index;    /*!< Specific edge index or FBE_U32_MAX to use bitmask */
    fbe_u32_t                edge_bitmask;  /*!< Bitmask of edges to add/remove hook for. */
}fbe_transport_control_set_edge_tap_hook_t;

typedef struct fbe_transport_control_remove_edge_tap_hook_s {
    fbe_object_id_t         object_id;   /*!< Object id of client to remove edge hook for. */
}fbe_transport_control_remove_edge_tap_hook_t;

struct fbe_base_edge_s;

typedef struct fbe_base_edge_s{
    struct fbe_base_edge_s * next_edge;
    fbe_edge_hook_function_t hook;

    fbe_object_id_t     client_id;
    fbe_object_id_t     server_id;

    fbe_edge_index_t    client_index;
    fbe_edge_index_t    server_index;

    fbe_path_state_t    path_state;
    fbe_path_attr_t     path_attr;
    fbe_transport_id_t  transport_id;
}fbe_base_edge_t;


void 
fbe_base_transport_trace(fbe_trace_level_t trace_level,
                         fbe_u32_t message_id,
                         const fbe_char_t * fmt, ...)
                         __attribute__((format(__printf_func__,3,4)));


fbe_status_t fbe_base_edge_init(fbe_base_edge_t * base_edge);
fbe_status_t fbe_base_edge_reinit(fbe_base_edge_t * base_edge);
fbe_status_t fbe_base_transport_get_path_state(fbe_base_edge_t * base_edge, fbe_path_state_t * path_state);
fbe_status_t fbe_base_transport_get_path_attributes(fbe_base_edge_t * base_edge, fbe_path_attr_t * path_attr);
fbe_status_t fbe_base_transport_set_path_attributes(fbe_base_edge_t * base_edge, fbe_path_attr_t  path_attr);
fbe_status_t fbe_base_transport_clear_path_attributes(fbe_base_edge_t * base_edge, fbe_path_attr_t  path_attr);
fbe_status_t fbe_base_transport_set_server_id(fbe_base_edge_t * base_edge, fbe_object_id_t server_id);
fbe_status_t fbe_base_transport_get_server_id(fbe_base_edge_t * base_edge, fbe_object_id_t * server_id);
fbe_status_t fbe_base_transport_set_server_index(fbe_base_edge_t * base_edge, fbe_edge_index_t server_index);
fbe_status_t fbe_base_transport_get_server_index(fbe_base_edge_t * base_edge, fbe_edge_index_t * server_index);
fbe_status_t fbe_base_transport_set_client_id(fbe_base_edge_t * base_edge, fbe_object_id_t client_id);
fbe_status_t fbe_base_transport_get_client_id(fbe_base_edge_t * base_edge, fbe_object_id_t * client_id);
fbe_status_t fbe_base_transport_set_client_index(fbe_base_edge_t * base_edge, fbe_edge_index_t client_index);
fbe_status_t fbe_base_transport_get_client_index(fbe_base_edge_t * base_edge, fbe_edge_index_t * client_index);
fbe_status_t fbe_base_transport_set_transport_id(fbe_base_edge_t * base_edge, fbe_transport_id_t transport_id);
fbe_status_t fbe_base_transport_get_transport_id(fbe_base_edge_t * base_edge, fbe_transport_id_t * transport_id);
fbe_status_t fbe_base_transport_set_hook_function(fbe_base_edge_t * base_edge, fbe_edge_hook_function_t hook);
fbe_status_t fbe_base_transport_get_hook_function(fbe_base_edge_t * base_edge, fbe_edge_hook_function_t * hook);

/*! @struct fbe_payload_ex_t 
 *  
 *  @brief The fbe payload contains a stack of payloads and allows the user
 *         of a payload to allocate allocate or release a payload for use
 *         at a given time.
 *  
 *         At any moment only one payload is valid, represented by the
 *         "current_operation".  The next operation which will be valid is
 *         identified by the next_operations (or NULL if there is no next
 *         operation).
 */
typedef void * fbe_queue_context_t;

typedef enum fbe_traffic_priority_e{
    FBE_TRAFFIC_PRIORITY_INVALID = 0,
    FBE_TRAFFIC_PRIORITY_VERY_LOW,
    FBE_TRAFFIC_PRIORITY_LOW,
    FBE_TRAFFIC_PRIORITY_NORMAL_LOW,
    FBE_TRAFFIC_PRIORITY_NORMAL,
    FBE_TRAFFIC_PRIORITY_NORMAL_HIGH,
    FBE_TRAFFIC_PRIORITY_HIGH,
    FBE_TRAFFIC_PRIORITY_VERY_HIGH,
    FBE_TRAFFIC_PRIORITY_URGENT,
    FBE_TRAFFIC_PRIORITY_LAST
}fbe_traffic_priority_t;

typedef enum fbe_transport_rq_method_e{
    FBE_TRANSPORT_RQ_METHOD_SAME_CORE,
    FBE_TRANSPORT_RQ_METHOD_NEXT_CORE,
    FBE_TRANSPORT_RQ_METHOD_ROUND_ROBIN,
}fbe_transport_rq_method_t;

typedef struct fbe_transport_timer_s {
    fbe_queue_element_t		queue_element;
    /* expiration_time is number of milliseconds since the system booted */
    fbe_time_t				expiration_time;
}fbe_transport_timer_t;

fbe_status_t fbe_transport_timer_init(void);
fbe_status_t fbe_transport_timer_destroy(void);

typedef fbe_u8_t stack_tracker_action;
#define PACKET_STACK_SET         0x0001
#define PACKET_STACK_COMPLETE    0x0002
#define PACKET_STACK_TIME_DELTA  0x0004
#define PACKET_RUN_QUEUE_PUSH    0x0008
#define PACKET_SL_ABORT          0x0010

#pragma pack(1)
typedef struct fbe_packet_tracker_entry_s {
    fbe_packet_completion_function_t  callback_fn;
    fbe_u32_t                         coarse_time;  /* if TIME_DELTA is set, this is the delta between set and complete */
    stack_tracker_action              action;           
} fbe_packet_tracker_entry_t;
#pragma pack()

#define FBE_PACKET_TRACKER_DEPTH   30

typedef struct fbe_packet_s {
    /* The address header */
    fbe_package_id_t                    package_id; /* should always be valid */
    fbe_service_id_t                    service_id; 
    fbe_class_id_t                      class_id;
    fbe_object_id_t                     object_id;
    /* End of address */

    /* The packets will arrive to the control entry of the usurper or
        to the io_entry of the executor.
        The executor needs to know from what edge packet arrived in order to redirect it 
        to the appropriate bouncer. 
    */
    FBE_ALIGN(8)fbe_base_edge_t * base_edge;

    fbe_packet_resource_priority_t      resource_priority; /* Priority for any resources allocated for this request */
    fbe_packet_resource_priority_t      saved_resource_priority; /* value to use for restoring resource priority when packet is reused */
    fbe_packet_attr_t                   resource_priority_boost; /* Used to determine if priority is already boosted by object */
    fbe_memory_request_t                memory_request; /* Used for async memory allocations */
    fbe_packet_io_stamp_t               io_stamp;      /* user specified per I/O stamp */
    
    /* The order is important!!! Please do not reorder */
    fbe_queue_element_t                 queue_element; /* used to queue the packet with object */
    fbe_u64_t                           magic_number;


    fbe_queue_context_t                 queue_context; /* Sometimes packets queued outside of the object scope
                                                       transport server is a good example. In this case we need 
                                                       to keep context inside the packet*/

    fbe_u8_t                            tag;            /*!< Block transport service will assign unique tag for each I/O if instructed to */
    fbe_packet_status_t                 packet_status;
    fbe_atomic_t                        packet_state; /* fbe_packet_state_t */

    fbe_packet_attr_t                   packet_attr;
    fbe_packet_priority_t               packet_priority;
    fbe_traffic_priority_t				traffic_priority;/*how important this packet it and what kind of load wil it create on the drive*/

    fbe_packet_cancel_function_t   packet_cancel_function;
    fbe_packet_cancel_context_t    packet_cancel_context;

    /* expiration_time is number of milliseconds since the system booted after which the packet should be timed out */
    /* It is strongly recommended to maintain this value properly to avoid "lost" packets */
    fbe_time_t							expiration_time;
    fbe_time_t                          physical_drive_service_time_ms; /*! Total time spent at PDO. */
    fbe_time_t                          physical_drive_IO_time_stamp; /* PDO IO  start time to measure other time stats */

    fbe_transport_timer_t				transport_timer;

    fbe_cpu_id_t				cpu_id; /* We will try to complete packet on this core */

    /* Subpacket area */
    struct fbe_packet_s         * master_packet;
    fbe_queue_element_t                 subpacket_queue_element;
    fbe_queue_head_t                    subpacket_queue_head;
    fbe_atomic_t						subpacket_count;
    fbe_atomic_t						callstack_depth;

    /* Completion stack area */
    fbe_semaphore_t 					completion_semaphore;
    fbe_packet_level_t                  current_level;
    fbe_packet_stack_t                  stack[FBE_PACKET_STACK_SIZE];
    /* we added a new array instead of modifying fbe_packet_stack_t to avoid padding panelty */
    fbe_u8_t                            tracker_index[FBE_PACKET_STACK_SIZE];
    fbe_u8_t                            tracker_index_wrapped;
    fbe_u8_t                            tracker_current_index;
    fbe_packet_tracker_entry_t          tracker_ring[FBE_PACKET_TRACKER_DEPTH];

    union {
        fbe_payload_ex_t		payload_ex;
    } 
    payload_union;

} fbe_packet_t;

fbe_status_t fbe_transport_set_coarse_time(fbe_u32_t current_tick);
fbe_u32_t fbe_transport_get_coarse_time(void);

//fbe_status_t fbe_transport_packet_clear_callstack_depth(fbe_packet_t * packet);
static __forceinline fbe_status_t fbe_transport_packet_clear_callstack_depth(fbe_packet_t * packet)
{
    packet->callstack_depth = 0;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_transport_run_queue_init(void);

fbe_status_t fbe_transport_run_queue_destroy(void);

fbe_status_t fbe_transport_run_queue_push(fbe_queue_head_t * tmp_queue, 
                                         fbe_packet_completion_function_t completion_function,
                                         fbe_packet_completion_context_t  completion_context);

fbe_status_t fbe_transport_run_queue_push_packet(fbe_packet_t * packet, fbe_transport_rq_method_t rq_method);
fbe_status_t fbe_transport_run_queue_push_request(fbe_memory_request_t * request);
fbe_status_t fbe_transport_run_queue_push_raid_request(fbe_queue_element_t *queue_element, fbe_cpu_id_t cpu_id);

static __forceinline fbe_status_t 
fbe_transport_get_status_code(fbe_packet_t * packet)
{
    return packet->packet_status.code;
}

static __forceinline fbe_u32_t 
fbe_transport_get_status_qualifier(fbe_packet_t * packet)
{
    return packet->packet_status.qualifier;
}
static __forceinline void 
fbe_transport_set_status(fbe_packet_t * packet, fbe_status_t code, fbe_u32_t qualifier)
{
    packet->packet_status.code = code;
    packet->packet_status.qualifier = qualifier;
}

/* Initialization related functions */
fbe_status_t fbe_transport_initialize_packet(fbe_packet_t * packet);
fbe_status_t fbe_transport_reuse_packet(fbe_packet_t * packet);
fbe_status_t fbe_transport_initialize_sep_packet(fbe_packet_t *packet);

/* This function does some cleanup, but do NOT release memory */
fbe_status_t fbe_transport_destroy_packet(fbe_packet_t * packet);

fbe_status_t fbe_transport_set_magic_number(fbe_packet_t * packet, fbe_u64_t magic_number);

fbe_status_t fbe_transport_set_address(fbe_packet_t * packet,
                                        fbe_package_id_t package_id,
                                        fbe_service_id_t service_id,
                                        fbe_class_id_t   class_id,
                                        fbe_object_id_t object_id);

/* State related functions */
fbe_packet_state_t fbe_transport_exchange_state(fbe_packet_t * packet, fbe_packet_state_t new_state);
fbe_bool_t fbe_transport_is_packet_cancelled(fbe_packet_t * packet);

fbe_status_t fbe_transport_set_cancel_function(  fbe_packet_t * packet, 
                                                fbe_packet_cancel_function_t packet_cancel_function,
                                                fbe_packet_cancel_context_t  packet_cancel_context);


fbe_status_t fbe_transport_cancel_packet(fbe_packet_t * packet);

fbe_status_t fbe_transport_copy_packet_status(fbe_packet_t * src, fbe_packet_t * dst);

/* Address related functions */
fbe_status_t fbe_transport_get_package_id(fbe_packet_t * packet, fbe_package_id_t * package_id);
fbe_status_t fbe_transport_set_package_id(fbe_packet_t * packet, fbe_package_id_t package_id);
fbe_status_t fbe_transport_get_service_id(fbe_packet_t * packet, fbe_service_id_t * service_id);
fbe_status_t fbe_transport_get_class_id(fbe_packet_t * packet, fbe_class_id_t * class_id);

fbe_status_t fbe_transport_get_object_id(fbe_packet_t * packet, fbe_object_id_t * object_id);
fbe_status_t fbe_transport_set_object_id(fbe_packet_t * packet, fbe_object_id_t object_id);


/* Queueing related functions */
fbe_packet_t * fbe_transport_queue_element_to_packet(fbe_queue_element_t * queue_element);

fbe_packet_t * fbe_transport_memory_request_to_packet(fbe_memory_request_t * memory_request);
fbe_packet_t * fbe_transport_subpacket_queue_element_to_packet(fbe_queue_element_t * queue_element);

fbe_packet_t * fbe_transport_timer_to_packet(fbe_transport_timer_t * transport_timer);
fbe_queue_element_t * fbe_transport_get_queue_element(fbe_packet_t * packet);

/* This function initializes the expiration time */
fbe_status_t fbe_transport_init_expiration_time(fbe_packet_t * packet);

/* This function sets the expiration time.  It does NOT start the timer */
fbe_status_t fbe_transport_set_expiration_time(fbe_packet_t * packet, fbe_time_t expiration_time);

/* This function propagates the expiration time from one packet to another */
fbe_status_t fbe_transport_propagate_expiration_time(fbe_packet_t *new_packet, fbe_packet_t *original_packet);

/* This function starts the expiration timer */
fbe_status_t fbe_transport_set_and_start_expiration_timer(fbe_packet_t * packet, fbe_time_t expiration_time_interval);

fbe_status_t fbe_transport_get_expiration_time(fbe_packet_t * packet, fbe_time_t *expiration_time);

fbe_status_t fbe_transport_get_physical_drive_io_stamp(fbe_packet_t * packet, fbe_time_t *io_stamp_time);
fbe_status_t fbe_transport_set_physical_drive_io_stamp(fbe_packet_t * packet, fbe_time_t io_stamp_time);

fbe_bool_t fbe_transport_is_packet_expired(fbe_packet_t * packet);

static __forceinline fbe_status_t 
fbe_transport_init_physical_drive_service_time(fbe_packet_t * packet)
{
    packet->physical_drive_service_time_ms = 0;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t 
fbe_transport_increment_physical_drive_service_time(fbe_packet_t * packet, fbe_time_t increment_time)
{
    packet->physical_drive_service_time_ms += increment_time;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t 
fbe_transport_modify_physical_drive_service_time(fbe_packet_t * packet, fbe_time_t service_time)
{
    packet->physical_drive_service_time_ms = service_time;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t 
fbe_transport_get_physical_drive_service_time(fbe_packet_t * packet, fbe_time_t *expiration_time)
{
    *expiration_time = packet->physical_drive_service_time_ms;
    return FBE_STATUS_OK;
}
/* This function propagates the expiration time from one packet to another */
static __forceinline fbe_status_t 
fbe_transport_propagate_physical_drive_service_time(fbe_packet_t *new_packet, fbe_packet_t *original_packet)
{
    /* Simply copy current expiration time from one packet to another */
    new_packet->physical_drive_service_time_ms = original_packet->physical_drive_service_time_ms;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t
fbe_transport_accumulate_physical_drive_service_time(fbe_packet_t *packet)
{
    fbe_time_t start_time;
    fbe_time_t current_op_service_time_ms;
    /* Expiration time holds our start time.  This is set inside io entry.
     */
    fbe_transport_get_expiration_time(packet, &start_time);
    /* Determine our time to service and add this on to the pdo service time.
     */
    current_op_service_time_ms = fbe_get_elapsed_milliseconds(start_time);
    fbe_transport_increment_physical_drive_service_time(packet, current_op_service_time_ms);

    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_transport_update_physical_drive_service_time(fbe_packet_t *packet)
{
    fbe_time_t start_time;
    fbe_time_t current_op_service_time_ms;
    /* io_stamp_time holds our start time.  This is set inside io entry.
     */
    fbe_transport_get_physical_drive_io_stamp(packet, &start_time);
    /* Determine our time to service and add this on to the pdo service time.
     */
    current_op_service_time_ms = fbe_get_elapsed_milliseconds(start_time);
    fbe_transport_increment_physical_drive_service_time(packet, current_op_service_time_ms);

    /* reset io_stamp for next accumulation*/
    fbe_transport_set_physical_drive_io_stamp(packet, fbe_get_time());

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_transport_remove_hook_function(fbe_base_edge_t *base_edge, fbe_packet_t *packet_p, fbe_payload_control_operation_opcode_t opcode);
fbe_status_t fbe_transport_complete_packet(fbe_packet_t * packet);
fbe_status_t fbe_transport_complete_packet_async(fbe_packet_t * packet); /* Will break the context */

fbe_status_t fbe_transport_add_subpacket(fbe_packet_t * packet, fbe_packet_t * base_subpacket);
fbe_status_t fbe_transport_set_master_packet(fbe_packet_t * packet, fbe_packet_t * base_subpacket);

fbe_status_t fbe_transport_remove_subpacket(fbe_packet_t * base_subpacket);
fbe_status_t fbe_transport_remove_subpacket_is_queue_empty(fbe_packet_t * base_subpacket, fbe_bool_t *b_is_empty);

fbe_status_t fbe_transport_destroy_subpackets(fbe_packet_t * packet);

fbe_packet_t * fbe_transport_get_master_packet(fbe_packet_t * packet);

fbe_status_t fbe_transport_set_completion_function(	fbe_packet_t * packet,
                                        fbe_packet_completion_function_t completion_function,
                                        fbe_packet_completion_context_t  completion_context);

fbe_status_t fbe_transport_set_sync_completion_type(fbe_packet_t * packet, fbe_transport_completion_type_t completion_type);

fbe_status_t fbe_transport_wait_completion(fbe_packet_t * packet);
fbe_status_t fbe_transport_wait_completion_ms(fbe_packet_t * packet, fbe_u32_t milliseconds);

/* Please NEVER use this function !!! */
fbe_status_t fbe_transport_unset_completion_function(fbe_packet_t * packet,
                                        fbe_packet_completion_function_t completion_function,
                                        fbe_packet_completion_context_t  completion_context);

fbe_memory_request_t * fbe_transport_get_memory_request(fbe_packet_t * packet);

typedef enum fbe_transport_resource_priority_adjustment_e {
    FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_NONE = 0,
    FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST,
    FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_SECOND,
    FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_THIRD,
    FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_MAX = FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_THIRD,
}fbe_transport_resource_priority_adjustment_t;

typedef enum fbe_transport_resource_priority_boost_flag_e {
    FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_LUN = 1,
    FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_RG = 2,
    FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_VD = 4,
    FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_PVD = 8,
}fbe_transport_resource_priority_boost_flag_t;

fbe_packet_resource_priority_t fbe_transport_get_resource_priority(fbe_packet_t * packet);
void fbe_transport_set_resource_priority(fbe_packet_t * packet, fbe_packet_resource_priority_t resource_priority);
void fbe_transport_save_resource_priority(fbe_packet_t * packet);
void fbe_transport_restore_resource_priority(fbe_packet_t * packet);
fbe_status_t fbe_transport_set_resource_priority_from_master(fbe_packet_t * master_packet, fbe_packet_t * subpacket);

fbe_packet_io_stamp_t fbe_transport_get_io_stamp(fbe_packet_t * packet);
fbe_status_t fbe_transport_set_io_stamp(fbe_packet_t * packet, fbe_packet_io_stamp_t io_stamp);
fbe_status_t fbe_transport_set_io_stamp_from_master(fbe_packet_t * master_packet, fbe_packet_t * subpacket);
fbe_packet_t * fbe_transport_payload_to_packet(fbe_payload_ex_t * payload);

fbe_status_t fbe_transport_set_np_lock_from_master(fbe_packet_t * master_packet, fbe_packet_t * subpacket);

fbe_status_t fbe_transport_increment_stack_level(fbe_packet_t * packet);
fbe_status_t fbe_transport_decrement_stack_level(fbe_packet_t * packet);

fbe_status_t fbe_transport_get_packet_attr(fbe_packet_t * packet, fbe_packet_attr_t * packet_attr);
fbe_status_t fbe_transport_set_packet_attr(fbe_packet_t * packet, fbe_packet_attr_t packet_attr);
fbe_status_t fbe_transport_clear_packet_attr(fbe_packet_t * packet, fbe_packet_attr_t packet_attr);

static __forceinline fbe_payload_ex_t * fbe_transport_get_payload_ex(fbe_packet_t * packet)
{
    return (&packet->payload_union.payload_ex);
}

fbe_payload_block_operation_t * fbe_transport_get_block_operation(fbe_packet_t * packet);
fbe_status_t fbe_transport_get_transport_id(fbe_packet_t * packet, fbe_transport_id_t * transport_id);

fbe_status_t fbe_transport_get_server_index(fbe_packet_t * packet, fbe_edge_index_t* server_index);
fbe_status_t fbe_transport_get_client_index(fbe_packet_t * packet, fbe_edge_index_t* client_index);

fbe_base_edge_t * fbe_transport_get_edge(fbe_packet_t * packet);

fbe_status_t fbe_transport_get_packet_priority(fbe_packet_t * packet, fbe_packet_priority_t * packet_priority);
fbe_status_t fbe_transport_set_packet_priority(fbe_packet_t * packet, fbe_packet_priority_t packet_priority);

fbe_status_t fbe_transport_set_tag(fbe_packet_t * packet, fbe_u8_t tag);
fbe_status_t fbe_transport_get_tag(fbe_packet_t * packet, fbe_u8_t * tag);

fbe_status_t fbe_transport_get_current_operation_status(fbe_packet_t * packet);

fbe_status_t
fbe_transport_build_control_packet(fbe_packet_t * packet, 
                                    fbe_payload_control_operation_opcode_t      control_code,
                                    fbe_payload_control_buffer_t            buffer,
                                    fbe_payload_control_buffer_length_t in_buffer_length,
                                    fbe_payload_control_buffer_length_t out_buffer_length,
                                    fbe_packet_completion_function_t        completion_function,
                                    fbe_packet_completion_context_t         completion_context);

fbe_status_t fbe_transport_set_timer(fbe_packet_t * packet,
                                     fbe_time_t timeout,
                                     fbe_packet_completion_function_t completion_function,
                                     fbe_packet_completion_context_t completion_context);

/* The packet will be completed immediatelly */
fbe_status_t fbe_transport_cancel_timer(fbe_packet_t * packet);


/* This function is DEPRECATED */
fbe_payload_control_operation_opcode_t fbe_transport_get_control_code(fbe_packet_t * packet);

fbe_status_t fbe_transport_enqueue_packet(fbe_packet_t * packet, fbe_queue_head_t * queue_head);
fbe_packet_t * fbe_transport_dequeue_packet(fbe_queue_head_t * queue_head);
fbe_status_t fbe_transport_remove_packet_from_queue(fbe_packet_t * packet);
fbe_status_t fbe_transport_init(void);
fbe_status_t fbe_transport_destroy(void);

void 
fbe_base_transport_trace(fbe_trace_level_t trace_level,
			 fbe_u32_t message_id,
			 const fbe_char_t * fmt, ...)
			 __attribute__((format(__printf_func__,3,4)));

fbe_status_t fbe_transport_get_traffic_priority(fbe_packet_t * packet, fbe_traffic_priority_t *priority);
fbe_status_t fbe_transport_set_traffic_priority(fbe_packet_t * packet, fbe_traffic_priority_t priority);

fbe_status_t fbe_transport_set_cpu_id(fbe_packet_t * packet, fbe_cpu_id_t cpu_id);
fbe_status_t fbe_transport_get_cpu_id(fbe_packet_t * packet, fbe_cpu_id_t * cpu_id);
fbe_status_t fbe_transport_set_monitor_object_id(fbe_packet_t * packet, fbe_object_id_t object_id);
fbe_status_t fbe_transport_get_monitor_object_id(fbe_packet_t * packet, fbe_object_id_t *object_id_p);
fbe_bool_t fbe_transport_is_monitor_packet(fbe_packet_t *packet_p, fbe_object_id_t object_id);
fbe_status_t fbe_transport_lock(fbe_packet_t * packet);
fbe_status_t fbe_transport_unlock(fbe_packet_t * packet);

fbe_status_t fbe_transport_run_queue_get_stats(void *stats);
fbe_status_t fbe_transport_run_queue_clear_stats(void);

fbe_status_t fbe_transport_record_callback_with_action(fbe_packet_t * packet,
                                                       fbe_packet_completion_function_t completion_function,
                                                       stack_tracker_action action);

/* Group policy structure */
typedef void (* fbe_transport_external_run_queue_t)(fbe_queue_element_t * qe, fbe_cpu_id_t cpu_id);

typedef struct fbe_physical_board_group_policy_info_s{
    fbe_transport_external_run_queue_t fbe_transport_external_run_queue_current;
    fbe_transport_external_run_queue_t fbe_transport_external_fast_queue_current;
}fbe_physical_board_group_policy_info_t;

typedef enum fbe_group_policy_control_code_e {
    FBE_BASE_BOARD_CONTROL_CODE_ENABLE_PP_GROUP_PRIORITY,
    FBE_BASE_BOARD_CONTROL_CODE_DISABLE_PP_GROUP_PRIORITY
} fbe_group_policy_control_code_e;

void fbe_transport_run_queue_set_external_handler(void (*handler) (fbe_queue_element_t *, fbe_cpu_id_t));
void fbe_transport_run_queue_get_external_handler(fbe_transport_external_run_queue_t * value);
void fbe_transport_enable_group_priority(void);
void fbe_transport_disable_group_priority(void);
void fbe_transport_fast_queue_set_external_handler(void (*handler) (fbe_queue_element_t *, fbe_cpu_id_t));
void fbe_transport_fast_queue_get_external_handler(fbe_transport_external_run_queue_t * value);
void fbe_transport_enable_fast_priority(void);
void fbe_transport_disable_fast_priority(void);


/* FBE_BVD_INTERFACE_CONTROL_CODE_GET_PERFORMANCE_STATS */
typedef struct fbe_transport_run_queue_stats_s{
    fbe_u32_t transport_run_queue_current_depth;
    fbe_u32_t transport_run_queue_max_depth;
    fbe_u64_t transport_run_queue_hist_depth[FBE_TRANSPORT_RUN_QUEUE_HIST_DEPTH_SIZE];
}fbe_transport_run_queue_stats_t;

static __forceinline fbe_status_t 
fbe_transport_memory_request_set_io_master(fbe_memory_request_t * memory_request, fbe_packet_t * packet)
{
    if(packet->payload_union.payload_ex.payload_memory_operation != NULL){
        memory_request->memory_io_master = &packet->payload_union.payload_ex.payload_memory_operation->memory_io_master; 
    } else {
        memory_request->memory_io_master = NULL; 
    }

    //memory_request->packet = packet;

    return FBE_STATUS_OK;
}

static __forceinline fbe_memory_io_master_t * 
fbe_transport_memory_request_get_io_master(fbe_packet_t * packet)
{
    if(packet->payload_union.payload_ex.payload_memory_operation != NULL){
       return &packet->payload_union.payload_ex.payload_memory_operation->memory_io_master; 
    } 
    return NULL; 
}

/*!*******************************************************************
 * @enum fbe_transport_error_precedence_t
 *********************************************************************
 * @brief
 *   Enumeration of the different error precedence.  This determines
 *   which error is reported for a multi-error I/O.  They are enumerated
 *   from lowest to highest weight.
 *
 *********************************************************************/
typedef enum fbe_transport_error_precedence_e {
    FBE_TRANSPORT_ERROR_PRECEDENCE_NO_ERROR         =   0,
    FBE_TRANSPORT_ERROR_PRECEDENCE_INVALID_REQUEST  =  10,
    FBE_TRANSPORT_ERROR_PRECEDENCE_NOT_READY        =  20,
    FBE_TRANSPORT_ERROR_PRECEDENCE_ABORTED          =  30,
    FBE_TRANSPORT_ERROR_PRECEDENCE_TIMEOUT          =  40,
    FBE_TRANSPORT_ERROR_PRECEDENCE_CANCELLED		=  50,
	FBE_TRANSPORT_ERROR_PRECEDENCE_NOT_EXIST		=  60,
    FBE_TRANSPORT_ERROR_PRECEDENCE_UNKNOWN          = 100,
} fbe_transport_error_precedence_t;

fbe_transport_error_precedence_t fbe_transport_get_error_precedence(fbe_status_t status);
fbe_status_t fbe_transport_update_master_status(fbe_status_t * master_status, fbe_status_t status);

#endif /* FBE_TRANSPORT_H */

