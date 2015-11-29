#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_neit_flare_package.h"

fbe_status_t
fbe_dest_api_init_record(fbe_dest_error_record_t * dest_error_record)
{
	fbe_zero_memory(dest_error_record, sizeof(fbe_dest_error_record_t));
	return FBE_STATUS_OK;
}