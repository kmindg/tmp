#ifndef FBE_CMS_EXERCIZER_INTERFACE_H
#define FBE_CMS_EXERCIZER_INTERFACE_H

#include "fbe/fbe_service.h"

typedef enum fbe_traffic_trace_control_code_e {
    FBE_CMS_EXERCISER_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_CMS_EXERCISER),
    FBE_CMS_EXERCISER_CONTROL_CODE_EXCLUSIVE_ALLOC_TEST, 
	FBE_CMS_EXERCISER_CONTROL_CODE_GET_INTERFACE_TEST_RESULTS,
    FBE_TRAFFIC_TRACE_CONTROL_CODE_LAST
} fbe_traffic_trace_control_code_t;

/*FBE_CMS_EXERCISER_CONTROL_CODE_EXCLUSIVE_ALLOC_TEST*/
typedef struct fbe_cms_exerciser_exclusive_alloc_test_s{
	fbe_u64_t					threads_per_cpu;
	fbe_cpu_affinity_mask_t		cpu_mask;
	fbe_u32_t					allocations_per_thread;
	fbe_u32_t					msec_delay_between_allocations;
}fbe_cms_exerciser_exclusive_alloc_test_t;

/*FBE_CMS_EXERCISER_CONTROL_CODE_GET_INTERFACE_TEST_RESULTS*/
typedef struct fbe_cms_exerciser_get_interface_test_results_s{
	fbe_bool_t	passed;
}fbe_cms_exerciser_get_interface_test_results_t;


#endif /* FBE_CMS_EXERCIZER_INTERFACE_H */


