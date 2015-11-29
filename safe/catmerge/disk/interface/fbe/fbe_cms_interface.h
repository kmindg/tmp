#ifndef FBE_CMS_INTERFACE_H
#define FBE_CMS_INTERFACE_H

/***************************************************************************/
/** @file fbe_cms_interface.h
***************************************************************************
*
* @brief
*  This file contains definitions of functions that are exported by the 
*  persistance service for use by external clients
* 
***************************************************************************/
#include "fbe_cms_lib.h"

/*State machine related stuff*/
#define FBE_CMS_MAX_HISTORY_TABLE_ENTRIES 0xf

enum fbe_cms_states_e{
    FBE_CMS_STATE_INIT = 0,
    FBE_CMS_STATE_DIRTY,
	FBE_CMS_STATE_CLEAN,
	FBE_CMS_STATE_CLEAN_CDCA,
	FBE_CMS_STATE_CLEANING,
	FBE_CMS_STATE_QUIESCING,
	FBE_CMS_STATE_QUIESCED,
	FBE_CMS_STATE_VAULT_LOAD,
	FBE_CMS_STATE_PEER_SYNC,
	FBE_CMS_STATE_DUMPING,
	FBE_CMS_STATE_DUMPED,
	FBE_CMS_STATE_FAILED,
    FBE_CMS_STATE_UNKNOWN,
};

typedef fbe_u64_t fbe_cms_state_t;

enum fbe_cms_events_e{
	FBE_CMS_EVENT_STARTUP = 0,           // CMS state machine has been initialized
    FBE_CMS_EVENT_CLEAN,            // CMS has become clean
    FBE_CMS_EVENT_DIRTY,            // CMS has become dirty
    FBE_CMS_EVENT_QUIESCE_COMPLETE,       // The CMS has finished quiescing
    FBE_CMS_EVENT_PEER_SYNC_COMPLETE,      // The cache has finished synching with the peer
    FBE_CMS_EVENT_LOCAL_VAULT_STATE_READY,       // The vault has transitioned to the ready state
    FBE_CMS_EVENT_LOCAL_VAULT_STATE_NOT_READY,    // The vault has transitioned out of the ready state
    FBE_CMS_EVENT_VAULT_DUMP_SUCCESS,      // CMS successfully dumped
    FBE_CMS_EVENT_VAULT_DUMP_FAIL,         // CMS dump failed
    FBE_CMS_EVENT_VAULT_LOAD_SUCCESS,      // CMS successfully loaded
    FBE_CMS_EVENT_VAULT_LOAD_SOFT_FAIL,     // CMS load failed with a recoverable error
    FBE_CMS_EVENT_VAULT_LOAD_HARD_FAIL,     // CMS load failed with an unrecoverable error
    FBE_CMS_EVENT_HARDWARE_OK,            // Hardware is ok; at least one SPS is fully charged
    FBE_CMS_EVENT_HARDWARE_DEGRADED,           // Hardware issues but not imminent shutdown (SPS not charged)
    FBE_CMS_EVENT_SHUTDOWN_IMMINENT,      // Shut down is imminent. Either on battery power or over temp
    FBE_CMS_EVENT_PEER_ALIVE,           // The peer SP is alive
    FBE_CMS_EVENT_PEER_DEAD,            // The peer SP is dead
    FBE_CMS_EVENT_DISABLE_WRITE_CACHING,     // Write caching has been disabled
    FBE_CMS_EVENT_ENABLE_WRITE_CACHING,      // Write caching has been enabled
    FBE_CMS_EVENT_PEER_STATE_INIT,         // Peer CMS state machine  transitioned to Init state
    FBE_CMS_EVENT_PEER_STATE_DIRTY,        // Peer CMS state machine  transitioned to Dirty state
    FBE_CMS_EVENT_PEER_STATE_CLEAN,        // Peer CMS state machine  transitioned to Clean state
    FBE_CMS_EVENT_PEER_STATE_CLEAN_CDCA,    // Peer CMS state machine  transitioned to Clean CDCA state
    FBE_CMS_EVENT_PEER_STATE_CLEANING,     // Peer CMS state machine  transitioned to Cleaning state
    FBE_CMS_EVENT_PEER_STATE_QUIESCING,    // Peer CMS state machine  transitioned to Quiescing state
    FBE_CMS_EVENT_PEER_STATE_QUIESCED,     // Peer CMS state machine  transitioned to Quiesced state
    FBE_CMS_EVENT_PEER_STATE_VAULT_LOAD,    // Peer CMS state machine  transitioned to Vault Load state
    FBE_CMS_EVENT_PEER_STATE_PEER_SYNC,     // Peer CMS state machine  transitioned to Peer Sync state
    FBE_CMS_EVENT_PEER_STATE_DUMPING,      // Peer CMS state machine  transitioned to Dumping state
    FBE_CMS_EVENT_PEER_STATE_DUMPED,       // Peer CMS state machine  transitioned to Dumped state
    FBE_CMS_EVENT_PEER_STATE_DUMP_FAIL,   // Peer CMS state machine  transitioned to DumpFailed state
    FBE_CMS_EVENT_PEER_STATE_VAULT_READY,   // The peer's vault has transitioned to the ready state
    FBE_CMS_EVENT_PEER_STATE_VAULT_NOT_READY,// The peer's vault has transitioned out of the ready state
    FBE_CMS_EVENT_PEER_STATE_SYNC_REQUEST,       // The peer is requesting synchronization with this SP
    FBE_CMS_EVENT_PEER_STATE_UNKNOWN,      // Peer CMS state machine  is in an unknown state
    FBE_CMS_EVENT_UNKNOWN                // Unknown event code
};

typedef fbe_u64_t fbe_cms_event_t;

enum fbe_cms_vault_lun_state_e{
    FBE_CMS_VAULT_LUN_STATE_NONE_READY = 0, 	// Vault LUN is not ready on any SP
    FBE_CMS_VAULT_LUN_STATE_MY_SP_READY,         // Vault LUN is ready on this SP
    FBE_CMS_VAULT_LUN_STATE_PEER_SP_READY,       // Vault LUN is ready on the peer SP
    FBE_CMS_VAULT_LUN_STATE_BOTH_READY,              // Vault LUN is ready on both SPs
    FBE_CMS_VAULT_LUN_STATE_MY_SP_NOT_READY,           // Vault LUN is not ready on this SP
    FBE_CMS_VAULT_LUN_STATE_PEER_SP_NOT_READY,         // Vault LUN is not ready on the peer SP
    FBE_CMS_VAULT_LUN_STATE_UNKNOWN
};

typedef fbe_u64_t fbe_cms_vault_lun_state_t;


enum fbe_cms_peer_state_e{
    FBE_CMS_PEER_STATE_DEAD = 0,     			// Peer SP is dead
    FBE_CMS_PEER_STATE_NOT_SYNCED,             	// Peer is alive but not synchronized
    FBE_CMS_PEER_STATE_SYNC_REQUESTED,         	// Peer is alive, synchronization requested
    FBE_CMS_PEER_STATE_ALIVE_AND_SYNCED,      	// Peer is alive and synchronized
    FBE_CMS_PEER_STATE_ALIVE,                      // Peer is alive. It may or may not be in-sync
    FBE_CMS_PEER_STATE_UNKNOWN
};

typedef fbe_u64_t fbe_cms_peer_state_t;

enum fbe_cms_hardware_state_e{
    FBE_CMS_HARDWARE_STATE_OK = 0,      		// At least one SPS is fully charged
    FBE_CMS_HARDWARE_STATE_DEGRADED,   			// SPS not fully charged
    FBE_CMS_HARDWARE_STATE_SHUTDOWN_IMMINENT,   // Shutdown Imminent (on battery or over-temp)
	FBE_CMS_HARDWARE_STATE_UNKNOWN
};

typedef fbe_u64_t fbe_cms_hardware_state_t;

typedef struct fbe_cms_sm_history_table_entry_s{
	fbe_cms_state_t				old_state;
	fbe_cms_state_t 			new_state;
	fbe_u64_t					event;
	fbe_system_time_t			time;
	fbe_cms_vault_lun_state_t	vault_state;
	fbe_bool_t					write_cache_enabled;
	fbe_bool_t					peer_alive;
	fbe_cms_peer_state_t		peer_state;
	fbe_cms_hardware_state_t	hw_state;
}fbe_cms_sm_history_table_entry_t;

/*FBE_CMS_CONTROL_CODE_GET_STATE_MACHINE_HISTORY*/
typedef struct fbe_cms_control_get_sm_history_s{
	fbe_cms_sm_history_table_entry_t	history_table[FBE_CMS_MAX_HISTORY_TABLE_ENTRIES];
}fbe_cms_control_get_sm_history_t;

/*FBE_CMS_CONTROL_CODE_GET_ACCESS_FUNC_PTRS*/
typedef struct fbe_cms_control_get_access_func_ptrs_s{
    fbe_cms_register_client_func_t			register_client_func;
    fbe_cms_unregister_client_func_t		unregister_client_func;
    fbe_cms_register_owner_func_t			register_owner_func;
    fbe_cms_unregister_owner_func_t			unregister_owner_func;

    fbe_cms_buff_alloc_excl_func_t			allocate_execlusive_func;
	fbe_cms_buff_cont_alloc_excl_func_t		allocate_cont_exclusive_func;
	fbe_cms_buff_commit_excl_func_t			commit_exclusive_func;
	fbe_cms_buff_free_excl_func_t			free_exclusive_func;
	fbe_cms_buff_request_abort_func_t		buffer_request_abort_func;
    fbe_cms_buff_get_excl_lock_func_t		get_exclusive_lock_func;
	fbe_cms_buff_get_excl_lock_func_t		get_shared_lock_func;
	fbe_cms_buff_release_excl_lock_func_t	release_exclusive_lock;
	fbe_cms_buff_release_excl_lock_func_t	release_shared_lock;
	fbe_cms_buff_free_all_unlocked_func_t	free_all_unlocks_func;
    fbe_cms_buff_get_excl_lock_func_t		get_first_exclusive_func;
	fbe_cms_buff_get_excl_lock_func_t		get_next_exclusive_func;
}fbe_cms_control_get_access_func_ptrs_t;

#endif /* FBE_CMS_INTERFACE_H */


