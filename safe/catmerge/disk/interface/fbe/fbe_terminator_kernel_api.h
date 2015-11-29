#ifndef FBE_TERMINATOR_KERNEL_API_H
#define FBE_TERMINATOR_KERNEL_API_H

#include <ntddk.h>
#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator_miniport_interface.h"

#define FBE_TERMINATOR_DEVICE_NAME "\\Device\\Terminator"
#define FBE_TERMINATOR_DEVICE_LINK "\\DosDevices\\TerminatorLink"

#define FBE_TERMINATOR_DEVICE_TYPE EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN

#define FBE_TERMINATOR_GET_INTERFACE 0xD00

#define FBE_TERMINATOR_GET_INTERFACE_IOCTL \
            EMCPAL_IOCTL_CTL_CODE(FBE_TERMINATOR_DEVICE_TYPE,  \
            FBE_TERMINATOR_GET_INTERFACE,     \
            EMCPAL_IOCTL_METHOD_BUFFERED,     \
            EMCPAL_IOCTL_FILE_ANY_ACCESS)

typedef struct fbe_terminator_api_s
{
    fbe_status_t (*init) (void);
} fbe_terminator_api_t;

typedef struct fbe_terminator_get_interface_s
{
    fbe_terminator_api_t terminator_api;
    fbe_terminator_miniport_interface_port_shim_sim_pointers_t miniport_api;
} fbe_terminator_get_interface_t;


#endif /*FBE_TERMINATOR_KERNEL_API_H*/
