#ifndef FBE_BASE_PACKAGE_H
#define FBE_BASE_PACKAGE_H

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_package.h"
#include "fbe_base.h"
#include "fbe_trace.h"

void
fbe_base_package_trace(fbe_package_id_t package_id,
                       fbe_trace_level_t trace_level,
                       fbe_u32_t message_id,
                       const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,4,5)));
void
fbe_base_package_startup_trace(fbe_package_id_t package_id,
                               fbe_trace_level_t trace_level,
                               fbe_u32_t message_id,
                               const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,4,5)));
#endif /* FBE_BASE_PACKAGE_H */
