#ifndef FBE_KMS_H
#define FBE_KMS_H

#include "fbe/fbe_types.h"

#define FBE_KMS_DEVICE_NAME "\\Device\\kms"
#define FBE_KMS_DEVICE_NAME_CHAR "\\Device\\kms"
#define FBE_KMS_DEVICE_LINK "\\DosDevices\\kmsLink"

#define FBE_KMS_DEVICE_TYPE EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN

/* Values in the range 0x000 to 0x7ff are reserved by Microsoft for public I/O control codes. */
#define FBE_KMS_MGMT_PACKET                        0x1001
#define FBE_KMS_GET_MGMT_ENTRY                     0x1002
#define FBE_KMS_REGISTER_APP_NOTIFICATION                   0x1003
#define FBE_KMS_NOTIFICATION_GET_EVENT                      0x1004
#define FBE_KMS_UNREGISTER_APP_NOTIFICATION             0x1005

#define FBE_KMS_MGMT_PACKET_IOCTL   EMCPAL_IOCTL_CTL_CODE(	FBE_KMS_DEVICE_TYPE,  \
												FBE_KMS_MGMT_PACKET,     \
												EMCPAL_IOCTL_METHOD_BUFFERED,     \
												EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_KMS_REGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_KMS_DEVICE_TYPE,  \
                                                                        FBE_KMS_REGISTER_APP_NOTIFICATION,     \
                                                                        EMCPAL_IOCTL_METHOD_BUFFERED,     \
                                                                        EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_KMS_NOTIFICATION_GET_EVENT_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_KMS_DEVICE_TYPE,  \
                                                                        FBE_KMS_NOTIFICATION_GET_EVENT,     \
                                                                        EMCPAL_IOCTL_METHOD_BUFFERED,     \
                                                                        EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define FBE_KMS_UNREGISTER_APP_NOTIFICATION_IOCTL   EMCPAL_IOCTL_CTL_CODE(FBE_KMS_DEVICE_TYPE,  \
                                                                          FBE_KMS_UNREGISTER_APP_NOTIFICATION,     \
                                                                          EMCPAL_IOCTL_METHOD_BUFFERED,     \
                                                                          EMCPAL_IOCTL_FILE_ANY_ACCESS)


#define FBE_KMS_GET_MGMT_ENTRY_IOCTL   EMCPAL_IOCTL_CTL_CODE( FBE_KMS_DEVICE_TYPE,  \
												FBE_KMS_GET_MGMT_ENTRY,     \
												EMCPAL_IOCTL_METHOD_BUFFERED,     \
												EMCPAL_IOCTL_FILE_ANY_ACCESS)

typedef enum
{
    FBE_KMS_CONTROL_CODE_INVALID = 0,

    FBE_KMS_CONTROL_CODE_SEND_COMMAND,

    /* Insert new control codes here. */

    FBE_KMS_CONTROL_CODE_LAST
}
fbe_kms_control_code_t;

#endif /* FBE_KMS_H */
