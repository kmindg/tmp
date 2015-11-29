#ifndef FBE_SEP_SHIM_KERNEL_INTERFACE_H
#define FBE_SEP_SHIM_KERNEL_INTERFACE_H

#include "fbe/fbe_types.h"
#include <ntddk.h>

EMCPAL_STATUS fbe_sep_shim_kernel_process_ioctl_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp);
EMCPAL_STATUS fbe_sep_shim_kernel_process_internal_ioctl_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP PIrp);

EMCPAL_STATUS fbe_sep_shim_kernel_process_read_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp);
EMCPAL_STATUS fbe_sep_shim_kernel_process_write_irp(PEMCPAL_DEVICE_OBJECT  PDeviceObject,PEMCPAL_IRP	PIrp);


#endif /*FBE_SEP_SHIM_KERNEL_INTERFACE_H*/
