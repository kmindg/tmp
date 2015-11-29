#ifndef FBE_PHYSICAL_PACKAGE_H
#define FBE_PHYSICAL_PACKAGE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_terminator_api.h"
//#include <devioctl.h>

#define FBE_PHYSICAL_PACKAGE_DEVICE_NAME "\\Device\\PhysicalPackage"
#define FBE_PHYSICAL_PACKAGE_DEVICE_NAME_CHAR "\\Device\\PhysicalPackage"
#define FBE_PHYSICAL_PACKAGE_DEVICE_LINK "\\DosDevices\\PhysicalPackageLink"

#define FBE_PHYSICAL_PACKAGE_DEVICE_TYPE EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN

#define FBE_PHYSICAL_PACKAGE_NUMBER_OF_CHUNKS 4000

fbe_status_t physical_package_init(void);
fbe_status_t physical_package_destroy(void);

fbe_status_t physical_package_get_control_entry(fbe_service_control_entry_t * service_control_entry);
fbe_status_t physical_package_get_io_entry(fbe_io_entry_function_t * io_entry);
fbe_status_t physical_package_get_specl_sfi_entry(speclSFIEntryType * specl_sfi_entry);

/* The following two functions allow more percise control over physical package init process */
fbe_status_t physical_package_init_services(void);
fbe_status_t physical_package_create_board_object(void);

fbe_service_control_entry_t physical_package_simulator_get_control_entry(void);
fbe_service_io_entry_t physical_package_simulator_get_io_entry(void);

/* Values in the range 0x000 to 0x7ff are reserved by Microsoft for public I/O control codes. */
#define FBE_PHYSICAL_PACKAGE_GET_MGMT_ENTRY 							0x802
#define FBE_PHYSICAL_PACKAGE_MGMT_PACKET								0x801
#define FBE_PHYSICAL_PACKAGE_REGISTER_APP_NOTIFICATION					0x803
#define FBE_PHYSICAL_PACKAGE_NOTIFICATION_GET_EVENT						0x804
#define FBE_PHYSICAL_PACKAGE_UNREGISTER_APP_NOTIFICATION				0x805
#define FBE_PHYSICAL_PACKAGE_GET_IO_ENTRY								0x806
#define FBE_PHYSICAL_PACKAGE_SEND_IO									0x807

#ifdef C4_INTEGRATED
#define FBE_CLI_SEND_PKT                                          0x810
#define FBE_CLI_SEND_PKT_SGL                                      0x811
#define FBE_CLI_GET_MAP                                           0x812
#endif /* C4_INTEGRATED - C4ARCH - IOCTL differnces - C4BUG - I suspect this is all un-neccessary */


#define FBE_PHYSICAL_PACKAGE_GET_MGMT_ENTRY_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
																FBE_PHYSICAL_PACKAGE_GET_MGMT_ENTRY,     \
																EMCPAL_IOCTL_METHOD_BUFFERED,     \
																EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_PHYSICAL_PACKAGE_MGMT_PACKET_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
															FBE_PHYSICAL_PACKAGE_MGMT_PACKET,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)


#define FBE_PHYSICAL_PACKAGE_GET_IO_ENTRY_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
															FBE_PHYSICAL_PACKAGE_GET_IO_ENTRY,     \
															EMCPAL_IOCTL_METHOD_BUFFERED,     \
															EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_PHYSICAL_PACKAGE_IO_PACKET_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
														FBE_PHYSICAL_PACKAGE_SEND_IO,     \
														EMCPAL_IOCTL_METHOD_BUFFERED,     \
														EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_PHYSICAL_PACKAGE_REGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
																		FBE_PHYSICAL_PACKAGE_REGISTER_APP_NOTIFICATION,     \
																		EMCPAL_IOCTL_METHOD_BUFFERED,     \
																		EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_PHYSICAL_PACKAGE_NOTIFICATION_GET_EVENT_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
																		FBE_PHYSICAL_PACKAGE_NOTIFICATION_GET_EVENT,     \
																		EMCPAL_IOCTL_METHOD_BUFFERED,     \
																		EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_PHYSICAL_PACKAGE_UNREGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
																		  FBE_PHYSICAL_PACKAGE_UNREGISTER_APP_NOTIFICATION,     \
																		  EMCPAL_IOCTL_METHOD_BUFFERED,     \
																		  EMCPAL_IOCTL_FILE_ANY_ACCESS)



#ifdef C4_INTEGRATED
#define FBE_CLI_SEND_PKT_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
										  FBE_CLI_SEND_PKT,     \
										  EMCPAL_IOCTL_METHOD_BUFFERED,     \
						    			  EMCPAL_IOCTL_FILE_ANY_ACCESS)

#endif /* C4_INTEGRATED - C4ARCH - IOCTL differnces - C4BUG - I suspect this is all un-neccessary */

#define FBE_CLI_SEND_PKT_SGL_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
										  FBE_CLI_SEND_PKT_SGL,     \
										  EMCPAL_IOCTL_METHOD_BUFFERED,     \
						    			  EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_CLI_GET_MAP_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_PHYSICAL_PACKAGE_DEVICE_TYPE,  \
										  FBE_CLI_GET_MAP,     \
										  EMCPAL_IOCTL_METHOD_BUFFERED,     \
						    			  EMCPAL_IOCTL_FILE_ANY_ACCESS)

#endif /* FBE_PHYSICAL_PACKAGE_H */
