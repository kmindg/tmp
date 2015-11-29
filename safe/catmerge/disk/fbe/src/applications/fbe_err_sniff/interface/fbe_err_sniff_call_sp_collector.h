#ifndef __FBE_ERR_SNIFF_CALL_SP_COLLECTOR_H__
#define __FBE_ERR_SNIFF_CALL_SP_COLLECTOR_H__

#include "fbe/fbe_types.h"
fbe_status_t fbe_err_sniff_call_sp_collector(void);
fbe_thread_t * fbe_err_sniff_spcollect_get_handle(void);
fbe_bool_t fbe_err_sniff_spcollect_is_initialized(void);
fbe_bool_t fbe_err_sniff_get_spcollect_run(void);
void fbe_err_sniff_set_spcollect_run(fbe_bool_t spcollect_run);

#endif
