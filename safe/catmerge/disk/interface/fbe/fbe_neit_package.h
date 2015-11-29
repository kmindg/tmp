#ifndef FBE_NEIT_PACKAGE_H
#define FBE_NEIT_PACKAGE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_neit.h"

#define FBE_NEIT_PACKAGE_DEVICE_NAME "\\Device\\NeitPackage"
#define FBE_NEIT_PACKAGE_DEVICE_NAME_CHAR "\\Device\\NeitPackage"
#define FBE_NEIT_PACKAGE_DEVICE_LINK "\\DosDevices\\NeitPackageLink"

#define FBE_NEIT_PACKAGE_DEVICE_TYPE EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN

fbe_status_t fbe_api_neit_package_common_init(void);
fbe_status_t fbe_api_neit_package_common_destroy(void);

//fbe_status_t neit_package_init(fbe_neit_package_load_params_t *params_p);
fbe_status_t neit_package_destroy(void);
fbe_status_t neit_package_get_record(fbe_neit_record_handle_t neit_record_handle, fbe_neit_error_record_t * neit_error_record);
fbe_status_t neit_package_get_control_entry(fbe_service_control_entry_t * service_control_entry);
fbe_status_t neit_package_disable(void);
/* Values in the range 0x000 to 0x7ff are reserved by Microsoft for public I/O control codes. */
#define FBE_NEIT_MGMT_PACKET                        0x901
#define FBE_NEIT_GET_MGMT_ENTRY                     0x902
#define FBE_NEIT_REGISTER_APP_NOTIFICATION                   0x903
#define FBE_NEIT_NOTIFICATION_GET_EVENT                      0x904
#define FBE_NEIT_UNREGISTER_APP_NOTIFICATION             0x905

#define FBE_NEIT_MGMT_PACKET_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
												FBE_NEIT_MGMT_PACKET,     \
												EMCPAL_IOCTL_METHOD_BUFFERED,     \
												EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_NEIT_REGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
                                                                        FBE_NEIT_REGISTER_APP_NOTIFICATION,     \
                                                                        EMCPAL_IOCTL_METHOD_BUFFERED,     \
                                                                        EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_NEIT_NOTIFICATION_GET_EVENT_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
                                                                        FBE_NEIT_NOTIFICATION_GET_EVENT,     \
                                                                        EMCPAL_IOCTL_METHOD_BUFFERED,     \
                                                                        EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_NEIT_UNREGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
                                                                          FBE_NEIT_UNREGISTER_APP_NOTIFICATION,     \
                                                                          EMCPAL_IOCTL_METHOD_BUFFERED,     \
                                                                          EMCPAL_IOCTL_FILE_ANY_ACCESS)


#define FBE_NEIT_GET_MGMT_ENTRY_IOCTL   EMCPAL_IOCTL_CTL_CODE( FBE_NEIT_PACKAGE_DEVICE_TYPE,  \
												FBE_NEIT_GET_MGMT_ENTRY,     \
												EMCPAL_IOCTL_METHOD_BUFFERED,     \
												EMCPAL_IOCTL_FILE_ANY_ACCESS)
#endif /* FBE_NEIT_PACKAGE_H */
