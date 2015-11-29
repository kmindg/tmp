#ifndef SEP_SERVICE_TABLE_H
#define SEP_SERVICE_TABLE_H

extern fbe_service_methods_t fbe_topology_service_methods;
extern fbe_service_methods_t fbe_memory_service_methods;
extern fbe_service_methods_t fbe_notification_service_methods;
extern fbe_service_methods_t fbe_service_manager_service_methods;
extern fbe_service_methods_t fbe_scheduler_service_methods;
extern fbe_service_methods_t fbe_event_log_service_methods;
extern fbe_service_methods_t fbe_metadata_service_methods;
extern fbe_service_methods_t fbe_trace_service_methods;
extern fbe_service_methods_t fbe_traffic_trace_service_methods;
extern fbe_service_methods_t fbe_sector_trace_service_methods;
extern fbe_service_methods_t fbe_job_service_methods;
extern fbe_service_methods_t fbe_event_service_methods;
extern fbe_service_methods_t fbe_cmi_service_methods;
extern fbe_service_methods_t fbe_persist_service_methods;
extern fbe_service_methods_t fbe_database_service_methods;
extern fbe_service_methods_t fbe_environment_limit_service_methods;
extern fbe_service_methods_t fbe_perfstats_service_methods;

/* Note we will compile different tables for different packages */
static const fbe_service_methods_t * sep_service_table[] = {&fbe_topology_service_methods,
                                                            &fbe_memory_service_methods,
                                                            &fbe_notification_service_methods,
                                                            &fbe_service_manager_service_methods,
                                                            &fbe_scheduler_service_methods,
                                                            &fbe_event_log_service_methods,
                                                            &fbe_metadata_service_methods,
                                                            &fbe_trace_service_methods,
                                                            &fbe_traffic_trace_service_methods,
                                                            &fbe_sector_trace_service_methods,
                                                            &fbe_job_service_methods,
                                                            &fbe_event_service_methods,
                                                            &fbe_cmi_service_methods,
                                                            &fbe_persist_service_methods,                                                            
                                                            &fbe_database_service_methods,
															&fbe_environment_limit_service_methods,
                                                            &fbe_perfstats_service_methods,
                                                            NULL };

#endif /*SEP_SERVICE_TABLE_H */
