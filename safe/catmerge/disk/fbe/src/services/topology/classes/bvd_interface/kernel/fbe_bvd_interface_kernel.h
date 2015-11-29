#ifndef BVD_INTERFACE_KERNEL_H
#define BVD_INTERFACE_KERNEL_H

#include <ntddk.h>
#include "fbe/fbe_lun.h"
#include "fbe/fbe_types.h"

#define FBE_BVD_LUN_DEVICE_TYPE EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN

typedef struct bvd_lun_device_exnention_s{
    PEMCPAL_DEVICE_OBJECT   device_object;
    fbe_object_id_t     bvd_object_id;
    fbe_char_t          cDevName[128];
    fbe_char_t          cDevLinkName[128];
}bvd_lun_device_exnention_t;


#endif /*BVD_INTERFACE_KERNEL_H*/
