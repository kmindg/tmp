#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_neit_flare_package.h"

fbe_status_t
fbe_neit_api_init_record(fbe_neit_error_record_t * neit_error_record)
{
	fbe_zero_memory(neit_error_record, sizeof(fbe_neit_error_record_t));
	return FBE_STATUS_OK;
}