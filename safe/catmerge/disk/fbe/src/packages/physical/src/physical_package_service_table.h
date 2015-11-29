#ifndef PHYSICAL_PACKAGE_SERVICE_TABLE_H
#define PHYSICAL_PACKAGE_SERVICE_TABLE_H

extern fbe_service_methods_t fbe_topology_service_methods;
extern fbe_service_methods_t fbe_memory_service_methods;
extern fbe_service_methods_t fbe_notification_service_methods;
extern fbe_service_methods_t fbe_service_manager_service_methods;
extern fbe_service_methods_t fbe_scheduler_service_methods;
/* extern fbe_service_methods_t fbe_terminator_service_methods; */
/* extern fbe_service_methods_t fbe_transport_service_methods; */
extern fbe_service_methods_t fbe_event_log_service_methods;
extern fbe_service_methods_t fbe_drive_configuration_service_methods;
extern fbe_service_methods_t fbe_trace_service_methods;
extern fbe_service_methods_t fbe_traffic_trace_service_methods;
extern fbe_service_methods_t fbe_perfstats_service_methods;
extern fbe_service_methods_t fbe_envstats_service_methods;

/* Note we will compile different tables for different packages */
static const fbe_service_methods_t * physical_package_service_table[] = {   &fbe_topology_service_methods,
                                                                            &fbe_memory_service_methods,
                                                                            &fbe_notification_service_methods,
                                                                            &fbe_service_manager_service_methods,
                                                                            &fbe_scheduler_service_methods,
                                                                            /* &fbe_terminator_service_methods, */
                                                                            /* &fbe_transport_service_methods, */
                                                                            &fbe_event_log_service_methods,
                                                                            &fbe_drive_configuration_service_methods,
                                                                            &fbe_trace_service_methods,
                                                                            &fbe_traffic_trace_service_methods,
                                                                            &fbe_perfstats_service_methods,
									    &fbe_envstats_service_methods,
                                                                            NULL };

#endif /*PHYSICAL_PACKAGE_SERVICE_TABLE_H */
