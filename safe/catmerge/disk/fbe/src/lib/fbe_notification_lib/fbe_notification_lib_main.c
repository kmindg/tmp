#include "fbe/fbe_types.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_notification_lib.h"


typedef struct fbe_state_to_bitmask_s{
	fbe_lifecycle_state_t		fbe_state;
	fbe_notification_type_t		notification_type;
}fbe_state_to_bitmask_t;

fbe_state_to_bitmask_t	state_to_bitmask_table [] =
{
	{FBE_LIFECYCLE_STATE_SPECIALIZE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE},
	{FBE_LIFECYCLE_STATE_ACTIVATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE},
	{FBE_LIFECYCLE_STATE_READY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY},
	{FBE_LIFECYCLE_STATE_HIBERNATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE},
	{FBE_LIFECYCLE_STATE_OFFLINE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_OFFLINE},
	{FBE_LIFECYCLE_STATE_FAIL, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL},
	{FBE_LIFECYCLE_STATE_DESTROY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY},
	{FBE_LIFECYCLE_STATE_PENDING_READY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_READY},
	{FBE_LIFECYCLE_STATE_PENDING_ACTIVATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_ACTIVATE},
	{FBE_LIFECYCLE_STATE_PENDING_HIBERNATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_HIBERNATE},
	{FBE_LIFECYCLE_STATE_PENDING_OFFLINE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_OFFLINE},
	{FBE_LIFECYCLE_STATE_PENDING_FAIL, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_FAIL},
	{FBE_LIFECYCLE_STATE_PENDING_DESTROY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY},
	{FBE_LIFECYCLE_STATE_INVALID, FBE_NOTIFICATION_TYPE_INVALID},
};

typedef struct fbe_package_to_bitmask_s{
	fbe_package_id_t						fbe_package;
	fbe_package_notification_id_mask_t		package_mask;
}fbe_package_to_bitmask_t;

fbe_package_to_bitmask_t	fbe_package_to_bits_table [] =
{
	{FBE_PACKAGE_ID_INVALID, FBE_PACKAGE_NOTIFICATION_ID_INVALID},
	{FBE_PACKAGE_ID_PHYSICAL, FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL},
	{FBE_PACKAGE_ID_NEIT, FBE_PACKAGE_NOTIFICATION_ID_NEIT},
	{FBE_PACKAGE_ID_SEP_0, FBE_PACKAGE_NOTIFICATION_ID_SEP_0},
	{FBE_PACKAGE_ID_ESP, FBE_PACKAGE_NOTIFICATION_ID_ESP},
	{FBE_PACKAGE_ID_KMS, FBE_PACKAGE_NOTIFICATION_ID_KMS},
	{FBE_PACKAGE_ID_LAST, FBE_PACKAGE_NOTIFICATION_ID_LAST},

};

fbe_status_t FBE_NOTIFICATION_CALL fbe_notification_convert_state_to_notification_type(fbe_lifecycle_state_t state, fbe_notification_type_t *notification_type)
{
	/*sanity check first*/
	if (state_to_bitmask_table[state].fbe_state != state){

		fbe_base_library_trace (FBE_LIBRARY_ID_LIFECYCLE,
					FBE_TRACE_LEVEL_ERROR,
					FBE_TRACE_MESSAGE_ID_INFO,
					"Can't match: state 0x%llX\n",
					(unsigned long long)state);

        return FBE_STATUS_GENERIC_FAILURE;
	}

	*notification_type = state_to_bitmask_table[state].notification_type;

	return FBE_STATUS_OK;

}

fbe_status_t FBE_NOTIFICATION_CALL fbe_notification_convert_notification_type_to_state(fbe_notification_type_t notification_type, fbe_lifecycle_state_t *state)
{
	fbe_state_to_bitmask_t *table = state_to_bitmask_table;
	fbe_u32_t				table_index = 0;

	while (notification_type != 0x1) {
		notification_type >>= 0x1;
		table_index++;
	}

	if (table_index >= FBE_LIFECYCLE_STATE_NOT_EXIST) {
		*state = FBE_LIFECYCLE_STATE_INVALID;
		return FBE_STATUS_GENERIC_FAILURE;
	}

	*state = table[table_index].fbe_state;

	return FBE_STATUS_OK;
}



fbe_status_t FBE_NOTIFICATION_CALL fbe_notification_convert_fbe_package_to_package_bitmap(fbe_package_id_t fbe_package, fbe_package_notification_id_mask_t *package_mask)
{
	/*sanity check first*/
	if (fbe_package_to_bits_table[fbe_package].fbe_package != fbe_package){

		fbe_base_library_trace (FBE_LIBRARY_ID_LIFECYCLE,
					FBE_TRACE_LEVEL_ERROR,
					FBE_TRACE_MESSAGE_ID_INFO,
					"Can't match: package 0x%llX\n",
					(unsigned long long)fbe_package);

        return FBE_STATUS_GENERIC_FAILURE;
	}

	*package_mask = fbe_package_to_bits_table[fbe_package].package_mask;

	return FBE_STATUS_OK;

}

fbe_status_t FBE_NOTIFICATION_CALL fbe_notification_convert_package_bitmap_to_fbe_package(fbe_package_notification_id_mask_t package_mask, fbe_package_id_t * fbe_package)
{
	fbe_package_to_bitmask_t *table = fbe_package_to_bits_table;

	while (table->fbe_package != FBE_PACKAGE_ID_LAST) {
		if (table->package_mask == package_mask) {
			*fbe_package = table->fbe_package;
			return FBE_STATUS_OK;

		}

		table++;
	}

	fbe_base_library_trace (FBE_LIBRARY_ID_LIFECYCLE,
				FBE_TRACE_LEVEL_ERROR,
				FBE_TRACE_MESSAGE_ID_INFO,
				"Can't match: package_mask 0x%llX\n",
				(unsigned long long)package_mask);

    return FBE_STATUS_GENERIC_FAILURE;
	
}

