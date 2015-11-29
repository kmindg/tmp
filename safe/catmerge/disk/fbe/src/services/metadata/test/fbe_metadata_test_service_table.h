#ifndef FBE_METADATA_TEST_SERVICE_TABLE_H
#define FBE_METADATA_TEST_SERVICE_TABLE_H

extern fbe_service_methods_t fbe_topology_service_methods;
extern fbe_service_methods_t fbe_memory_service_methods;
extern fbe_service_methods_t fbe_service_manager_service_methods;
extern fbe_service_methods_t fbe_metadata_service_methods;
extern fbe_service_methods_t fbe_trace_service_methods;
extern fbe_service_methods_t fbe_cmi_service_methods;


/* Note we will compile different tables for different packages */
static const fbe_service_methods_t * metadata_test_service_table[] = {&fbe_topology_service_methods,
															&fbe_memory_service_methods,															
															&fbe_service_manager_service_methods,															
															&fbe_metadata_service_methods,
															&fbe_trace_service_methods,
															&fbe_cmi_service_methods,
															NULL };

#endif /* FBE_METADATA_TEST_SERVICE_TABLE_H */
