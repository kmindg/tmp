/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                bgsl_service_manager.h
 ***************************************************************************/

#ifndef BGSL_SERVICE_MANAGER_H
#define BGSL_SERVICE_MANAGER_H

#include "fbe/bgsl_types.h"
#include "bgsl_base_service.h"

/* MUST be called before BGSL_BASE_SERVICE_CONTROL_CODE_INIT packet */
bgsl_status_t bgsl_service_manager_init_service_table(const bgsl_service_methods_t * service_table[], bgsl_package_id_t package_id);

bgsl_status_t bgsl_service_manager_send_control_packet(bgsl_packet_t * packet);

/* uSimCtrl instrumentation code */
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
typedef const bgsl_service_methods_t ** (*usimctrl_transform_service_table_ptr_t)(const bgsl_service_methods_t * service_table[], bgsl_package_id_t package_id);

/* Setter of function ptr for that performs service table transformation */
bgsl_status_t usimctrl_transform_service_table_ptr_set(const usimctrl_transform_service_table_ptr_t ptr);
#endif

#endif /* BGSL_SERVICE_MANAGER_H */