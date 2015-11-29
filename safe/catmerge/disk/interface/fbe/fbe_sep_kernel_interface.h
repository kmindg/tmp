#ifndef FBE_SEP_KERNEL_INTERFACE_H
#define FBE_SEP_KERNEL_INTERFACE_H

#include <ntddk.h>
#include "EmcPAL_DriverShell.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_notification_interface.h"

fbe_status_t sep_get_driver_object_ptr(PEMCPAL_NATIVE_DRIVER_OBJECT *driver_ptr);

#endif /*FBE_SEP_KERNEL_INTERFACE_H*/
