#ifndef CACHE_TO_MCR_TRANSPORT_IO_INTERFACE_H
#define CACHE_TO_MCR_TRANSPORT_IO_INTERFACE_H

# include "simulation/VirtualFlareDevice.h"
# include "simulation/VirtualFlareDriver.h"
# include "flare_ioctls.h"
#include "fbe/fbe_types.h"
#include "simulation/cache_to_mcr_transport.h"


EMCPAL_STATUS CACHE_TO_MCR_PUBLIC cache_to_mcr_transport_send_irp(PBasicIoRequest irp, cache_to_mcr_lu_info_t * mcr_lu_info);
EMCPAL_STATUS CACHE_TO_MCR_PUBLIC cache_to_mcr_transport_send_create_device(PBasicIoRequest irp,
																	   const char *deviceName,
																	   get_dev_info_completion p,
																	   void * context);

EMCPAL_STATUS CACHE_TO_MCR_PUBLIC cache_to_mcr_transport_create_lun(const char *path, DiskIdentity *config);
EMCPAL_STATUS CACHE_TO_MCR_PUBLIC cache_to_mcr_transport_destroy_lun(const char *path, DiskIdentity *config);


#endif /*CACHE_TO_MCR_TRANSPORT_IO_INTERFACE_H*/
