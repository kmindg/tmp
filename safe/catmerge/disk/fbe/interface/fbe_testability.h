// File: fbe_testability.h

#ifndef FBE_TESTABILITY_H
#define FBE_TESTABILITY_H

//-----------------------------------------------------------------------------
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------
 
//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT:
//
//  This header file contains the testability hook defines and structures.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//


//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):

#define SCHEDULER_HOOK_SET   0x1
#define SCHEDULER_HOOK_UNSET 0x2


/*!*******************************************************************
 * @enum fbe_scheduler_hook_state_t
 *********************************************************************
 * @brief enum of scheduler hook states.
 *
 *********************************************************************/
typedef enum fbe_scheduler_hook_state_e
{
    SCHEDULER_MONITOR_STATE_INVALID = 0,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REMAP,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REBUILD,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_UNQUIESCE,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_CHANGE_CONFIG,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_EVENT,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_BACKGROUND_CONDITION,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_DOWNLOAD,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_DEGRADED,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_PACO,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_INCREASE,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_RESTORE,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_WEAR_LEVELING_QUERY,
    SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_ERRORS,

    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EOL,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD_PERMISSION,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION_IO,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VALIDATE_KEYS,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_INIT_VALIDATION_AREA,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_FAIL,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SWAP_PENDING,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_PASSIVE_REQUEST,
    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD,

    SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
    SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN,
    SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP,
    SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
    SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY,
    SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
    SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,

    SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_UPDATE,
    SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
    SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST,

    SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY,
    SCHEDULER_MONITOR_STATE_LUN_ZERO,
    SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
    SCHEDULER_MONITOR_STATE_LUN_CLEAN_DIRTY,

    SCHEDULER_MONITOR_STATE_BASE_CONFIG_OUT_OF_HIBERNATE,

    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT,

    SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,

    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT,

    SCHEDULER_MONITOR_STATE_BASE_CONFIG_UNQUIESCE,

    SCHEDULER_MONITOR_STATE_RAID_GROUP_NON_PAGED_INIT,

    SCHEDULER_MONITOR_STATE_LAST,
}
fbe_scheduler_hook_state_t;

/*!*******************************************************************
 * @enum fbe_scheduler_hook_substate_t
 *********************************************************************
 * @brief enum of scheduler hook sub states.
 * @note Some hook substates can only log, and not pause.
 * @todo Some log-only hooks are identified below, identify any others
 *
 *********************************************************************/
typedef enum fbe_scheduler_hook_substate_e
{
    FBE_SCHEDULER_MONITOR_SUB_STATE_INVALID = 0,

    FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
    FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED,
    FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_DOWNSTREAM_PERMISSION,
    FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION, /* NOTE: this substate is log-only, it cannot pause */
    FBE_RAID_GROUP_SUBSTATE_REBUILD_DOWNSTREAM_HIGHER_PRIORITY,
    FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION,  
    FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND,                   /* NOTE: this substate is log-only, it cannot pause */
    FBE_RAID_GROUP_SUBSTATE_REBUILD_COMPLETED,
    FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST,
    FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED,
    FBE_RAID_GROUP_SUBSTATE_JOIN_DONE,
    FBE_RAID_GROUP_SUBSTATE_JOIN_SYNCED,
    FBE_RAID_GROUP_SUBSTATE_QUIESCE_START,
    FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
    FBE_RAID_GROUP_SUBSTATE_UNQUIESCE_START,
    FBE_RAID_GROUP_SUBSTATE_UNQUIESCE_DONE,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START_CHK_DRIVE,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_ENTRY,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_RECONSTRUCT_PAGED_ACTIVATE,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_DOWNSTREAM_HIGHER_PRIORITY,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_NO_PERMISSION,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PIN_CLEAN,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_UNPIN_DIRTY,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_BUSY,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PEER_FLUSH_PAGED,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_BEFORE_INITIAL_KEY,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_KEYS_INCORRECT,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAUSED,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_UNPAUSED,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_SET_REKEY_COMPLETE,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_INVALIDATE,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_DONE,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_MOVE_CHKPT,
    FBE_RAID_GROUP_SUBSTATE_DOWNLOAD_START,
    FBE_RAID_GROUP_SUBSTATE_EMEH_DEGRADED_START,
    FBE_RAID_GROUP_SUBSTATE_EMEH_DEGRADED_DONE,
    FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_START,
    FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_DONE,
    FBE_RAID_GROUP_SUBSTATE_EMEH_INCREASE_START,
    FBE_RAID_GROUP_SUBSTATE_EMEH_INCREASE_DONE,
    FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_START,
    FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_DONE,
    FBE_RAID_GROUP_SUBSTATE_WEAR_LEVELING_START,

    FBE_PROVISION_DRIVE_SUBSTATE_JOIN_REQUEST,
    FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED,
    FBE_PROVISION_DRIVE_SUBSTATE_JOIN_DONE,
    FBE_PROVISION_DRIVE_SUBSTATE_JOIN_SYNCED,

    FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
    FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION,
    FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY,
    FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT,
    FBE_PROVISION_DRIVE_SUBSTATE_REMAP_EVENT,
    FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK,
    FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT,
    FBE_PROVISION_DRIVE_SUBSTATE_SET_EOL,
    FBE_PROVISION_DRIVE_SUBSTATE_CLEAR_EOL,
    FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_DISABLED,
    FBE_PROVISION_DRIVE_SUBSTATE_SCRUB_INTENT,
    FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN,
    FBE_PROVISION_DRIVE_SUBSTATE_DS_HEALTH_BROKEN_MD_UPDATED,
    FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_DONE,


    FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
    FBE_VIRTUAL_DRIVE_SUBSTATE_NEED_PROACTIVE_SPARE,
    FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
    FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
    FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET,
    FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
    FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
    FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE,
    FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_DENIED,
    FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED,
    FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE,
    FBE_VIRTUAL_DRIVE_SUBSTATE_SET_PASS_THRU_CONDITION,
    FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE,
    FBE_VIRTUAL_DRIVE_SUBSTATE_PASS_THRU_MODE_SET,
    FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
    FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_DISCONNECTED,
    FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE_SET_CHECKPOINT,
    FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_SET_CHECKPOINT,
    FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
    FBE_VIRTUAL_DRIVE_SUBSTATE_JOB_IN_PROGRESS,
    FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY,
    FBE_VIRTUAL_DRIVE_SUBSTATE_BROKEN,
    FBE_VIRTUAL_DRIVE_SUBSTATE_PAGED_METADATA_VERIFY_START,
    FBE_VIRTUAL_DRIVE_SUBSTATE_PAGED_METADATA_VERIFY_COMPLETE,

    FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
    FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,    
    FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE,
    FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2,
    FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN,
    FBE_RAID_GROUP_SUBSTATE_EVAL_RL_ALLOW_CONTINUE_BROKEN,
    FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST,
    FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED,
    FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_DONE,
    FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED2,
    FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST,
    FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED,
    FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,  
    FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN,
	FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT,
    FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED,
    FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR,        
    FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,      
    FBE_RAID_GROUP_SUBSTATE_START_METADATA_VERIFY,
    FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_INCOMPLETE_WRITE,
    FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_NEEDS_REBUILD,

    FBE_RAID_GROUP_SUBSTATE_PAGED_RECONSTRUCT_SETUP,
    FBE_RAID_GROUP_SUBSTATE_PAGED_RECONSTRUCT_START,
    FBE_RAID_GROUP_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE,

    FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION,
    FBE_RAID_GROUP_SUBSTATE_VERIFY_SEND,
    FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START,
    FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START,
    FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START,
    FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START,    
    FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START,
    FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED,
    FBE_RAID_GROUP_SUBSTATE_VERIFY_PAGED_COMPLETE,
    FBE_RAID_GROUP_SUBSTATE_VERIFY_ALL_VERIFIES_COMPLETE,

    FBE_RAID_GROUP_SUBSTATE_EVENT_COPY_BL_REFRESH_NEEDED,
    FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESH_NEEDED,
    FBE_RAID_GROUP_SUBSTATE_BACKGROUND_COND_BL_REFRESHED,

    FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_CHECK,

    FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST,
    FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_STARTED,
    FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED,
    FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST,
    FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_LOAD_FROM_DISK,

    FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED,
	FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED,
	FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION,
	FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY,

	FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
	FBE_PROVISION_DRIVE_SUBSTATE_UNCONSUMED_ZERO_SEND,
    FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION,

    FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED,
    FBE_LUN_SUBSTATE_ZERO_START,
    FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED,
    FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
    FBE_LUN_SUBSTATE_EVAL_ERR_ACTIVE_GRANTED,
    FBE_LUN_SUBSTATE_EVAL_ERR_PEER_GRANTED,
    FBE_LUN_SUBSTATE_EVAL_ERR_ACTIVE_DENIED,
    FBE_LUN_SUBSTATE_EVAL_ERR_PEER_DENIED,
    FBE_LUN_SUBSTATE_LAZY_CLEAN_START,

    FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START,
    FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE,
    FBE_RAID_GROUP_SUBSTATE_PENDING,
    FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_REQUEST,
    FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_STARTED,  
    FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_STARTED2,    
    FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_QUIESCE,
    FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_QUIESCE_ERROR,    
    FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_DONE,    
    FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_ACTIVE,
    FBE_RAID_GROUP_SUBSTATE_CHANGE_CONFIG_ERROR_PASSIVE,

    FBE_BASE_CONFIG_SUBSTATE_OUT_OF_HIBERNATION,

	FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE,

	FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_FIRST_CREATE,

    FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_ENTRY,
    FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_SLOTS_ALLOCATED,
    FBE_RAID_GROUP_SUBSTATE_JOURNAL_REMAP_DONE,

    FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_PERMISSION,

    FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_EDGES_NOT_READY,

    FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT,

    FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_SELECT,
    FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_POWERCYCLE,
    FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_FW_DOWNLOAD_DELAY,

    FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START,
    FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_INITIATE,
    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_REQUEST,
    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_SUCCESS,
    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_INCORRECT_KEYS,
    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_KEY_WRAP_ERROR,
    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_BAD_KEY_HANDLE,
    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_NOT_ENABLED,
    FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE,
    FBE_PROVISION_DRIVE_SUBSTATE_SET_SWAP_PENDING,
    FBE_PROVISION_DRIVE_SUBSTATE_SWAP_PENDING_SET,
    FBE_PROVISION_DRIVE_SUBSTATE_CLEAR_SWAP_PENDING,
    FBE_PROVISION_DRIVE_SUBSTATE_SWAP_PENDING_CLEARED,

    FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_RESET,

    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETING,
    FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY,

    FBE_RAID_GROUP_SUBSTATE_METADATA_VERIFY_SPECIALIZE,
    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_REGISTER_KEYS,
    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_UNREGISTER_KEYS,

    FBE_BASE_CONFIG_SUBSTATE_UNQUIESCE_METADATA_MEMORY_UPDATED,
    FBE_RAID_GROUP_SUBSTATE_NONPAGED_INIT,

    FBE_RAID_GROUP_SUBSTATE_CLEAR_ERRORS_ENTRY,
    /* Add new substates here.
     */
    
    FBE_SCHEDULER_MONITOR_SUB_STATE_LAST,
}
fbe_scheduler_hook_substate_t;

#define SCHEDULER_CHECK_STATES        0x1
#define SCHEDULER_CHECK_VALS_EQUAL    0x2
#define SCHEDULER_CHECK_VALS_LT       0x3
#define SCHEDULER_CHECK_VALS_GT       0x4

#define SCHEDULER_DEBUG_ACTION_LOG       0x1
#define SCHEDULER_DEBUG_ACTION_CONTINUE  0x2
#define SCHEDULER_DEBUG_ACTION_PAUSE     0x3
#define SCHEDULER_DEBUG_ACTION_CLEAR     0x4
#define SCHEDULER_DEBUG_ACTION_ONE_SHOT  0x5

//-----------------------------------------------------------------------------
//  ENUMERATIONS:

typedef enum fbe_scheduler_hook_status_e{
    FBE_SCHED_STATUS_OK = 0,
    FBE_SCHED_STATUS_DONE,
    FBE_SCHED_STATUS_ERROR
}fbe_scheduler_hook_status_t;

//-----------------------------------------------------------------------------
//  TYPEDEFS:
//

typedef fbe_scheduler_hook_status_t (* fbe_scheduler_debug_hook_function_t) (fbe_object_id_t object_id, fbe_u32_t monitor_state, fbe_u32_t monitor_substate, fbe_u64_t val1, fbe_u64_t val2);

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES:

#endif // FBE_TESTABILITY_H
