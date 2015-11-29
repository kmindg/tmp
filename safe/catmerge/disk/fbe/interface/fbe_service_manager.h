/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                fbe_service_manager.h
 ***************************************************************************/

#ifndef FBE_SERVICE_MANAGER_H
#define FBE_SERVICE_MANAGER_H

#include "fbe/fbe_types.h"
#include "fbe_base_service.h"
#include "fbe/fbe_service_manager_interface.h"

/* MUST be called before FBE_BASE_SERVICE_CONTROL_CODE_INIT packet */
fbe_status_t fbe_service_manager_init_service_table(const fbe_service_methods_t * service_table[]);

#if 0/*FBE3, sharel: moved outside so it's visible to the shim that needs to use new FBE API to set this entry
	this can be moved back here once we take out Flare*/
fbe_status_t fbe_service_manager_send_control_packet(fbe_packet_t * packet);
#endif /*FBE3*/

#endif /* FBE_SERVICE_MANAGER_H */