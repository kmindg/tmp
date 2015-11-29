#ifndef FBE_ESP_H
#define FBE_ESP_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_class.h"

/*even thoough we don't have a drive interface we need these since we use fbe API generically to communicate with them*/
#define FBE_ESP_DEVICE_TYPE EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN
#define FBE_ESP_DEVICE_NAME "\\Device\\esp"
#define FBE_ESP_DEVICE_NAME_CHAR "\\Device\\esp"
#define FBE_ESP_DEVICE_LINK "\\DosDevices\\espLink"

#define EspBPTKey               "EspBPT"
#define EspTraceLevelKey        "TraceLevel"

/* Values in the range 0x000 to 0x7ff are reserved by Microsoft for public I/O control codes. */
#define FBE_ESP_MGMT_PACKET								0x301
#define FBE_ESP_GET_MGMT_ENTRY 							0x302
#define FBE_ESP_REGISTER_APP_NOTIFICATION				0x303
#define FBE_ESP_NOTIFICATION_GET_EVENT					0x304
#define FBE_ESP_UNREGISTER_APP_NOTIFICATION				0x305 
#define FBE_ESP_GET_IO_ENTRY							0x306
#define FBE_ESP_SEND_IO									0x307
#define FBE_ESP_SET_SEP_CTRL_ENTRY						0x308

#define FBE_ESP_MGMT_PACKET_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_ESP_DEVICE_TYPE,  \
															FBE_ESP_MGMT_PACKET,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_ESP_GET_MGMT_ENTRY_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_ESP_DEVICE_TYPE,  \
																FBE_ESP_GET_MGMT_ENTRY,     \
																EMCPAL_IOCTL_METHOD_BUFFERED,     \
																EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_ESP_REGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_ESP_DEVICE_TYPE,  \
																		FBE_ESP_REGISTER_APP_NOTIFICATION,     \
																		EMCPAL_IOCTL_METHOD_BUFFERED,     \
																		EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_ESP_NOTIFICATION_GET_EVENT_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_ESP_DEVICE_TYPE,  \
																		FBE_ESP_NOTIFICATION_GET_EVENT,     \
																		EMCPAL_IOCTL_METHOD_BUFFERED,     \
																		EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_ESP_UNREGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_ESP_DEVICE_TYPE,  \
																		  FBE_ESP_UNREGISTER_APP_NOTIFICATION,     \
																		  EMCPAL_IOCTL_METHOD_BUFFERED,     \
																		  EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_ESP_GET_IO_ENTRY_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_ESP_DEVICE_TYPE,  \
															FBE_ESP_GET_IO_ENTRY,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_ESP_SET_SEP_CTRL_ENTRY_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_ESP_DEVICE_TYPE,  \
															FBE_ESP_SET_SEP_CTRL_ENTRY,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

fbe_status_t esp_init(void);
fbe_status_t esp_destroy(void);

fbe_status_t 
fbe_esp_send_usurper_cmd(fbe_payload_control_operation_opcode_t	control_code,    
                         fbe_class_id_t                         class_id,
                         fbe_payload_control_buffer_t           *buffer,
                         fbe_payload_control_buffer_length_t    buffer_length);

fbe_status_t esp_get_control_entry(fbe_service_control_entry_t * service_control_entry);
fbe_status_t esp_get_io_entry(fbe_io_entry_function_t * io_entry);

/* The following function allow more percise control over ESP init process*/
fbe_status_t esp_init_services(void);

fbe_status_t fbe_api_esp_common_init(void);
fbe_status_t fbe_api_esp_common_destroy(void);

fbe_u8_t* fbe_get_registry_path(void);
fbe_u8_t* fbe_get_fup_registry_path(void);


#endif /* FBE_ESP_H */
