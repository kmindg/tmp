#include "fbe_medic.h"

typedef struct fbe_medic_table_entry_s {
	fbe_medic_action_priority_t medic_action_priority;
	fbe_medic_action_t			medic_action;
}fbe_medic_table_entry_t;

fbe_status_t
fbe_medic_get_action_priority(fbe_medic_action_t medic_action, fbe_medic_action_priority_t * medic_action_priority)
{
	* medic_action_priority = medic_action;

	if(*medic_action_priority < FBE_MEDIC_ACTION_LAST){
		return FBE_STATUS_OK;
	} else {
		return FBE_STATUS_GENERIC_FAILURE;
	}
}

fbe_medic_action_priority_t fbe_medic_get_highest_priority(fbe_medic_action_priority_t first, fbe_medic_action_priority_t second)
{
	return ((first >= second) ? first : second);
}
