#ifndef FBE_DRIVER_NOTIFICATION_INTERFACE_H
#define FBE_DRIVER_NOTIFICATION_INTERFACE_H

#include "fbe/fbe_package.h"
#include "fbe/fbe_notification_interface.h"

void fbe_driver_notification_init (fbe_package_id_t package_id);
void fbe_driver_notification_destroy (void);
EMCPAL_STATUS fbe_driver_notification_register_application (PEMCPAL_IRP PIrp);
EMCPAL_STATUS fbe_driver_notification_get_next_event(PEMCPAL_IRP PIrp);
EMCPAL_STATUS fbe_driver_notification_unregister_application(PEMCPAL_IRP PIrp);

#endif /* FBE_DRIVER_NOTIFICATION_INTERFACE_H */
