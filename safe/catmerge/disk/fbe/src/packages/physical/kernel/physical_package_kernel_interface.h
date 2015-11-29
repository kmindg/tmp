#ifndef PHYSICAL_PACKAGE_KERNEL_INTERFACE_H
#define PHYSICAL_PACKAGE_KERNEL_INTERFACE_H

void physical_package_notification_init (void);
void physical_package_notification_destroy (void);
EMCPAL_STATUS physical_package_notification_register_application (PEMCPAL_IRP PIrp);
EMCPAL_STATUS physical_package_notification_get_next_event(PEMCPAL_IRP PIrp);
EMCPAL_STATUS physical_package_notification_unregister_application(PEMCPAL_IRP PIrp);

/* NTSTATUS physical_package_process_io_packet_ioctl(PIRP PIrp); */



#endif /* PHYSICAL_PACKAGE_KERNEL_INTERFACE_H */
