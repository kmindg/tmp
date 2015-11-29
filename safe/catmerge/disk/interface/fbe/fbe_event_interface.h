#ifndef FBE_EVENT_INTERFACE_H
#define FBE_EVENT_INTERFACE_H

typedef void* fbe_event_context_t;

typedef enum fbe_event_type_e {
    FBE_EVENT_TYPE_INVALID,
	FBE_EVENT_TYPE_EDGE_STATE_CHANGE, /* Indicates that edge memory changed */
	FBE_EVENT_TYPE_DATA_REQUEST, /* Request to send the data */
	FBE_EVENT_TYPE_PERMIT_REQUEST, /* Request permit to perform backgroud operation */
	FBE_EVENT_TYPE_VERIFY_REPORT, /* Request to send verify report */
	FBE_EVENT_TYPE_EDGE_TRAFFIC_LOAD_CHANGE, /*the traffic load one the edge change for better or worse*/
	FBE_EVENT_TYPE_SPARING_REQUEST,
	FBE_EVENT_TYPE_COPY_REQUEST,
	FBE_EVENT_TYPE_POWER_SAVING_CONFIG_CHANGED,
	FBE_EVENT_TYPE_PEER_OBJECT_ALIVE,
	FBE_EVENT_TYPE_PEER_MEMORY_UPDATE,
	FBE_EVENT_TYPE_PEER_NONPAGED_WRITE,
	FBE_EVENT_TYPE_PEER_NONPAGED_CHANGED, 
	FBE_EVENT_TYPE_PEER_NONPAGED_CHKPT_UPDATE, /* Peer moved a chkpt. */    
    FBE_EVENT_TYPE_ATTRIBUTE_CHANGED,/*let the client know the attribute on the edge is now changed*/
    FBE_EVENT_TYPE_EVENT_LOG,
    FBE_EVENT_TYPE_PEER_CONTACT_LOST,
    FBE_EVENT_TYPE_DOWNLOAD_REQUEST,    /* Request to check whether ODFU could start on the RG */
    FBE_EVENT_TYPE_ABORT_COPY_REQUEST,  /* Request to abort copy and swap-out source drive. */
    FBE_EVENT_TYPE_LAST
} fbe_event_type_t;

typedef enum fbe_permit_event_type_e {
    FBE_PERMIT_EVENT_TYPE_ZERO_REQUEST,
    FBE_PERMIT_EVENT_TYPE_IS_CONSUMED_REQUEST,
    FBE_PERMIT_EVENT_TYPE_VERIFY_REQUEST,
    FBE_PERMIT_EVENT_TYPE_REBUILD_REQUEST,
    FBE_PERMIT_EVENT_TYPE_COPY_REQUEST,
    FBE_PERMIT_EVENT_TYPE_VERIFY_INVALIDATE_REQUEST,
}fbe_permit_event_type_t;

typedef enum fbe_data_event_type_e{
    FBE_DATA_EVENT_TYPE_MARK_NEEDS_REBUILD,
    FBE_DATA_EVENT_TYPE_MARK_NEEDS_REBUILD_FAIL,
    FBE_DATA_EVENT_TYPE_REMAP,
    FBE_DATA_EVENT_TYPE_MARK_VERIFY,
    FBE_DATA_EVENT_TYPE_MARK_IW_VERIFY,
}fbe_data_event_type_t;

typedef enum fbe_event_log_event_type_e {
    FBE_EVENT_LOG_EVENT_TYPE_LUN_BV_STARTED,
    FBE_EVENT_LOG_EVENT_TYPE_LUN_BV_COMPLETED
}fbe_event_log_event_type_t;




#endif /* FBE_EVENT_INTERFACE_H */
