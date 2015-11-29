#ifndef NEIT_PACKAGE_SERVICE_TABLE_H
#define NEIT_PACKAGE_SERVICE_TABLE_H

extern fbe_service_methods_t fbe_service_manager_service_methods;
extern fbe_service_methods_t fbe_memory_service_methods;
extern fbe_service_methods_t fbe_protocol_error_injection_service_methods;
extern fbe_service_methods_t fbe_logical_error_injection_service_methods;
extern fbe_service_methods_t fbe_rdgen_service_methods;
extern fbe_service_methods_t fbe_trace_service_methods;
extern fbe_service_methods_t fbe_cmi_service_methods;
extern fbe_service_methods_t fbe_raw_mirror_service_methods;
/*extern fbe_service_methods_t fbe_cms_exerciser_service_methods;*/
extern fbe_service_methods_t fbe_notification_service_methods;
extern fbe_service_methods_t fbe_dest_service_methods;


/* Note we will compile different tables for different packages */
static const fbe_service_methods_t * neit_package_service_table[] = {&fbe_service_manager_service_methods,
																	 &fbe_memory_service_methods,
																	 &fbe_protocol_error_injection_service_methods, 
																	 &fbe_logical_error_injection_service_methods,
                                                                     &fbe_rdgen_service_methods,
                                                                     &fbe_trace_service_methods,
                                                                     &fbe_cmi_service_methods,
                                                                     &fbe_raw_mirror_service_methods,
																	 /*&fbe_cms_exerciser_service_methods,*/
																	 &fbe_notification_service_methods,
																	 &fbe_dest_service_methods,
                                                                     NULL};

#endif /*NEIT_PACKAGE_SERVICE_TABLE_H */
