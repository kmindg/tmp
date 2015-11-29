#ifndef FBE_TERMINATOR_COMMON_H
#define FBE_TERMINATOR_COMMON_H

#include "csx_ext.h"
#include "fbe_trace.h"

#define TERMINATOR_MAX_IO_SIZE 2048 * 1024
/* In microseconds */
#define TERMINATOR_IO_WARNING_TIME 100000

void * fbe_terminator_allocate_memory (fbe_u32_t  NumberOfBytes);
void fbe_terminator_free_memory (void *  P);
void fbe_terminator_sleep (fbe_u32_t msec);

void terminator_trace(fbe_trace_level_t trace_level, fbe_trace_message_id_t message_id, const char * fmt, ...) __attribute__((format(__printf_func__,3,4)));
void terminator_trace_report(fbe_trace_level_t trace_level,
                             fbe_u32_t message_id,
                             const fbe_u8_t * fmt, 
                             va_list args);

#endif /*FBE_TERMINATOR_COMMON_H*/

