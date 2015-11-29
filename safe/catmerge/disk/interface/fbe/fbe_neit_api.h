#ifndef FBE_NEIT_API_H
#define FBE_NEIT_API_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_neit.h"

fbe_status_t fbe_neit_api_init (void);
fbe_status_t fbe_neit_api_destroy (void);
fbe_status_t fbe_neit_api_init_record(fbe_neit_error_record_t * neit_error_record);
fbe_status_t fbe_neit_api_add_record(fbe_neit_error_record_t * neit_error_record, fbe_neit_record_handle_t * neit_record_handle);
fbe_status_t fbe_neit_api_remove_record(fbe_neit_record_handle_t neit_record_handle);
fbe_status_t fbe_neit_api_remove_object(fbe_object_id_t object_id);
fbe_status_t fbe_neit_api_start(void);
fbe_status_t fbe_neit_api_stop(void);
fbe_status_t fbe_neit_api_get_record(fbe_neit_record_handle_t neit_record_handle, fbe_neit_error_record_t * neit_error_record);

#endif /*FBE_NEIT_API_H */
