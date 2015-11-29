#ifndef FBE_SEP_H
#define FBE_SEP_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_scheduler_interface.h"

#define FBE_SEP_DEVICE_NAME "\\Device\\sep"
#define FBE_SEP_DEVICE_NAME_CHAR "\\Device\\sep"
#define FBE_SEP_DEVICE_LINK "\\DosDevices\\sepLink"

#define FBE_SEP_DEVICE_TYPE EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN
#define FBE_LUN_DEVICE_TYPE EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN

/*!*******************************************************************
 * @enum fbe_sep_load_flags_t
 *********************************************************************
 * @brief
 *  Flags to tell us how to load sep.
 *
 *********************************************************************/
typedef enum fbe_sep_load_flags_e
{
    FBE_SEP_LOAD_FLAG_NONE              = 0x0000,
    FBE_SEP_LOAD_FLAG_USE_SIM_PSL       = 0x0001,
    FBE_SEP_LOAD_FLAG_BOOTFLASH_MODE    = 0x0002,
    FBE_SEP_LOAD_FLAG_NEXT              = 0x0004,
}
fbe_sep_load_flags_t;

/*!*******************************************************************
 * @struct fbe_sep_package_load_params_t
 *********************************************************************
 * @brief The load parameters for our sep package.
 *
 *********************************************************************/
typedef struct fbe_sep_package_load_params_s
{
    fbe_u32_t raid_library_debug_flags;
    fbe_u32_t raid_group_debug_flags;
    fbe_u32_t virtual_drive_debug_flags;
    fbe_u32_t pvd_class_debug_flags;
	fbe_u32_t lifecycle_class_debug_flags;
    fbe_trace_level_t default_trace_level; /*!< Default trace level to set. */
    fbe_trace_error_limit_t error_limit;
    fbe_trace_error_limit_t cerr_limit;
    fbe_scheduler_debug_hook_t scheduler_hooks[MAX_SCHEDULER_DEBUG_HOOKS];
    fbe_memory_dps_init_parameters_t init_data_params;
    fbe_sep_load_flags_t flags;
    fbe_block_count_t ext_pool_slice_blocks;
}
fbe_sep_package_load_params_t;

fbe_status_t sep_init(fbe_sep_package_load_params_t *params_p);
fbe_status_t sep_destroy(void);

fbe_status_t sep_get_control_entry(fbe_service_control_entry_t * service_control_entry);
fbe_status_t sep_get_io_entry(fbe_io_entry_function_t * io_entry);

/* The following two functions allow more percise control over physical package init process */
fbe_status_t sep_init_services(fbe_sep_package_load_params_t *params_p);
fbe_bool_t sep_is_service_mode(void);

void sep_get_memory_parameters(  fbe_u32_t *sep_number_of_chunks,
                                 fbe_u32_t *platform_max_memory_mb,
                                 fbe_environment_limits_t *env_limits_p);
void sep_get_dps_memory_params(fbe_memory_dps_init_parameters_t *dps_params_p);
void fbe_sep_init_set_simulated_core_count(fbe_u32_t core_count);

/* Values in the range 0x000 to 0x7ff are reserved by Microsoft for public I/O control codes. */
#define FBE_SEP_MGMT_PACKET								0x201
#define FBE_SEP_GET_MGMT_ENTRY 							0x202
#define FBE_SEP_REGISTER_APP_NOTIFICATION					0x203
#define FBE_SEP_NOTIFICATION_GET_EVENT						0x204
#define FBE_SEP_UNREGISTER_APP_NOTIFICATION				0x205 
#define FBE_SEP_GET_IO_ENTRY								0x206
#define FBE_SEP_SEND_IO									0x207
#define FBE_SEP_IS_SERVICE_MODE							0x208
#define FBE_SEP_GET_SEMV                                              0x209
#define FBE_SEP_SET_SEMV                                              0x20A
#define FBE_SEP_ENABLE_BGS_SINGLE_SP                                   0x20B

#define FBE_SEP_MGMT_PACKET_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_SEP_DEVICE_TYPE,  \
															FBE_SEP_MGMT_PACKET,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_SEP_GET_MGMT_ENTRY_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_SEP_DEVICE_TYPE,  \
																FBE_SEP_GET_MGMT_ENTRY,     \
																EMCPAL_IOCTL_METHOD_BUFFERED,     \
																EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_SEP_REGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_SEP_DEVICE_TYPE,  \
																		FBE_SEP_REGISTER_APP_NOTIFICATION,     \
																		EMCPAL_IOCTL_METHOD_BUFFERED,     \
																		EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_SEP_NOTIFICATION_GET_EVENT_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_SEP_DEVICE_TYPE,  \
																		FBE_SEP_NOTIFICATION_GET_EVENT,     \
																		EMCPAL_IOCTL_METHOD_BUFFERED,     \
																		EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_SEP_UNREGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_SEP_DEVICE_TYPE,  \
																		  FBE_SEP_UNREGISTER_APP_NOTIFICATION,     \
																		  EMCPAL_IOCTL_METHOD_BUFFERED,     \
																		  EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_SEP_GET_IO_ENTRY_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_SEP_DEVICE_TYPE,  \
															FBE_SEP_GET_IO_ENTRY,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_SEP_IS_SERVICE_MODE_IOCTL   EMCPAL_IOCTL_CTL_CODE( FBE_SEP_DEVICE_TYPE,    \
															FBE_SEP_IS_SERVICE_MODE,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_SEP_SET_SEMV_IOCTL   EMCPAL_IOCTL_CTL_CODE( FBE_SEP_DEVICE_TYPE,    \
                                                                                FBE_SEP_SET_SEMV,     \
                                                                                EMCPAL_IOCTL_METHOD_BUFFERED,     \
                                                                                EMCPAL_IOCTL_FILE_ANY_ACCESS)
						    
#define FBE_SEP_GET_SEMV_IOCTL   EMCPAL_IOCTL_CTL_CODE( FBE_SEP_DEVICE_TYPE,    \
                                                                               FBE_SEP_GET_SEMV,     \
                                                                               EMCPAL_IOCTL_METHOD_BUFFERED,     \
                                                                               EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_SEP_ENABLE_BACKGROUND_SVC_SINGLE_SP   EMCPAL_IOCTL_CTL_CODE( FBE_SEP_DEVICE_TYPE,    \
                                                                               FBE_SEP_ENABLE_BGS_SINGLE_SP,     \
                                                                               EMCPAL_IOCTL_METHOD_BUFFERED,     \
                                                                               EMCPAL_IOCTL_FILE_ANY_ACCESS)

                                                                               
#endif /* FBE_SEP_H */
