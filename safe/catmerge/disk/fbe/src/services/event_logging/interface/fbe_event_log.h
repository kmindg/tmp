#ifndef FBE_EVENT_LOG_H
#define FBE_EVENT_LOG_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_event_log_api.h"

fbe_status_t fbe_event_log_write_log(fbe_u32_t msg_id,
                                     fbe_u8_t *dump_data,
                                     fbe_u32_t dump_data_size,
                                     fbe_char_t *fmt, 
                                     va_list argList);

fbe_status_t fbe_event_log_destroy_log(void);
fbe_status_t fbe_event_log_init_log(void);
fbe_status_t fbe_event_log_clear_log(void);
fbe_status_t fbe_event_log_get_msg_count(fbe_u32_t msg_id, fbe_u32_t *count);
fbe_bool_t fbe_event_log_message_check(fbe_u32_t msg_id, fbe_u8_t  *msg);
fbe_status_t fbe_event_log_get_log(fbe_u32_t event_log_index,fbe_event_log_info_t *event_log_info);
fbe_u32_t  fbe_event_get_current_index(void);
fbe_bool_t  fbe_event_get_is_complete_buffer_in_use(void);

#endif /* FBE_EVENT_LOG_H */
