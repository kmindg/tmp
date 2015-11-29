#ifndef PHYSICAL_PACKAGE_SIMULATOR_SERVICE_TABLE_H
#define PHYSICAL_PACKAGE_SIMULATOR_SERVICE_TABLE_H

#include "fbe/fbe_service.h"

extern fbe_service_methods_t fbe_notification_service_methods;
extern fbe_service_methods_t fbe_topology_service_simulator_methods;
extern fbe_service_methods_t fbe_service_manager_service_methods;

/* Note we will compile different tables for different packages */
static const fbe_service_methods_t * physical_package_simulator_service_table[] = {&fbe_notification_service_methods,
																				   &fbe_topology_service_simulator_methods,
																				   &fbe_service_manager_service_methods,
																				   NULL };

#endif /*PHYSICAL_PACKAGE_SIMULATOR_SERVICE_TABLE_H */
