#ifndef FBE_DEST_PRIVATE_H
#define FBE_DEST_PRIVATE_H

#include "csx_ext.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"

#define FBE_DEST_INVALID 0xFFFFFFFF


typedef struct fbe_dest_timer_s {
	fbe_queue_element_t		queue_element;
    fbe_packet_t            *packet;
	/* expiration_time is number of milliseconds since the system booted */
	fbe_time_t				expiration_time;
}fbe_dest_timer_t;

/* fbe_dest_init.c */
void dest_log_error(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));
void dest_log_info(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));
void dest_log_debug(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));

/* fbe_dest_timer.c */
fbe_status_t    fbe_dest_timer_init(void);
fbe_status_t    fbe_dest_timer_destroy(void);
fbe_status_t    fbe_dest_set_timer(fbe_packet_t * packet,                                                                     
                                   fbe_time_t timeout_msec,
                                   fbe_packet_completion_function_t completion_function,
                                   fbe_packet_completion_context_t completion_context);
fbe_status_t    fbe_dest_cancel_timer(fbe_dest_timer_t * timer);

#endif /*FBE_DEST_PRIVATE_H */
