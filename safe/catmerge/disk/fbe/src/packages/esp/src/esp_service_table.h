#ifndef ESP_SERVICE_TABLE_H
#define ESP_SERVICE_TABLE_H

extern fbe_service_methods_t fbe_memory_service_methods;
extern fbe_service_methods_t fbe_notification_service_methods;
extern fbe_service_methods_t fbe_service_manager_service_methods;
extern fbe_service_methods_t fbe_scheduler_service_methods;
extern fbe_service_methods_t fbe_event_log_service_methods;
extern fbe_service_methods_t fbe_topology_service_methods;
extern fbe_service_methods_t fbe_event_service_methods;
extern fbe_service_methods_t fbe_trace_service_methods;
extern fbe_service_methods_t fbe_cmi_service_methods;
extern fbe_service_methods_t fbe_eir_service_methods;
extern fbe_service_methods_t fbe_environment_limit_service_methods;

/* Note we will compile different tables for different packages */
static const fbe_service_methods_t * esp_service_table[] CSX_MAYBE_UNUSED = {&fbe_memory_service_methods, 
                                                            &fbe_notification_service_methods,
                                                            &fbe_service_manager_service_methods,
                                                            &fbe_scheduler_service_methods,
                                                            &fbe_event_log_service_methods,
                                                            &fbe_topology_service_methods,
                                                            &fbe_event_service_methods,
                                                            &fbe_trace_service_methods,
                                                            &fbe_cmi_service_methods,
                                                            &fbe_eir_service_methods,
                                                            &fbe_environment_limit_service_methods,
                                                            NULL };

#endif /*ESP_SERVICE_TABLE_H */

