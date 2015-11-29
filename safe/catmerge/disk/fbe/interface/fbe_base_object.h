#ifndef FBE_BASE_OBJECT_H
#define FBE_BASE_OBJECT_H

#include "fbe_base.h"
#include "fbe_diplex.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_object.h"

#include "fbe/fbe_transport.h"
#include "fbe/fbe_transport.h"

#include "fbe_scheduler.h"
#include "fbe_event.h"
#include "fbe_lifecycle.h"

enum { 
	FBE_PHYSICAL_OBJECT_LEVEL_INVALID,
	FBE_PHYSICAL_OBJECT_LEVEL_BOARD,
	FBE_PHYSICAL_OBJECT_LEVEL_PORT,
	FBE_PHYSICAL_OBJECT_LEVEL_FIRST_LCC,
    FBE_PHYSICAL_OBJECT_LEVEL_FIRST_ENCLOSURE
};

typedef enum fbe_base_object_mgmt_flags_e {
#if 0
	FBE_BASE_OBJECT_FLAG_INIT_REQUIRED           = 0x00000002, /* Set when class_id is changed */
#endif	
	FBE_BASE_OBJECT_FLAG_MGMT_PACKET_CANCELED    = 0x00000001, /* Set when packet is canceled */ 
	FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE  = 0x00000002
} fbe_base_object_mgmt_flags_t;

/*
 * These are the lifecycle condition ids for a base object.
 */
typedef enum fbe_base_object_cond_id_e {
    FBE_BASE_OBJECT_LIFECYCLE_COND_SPECIALIZE = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BASE_OBJECT),
	FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE,
	FBE_BASE_OBJECT_LIFECYCLE_COND_HIBERNATE,
	FBE_BASE_OBJECT_LIFECYCLE_COND_OFFLINE,
	FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL,
	FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY,

	/* This condition is set when clas_id is changed */
	FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,

	/* This condition is set when there are canceled packets on the queue */ 
	FBE_BASE_OBJECT_LIFECYCLE_COND_PACKET_CANCELED,

	FBE_BASE_OBJECT_LIFECYCLE_COND_DRAIN_USERPER,

	FBE_BASE_OBJECT_LIFECYCLE_COND_LAST /* Must be last. */
} fbe_base_object_cond_id_t;

#define FBE_BASE_OBJECT_LIFECYCLE_COND_MAX FBE_LIFECYCLE_COND_MAX(FBE_BASE_OBJECT_LIFECYCLE_COND_LAST)

extern fbe_lifecycle_const_base_t FBE_LIFECYCLE_CONST_DATA(base_object);

typedef fbe_u32_t fbe_object_mgmt_attributes_t;

typedef fbe_u32_t fbe_physical_object_level_t;

typedef struct fbe_base_object_mgmt_create_object_s{
	fbe_class_id_t	class_id;
	fbe_object_id_t object_id;
	fbe_object_mgmt_attributes_t mgmt_attributes;
}fbe_base_object_mgmt_create_object_t;

/* FBE_BASE_OBJECT_CONTROL_CODE_GET_NEXT_ADDRESS */
typedef struct fbe_base_object_mgmt_get_next_address_s{
	fbe_address_t    current_address;     /* IN */
	fbe_address_t    address;     /* OUT */
}fbe_base_object_mgmt_get_next_address_t;

/* FBE_BASE_OBJECT_CONTROL_CODE_GET_LEVEL */
typedef struct fbe_base_object_mgmt_get_level_s{
	fbe_physical_object_level_t    physical_object_level; /* OUT */
}fbe_base_object_mgmt_get_level_t;

/* FBE_BASE_OBJECT_CONTROL_CODE_LOG_LIFECYCLE_TRACE */
typedef struct fbe_base_object_mgmt_log_lifecycle_trace_s {
	fbe_u32_t entry_count;
} fbe_base_object_mgmt_log_lifecycle_trace_t;

typedef struct fbe_class_methods_s {
    fbe_class_id_t  class_id;
    fbe_status_t (* load)(void);
    fbe_status_t (* unload)(void);
    fbe_status_t (* create_object)(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
    fbe_status_t (* destroy_object)( fbe_object_handle_t object_handle);
	fbe_status_t (* control_entry)(fbe_object_handle_t object_handle, fbe_packet_t * packet);
	fbe_status_t (* event_entry)(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
	fbe_status_t (* io_entry)(fbe_object_handle_t object_handle, fbe_packet_t * packet);
	fbe_status_t (* monitor_entry)(fbe_object_handle_t object_handle, fbe_packet_t * packet);
}fbe_class_methods_t;

/* Public methods */
fbe_object_id_t fbe_base_object_scheduler_element_to_object_id(fbe_scheduler_element_t * scheduler_element);

fbe_status_t fbe_base_object_scheduler_element_is_object_valid(fbe_scheduler_element_t * scheduler_element);

/* This function is DEPRECATED */
void fbe_base_object_mgmt_create_object_init(fbe_base_object_mgmt_create_object_t * base_object_mgmt_create_object,
																	fbe_class_id_t	class_id, 
																	fbe_object_id_t object_id);
fbe_status_t  
fbe_base_object_mgmt_get_class_id(fbe_base_object_mgmt_create_object_t * base_object_mgmt_create_object,
								  fbe_class_id_t	* class_id);

#endif /* FBE_BASE_OBJECT_H */
