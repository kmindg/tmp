 #ifndef FBE_ENVIRONMENT_LIMIT_PRIVATE_H
 #define FBE_ENVIRONMENT_LIMIT_PRIVATE_H

#include "csx_ext.h"
 #include "fbe/fbe_winddk.h"
 #include "fbe/fbe_types.h"
 #include "fbe/fbe_api_environment_limit_interface.h"
 
void fbe_environment_limit_trace(fbe_trace_level_t trace_level,
                                 fbe_trace_message_id_t message_id,
                                 const char * fmt, ...) __attribute__((format(__printf_func__,3,4)));
fbe_status_t fbe_environment_limit_init_limits(fbe_environment_limits_t *env_limits);
fbe_status_t fbe_environment_limit_clear_limits(void);


#endif /* FBE_ENVIRONMENT_LIMIT_PRIVATE_H */
