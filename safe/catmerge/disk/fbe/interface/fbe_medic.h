#ifndef FBE_MEDIC_H
#define FBE_MEDIC_H
#include "fbe/fbe_types.h"

/*!*******************************************************************
 * @enum fbe_medic_action_t
 *********************************************************************
 * @brief The order in the enum is the order of the relative priority
 *        of these background operations.
 *
 * @note The priority order of verifies must be kept consistent with 
 * fbe_raid_group_verify_get_next_verify_type().
 *
 *********************************************************************/
typedef enum fbe_medic_action_e {
    FBE_MEDIC_ACTION_IDLE = 0, /*!< This means no background action is being taken by the object. */
    FBE_MEDIC_ACTION_SNIFF,
    FBE_MEDIC_ACTION_RO_VERIFY,
    FBE_MEDIC_ACTION_USER_VERIFY,
    FBE_MEDIC_ACTION_ZERO,
    FBE_MEDIC_ACTION_ENCRYPTION_REKEY,
    FBE_MEDIC_ACTION_SYSTEM_VERIFY,  /* See note above before changing verify order. */
    FBE_MEDIC_ACTION_ERROR_VERIFY,    
    FBE_MEDIC_ACTION_INCOMPLETE_WRITE_VERIFY,    
    FBE_MEDIC_ACTION_METADATA_VERIFY,
    FBE_MEDIC_ACTION_COPY,
    FBE_MEDIC_ACTION_REBUILD,
    FBE_MEDIC_ACTION_REPLACEMENT,
    FBE_MEDIC_ACTION_DEGRADED,
    FBE_MEDIC_ACTION_HIGHEST_PRIORITY, /*!< Used by events that do not wish to take background ops into consideration. */

    FBE_MEDIC_ACTION_LAST
}fbe_medic_action_t;

typedef fbe_u32_t fbe_medic_action_priority_t;

fbe_status_t fbe_medic_get_action_priority(fbe_medic_action_t medic_action, fbe_medic_action_priority_t * medic_action_priority);
fbe_medic_action_priority_t fbe_medic_get_highest_priority(fbe_medic_action_priority_t first, fbe_medic_action_priority_t second);

#endif /* FBE_MEDIC_H */
