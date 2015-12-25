#ifndef FBE_BLOCK_TRANSPORT_H
#define FBE_BLOCK_TRANSPORT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_block_transport.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions and the related structures, 
 *  enums and definitions for the block transport.
 * 
 *  The block transport is an FBE functional transport that is used for 
 *  logical block commands such as read and write.
 *
 *  The definition of the block transport payload is the 
 *  @ref fbe_payload_block_operation_t.  And the valid opcodes for this functional
 *  transport are the @ref fbe_payload_block_operation_opcode_t for functional packets
 *  and the @ref fbe_block_transport_control_code_t for control packets.
 *
 * @version
 *   6/2008:  Created. Peter Puhov.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_transport.h"
#include "fbe/fbe_atomic.h"
#include "fbe_base_transport.h"
#include "fbe_event.h"

/*! @struct fbe_block_edge_t  
 *  
 *  @brief This is the definition of the storage extent protocol edge, which is
 *  used for block operations. e.g. read, write, etc.
 *  
 *  @ingroup fbe_block_edge
 */
typedef struct fbe_block_edge_s{

    /*! This must always be first in the edge, since it is essentially the base  
     *  class for all edges.
     */
    fbe_base_edge_t base_edge;

     /* Block edges may cross package boundary */
    fbe_package_id_t server_package_id; /* Package id of the server */
    fbe_package_id_t client_package_id; /* Package id of the client */

    /*! @note This is the capacity in blocks of the extent exported by the 
     *        server on this edge.  This capacity is in terms of the client
     *        block size.
     *  @ref  fbe_block_edge_t::block_edge_geometry
     *        Use the value of block_edge_geometry to determine the alignment
     *        required.
     */
    fbe_lba_t capacity;

    /*! This is the block offset of this extent from the start of the 
     *  device.  Currently this is not used since all edges import the entire
     *  capacity.
     */
    fbe_lba_t offset;

    fbe_medic_action_priority_t medic_action_priority;

    fbe_block_edge_geometry_t   block_edge_geometry;
    fbe_traffic_priority_t      traffic_priority;

    /*! How long does the client of this edge expect us to become ready in.
    this is used for hiebrnation. When LU is connected to RAID, that's how it tells raid what the LU user expects 
    for spin up time
     */
    fbe_u64_t                   time_to_become_readay_in_sec;

    fbe_status_t (* io_entry)(fbe_object_handle_t object_handle, fbe_packet_t * packet);
    fbe_object_handle_t object_handle;

}fbe_block_edge_t;

/* BLOCK transport commands */

/*! @defgroup fbe_block_transport_usurper_interface FBE Block Transport Usurper Interface 
 *  This is the set of APIs that make up the FBE block transport usurper
 *  interface.
 *  @ingroup fbe_block_transport_interfaces
 * @{
 */

/*! @enum fbe_block_transport_control_code_t  
 *  
 *  @brief These are the set of control codes specific to the block transport
 *         that may be sent on a block edge.
 */
typedef enum fbe_block_transport_control_code_e {

    /*! This is an invalid control code and must always be first.  
     *  We define this in terms of the transport invalid control code in order
     *  that all the other control codes start with this offset.
     */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_INVALID = FBE_TRANSPORT_CONTROL_CODE_INVALID_DEF(FBE_TRANSPORT_ID_BLOCK),

    FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE, /*!< Configuration server sends this command to the SEP object */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_DESTROY_EDGE, /*!< Configuration server sends this command to SEP objects to destroy edge. */

    FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE, /*!< Client wants to attach edge to the server */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE, /*!< Client wants to detach edge from the server */

    FBE_BLOCK_TRANSPORT_CONTROL_CODE_OPEN_EDGE, /*!< Client wants to clear FBE_BLOCK_PATH_ATTR_CLOSED attribute */

    FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_PATH_STATE, /*!< Userper command for testability */

    FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO, /*!< Userper command for testability */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK, /*!< Userper command for testability.  */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK, /*!< Remove edge tap hook. */

    FBE_BLOCK_TRANSPORT_CONTROL_CODE_VALIDATE_CAPACITY, /*!< Client wants to validate the capacity with the server */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_MAX_UNUSED_EXTENT_SIZE, /*!< Client wants to get the max free space available from the server */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLIENT_HIBERNATING, /*!< The specified client is hibernating so mark the edge attribue*/
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_EXIT_HIBERNATION,/*!< The client is asking the server to get out of hibernation so it can use it*/
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_NEGOTIATE_BLOCK_SIZE,/*!< Negotiate sent via a control packet. */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND,    /*!< This is sent to send the block operation to object*/

    /*! This informs the lower levels of logical errors occuring at the upper (raid) levels. */ 
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS,

    /*! Used by upper levels to clear any error flags set at the lower levels.
     */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLEAR_LOGICAL_ERRORS,     

    FBE_BLOCK_TRANSPORT_CONTROL_CODE_LOGICAL_DRIVE_STATE_CHANGED, /*!< Client indicates the logical state of the drive as changed*/

    /*! Get the details on our I/O throttling parameters. */
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_THROTTLE_INFO,

    /*! Set the details on our I/O throttling parameters. */ 
    FBE_BLOCK_TRANSPORT_CONTROL_CODE_SET_THROTTLE_INFO,

	FBE_BLOCK_TRANSPORT_CONTROL_CODE_IO_COMMAND,    /*!< This is sent to send the I/O to object it is very similar to FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND */

    FBE_BLOCK_TRANSPORT_CONTROL_CODE_LAST /*!< This must always be last. */
} fbe_block_transport_control_code_t;

typedef enum fbe_edge_flags_e
{
    FBE_EDGE_FLAG_INVALID,
    FBE_EDGE_FLAG_IGNORE_OFFSET,
} fbe_edge_flags_t;

/*! @struct fbe_block_transport_control_create_edge_t 
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE.
 */
typedef struct fbe_block_transport_control_create_edge_s{
    fbe_object_id_t   server_id;        /* object id of the functional transport server */

    fbe_edge_index_t client_index;     /* index of the client edge (second edge of mirror object for example) */

    /*! @note This is the capacity in blocks of the extent exported by the 
     *        server on this edge.  This capacity is in terms of the client
     *        block size.
     *  @ref  fbe_block_edge_t::block_edge_geometry
     *        Use the value of block_edge_geometry to determine the alignment
     *        required.
     */
    fbe_lba_t capacity;

    /*! This is the block offset of this extent from the start of the device. 
     *  As with capacity this is in terms of the client block size.
     */
    fbe_lba_t offset;

    /*! This is an edge flag */
    fbe_edge_flags_t   edge_flags;

}fbe_block_transport_control_create_edge_t;


/*! @struct fbe_block_transport_control_create_edge_t 
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_DESTROY_EDGE.
 */
typedef struct fbe_block_transport_control_destroy_edge_s{
    fbe_edge_index_t  client_index;     /* index of the client edge (second edge of mirror object for example) */
}fbe_block_transport_control_destroy_edge_t;


/*! @struct fbe_block_transport_control_attach_edge_t 
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE.
 */
typedef struct fbe_block_transport_control_attach_edge_s{
    fbe_block_edge_t * block_edge; /* Pointer to block edge. The memory belong to the client, so only client can issue command */
}fbe_block_transport_control_attach_edge_t;

/*! @struct fbe_block_transport_control_detach_edge_t 
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE.
 */
typedef struct fbe_block_transport_control_detach_edge_s{
    fbe_block_edge_t * block_edge; /*!< Pointer to block edge. The memory belong to the client, so only client can issue command */
}fbe_block_transport_control_detach_edge_t;

/*! @struct fbe_block_transport_control_open_edge_t
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_OPEN_EDGE.
 */
typedef struct fbe_block_transport_control_open_edge_s{
    fbe_u32_t reserved; /*!< Not currently used, but reserved for future use. */
}fbe_block_transport_control_open_edge_t;
/*! @} */ /* end of group fbe_block_transport_usurper_interface */

/*! @struct fbe_block_transport_control_get_edge_info_t
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO.
 *  This allows us to return information about the block edge.
 */
typedef struct fbe_block_transport_control_get_edge_info_s {
    /*! This is always first and contains all the information about the 
     *  base class for this edge. 
     */
    fbe_base_transport_control_get_edge_info_t base_edge_info;

    /*! Capacity in blocks exported by the server.*/
    fbe_lba_t capacity;
     /*! This is the block offset of this extent from the start of the 
     *  device.  Currently this is not used since all edges import the entire
     *  capacity.
     */
    fbe_lba_t offset;

    /*! Block size in bytes exported by the server. */ 
    fbe_block_size_t exported_block_size;

    /*! Block size in bytes imported by the server. */ 
    fbe_block_size_t imported_block_size;

    /*! Actual pointer to the edge. */ 
    fbe_u64_t edge_p;
}fbe_block_transport_control_get_edge_info_t;

/*!*******************************************************************
 * @struct fbe_block_transport_placement_t
 *********************************************************************
 * @brief
 *  This is the enumeration of block placement options.
 *********************************************************************/
typedef enum fbe_block_transport_placement_e
{
    FBE_BLOCK_TRANSPORT_FIRST_FIT,
    FBE_BLOCK_TRANSPORT_BEST_FIT,
    FBE_BLOCK_TRANSPORT_SPECIFIC_LOCATION
}fbe_block_transport_placement_t;

/*! @struct fbe_block_transport_control_validate_capacity_t 
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_VALIDATE_CAPACITY.
 */
typedef struct fbe_block_transport_control_validate_capacity_s{
    fbe_lba_t       capacity;
    fbe_edge_index_t    client_index;           /* edge client index when attached an edge to server */
    fbe_lba_t       block_offset;               /* block offset when attached an edge to server */
    fbe_block_transport_placement_t placement;
    fbe_bool_t      b_ignore_offset;            /* Ignore the offset when determining of capacity fits */
}fbe_block_transport_control_validate_capacity_t;

/*! @struct fbe_block_transport_control_get_max_unused_extent_size_t 
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_MAX_UNUSED_EXTENT_SIZE.
 */
typedef struct fbe_block_transport_control_get_max_unused_extent_size_s{
    fbe_lba_t       extent_size;
}fbe_block_transport_control_get_max_unused_extent_size_t;

/*!*******************************************************************
 * @struct fbe_block_transport_get_throttle_info_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_THROTTLE_INFO
 *
 *********************************************************************/
typedef struct fbe_block_transport_get_throttle_info_s {
    fbe_u64_t outstanding_io_count;               /*!< Number of I/Os currently in flight. */
    fbe_u32_t outstanding_io_max;                 /*!< The maximum number of outstanding IO's */
	fbe_block_count_t io_throttle_count;		  /*!< This will be used to throrrle the I/O's */
	fbe_block_count_t io_throttle_max;			  /*!< This will be used to throrrle the I/O's */
    fbe_u32_t io_credits_max;                      /*!< Max outstanding before we queue.*/
    fbe_u32_t outstanding_io_credits;              /*!< Current outstanding operations.*/
    fbe_u32_t queue_length[FBE_PACKET_PRIORITY_LAST - 1];
}fbe_block_transport_get_throttle_info_t;

/*!*******************************************************************
 * @struct fbe_block_transport_set_throttle_info_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_BLOCK_TRANSPORT_CONTROL_CODE_SET_THROTTLE_INFO
 *
 *********************************************************************/
typedef struct fbe_block_transport_set_throttle_info_s {
    fbe_u32_t outstanding_io_max;                 /*!< The maximum number of outstanding IO's */
	fbe_block_count_t io_throttle_max;			  /*!< This will be used to throrrle the I/O's */
    fbe_u32_t io_credits_max;                     /*!< Max cost outstanding before we queue.*/
}fbe_block_transport_set_throttle_info_t;

/*!*******************************************************************
 * @struct fbe_block_transport_error_type_t
 *********************************************************************
 * @brief
 *  Enumeration for type of logical errors for the 
 *  block tranpsort object.
 *********************************************************************/
typedef enum fbe_block_transport_error_type_e
{
    FBE_BLOCK_TRANSPORT_ERROR_TYPE_INVALID = 0,
    FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_MULTI_BITS,  /*! An unexpected CRC error with multi-bits were received. */
    FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_SINGLE_BIT,  /*! An unexpected CRC error with a single bit was received. */
    FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC,
    FBE_BLOCK_TRANSPORT_ERROR_TYPE_TIMEOUT,
    FBE_BLOCK_TRANSPORT_ERROR_TYPE_LAST,
}
fbe_block_transport_error_type_t;

/*!****************************************************************************
 *    
 * @enum fbe_block_transport_logical_state_e
 *  
 * @brief 
 *   This enumeration contains logical states of the drive
 ******************************************************************************/
typedef enum fbe_block_transport_logical_state_e{
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED,  /*!< drives have not be notified of a state change yet >*/
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE,        /*!< drive is visible from user perspective >*/
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_EOL,    /*!< drive was logically failed since it was marked EOL >*/
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LINK_FAULT,
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_DRIVE_FAULT,
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_NON_EQ,  /*!< drive doesn't support Enhanced Queuing >*/
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_INVALID_IDENTITY,   /*!< Invalid attributes in the inquiry >*/
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_LE,  /*!< Low Endurance not supported >*/
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_RI,  /*!< Read Intensive not supported >*/
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_HDD_520, /*!< Read Intensive not supported >*/
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LESS_12G_LINK,   /*!< max supported link rate < 12G */
    FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_OTHER,  /*!< general case for issues not covered above >*/
}fbe_block_transport_logical_state_t;

/*! @struct fbe_block_transport_control_logical_error_t 
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS.
 */
typedef struct fbe_block_transport_control_logical_error_s{
    fbe_block_transport_error_type_t error_type;  /* ! This corresponds to the block transport error types.*/
    fbe_object_id_t pvd_object_id; /*! Object ID of the PVD that we want to give this info to. */
}fbe_block_transport_control_logical_error_t;

typedef enum fbe_block_path_attr_flags_e {
    FBE_BLOCK_PATH_ATTR_FLAGS_NONE                          = 0x00000000,
    FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE                   = 0x00000001, /*!< When set indicates physical drive is about tie which should trigger proactive sparing in Flare */
    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING         = 0x00000002, /*!< When set indicates the client above us is hibernating so it's safe for us to hibrenate */
    FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_END_OF_LIFE         = 0x00000004, /*!< When set indicates call home event for field personal to initiate proactive sparing via NST*/
    FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_KILL                = 0x00000008, /*!< When set indicates call home event for field personal to replace drive via NST*/
    FBE_BLOCK_PATH_ATTR_FLAGS_CHECK_QUEUED_IO_TIMER         = 0x00000010, /*!< When set indicates triggers call to check on timer for queued IOs in Upper DH */
    FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS                = 0x00000020, /*! Indicates path cannot be used due to timeout errors. 
                                                                           *  Raid causes this to be set when it is receiving io failed/retryable errors
                                                                           *  and the I/O has taken too long.*/
    FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN              = 0x00000040,
    FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED                      = 0x00000080, /*!< Indicates to upstream that object is degraded (non-optimal). */
    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED            = 0x00000100,
    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE  = 0x00000200, /*!< set when the lun was N seconds w/o IO but it's not hibernation yet. It will let upper layers some time to drain IO */
    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_CAN_SAVE_POWER         = 0x00000400, /*!< the client is 'qualified' to save power and if we want we can ask it to */
    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ           = 0x00000800, /*!< Request to start firmware download. */
    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT         = 0x00001000, /*!< Tell the client the download is granted */
    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL   = 0x00002000, /*!< Tell the client it is a fast download */
    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_TRIAL_RUN = 0x00004000, /*!< Request to start download trial run. */
    FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT                    = 0x00008000,
    FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT                   = 0x00010000,
    FBE_BLOCK_PATH_ATTR_FLAGS_CLEAR_DRIVE_FAULT_PENDING     = 0x00020000,
    FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST          = 0x00040000,
    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET          = 0x00080000,
    FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD        = 0x00100000, /*!< Indicates that downstream object is degraded and needs to be rebuilt by parent.*/
    FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED                 = 0x00200000, /*!< Keys are needed for this edge */
    FBE_BLOCK_PATH_ATTR_FLAGS_WEAR_LEVEL_REQUIRED           = 0x00400000, /*!< Wear leveling reporting required for this edge */
    /* Mask for collection of path attributes.
     */
    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK  = (FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ |
                                                       FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT |
                                                       FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL |
                                                       FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_TRIAL_RUN),

    FBE_BLOCK_PATH_ATTR_FLAGS_FAULT_MASK            = (FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT                |
                                                       FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT               |
                                                       FBE_BLOCK_PATH_ATTR_FLAGS_CLEAR_DRIVE_FAULT_PENDING   ),
    FBE_BLOCK_PATH_ATTR_ALL                                 = 0xFFFFFFFF    /* Initialization value */
}fbe_block_path_attr_flags_t; /* This type may be used for debug purposes ONLY */


/* Block transport server */

typedef enum fbe_block_transport_event_type_e{
    FBE_BLOCK_TRASPORT_EVENT_TYPE_INVALID,
    FBE_BLOCK_TRASPORT_EVENT_TYPE_IO_WAITING_ON_QUEUE,
    FBE_BLOCK_TRASPORT_EVENT_TYPE_UNEXPECTED_ERROR,

}fbe_block_transport_event_type_t;

/*! @defgroup fbe_block_transport_server FBE Block Transport Server APIs
 *  This is the set of APIs that make up the FBE block transport server.
 *  This server is a structure used for managing block edges.
 *  This include managing the I/Os on those edges and the edges themselves.
 *  
 *  @ingroup fbe_block_transport_interfaces
 * @{
 */
/*! @struct fbe_block_transport_const_t  
 *  
 *  @brief This is a set of defines such as an entry function, which are
 *         constant for the block transport server.
 */

typedef void * fbe_block_trasnport_event_context_t;
typedef fbe_status_t (* fbe_block_transport_event_t)(fbe_block_transport_event_type_t event_type, fbe_block_trasnport_event_context_t context);
typedef fbe_block_count_t (* fbe_block_transport_calc_io_throttle_func_t)(struct fbe_base_object_s *object_p, fbe_payload_block_operation_t *block_op_p);
typedef fbe_u32_t (* fbe_block_transport_calc_num_ios_func_t)(struct fbe_base_object_s *object_p, 
                                                              fbe_payload_block_operation_t *block_op_p,
                                                              fbe_packet_t *packet_p,
                                                              fbe_bool_t *b_do_not_queue);

typedef const struct fbe_block_transport_const_s {
    fbe_transport_entry_t block_transport_entry;       /*!< class specific block transport entry */
    fbe_block_transport_event_t     block_transport_event_entry; /*!< pointer of function for object to process events from block trasport */       
    fbe_status_t (* io_entry)(fbe_object_handle_t object_handle, fbe_packet_t * packet); /*!< class specific io transport entry */
    fbe_block_transport_calc_io_throttle_func_t throttle_calc_fn;/*!< Fn to calc how much to add to throttle count per I/O. */
    fbe_block_transport_calc_num_ios_func_t calc_io_credits_fn;/*!< Fn to calc how much to add to throttle count per I/O. */
} fbe_block_transport_const_t;

/*! @enum fbe_block_transport_flags_e  
 *  
 *  @brief These are the set of flags specific to the block transport.
 */
enum fbe_block_transport_flags_e {
    FBE_BLOCK_TRANSPORT_FLAGS_HOLD = 0x00000001, /*!< When set prevents I/O's from being sent to the object */
    FBE_BLOCK_TRANSPORT_FLAGS_TAGS_ENABLED = 0x00000002, /*!< When set tags will be allocated */
    FBE_BLOCK_TRANSPORT_FLAGS_FLUSH_AND_BLOCK = 0x00000004, /*!< When set indicates I/O is being flushed for LUN destroy; new I/O is blocked */
    FBE_BLOCK_TRANSPORT_FLAGS_IO_IN_PROGRESS =  0x00000010, /* This flag marks we have IO in progress so we can tell not to hibernate */
    FBE_BLOCK_TRANSPORT_FLAGS_COMPLETE_EVENTS_ON_DESTROY =  0x00000020, /* This flag marks we have object destroy in progress so
                                                                                                                        complete events without sending  it to object */    
    FBE_BLOCK_TRANSPORT_FLAGS_EVENT_ON_SW_ERROR =  0x00000040,/*! If there is a software error give the object an event. */

    /* Private flages */
    FBE_BLOCK_TRANSPORT_ENABLE_FORCE_COMPLETION = 0x00010000, /*!< When set I/O will be completed forcefully. This flag will be set in 
                                                              NON READY states only */

    FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT = 0x00020000, /*!< When set block transport will not call block_transport_entry, but return 
                                                         FBE_STATUS_OK instead */
    FBE_BLOCK_TRANSPORT_ENABLE_DEGRADED_QUEUE_RATIO = 0x00040000, /*!< When set the Queue processing ratio will change from the 
                                                                       standard 2:1 for normal:low priority*/
};

typedef fbe_atomic_t fbe_block_transport_attributes_t;


/*! @struct fbe_block_transport_block_operation_info_t
 *  
 *  @brief This is the structure passed as input with the block transport
 *  control code of @ref FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND.
 *  This allows us to send block operation return result of that.
 */
typedef struct fbe_block_transport_block_operation_info_s {
    /*! This contains all the information about the 
     *  block information
     */
    fbe_payload_block_operation_t block_operation;

    /*! Verify error counts pointer.
     * the usurper command's buffer is always marshalled across 
     * the connection from test to SP and back */
    fbe_raid_verify_error_counts_t verify_error_counts;

	/* In some cases we may need MCR to calculate checksum on Write */
	fbe_bool_t is_checksum_requiered;
}fbe_block_transport_block_operation_info_t;

#pragma pack(1) /* This is more generic way to sand I/O 32 / 64 bit */
typedef struct fbe_block_transport_io_operation_info_s {
    /*! This is the operation that this payload represents, e.g. read, write, etc. */
	fbe_payload_block_operation_opcode_t	block_opcode;

	/*! This is the logical block address of this operation. */
	fbe_lba_t								lba;

    /*! This is the number of blocks represented by this operation. */
    fbe_block_count_t						block_count;

    /*! This is the size in bytes of the block size the client expects to be operating on. */
    fbe_block_size_t						block_size;

    /*! This is an output field that is expected to be filled in 
     *  by the server.  The client will examine this field to determine if the
     *  operation was successful or not.
     */
    fbe_payload_block_operation_status_t	status;

    /*! This provides additional information about the status of this operation. 
     *  If there is no additional status, then this field will be set to
     *  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE by the server.
     */
    fbe_payload_block_operation_qualifier_t status_qualifier;

	/* In some cases we may need MCR to calculate checksum on Write */
	fbe_bool_t is_checksum_requiered;
}fbe_block_transport_io_operation_info_t;
#pragma pack()


enum fbe_block_transport_server_gate_e{
    FBE_BLOCK_TRANSPORT_SERVER_GATE_BIT     = 0x10000000,
    FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK    = 0x0FFFFFFF,
};

/*! @struct fbe_block_transport_server_t
 *  
 *  @brief This is the main structure of the block transport server.
 *         All objects which export block edges need to have one of these
 *         structures within them.
 */
typedef struct fbe_block_transport_server_s {

    /*! This must always be first, since it is essentially the base class for  
     *  all transport servers. 
     */
    fbe_base_transport_server_t base_transport_server;    

    FBE_ALIGN(16)fbe_atomic_t    outstanding_io_count;         /*!< Number of I/Os currently in flight. */
    fbe_u32_t                    outstanding_io_max;           /*!< The maximum number of outstanding IO's */
	fbe_block_count_t			 io_throttle_count;			   /*!< This will be used to throrrle the I/O's */
	fbe_block_count_t			 io_throttle_max;			   /*!< This will be used to throrrle the I/O's */
    fbe_u32_t			         outstanding_io_credits;	   /*!< Used to throttle based on cost of I/Os */
	fbe_u32_t			         io_credits_max;	           /*!< Max cost we are allowed outstanding before we fail or queue. */
    FBE_ALIGN(16)fbe_spinlock_t   queue_lock;                  /*!< Lock to protect all the queues. */

    fbe_queue_head_t    queue_head[FBE_PACKET_PRIORITY_LAST - 1];

    fbe_u8_t            io_credits_per_queue[FBE_PACKET_PRIORITY_LAST - 1];
    fbe_u8_t            outstanding_io_count_per_queue[FBE_PACKET_PRIORITY_LAST - 1];

    fbe_u8_t            total_io_credits;


    fbe_u32_t       tag_bitfield;                 /*!< Used for tag allocations */
    fbe_lba_t       capacity;                     /*!< This is the number of blocks exported. */
    fbe_block_transport_const_t * block_transport_const; /*!< class specific block transport constant data */
    fbe_block_trasnport_event_context_t event_context;/*!< ccontext for events sent from the block transport to the object */
    FBE_ALIGN(16)fbe_block_transport_attributes_t    attributes; /*!< Collection of block transport flags. See @ref fbe_block_transport_flags_e. */
    fbe_traffic_priority_t      current_priority;/*!< what is the the traffic priority load this server have on its edges */
    fbe_time_t      last_io_timestamp;/*!< what was the last time we served an IO from the client, we use that for power saving */

    fbe_status_t    force_completion_status; /* will be used to forcefully complete incoming I/O's */
    fbe_lba_t   default_offset;/*!< where does the lba offset start on this edge*/
}fbe_block_transport_server_t;

/*FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLIENT_HIBERNATING*/
typedef struct fbe_block_client_hibernating_info_s{
    fbe_u64_t       max_latency_time_is_sec;/*!< what is the maximum time the client is willing wait for object to become ready */
    fbe_bool_t      active_side;/*!< are we sending the command from the active side */
}fbe_block_client_hibernating_info_t;

/*!**************************************************************
 * @fn block_transport_server_set_attributes
 ****************************************************************
 * @brief
 *  This function will set attributes bits
 *
 * @param bits - The bits to set.
 *
 * @return None
 *
 ****************************************************************/
static __forceinline void
block_transport_server_set_attributes(
    fbe_block_transport_server_t *block_transport_server,
    fbe_u64_t bits)
{
    fbe_atomic_or(&block_transport_server->attributes, bits);
}

/*!**************************************************************
 * @fn block_transport_server_clear_attributes
 ****************************************************************
 * @brief
 *  This function will clear attributes bits
 *
 * @param bits - The bits to clear.
 *
 * @return None
 *
 ****************************************************************/
static __forceinline void
block_transport_server_clear_attributes(
    fbe_block_transport_server_t *block_transport_server,
    fbe_u64_t bits)
{
    fbe_atomic_and(&block_transport_server->attributes, ~bits);
}

/*!**************************************************************
 * @fn block_transport_server_test_attributes
 ****************************************************************
 * @brief
 *  This function will test if some attributes set
 *
 * @param bits - The bits to test.
 *
 * @return the set bits
 *
 ****************************************************************/
static __forceinline fbe_u64_t
block_transport_server_test_attributes(
    fbe_block_transport_server_t *block_transport_server,
    fbe_u64_t bits)
{
    fbe_u64_t test_result = block_transport_server->attributes & bits;
    return test_result;
}

#define FBE_BLOCK_TRANSPORT_NORMAL_QUEUE_RATIO_ADDEND 0
void fbe_block_transport_server_set_queue_ratio_addend(fbe_u8_t normal_queue_ratio_addend);
fbe_u8_t fbe_block_transport_server_get_queue_ratio_addend(void);

/*!**************************************************************
 * @fn block_transport_server_get_credits_from_priority
 ****************************************************************
 * @brief
 *  This function will return the number of I/O credits to assign
 *  to a specified priority level when resetting the credits.
 *
 * @param priority - The priority level.
 *
 * @return fbe_u32_t, the number of credits to assign.   
 *
 ****************************************************************/
static __forceinline fbe_u8_t
block_transport_server_get_credits_from_priority(fbe_u32_t priority, fbe_block_transport_attributes_t attributes)
{
    //return(1 << (priority - 1));
    //low = 1, normal = 4, urgent = 12
    //return  (priority << (priority - 1));

    fbe_u8_t credits = (1 << (priority - 1));
    // let's set this more explicitly so we can play with it
    if (priority == FBE_PACKET_PRIORITY_NORMAL && 
        (attributes & FBE_BLOCK_TRANSPORT_ENABLE_DEGRADED_QUEUE_RATIO))
    {
        credits = credits + fbe_block_transport_server_get_queue_ratio_addend();
    }

    return credits;
} 

/*!**************************************************************
 * @fn block_transport_server_reset_io_credits
 ****************************************************************
 * @brief
 *  This function should be called every time the number of total
 *  I/O credits reaches zero. It will reset the total I/O credits
 *  as well as the per-queue I/O credits.
 *
 * @param block_transport_server - The server.
 *
 * @return void   
 *
 ****************************************************************/
static __forceinline void 
block_transport_server_reset_io_credits(fbe_block_transport_server_t * block_transport_server)
{
    fbe_u32_t   priority;
    fbe_u32_t   index;
    fbe_u8_t    credits;


    block_transport_server->total_io_credits = 0;

    for(priority = FBE_PACKET_PRIORITY_INVALID + 1; priority < FBE_PACKET_PRIORITY_LAST; priority++)
    {
        index = priority - 1;
        credits = block_transport_server_get_credits_from_priority(priority, block_transport_server->attributes);

        block_transport_server->io_credits_per_queue[index] = credits; /* change to 0 to disable starvation prevention */
        block_transport_server->total_io_credits += credits;
    }
}

/*!**************************************************************
 * @fn block_transport_server_increment_outstanding_io_count
 ****************************************************************
 * @brief
 *  This function should be called when a packet has been
 *  sent. It will increment the outstanding I/O count.
 *
 * @param block_transport_server - The server.
 * @param packet - The packet which has just been sent.
 *
 * @return void   
 *
 ****************************************************************/
static __forceinline void 
block_transport_server_increment_outstanding_io_count(
    fbe_block_transport_server_t * block_transport_server, fbe_packet_t * packet)
{
    fbe_packet_priority_t priority;
    fbe_u32_t index;

    fbe_atomic_increment(&block_transport_server->outstanding_io_count);
    block_transport_server->total_io_credits--;

    fbe_transport_get_packet_priority(packet, &priority);
    index = priority - 1;

	block_transport_server->outstanding_io_count_per_queue[index]++;

    if(block_transport_server->total_io_credits == 0)
    {
        block_transport_server_reset_io_credits(block_transport_server);
    }
    else if(block_transport_server->io_credits_per_queue[index] > 0)
    {
        block_transport_server->io_credits_per_queue[index]--;
    }
}

/*!**************************************************************
 * @fn block_transport_server_decrement_outstanding_io_count
 ****************************************************************
 * @brief
 *  This function should be called when a packet has been
 *  completed. It will decrement the outstanding I/O count.
 *
 * @param block_transport_server - The server.
 * @param packet - The packet which has just completed.
 *
 * @return void   
 *
 ****************************************************************/
static __forceinline void 
block_transport_server_decrement_outstanding_io_count(
    fbe_block_transport_server_t * block_transport_server, fbe_packet_t * packet)
{
    /* We don't currently use the packet for anything */
    FBE_UNREFERENCED_PARAMETER(packet);
    fbe_atomic_decrement(&block_transport_server->outstanding_io_count);
	block_transport_server->outstanding_io_count_per_queue[packet->packet_priority - 1]--;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_init(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  This is the initialization function for the block transport server.
 *  Users of a block transport server must initialize the server
 *  before use.
 * 
 *  This function initializes the queues, locks and counters of
 *  the block transport.
 * 
 *  This function also initializes the base transport server that
 *  is within the block transport server.
 *
 * @param block_transport_server - The server to initialize.               
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_init(fbe_block_transport_server_t * block_transport_server)
{
    fbe_u32_t i;

    for(i = 0; i < (FBE_PACKET_PRIORITY_LAST -1); i++)
    {
        fbe_queue_init(&block_transport_server->queue_head[i]);
    }

    block_transport_server_reset_io_credits(block_transport_server);

    fbe_spinlock_init(&block_transport_server->queue_lock);
    block_transport_server->outstanding_io_count = 0;
    block_transport_server->outstanding_io_max = 0;

    block_transport_server->io_throttle_count = 0;
    block_transport_server->io_throttle_max = 0;
    block_transport_server->outstanding_io_credits = 0;
    block_transport_server->io_credits_max = 0;

    block_transport_server->block_transport_const = NULL;
    block_transport_server->attributes = 0;
    block_transport_server->capacity = 0;
    block_transport_server->current_priority = FBE_TRAFFIC_PRIORITY_VERY_LOW;
    block_transport_server->last_io_timestamp = fbe_get_time();/*start with a base line that makes semse*/
    return fbe_base_transport_server_init((fbe_base_transport_server_t *) block_transport_server);
}

/*!**************************************************************
 * @fn fbe_block_transport_server_destroy(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  This is the destroy function for the block transport server.
 * 
 *  This function also destroys the base transport server, which
 *  is a part of the block transport server.
 * 
 * @remarks 
 *  There is an assumption that the caller has verified that the
 *  queues are empty before calling this function.
 *
 * @param block_transport_server - The server to destroy.               
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_destroy(fbe_block_transport_server_t * block_transport_server)
{
    /* TODO check if all queues are empty */
    fbe_spinlock_destroy(&block_transport_server->queue_lock);

    return fbe_base_transport_server_destroy((fbe_base_transport_server_t *) block_transport_server);
}

/*!**************************************************************
 * @fn fbe_block_transport_server_lock(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  This obtains the lock on the block transport server.
 *
 * @param block_transport_server - The server to lock.               
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_lock(fbe_block_transport_server_t * block_transport_server)
{
    return fbe_base_transport_server_lock((fbe_base_transport_server_t *) block_transport_server);
}

/*!**************************************************************
 * @fn fbe_block_transport_server_unlock(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  This releases the lock on the block transport server.
 * 
 * @remarks This function assumes that the lock is already held.
 *
 * @param block_transport_server - The server to unlock.               
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_unlock(fbe_block_transport_server_t * block_transport_server)
{
    return fbe_base_transport_server_unlock((fbe_base_transport_server_t *) block_transport_server);
}

/*!**************************************************************
 * @fn fbe_block_transport_server_attach_edge(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_block_edge_t * block_edge,
 *         fbe_lifecycle_const_t * p_class_const,
 *         struct fbe_base_object_s * base_object)
 ****************************************************************
 * @brief
 *  This function attaches an edge to the input block transport server.
 * 
 * @remarks This function assumes that this edge is not already
 *          attached to this server.
 *
 * @param block_transport_server - The server to attach for.
 * @param block_edge - The block edge to attach to this server.
 *                     It should not be attached to this or any other
 *                     server.
 * @param p_class_const - This is the class lifecycle constant.
 *                        This is used in conjunction with the base_object
 *                        to determine what state to put the edge into
 *                        when it is attached.
 * @param base_object - This is the ptr to the base object that we are
 *                      attaching this edge for.
 *                      This is used in conjunction with the p_class_const to
 *                      determine what state to put the edge into when it is
 *                      attached.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_attach_edge(fbe_block_transport_server_t * block_transport_server, 
                                       fbe_block_edge_t * block_edge,
                                       fbe_lifecycle_const_t * p_class_const,
                                       struct fbe_base_object_s * base_object);

fbe_status_t
fbe_block_transport_server_attach_preserve_server(fbe_block_transport_server_t * block_transport_server, 
                                                  fbe_block_edge_t * block_edge,
                                                  fbe_lifecycle_const_t * p_class_const,
                                                  struct fbe_base_object_s * base_object);
/*!**************************************************************
 * @fn fbe_block_transport_server_detach_edge(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_block_edge_t * block_edge)
 ****************************************************************
 * @brief
 *  This function detaches an edge from the input block transport server.
 * 
 * @remarks This function assumes that this edge already
 *          attached to this server and not any other server.
 *
 * @param block_transport_server - The server to detach for.
 * @param block_edge - The block edge to detach from this server.
 *                     It should be attached to this server and should not be
 *                     attached to any other server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_detach_edge(fbe_block_transport_server_t * block_transport_server, fbe_block_edge_t * block_edge)
{
    return fbe_base_transport_server_detach_edge((fbe_base_transport_server_t *) block_transport_server, (fbe_base_edge_t *) block_edge);
}

/*!**************************************************************
 * @fn fbe_block_transport_server_get_number_of_clients(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_u32_t * number_of_clients)
 ****************************************************************
 * @brief
 *  This function returns the number of client edges connected to the input
 *  block transport server.
 *
 * @param block_transport_server - The block transport server.
 * @param number_of_clients - The pointer to the number of clients
 *                            to return.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_get_number_of_clients(fbe_block_transport_server_t * block_transport_server, fbe_u32_t * number_of_clients)
{
    return fbe_base_transport_server_get_number_of_clients((fbe_base_transport_server_t *) block_transport_server, number_of_clients);
}

fbe_status_t fbe_block_transport_server_get_path_attr(fbe_block_transport_server_t * block_transport_server,
                                                      fbe_edge_index_t  server_index, 
                                                      fbe_path_attr_t *path_attr_p);

fbe_status_t fbe_block_transport_server_set_path_attr(fbe_block_transport_server_t * block_transport_server,
                                                        fbe_edge_index_t  server_index, 
                                                        fbe_path_attr_t path_attr);
fbe_status_t fbe_block_transport_server_set_path_attr_all_servers(fbe_block_transport_server_t * block_transport_server,
                                                                  fbe_path_attr_t path_attr);
fbe_status_t fbe_block_transport_server_set_path_attr_without_event_notification(fbe_block_transport_server_t * block_transport_server,
                                                        fbe_object_id_t  client_id, 
                                                        fbe_path_attr_t path_attr);
fbe_status_t fbe_block_transport_server_set_edge_path_state(fbe_block_transport_server_t * block_transport_server,
                                                            fbe_edge_index_t  server_index, 
                                                            fbe_path_state_t path_state);

fbe_status_t fbe_block_transport_server_send_event(fbe_block_transport_server_t * block_transport_server, 
                                                    fbe_event_t * event);

fbe_status_t fbe_block_transport_server_is_lba_range_consumed(fbe_block_transport_server_t * block_transport_server,
                                                              fbe_lba_t lba,
                                                              fbe_block_count_t block_count,
                                                              fbe_bool_t * is_consumed_range_p);

fbe_status_t fbe_block_transport_correct_server_index(fbe_block_transport_server_t * block_transport_server);
fbe_status_t fbe_block_transport_server_clear_path_attr(fbe_block_transport_server_t * block_transport_server,
                                                        fbe_edge_index_t  server_index, 
                                                        fbe_path_attr_t path_attr);
fbe_status_t fbe_block_transport_server_clear_path_attr_all_servers(fbe_block_transport_server_t * block_transport_server,
                                                                    fbe_path_attr_t path_attr);
fbe_status_t fbe_block_transport_server_propogate_path_attr_all_servers(fbe_block_transport_server_t* block_transport_server,
                                                                        fbe_path_attr_t path_attr);
fbe_status_t fbe_block_transport_server_propogate_path_attr_stop_with_error(fbe_block_transport_server_t* block_transport_server,
                                                                            fbe_path_attr_t path_attr);
fbe_status_t fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(fbe_block_transport_server_t * block_transport_server,
                                                     fbe_path_attr_t path_attr, fbe_path_attr_t mask);

fbe_status_t fbe_block_transport_server_set_path_state(fbe_block_transport_server_t * block_transport_server, fbe_path_state_t path_state,
                                                       fbe_block_path_attr_flags_t attribute_exception);
fbe_status_t fbe_block_transport_server_set_path_traffic_priority(fbe_block_transport_server_t * block_transport_server, fbe_traffic_priority_t traffic_priority);

fbe_status_t fbe_block_transport_server_set_default_offset(fbe_block_transport_server_t * block_transport_server, fbe_lba_t default_offset);
fbe_lba_t fbe_block_transport_server_get_default_offset(fbe_block_transport_server_t * block_transport_server);
fbe_lifecycle_status_t fbe_block_transport_drain_event_queue(fbe_block_transport_server_t * block_transport_server);

fbe_status_t fbe_block_transport_server_get_minimum_capacity_required(fbe_block_transport_server_t * block_transport_server,
                                                                      fbe_lba_t *exported_offset,
                                                                      fbe_lba_t *minimum_capacity_required);
void fbe_block_transport_server_abort_monitor_operations(fbe_block_transport_server_t * block_transport_server,
                                                         fbe_object_id_t object_id);
void fbe_block_transport_server_get_throttle_info(fbe_block_transport_server_t * block_transport_server,
                                                  fbe_block_transport_get_throttle_info_t *get_throttle_info);

fbe_status_t fbe_block_transport_server_set_throttle_info(fbe_block_transport_server_t * block_transport_server,
                                                          fbe_block_transport_set_throttle_info_t *set_throttle_info);
void fbe_block_transport_server_get_available_io_credits(fbe_block_transport_server_t * bts, fbe_u32_t *available_io_credits_p);
fbe_status_t fbe_block_transport_server_get_server_index_for_lba(fbe_block_transport_server_t * block_transport_server,
                                                                 fbe_lba_t lba,
                                                                 fbe_edge_index_t * server_index_p);
fbe_status_t fbe_block_transport_align_io(fbe_block_size_t alignment_blocks,
                                          fbe_lba_t *lba_p,
                                          fbe_block_count_t *blocks_p);
fbe_status_t fbe_block_transport_server_set_block_size(fbe_block_transport_server_t* block_transport_server,
                                                       fbe_block_edge_geometry_t block_edge_geometry);
/*!**************************************************************   
 * @fn fbe_block_transport_server_is_edge_connected(
 *           fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  This function returns true if there is at least one edge
 *  connected.
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_bool_t   
 *
 ****************************************************************/
static __forceinline  fbe_bool_t
fbe_block_transport_server_is_edge_connected(fbe_block_transport_server_t * block_transport_server)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *)block_transport_server;

    return (NULL != base_transport_server->client_list) ? FBE_TRUE : FBE_FALSE;
}


/*!**************************************************************
 * @fn fbe_block_transport_server_get_client_edge_by_client_id(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function returns the edge that is in use by the given
 *  input client object id.
 *
 * @param block_transport_server - The block transport server.
 * @param object_id - Object id of client to search for
 *                    in the list of attached edges.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_block_edge_t *
fbe_block_transport_server_get_client_edge_by_client_id(fbe_block_transport_server_t * block_transport_server,
                                                            fbe_object_id_t object_id)
{
    return (fbe_block_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *) block_transport_server,
                                                                                            object_id);
}

static __forceinline fbe_block_edge_t *
fbe_block_transport_server_get_client_edge_by_server_index(fbe_block_transport_server_t * block_transport_server,
                                                           fbe_edge_index_t server_index)
{
    return (fbe_block_edge_t *)fbe_base_transport_server_get_client_edge_by_server_index((fbe_base_transport_server_t *) block_transport_server,
                                                                                         server_index);
}

fbe_lifecycle_status_t
fbe_block_transport_server_pending(fbe_block_transport_server_t * block_transport_server,
                                       fbe_lifecycle_const_t * p_class_const,
                                       struct fbe_base_object_s * base_object);

fbe_lifecycle_status_t
fbe_block_transport_server_pending_with_exception(fbe_block_transport_server_t * block_transport_server,
                                                  fbe_lifecycle_const_t * p_class_const,
                                                  struct fbe_base_object_s * base_object,
                                                  fbe_block_path_attr_flags_t attributes);
/*!**************************************************************
 * @fn fbe_block_transport_server_set_outstanding_io_max(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_u32_t outstanding_io_max)
 ****************************************************************
 * @brief
 *  This function sets the max number of outstanding I/Os
 *  allowed before this transport server will start queueing packets.
 *
 * @param block_transport_server - The block transport server.
 * @param outstanding_io_max - The max number of outstanding I/Os
 *                             this transport server will allow before
 *                             beginning to queue packets to the block
 *                             transport server queues.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_set_outstanding_io_max(fbe_block_transport_server_t * block_transport_server, fbe_u32_t outstanding_io_max)
{
    block_transport_server->outstanding_io_max = outstanding_io_max;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_get_outstanding_io_count(fbe_block_transport_server_t *block_transport_server)
 ****************************************************************
 * @brief
 *  This function returns the total outstanding IOs right now
 *
 * @param block_transport_server - The block transport server.
 * @param total_count - return value
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t fbe_block_transport_server_get_outstanding_io_count(fbe_block_transport_server_t *block_transport_server,
                                                                                        fbe_u32_t *total_count)
{
    *total_count = (fbe_u32_t)(block_transport_server->outstanding_io_count & FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_get_outstanding_io_max(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_u32_t outstanding_io_max)
 ****************************************************************
 * @brief
 *  This function returns the max number of outstanding I/Os
 *  allowed before this transport server will start queueing packets.
 *
 * @param block_transport_server - The block transport server.
 * @param outstanding_io_max - return value
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_get_outstanding_io_max(fbe_block_transport_server_t * block_transport_server, fbe_u32_t *outstanding_io_max)
{
    *outstanding_io_max = block_transport_server->outstanding_io_max;
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_block_transport_server_set_block_transport_const(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_block_transport_const_t * block_transport_const)
 ****************************************************************
 * @brief
 *  Sets the block transport constant into the block transport server.
 *  This includes the block transport entry function, which will
 *  be called in order to start a new request.
 *
 * @param block_transport_server - The block transport server.
 * @param block_transport_const - This includes constant data
 *                                which needs to get saved in
 *                                the block transport server including
 *                                our transport entry function.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_set_block_transport_const(fbe_block_transport_server_t * block_transport_server,
                                                     fbe_block_transport_const_t * block_transport_const,
                                                     fbe_block_trasnport_event_context_t event_context)
{
    block_transport_server->block_transport_const = block_transport_const;
    block_transport_server->event_context = event_context;

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_is_hold_set(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  Checks if the block transport server is in HOLD mode or not.
 *  This prevents the block transport server from sending I/O's to the object.
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_bool_t
fbe_block_transport_server_is_hold_set(fbe_block_transport_server_t * block_transport_server)
{
    return (((block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_HOLD) == FBE_BLOCK_TRANSPORT_FLAGS_HOLD) ? FBE_TRUE : FBE_FALSE);
}

/*!**************************************************************
 * @fn fbe_block_transport_server_hold(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  Sets the block transport server into the HOLD mode.
 *  This prevents the block transport server from sending I/O's to the object.
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_hold(fbe_block_transport_server_t * block_transport_server)
{
    block_transport_server_set_attributes(block_transport_server,
                                          FBE_BLOCK_TRANSPORT_FLAGS_HOLD);
    fbe_atomic_or(&block_transport_server->outstanding_io_count, FBE_BLOCK_TRANSPORT_SERVER_GATE_BIT);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_block_transport_server_set_handle_sw_errors()
 ****************************************************************
 * @brief
 *  Sets the block transport server to handle sw errors.
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_set_handle_sw_errors(fbe_block_transport_server_t * block_transport_server)
{
    block_transport_server_set_attributes(block_transport_server,
                                          FBE_BLOCK_TRANSPORT_FLAGS_EVENT_ON_SW_ERROR);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_flush_and_block(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  Enables the block transport server FLUSH AND BLOCK I/O mode.
 *  This ensures I/O in progress to the object is completed and new 
 *  I/O to the object is blocked.
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_flush_and_block(fbe_block_transport_server_t * block_transport_server)
{
    block_transport_server_set_attributes(block_transport_server,
                                          FBE_BLOCK_TRANSPORT_FLAGS_FLUSH_AND_BLOCK);
    fbe_atomic_or(&block_transport_server->outstanding_io_count, FBE_BLOCK_TRANSPORT_SERVER_GATE_BIT);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_enable_tags(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  Sets the block transport server into the TAGS_ENABLED mode.
 *  This allocates uniqie tag for every I/O sent to the object.
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_enable_tags(fbe_block_transport_server_t * block_transport_server)
{
    block_transport_server_set_attributes(block_transport_server,
                                          FBE_BLOCK_TRANSPORT_FLAGS_TAGS_ENABLED);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_resume(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  Disables the block transport server HOLD mode.
 *  This allows the block transport server to send I/O's to the object again.
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_resume(fbe_block_transport_server_t * block_transport_server)
{
    block_transport_server_clear_attributes(block_transport_server,
                                            FBE_BLOCK_TRANSPORT_FLAGS_HOLD);
    fbe_atomic_and(&block_transport_server->outstanding_io_count, ~FBE_BLOCK_TRANSPORT_SERVER_GATE_BIT);

    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_block_transport_server_enable_force_completion(fbe_block_transport_server_t * block_transport_server)
{
    block_transport_server_set_attributes(block_transport_server,
                                          FBE_BLOCK_TRANSPORT_ENABLE_FORCE_COMPLETION);
    fbe_atomic_or(&block_transport_server->outstanding_io_count, FBE_BLOCK_TRANSPORT_SERVER_GATE_BIT);
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_block_transport_server_disable_force_completion(fbe_block_transport_server_t * block_transport_server)
{
    block_transport_server_clear_attributes(block_transport_server,
                                            FBE_BLOCK_TRANSPORT_ENABLE_FORCE_COMPLETION);
    fbe_atomic_and(&block_transport_server->outstanding_io_count, ~FBE_BLOCK_TRANSPORT_SERVER_GATE_BIT);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_unflush_and_block(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  Disables the block transport server FLUSH AND BLOCK I/O mode.
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_unflush_and_block(fbe_block_transport_server_t * block_transport_server)
{
    block_transport_server_clear_attributes(block_transport_server,
                                            FBE_BLOCK_TRANSPORT_FLAGS_FLUSH_AND_BLOCK);
    fbe_atomic_and(&block_transport_server->outstanding_io_count, ~FBE_BLOCK_TRANSPORT_SERVER_GATE_BIT);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_block_transport_server_set_capacity(fbe_block_transport_server_t *block_transport_server_p, fbe_lba_t capacity);
fbe_status_t fbe_block_transport_server_get_capacity(fbe_block_transport_server_t *block_transport_server_p, fbe_lba_t *capacity_p);
fbe_status_t fbe_block_transport_server_set_traffic_priority(fbe_block_transport_server_t *block_transport_server_p, fbe_traffic_priority_t set_priority);
fbe_status_t fbe_block_transport_server_get_traffic_priority(fbe_block_transport_server_t *block_transport_server_p, fbe_traffic_priority_t *get_priority);
fbe_status_t fbe_block_transport_server_get_lowest_ready_latency_time(fbe_block_transport_server_t *block_transport_server_p, fbe_u64_t *lowest_latency_in_sec);

fbe_status_t fbe_block_transport_server_set_time_to_become_ready(fbe_block_transport_server_t *block_transport_server_p,
                                                                 fbe_object_id_t client_id,
                                                                 fbe_u64_t time_to_ready_in_sec);

static __forceinline fbe_status_t
fbe_block_transport_server_get_last_io_time(fbe_block_transport_server_t * block_transport_server, fbe_time_t *last_io_time)
{
   *last_io_time = block_transport_server->last_io_timestamp;
   return FBE_STATUS_OK;
}

static __forceinline fbe_status_t
fbe_block_transport_server_set_last_io_time(fbe_block_transport_server_t * block_transport_server, fbe_time_t last_io_time)
{
   block_transport_server->last_io_timestamp = last_io_time;
   return FBE_STATUS_OK;
}

fbe_bool_t fbe_block_transport_server_all_clients_hibernating(fbe_block_transport_server_t *block_transport_server_p);

fbe_status_t
fbe_block_transport_server_bouncer_entry(fbe_block_transport_server_t * block_transport_server,
                                         fbe_packet_t * packet,
                                         fbe_transport_entry_context_t context);

/*!**************************************************************
 * @fn fbe_block_transport_server_process_canceled_packets(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  This function completes all canceled packets from the block transport queues.
 *  
 *  This function should be called in monitor context only. (FBE_BASE_OBJECT_LIFECYCLE_COND_PACKET_CANCELED)
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t fbe_block_transport_server_process_canceled_packets(fbe_block_transport_server_t * block_transport_server);

/*! @} */ /* end of group fbe_block_transport_server */


/* Client functions */

/*! @addtogroup fbe_block_edge FBE Block Transport Edge 
 *  @{ 
 */ 
/* Start of group fbe_block_edge */

/*!**************************************************************
 * @fn fbe_block_transport_get_path_state(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_path_state_t * path_state)
 ****************************************************************
 * @brief
 *  Fetches the path state from the edge.  The path state
 *  gets set into the path_state ptr, which is passed in.
 *
 * @param block_edge - This is the edge to fetch the state from.
 *                     We assume the edge has been initialized.
 * @param path_state - The pointer to the path state variable
 *                     which will get filled in.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_get_path_state(fbe_block_edge_t * block_edge, fbe_path_state_t * path_state)
{   
    return fbe_base_transport_get_path_state((fbe_base_edge_t *) block_edge, path_state);
}

/*!**************************************************************
 * @fn fbe_block_transport_get_path_attributes(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_path_attr_t * path_attr)
 ****************************************************************
 * @brief
 *  Fetches the path attributes from the edge.  The path attributes
 *  gets set into the path_attr ptr, which is passed in.
 *
 * @param block_edge - This is the edge to fetch the attributes from.
 *                     We assume the edge has been initialized.
 * @param path_attr - The pointer to the path attrributes variable
 *                     which will get filled in.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_get_path_attributes(fbe_block_edge_t * block_edge, fbe_path_attr_t * path_attr)
{   
    return fbe_base_transport_get_path_attributes((fbe_base_edge_t *) block_edge, path_attr);
}

/*!**************************************************************
 * @fn fbe_block_transport_set_path_attributes(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_path_attr_t  path_attr)
 ****************************************************************
 * @brief
 *  Sets the path attributes to the edge.  The path attributes
 *  gets set into the path_attr ptr, which is passed in.
 *
 * @param block_edge - This is the edge to fetch the attributes from.
 *                     We assume the edge has been initialized.
 * @param path_attr - The the path attrributes variable
 *                     which will get filled in.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_set_path_attributes(fbe_block_edge_t * block_edge, fbe_path_attr_t  path_attr)
{   
    return fbe_base_transport_set_path_attributes((fbe_base_edge_t *) block_edge, path_attr);
}

/*!**************************************************************
 * @fn fbe_block_transport_clear_path_attributes(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_path_attr_t  path_attr)
 ****************************************************************
 * @brief
 *  clears the path attributes to the edge.
 *
 * @param block_edge - This is the edge to fetch the attributes from.
 *                     We assume the edge has been initialized.
 * @param path_attr - The the path attrributes value
 *                    to get clear.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_clear_path_attributes(fbe_block_edge_t * block_edge, fbe_path_attr_t  path_attr)
{   
    return fbe_base_transport_clear_path_attributes((fbe_base_edge_t *) block_edge, path_attr);
}


/*!**************************************************************
 * @fn fbe_block_transport_is_download_attr(
 *                            fbe_block_edge_t * block_edge)
 ****************************************************************
 * @brief
 *  Check if the attribute has the download related flags set
 *
 * @param block_edge - This is the edge check.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_bool_t
fbe_block_transport_is_download_attr(fbe_block_edge_t * block_edge)
{   
    return (((fbe_base_edge_t *)block_edge)->path_attr &
            (FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ |
             FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT |
             FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL));
}

/*!**************************************************************
 * @fn fbe_block_transport_set_server_id(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_object_id_t server_id)
 ****************************************************************
 * @brief
 *  Sets the server id, which is simply an object id of the server object, into
 *  the edge.  No assumption is made about the contents of the
 *  rest of the edge.
 *
 * @param block_edge - This is the edge to set the server for.
 * @param server_id - The server's object id to set into the edge.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_set_server_id(fbe_block_edge_t * block_edge, fbe_object_id_t server_id)
{   
    return fbe_base_transport_set_server_id((fbe_base_edge_t *) block_edge, server_id);
}

/*!**************************************************************
 * @fn fbe_block_transport_get_server_id(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_object_id_t *server_id)
 ****************************************************************
 * @brief
 *  Gets the server id from the passed in edge.
 *  We assume this server_id was previously initialized in the edge,
 *  but we make no assumptions about the rest of the edge.
 *
 * @param block_edge - This is the edge to fetch the id from.
 *                     We assume the edge has been initialized.
 * @param server_id - The pointer to an object id where we will return the
 *                    server's object id.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_get_server_id(fbe_block_edge_t * block_edge, fbe_object_id_t * server_id)
{   
    return fbe_base_transport_get_server_id((fbe_base_edge_t *) block_edge, server_id);
}


/*!**************************************************************
 * @fn fbe_block_transport_get_server_index(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_edge_index_t * server_index)
 ****************************************************************
 * @brief
 *  Gets the server index from the passed in edge.
 *  We assume this server_index was previously initialized in the edge, but we
 *  make no assumptions about the rest of the edge.
 *
 * @param block_edge - This is the edge to fetch the index from.
 *                     We assume the edge server index has been initialized.
 * @param server_index - The pointer to an object id where we will return the
 *                       server's index.
 *                       This allows us to distinguish between different edges
 *                       that are being served.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_get_server_index(fbe_block_edge_t * block_edge, fbe_edge_index_t * server_index)
{   
    return fbe_base_transport_get_server_index((fbe_base_edge_t *) block_edge, server_index);
}

/*!**************************************************************
 * @fn fbe_block_transport_set_server_index(
 *                            fbe_block_edge_t * block_edge,
 *                            fbe_edge_index_t server_index)
 ****************************************************************
 * @brief
 *  Sets the server index for the passed in edge. We make no assumptions about
 *  the contents of the edge.
 *
 * @param block_edge - This is the edge to set the index for.
 * @param server_index - The server index to set into the edge.
 *                       This allows us to distinguish between different edges
 *                       that are being served.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_set_server_index(fbe_block_edge_t * block_edge, fbe_edge_index_t server_index)
{   
    return fbe_base_transport_set_server_index((fbe_base_edge_t *) block_edge, server_index);
}

/*!**************************************************************
 * @fn fbe_block_transport_set_client_id(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_object_id_t client_id)
 ****************************************************************
 * @brief
 *  Sets the client's object id for the passed in edge.
 *  We make no assumptions about the contents of the edge.
 *
 * @param block_edge - This is the edge to set the client id for.
 * @param client_id - Sets the client's object id for this edge.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_set_client_id(fbe_block_edge_t * block_edge, fbe_object_id_t client_id)
{   
    return fbe_base_transport_set_client_id((fbe_base_edge_t *) block_edge, client_id);
}

/*!**************************************************************
 * @fn fbe_block_transport_get_client_id(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_object_id_t *client_id)
 ****************************************************************
 * @brief
 *  Gets the client's object id for the passed in edge.
 *  We assume this client id was already initialized in the edge.
 *
 * @param block_edge - This is the edge to set the client id for.
 * @param client_id - Pointer to the client id to return for this edge.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_get_client_id(fbe_block_edge_t * block_edge, fbe_object_id_t * client_id)
{   
    return fbe_base_transport_get_client_id((fbe_base_edge_t *) block_edge, client_id);
}

/*!**************************************************************
 * @fn fbe_block_transport_get_client_index(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_edge_index_t * client_index)
 ****************************************************************
 * @brief
 *  Gets the client's index for the passed in edge.
 *  We assume this client index was already initialized in the edge.
 *
 * @param block_edge - This is the edge to set the client index for.
 * @param client_index - Pointer to the client index to return for this edge.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_get_client_index(fbe_block_edge_t * block_edge, fbe_edge_index_t * client_index)
{   
    return fbe_base_transport_get_client_index((fbe_base_edge_t *) block_edge, client_index);
}

/*!**************************************************************
 * @fn fbe_block_transport_set_client_index(
 *                            fbe_block_edge_t * block_edge, 
 *                            fbe_edge_index_t client_index)
 ****************************************************************
 * @brief
 *  Sets the client's index for the passed in edge.
 *  We make no assumptions about the contents of the edge.
 *
 * @param block_edge - This is the edge to set the client index for.
 * @param client_index - Sets the client's index for this edge.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_set_client_index(fbe_block_edge_t * block_edge, fbe_edge_index_t client_index)
{   
    return fbe_base_transport_set_client_index((fbe_base_edge_t *) block_edge, client_index);
}

/*!**************************************************************
 * @fn fbe_block_transport_set_transport_id(
 *                            fbe_block_edge_t * block_edge)
 ****************************************************************
 * @brief
 *  Sets the transport id in the edge to FBE_TRANSPORT_ID_BLOCK.
 *  We make no assumptions about the contents of the edge.
 *
 * @param block_edge - This is the edge to set the transport id for.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_set_transport_id(fbe_block_edge_t * block_edge)
{
    return fbe_base_transport_set_transport_id((fbe_base_edge_t *) block_edge, FBE_TRANSPORT_ID_BLOCK);
}

/*!**************************************************************
 * @fn fbe_block_transport_edge_set_capacity(
 *                            fbe_block_edge_t *const block_edge_p,
 *                            const fbe_lba_t capacity)
 ****************************************************************
 * @brief
 *  Sets the capacity of the edge in blocks.
 *  The block size that this capacity is in units of is the
 *  exported block size for this edge.
 *  We make no assumption about any of the other values in the edge
 *  being initialized.
 *
 * @param block_edge_p - This is the edge to set the capacity for.
 * @param capacity - The capacity in blocks to set into the edge.
 *                   The block size this capacity is in units of
 *                   is the exported block size.
 *
 * @return None.
 ****************************************************************/
static __forceinline void
fbe_block_transport_edge_set_capacity(fbe_block_edge_t *const block_edge_p,
                                      const fbe_lba_t capacity)
{
    block_edge_p->capacity = capacity;
    return;
}
/*!**************************************************************
 * @fn fbe_block_transport_edge_get_capacity(
 *                            const fbe_block_edge_t *const block_edge_p)
 ****************************************************************
 * @brief
 *  Returns the capacity of the edge in blocks.
 *  The block size that this capacity is in units of is the
 *  exported block size for this edge.
 *  We make no assumption about any of the other values in the edge
 *  being initialized.
 *
 * @param block_edge_p - This is the edge to get the capacity for. 
 *
 * @return fbe_lba_t
 *         The capacity in blocks to set into the edge.
 *         The block size this capacity is in units of is the
 *         exported block size.
 ****************************************************************/
fbe_status_t fbe_block_transport_edge_get_capacity(fbe_block_edge_t * block_edge, fbe_lba_t * capacity);

/*!**************************************************************
 * fbe_block_transport_edge_set_offset()
 ****************************************************************
 * @brief
 *  Sets the offset of the edge in blocks.
 *  We make no assumption about any of the other values in the edge
 *  being initialized.
 *
 * @param block_edge_p - This is the edge to set the capacity for.
 * @param offset - The offset in blocks to set into the edge.
 *                   The block size this offset is in units
 *                   of is the exported block size.
 *
 * @return None.
 ****************************************************************/
static __forceinline void
fbe_block_transport_edge_set_offset(fbe_block_edge_t *const block_edge_p,
                                    const fbe_lba_t offset)
{
    block_edge_p->offset = offset;
    return;
}

/*!**************************************************************
 * fbe_block_transport_edge_get_offset()
 ****************************************************************
 * @brief
 *  Returns the offset of the edge in blocks.
 *  We make no assumption about any of the other values in the edge
 *  being initialized.
 *
 * @param block_edge_p - This is the edge to get the offset for. 
 *
 * @return fbe_lba_t
 *         The offset in blocks from the edge.
 ****************************************************************/
static __forceinline fbe_lba_t
fbe_block_transport_edge_get_offset(const fbe_block_edge_t *const block_edge_p)
{
     return block_edge_p->offset;
}

/*!**************************************************************
 * @fn fbe_block_transport_is_lba_range_overlaps_edge_extent()
 ****************************************************************
 * @brief
 *  This function determines if the specified LBA range overlaps
 *  with the range of the specified edge extent.
 *
 * @param in_lba            - The block address to examine.
 * @param in_block_edge_p   - Pointer to edge to examine.
 *
 * @return fbe_bool_t       - TRUE if lba is within extent.
 *
 ****************************************************************/
static __forceinline fbe_bool_t
fbe_block_transport_is_lba_range_overlaps_edge_extent(fbe_lba_t lba,
                                                      fbe_block_count_t block_count,
                                                      fbe_block_edge_t * block_edge_p)
{
    fbe_lba_t   block_offset = block_edge_p->offset;

    if (block_edge_p->base_edge.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET)
    {
        block_offset = 0;
    }
    if(((lba >= block_offset) &&
        (lba < (block_offset + block_edge_p->capacity))) ||
       ((block_offset >= lba) &&
        (block_offset < (lba + block_count))))
    {
        return TRUE;
    }
    return FALSE;
}

/*!**************************************************************
 * @fn fbe_block_transport_is_lba_in_edge_extent()
 ****************************************************************
 * @brief
 *  This function determines if the specified LBA is within the 
 *  range of the specified edge's extent.
 *
 * @param in_lba            - The block address to examine.
 * @param in_block_edge_p   - Pointer to edge to examine.
 *
 * @return fbe_bool_t       - TRUE if lba is within extent.
 *
 ****************************************************************/
static __forceinline fbe_bool_t
fbe_block_transport_is_lba_in_edge_extent(fbe_lba_t lba,
                                          fbe_block_edge_t * block_edge_p)
{
    if ((lba >= block_edge_p->offset) &&
        (lba < (block_edge_p->offset + block_edge_p->capacity)))
    {
        return TRUE;  
    }
    return FALSE;
}

/*!**************************************************************
 * @fn fbe_block_transport_is_lba_below_edge_extent()
 ****************************************************************
 * @brief
 *  This function determines if the specified LBA is below the 
 *  starting offset of the specified edge's extent.
 *
 * @param in_lba            - The block address to examine.
 * @param in_block_edge_p   - Pointer to edge to examine.
 *
 * @return fbe_bool_t       - TRUE if lba is below the extent.
 *
 ****************************************************************/
static __forceinline fbe_bool_t
fbe_block_transport_is_lba_below_edge_extent(fbe_lba_t lba,
                                             fbe_block_edge_t * in_block_edge_p)
{
    if (lba < in_block_edge_p->offset)
    {
        return TRUE;
    }
    return FALSE;
}

/*!**************************************************************
 * @fn fbe_block_transport_is_lba_above_edge_extent()
 ****************************************************************
 * @brief
 *  This function determines if the specified LBA  is beyond the
 *  range of the extexnt.
 *
 * @param in_lba            - The block address to examine.
 * @param in_block_edge_p   - Pointer to edge to examine.
 *
 * @return fbe_bool_t       - TRUE if lba is beyond extent.
 *
 ****************************************************************/
static __forceinline fbe_bool_t
fbe_block_transport_is_lba_above_edge_extent(fbe_lba_t lba,
                                            fbe_block_edge_t *block_edge_p)
{
    if (lba >= (block_edge_p->offset + block_edge_p->capacity))
    {
        return TRUE;
    }
    return FALSE;
}

static __forceinline fbe_status_t
fbe_block_transport_edge_get_hook(const fbe_block_edge_t *const block_edge_p, fbe_edge_hook_function_t * hook)
{
     return fbe_base_transport_get_hook_function((fbe_base_edge_t *)block_edge_p, hook);
}

static __forceinline fbe_status_t
fbe_block_transport_edge_set_hook(fbe_block_edge_t * block_edge, fbe_edge_hook_function_t hook)
{   
    return fbe_base_transport_set_hook_function((fbe_base_edge_t *) block_edge, hook);
}

static __forceinline fbe_status_t
fbe_block_transport_edge_remove_hook(fbe_block_edge_t *block_edge, fbe_packet_t *packet_p)
{   
    return fbe_base_transport_remove_hook_function((fbe_base_edge_t *)block_edge, packet_p, FBE_BLOCK_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK);
}

static __forceinline fbe_medic_action_priority_t
fbe_block_transport_edge_get_priority(fbe_block_edge_t *const block_edge_p)
{
    return block_edge_p->medic_action_priority;
   
}

static __forceinline void
fbe_block_transport_edge_set_priority(fbe_block_edge_t *const block_edge_p, fbe_medic_action_priority_t set_priority)
{
     block_edge_p->medic_action_priority = set_priority;
}

static __forceinline fbe_traffic_priority_t
fbe_block_transport_edge_get_traffic_priority(fbe_block_edge_t *const block_edge_p)
{
    return block_edge_p->traffic_priority;
   
}

static __forceinline fbe_traffic_priority_t
fbe_block_transport_edge_get_highest_traffic_priority(fbe_traffic_priority_t edge_1, fbe_traffic_priority_t edge_2)
{
    return ((edge_1 > edge_2) ? edge_1 : edge_2);
   
}

static __forceinline void
fbe_block_transport_edge_set_traffic_priority(fbe_block_edge_t *const block_edge_p, fbe_traffic_priority_t set_priority)
{
     block_edge_p->traffic_priority = set_priority;
}

static __forceinline fbe_u64_t
fbe_block_transport_edge_get_time_to_become_ready(fbe_block_edge_t *const block_edge_p)
{
    return block_edge_p->time_to_become_readay_in_sec;   
}


static __forceinline void
fbe_block_transport_edge_set_time_to_become_ready(fbe_block_edge_t *const block_edge_p, fbe_u64_t time_to_readt_in_sec)
{
     block_edge_p->time_to_become_readay_in_sec = time_to_readt_in_sec;
}


fbe_status_t fbe_block_transport_server_set_upgrade_flag(fbe_block_transport_server_t * block_transport_server);
fbe_status_t fbe_block_transport_server_clear_upgrade_flag(fbe_block_transport_server_t * block_transport_server);


/*! @} */ /* End of group fbe_block_edge */

fbe_status_t fbe_block_transport_edge_set_server_package_id(fbe_block_edge_t * block_edge_p, fbe_package_id_t server_package_id);
fbe_status_t fbe_block_transport_edge_get_server_package_id( fbe_block_edge_t * block_edge_p, fbe_package_id_t * server_package_id);
fbe_status_t fbe_block_transport_edge_set_client_package_id(fbe_block_edge_t * block_edge_p, fbe_package_id_t client_package_id);
fbe_status_t fbe_block_transport_edge_get_client_package_id( fbe_block_edge_t * block_edge_p, fbe_package_id_t * client_package_id);


fbe_status_t fbe_block_transport_send_control_packet(fbe_block_edge_t * block_edge, fbe_packet_t * packet);
fbe_status_t fbe_block_transport_send_functional_packet(fbe_block_edge_t * block_edge, fbe_packet_t * packet);
fbe_status_t fbe_block_transport_send_no_autocompletion(fbe_block_edge_t * block_edge, fbe_packet_t * packet,
                                                        fbe_bool_t b_check_capacity);
fbe_status_t fbe_block_transport_send_funct_packet_params(fbe_block_edge_t * block_edge, 
                                                          fbe_packet_t * packet,
                                                          fbe_bool_t b_check_capacity);

fbe_status_t fbe_block_transport_server_update_path_state(fbe_block_transport_server_t * block_transport_server,
                                                          fbe_lifecycle_const_t * p_class_const,
                                                          struct fbe_base_object_s * base_object,
                                                          fbe_block_path_attr_flags_t attribute_exception);
fbe_status_t fbe_block_transport_find_first_upstream_edge_index_and_obj_id
(
    fbe_block_transport_server_t*   in_block_transport_server_p,
    fbe_edge_index_t*               out_block_edge_index_p,
    fbe_object_id_t*                out_object_id_p
);

fbe_status_t fbe_block_transport_find_edge_and_obj_id_by_lba(fbe_block_transport_server_t*   in_block_transport_server_p,
                                                             fbe_lba_t                       in_lba,
                                                             fbe_object_id_t*                out_object_id_p,
                                                             fbe_block_edge_t**              out_block_edge_pp);

fbe_status_t fbe_block_transport_edge_set_geometry(fbe_block_edge_t * block_edge, fbe_block_edge_geometry_t block_edge_geometry);
fbe_status_t fbe_block_transport_edge_get_geometry(fbe_block_edge_t * block_edge, fbe_block_edge_geometry_t * block_edge_geometry);

fbe_status_t fbe_block_transport_edge_set_path_attr(fbe_block_edge_t * block_edge, fbe_path_attr_t path_attr);

fbe_status_t fbe_block_transport_get_optimum_block_size(fbe_block_edge_geometry_t block_edge_geometry, fbe_optimum_block_size_t * optimum_block_size);
fbe_status_t fbe_block_transport_get_exported_block_size(fbe_block_edge_geometry_t block_edge_geometry, fbe_block_size_t  * block_size);
fbe_status_t fbe_block_transport_get_physical_block_size(fbe_block_edge_geometry_t block_edge_geometry, fbe_block_size_t  * block_size);

fbe_status_t fbe_block_transport_edge_get_optimum_block_size(fbe_block_edge_t * block_edge, fbe_optimum_block_size_t * optimum_block_size);
fbe_status_t fbe_block_transport_edge_get_exported_block_size(fbe_block_edge_t * block_edge, fbe_block_size_t  * block_size);

fbe_status_t
fbe_block_transport_server_validate_capacity(fbe_block_transport_server_t * block_transport_server, 
                                             fbe_lba_t         *capacity,
                                             fbe_u32_t          placement,
                                             fbe_bool_t         b_ignore_offset,
                                             fbe_edge_index_t  *client_index,
                                             fbe_lba_t         *block_offset);

fbe_status_t
fbe_block_transport_server_get_max_unused_extent_size(fbe_block_transport_server_t * block_transport_server, 
                              fbe_lba_t * extent_size);

fbe_status_t 
fbe_block_transport_server_can_object_be_removed_when_edge_is_removed(fbe_block_transport_server_t * block_transport_server,
                                                                      fbe_object_id_t object_id, fbe_bool_t *can_object_be_removed);

fbe_status_t fbe_block_transport_server_set_medic_priority(fbe_block_transport_server_t * block_transport_server,
                                                           fbe_medic_action_priority_t set_priority);

fbe_status_t fbe_block_transport_server_flush_and_block_io_for_destroy(fbe_block_transport_server_t* block_transport_server);
fbe_lifecycle_status_t fbe_block_transport_server_drain_all_queues(fbe_block_transport_server_t * block_transport_server,
                                                                   fbe_lifecycle_const_t * p_class_const,
                                                                   struct fbe_base_object_s * base_object);
fbe_status_t fbe_block_transport_server_process_io_from_queue(fbe_block_transport_server_t* block_transport_server);
fbe_status_t fbe_block_transport_server_process_io_from_queue_sync(fbe_block_transport_server_t* block_transport_server);

fbe_status_t fbe_block_transport_server_power_saving_config_changed(fbe_block_transport_server_t * block_transport_server);

fbe_status_t 
fbe_block_transport_send_block_operation(fbe_block_transport_server_t *block_transport_server_p,
                                         fbe_packet_t * packet_p,
                                         struct fbe_base_object_s * base_object_p);

/*! @} */ /* end of group fbe_block_transport_interfaces */

fbe_status_t fbe_block_transport_server_find_next_consumed_lba(fbe_block_transport_server_t * block_transport_server,
                                                               fbe_lba_t lba,
                                                               fbe_lba_t * next_consumed_lba_p);

fbe_status_t fbe_block_transport_server_get_end_of_extent(fbe_block_transport_server_t * block_transport_server,
                                                          fbe_lba_t lba,
                                                          fbe_block_count_t blocks,
                                                          fbe_lba_t * end_lba_p);

fbe_status_t fbe_block_transport_server_find_last_consumed_lba(fbe_block_transport_server_t * block_transport_server,
                                                               fbe_lba_t *last_consumed_lba_p);

fbe_status_t fbe_block_transport_server_set_stack_limit(fbe_block_transport_server_t * block_transport_server);

fbe_status_t fbe_block_transport_is_empty(fbe_block_transport_server_t * block_transport_server, fbe_bool_t * is_empty);

fbe_status_t fbe_block_transport_server_find_max_consumed_lba(fbe_block_transport_server_t * block_transport_server,
                                                              fbe_lba_t *max_consumed_lba_p);

/*!**************************************************************
 * @fn fbe_block_transport_server_set_io_throttle_max(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_block_count_t io_throttle_max)
 ****************************************************************
 * @brief
 *  This function sets the max number of outstanding block per drive.
 *  allowed before this transport server will start queueing packets.
 *
 * @param block_transport_server - The block transport server.
 * @param io_throttle_max - The max number of outstanding blocks
 *                             this transport server will allow before
 *                             beginning to queue packets to the block
 *                             transport server queues.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_set_io_throttle_max(fbe_block_transport_server_t * block_transport_server, fbe_block_count_t io_throttle_max)
{
    block_transport_server->io_throttle_max = io_throttle_max;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_block_transport_server_set_io_credits_max()
 ****************************************************************
 * @brief
 *  This function sets the max interpreted weight of I/Os
 *  before we will start to queue or fail I/os.
 *
 * @param block_transport_server - The block transport server.
 * @param io_weight_max - max weight of I/Os before we queue.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_block_transport_server_set_io_credits_max(fbe_block_transport_server_t * block_transport_server, fbe_u32_t io_weight_max)
{
    block_transport_server->io_credits_max = io_weight_max;
    return FBE_STATUS_OK;
}


#endif /* FBE_BLOCK_TRANSPORT_H */
