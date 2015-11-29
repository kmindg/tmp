#ifndef FBE_NOTIFICATION_INTERFACE_H
#define FBE_NOTIFICATION_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_job_service_interface.h"
#include "fbe/fbe_job_service_interface.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_job_service_interface.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_raid_group.h"

enum fbe_notification_type_e{
	FBE_NOTIFICATION_TYPE_INVALID = 								0x00000000,
	FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE =              0x00000001,    /*!< SPECIALIZE State */
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE =                0x00000002,    /*!< ACTIVATE State */
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY =                   0x00000004,    /*!< READY State */           
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE =               0x00000008,    /*!< HIBERNATE State */      
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_OFFLINE =                 0x00000010,   /*!< OFFLINE State */          
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL =                    0x00000020,   /*!< FAIL State */             
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY =                 0x00000040,   /*!< DESTROY State */
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_READY =           0x00000080,   /*!< PENDING_READY State */
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_ACTIVATE =        0x00000100,  /*!< PENDING_ACTIVATE State */
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_HIBERNATE =       0x00000200,  /*!< PENDING_HIBERNATE State */
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_OFFLINE =         0x00000400,  /*!< PENDING_OFFLINE State */
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_FAIL =            0x00000800,  /*!< PENDING_FAIL State */
    FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY =         0x00001000, /*!< PENDING_DESTROY State */
	FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE =				0x00001FFF, /*any of the lifecycle state*/
	FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_NON_PENDING_STATE_CHANGE =	0x0000007F, /*any of the non pending lifecycle state*/
    FBE_NOTIFICATION_TYPE_END_OF_LIFE =								0x00002000,
	FBE_NOTIFICATION_TYPE_RECOVERY =								0x00004000,
	FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED =						0x00008000,
	FBE_NOTIFICATION_TYPE_CALLHOME =								0x00010000,
    FBE_NOTIFICATION_TYPE_CHECK_QUEUED_IO = 						0x00020000,
	FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED =				0x00040000,
    FBE_NOTIFICATION_TYPE_SWAP_INFO = 								0x00080000,
    FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED = 					0x00100000,
    FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED = 						0x00200000,
	FBE_NOTIFICATION_TYPE_FBE_ERROR_TRACE = 						0x00400000,
	FBE_NOTIFICATION_TYPE_CMS_TEST_DONE = 							0x00800000, /* This should be removed */
	FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION = 					0x01000000,
    FBE_NOTIFICATION_TYPE_ZEROING = 								0x02000000,
    FBE_NOTIFICATION_TYPE_LU_DEGRADED_STATE_CHANGED =               0x04000000,
    FBE_NOTIFICATION_TYPE_ENCRYPTION_STATE_CHANGED =				0x08000000,
	FBE_NOTIFICATION_TYPE_ALL_WITHOUT_PENDING_STATES = 				0xFFFFE07F,
	FBE_NOTIFICATION_TYPE_ALL = 									0xFFFFFFFF
};

typedef fbe_u64_t fbe_notification_type_t;/*to be used with fbe_notification_type_e*/

typedef struct fbe_notification_data_changed_info_s{
	fbe_u64_t device_mask;
	fbe_device_physical_location_t phys_location;
    fbe_device_data_type_t data_type;
}fbe_notification_data_changed_info_t;

typedef enum fbe_call_hme_notify_rsn_type_e{
	NOTIFY_DIEH_END_OF_LIFE_THRSLD_RCHD=0,
	NOTIFY_DIEH_FAIL_THRSLD_RCHD,
}fbe_call_hme_notify_rsn_type_t;

typedef struct fbe_error_trace_info_s{
    fbe_char_t bytes[325]; //FBE_TRACE_STAMP_SIZE+FBE_TRACE_MSG_SIZE+1(\0)
} fbe_error_trace_info_t;

typedef struct fbe_notification_encryption_info_s{
	fbe_base_config_encryption_state_t encryption_state;
	fbe_object_id_t object_id;
	fbe_config_generation_t generation_number;
	fbe_u32_t   control_number; /* This is object specified number e.g. RG Number, pool ID etc., */
	fbe_u32_t width;
	fbe_bool_t ignore_state;
	union {
		struct {
			fbe_spare_swap_command_t swap_command_type;
			fbe_edge_index_t edge_index;
			fbe_object_id_t upstream_object_id; 
		} vd; /* This is used for VD notifications */
	} u;
}fbe_notification_encryption_info_t;

typedef struct fbe_notification_info_s{
	fbe_notification_type_t						notification_type;
	fbe_package_id_t							source_package;/*needs to be in a non bitmap style since there are arrays they use that*/
	fbe_class_id_t								class_id;
	fbe_topology_object_type_t					object_type;
    
	union {
		fbe_lifecycle_state_t	lifecycle_state;
		fbe_job_service_info_t  job_service_error_info;		
		fbe_call_hme_notify_rsn_type_t call_home_reason;
		//fbe_job_action_state_t  job_action_state; 
		fbe_notification_data_changed_info_t data_change_info;
        fbe_job_service_swap_notification_info_t swap_info;
		fbe_error_trace_info_t error_trace_info;
		fbe_object_zero_notification_data_t object_zero_notification;
		fbe_raid_group_reconstruction_notification_data_t data_reconstruction;
        fbe_bool_t              lun_degraded_flag;
		fbe_notification_encryption_info_t encryption_info;
	}notification_data;

}fbe_notification_info_t;

typedef void * fbe_notification_context_t;
typedef fbe_status_t (* fbe_notification_function_t) (fbe_object_id_t object_id, 
													  fbe_notification_info_t notification_info,
													  fbe_notification_context_t context);

typedef struct fbe_notification_element_s {
	fbe_queue_element_t					queue_element; /* MUST be first for now */
	fbe_notification_function_t 		notification_function;
	fbe_notification_context_t  		notification_context;
	fbe_package_id_t					targe_package;
	fbe_notification_registration_id_t	registration_id;
	fbe_notification_type_t				notification_type;/*what kind of notification that user wants to get*/
	fbe_topology_object_type_t			object_type;/*what type of object is sending the notification*/
	fbe_atomic_t						ref_count;
}fbe_notification_element_t;


typedef enum fbe_notification_control_code_e {
	FBE_NOTIFICATION_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_NOTIFICATION),
	FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
	FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,

	FBE_NOTIFICATION_CONTROL_CODE_LAST
} fbe_notification_control_code_t;

typedef fbe_u32_t application_id_t;

typedef struct notification_data_s {
	fbe_queue_element_t						queue_element; /* MUST be first for now */
	fbe_object_id_t 						object_id;
	fbe_object_id_t 						object_id_pad; /* SAFEMESS - shared 32/64 structure must be padded for alignment */
	fbe_notification_info_t 				notification_info;
	fbe_u64_t 								context;
	application_id_t						application_id;
	application_id_t						application_id_pad; /* SAFEMESS - shared 32/64 structure must be padded for alignment */
} notification_data_t;

#define APP_NAME_LENGTH 256
typedef struct fbe_package_register_app_notify_s{
	application_id_t application_id; /* OUTPUT */
	fbe_char_t registered_app_name[APP_NAME_LENGTH];/*INPUT so we can trace who is registered*/
}fbe_package_register_app_notify_t;

/*FAKE structure to fix alignment problems*/
typedef struct fbe_queue_element_32_s{
	fbe_u32_t  next;
	fbe_u32_t  prev;
}fbe_queue_element_32_t;

typedef struct notification_data_32_s {
	fbe_queue_element_32_t		queue_element;
	fbe_object_id_t 			object_id;
	fbe_notification_info_t 	notification_info;
	fbe_u32_t  					context;
	application_id_t			application_id;
} notification_data_32_t;


#endif /* FBE_NOTIFICATION_INTERFACE_H */

