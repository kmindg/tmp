#ifndef TEMINATOR_DRIVE_SAS_WRITE_BUFFER_TIMER_H
#define TEMINATOR_DRIVE_SAS_WRITE_BUFFER_TIMER_H

#include "fbe_terminator.h"
#include "fbe/fbe_time.h"

typedef struct write_buffer_timer_context_s
{
    fbe_terminator_io_t             *terminator_io;        
} write_buffer_timer_context_t;

typedef struct terminator_sas_drive_write_buffer_timer_s {
    fbe_queue_element_t          queue_element;
    write_buffer_timer_context_t context;
	fbe_time_t                   expiration_time;
}terminator_sas_drive_write_buffer_timer_t;

fbe_status_t terminator_sas_drive_write_buffer_timer_init(void);
fbe_status_t terminator_sas_drive_write_buffer_timer_destroy(void);
fbe_status_t terminator_sas_drive_write_buffer_set_timer(fbe_time_t timeout_msec, write_buffer_timer_context_t context);
void sas_drive_process_payload_write_buffer_timer_func(write_buffer_timer_context_t *context);

#endif
