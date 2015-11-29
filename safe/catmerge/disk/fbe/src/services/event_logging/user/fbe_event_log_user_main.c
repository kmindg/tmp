#include "fbe_event_log.h"

fbe_status_t fbe_event_log_write_log(fbe_u32_t msg_id,
                                     fbe_u8_t *dump_data,
                                     fbe_u32_t dump_data_size,
                                     fbe_char_t *fmt,  
                                     va_list argList)
{
    return(FBE_STATUS_OK);
}

fbe_status_t fbe_event_log_init_log()
{
    return FBE_STATUS_OK;
}

fbe_status_t fbe_event_log_destroy_log()
{
    return FBE_STATUS_OK;
}

fbe_status_t fbe_event_log_clear_log()
{
    return FBE_STATUS_OK;
}

fbe_status_t fbe_event_log_get_msg_count(fbe_u32_t msg_id, fbe_u32_t *count)
{
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_event_log_message_check(fbe_u32_t msg_id, fbe_u8_t  *msg)
{
    return FBE_TRUE;
}
fbe_status_t fbe_event_log_get_log(fbe_u32_t event_log_index,fbe_event_log_info_t *event_log_info)
{
    return FBE_TRUE;
}
fbe_u32_t  fbe_event_get_current_index()
{
     return FBE_UNSIGNED_MINUS_1;
}
fbe_bool_t  fbe_event_get_is_complete_buffer_in_use()
{
     return FBE_TRUE;
}