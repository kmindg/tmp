
#ifndef FBE_PERFSTATS_PRIVATE_H
#define FBE_PERFSTATS_PRIVATE_H

#include "fbe/fbe_types.h"
#include "fbe_cmi.h"
#include "fbe/fbe_trace_interface.h"

#define	FBE_PERFSTATS_VERSION					1
void fbe_perfstats_trace(fbe_trace_level_t trace_level, const char * fmt,  ...) __attribute__ ((format(__printf_func__, 2, 3)));
fbe_status_t perfstats_allocate_perfstats_memory(fbe_package_id_t package_id, void **container_p);
fbe_status_t perfstats_deallocate_perfstats_memory(fbe_package_id_t package_id, void **container_p);
fbe_status_t perfstats_get_mapped_stats_container(fbe_u64_t *user_va);

#endif /*FBE_PERFSTATS_PRIVATE_H*/
