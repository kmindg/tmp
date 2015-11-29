#ifndef FBE_SCHEDULER_INTERFACE_H
#define FBE_SCHEDULER_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_atomic.h"

#define FBE_SCHEDULER_MAX_CORES FBE_CPU_ID_MAX
#define MAX_SCHEDULER_DEBUG_HOOKS 64

typedef enum fbe_scheduler_control_code_e {
	FBE_SCHEDULER_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_SCHEDULER),
	FBE_SCHEDULER_CONTROL_CODE_SET_CREDITS,
	FBE_SCHEDULER_CONTROL_CODE_GET_CREDITS,
	FBE_SCHEDULER_CONTROL_CODE_ADD_CPU_CREDITS_PER_CORE,
	FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_PER_CORE,
	FBE_SCHEDULER_CONTROL_CODE_ADD_CPU_CREDITS_ALL_CORES,
	FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_ALL_CORES,
	FBE_SCHEDULER_CONTROL_CODE_ADD_MEMORY_CREDITS,
	FBE_SCHEDULER_CONTROL_CODE_REMOVE_MEMORY_CREDITS,
	FBE_SCHEDULER_CONTROL_CODE_SET_SCALE,
	FBE_SCHEDULER_CONTROL_CODE_GET_SCALE,
	FBE_SCHEDULER_CONTROL_CODE_SET_DEBUG_HOOK,                 // CC for setting up a scheduler debug hook (used to pause different monitor states)
	FBE_SCHEDULER_CONTROL_CODE_GET_DEBUG_HOOK,                 // CC for getting a scheduler debug hook
	FBE_SCHEDULER_CONTROL_CODE_CLEAR_DEBUG_HOOKS,              // CC for clearing all of the debug hooks
	FBE_SCHEDULER_CONTROL_CODE_DELETE_DEBUG_HOOK,              // CC for deleting a scheduler debug hook
	FBE_SCHEDULER_CONTROL_CODE_LAST
} fbe_scheduler_control_code_t;


/*!********************************************************************* 
 * @struct fbe_scheduler_credit_t 
 *  
 * @brief 
 *   information about the credir requested or granted. All units are per second
 *
 * @ingroup fbe_api_scheduler_interface
 **********************************************************************/
typedef struct fbe_scheduler_credit_s{
	fbe_atomic_t	cpu_operations_per_second; /*!< How many cpcu cycles per seconds per core*/
	fbe_atomic_t 	mega_bytes_consumption; /*!< How many MB operation consumes*/
}fbe_scheduler_credit_t;

/*FBE_SCHEDULER_CONTROL_CODE_SET_CREDITS
FBE_SCHEDULER_CONTROL_CODE_GET_CREDITS
set/get all the credits in the system at once*/
typedef struct fbe_scheduler_set_credits_s{
	fbe_atomic_t				cpu_operations_per_second[FBE_SCHEDULER_MAX_CORES];/*per cpu*/
	fbe_atomic_t				mega_bytes_consumption;/*shard between all cpus*/
}fbe_scheduler_set_credits_t;

/*FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_PER_CORE*/
/*FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_PER_CORE*/
/*set for each core the amount of credits it has*/
typedef struct fbe_scheduler_change_cpu_credits_per_core_s{
	fbe_s32_t 					core;
	fbe_atomic_t				cpu_operations_per_second;
}fbe_scheduler_change_cpu_credits_per_core_t;

/*FBE_SCHEDULER_CONTROL_CODE_ADD_CPU_CREDITS_ALL_CORES*/
/*FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_ALL_CORES*/
/*change all cores their credits at once*/
typedef struct fbe_scheduler_change_cpu_credits_all_cores_s{
    fbe_atomic_t				cpu_operations_per_second;
}fbe_scheduler_change_cpu_credits_all_cores_t;

/*FBE_SCHEDULER_CONTROL_CODE_ADD_MEMORY_CREDITS*/
/*FBE_SCHEDULER_CONTROL_CODE_REMOVE_MEMORY_CREDITS*/
/*change only the amount of memory allocatd for SEP*/
typedef struct fbe_scheduler_change_memory_credits_s{
    fbe_atomic_t		mega_bytes_consumption_change;
}fbe_scheduler_change_memory_credits_t;

/*FBE_SCHEDULER_CONTROL_CODE_SET_SCALE*/
/*FBE_SCHEDULER_CONTROL_CODE_GET_SCALE*/
typedef struct fbe_scheduler_set_scale_s{
    fbe_atomic_t		scale;
}fbe_scheduler_set_scale_t;

typedef struct fbe_scheduler_debug_hook_s{
	fbe_object_id_t object_id;
	fbe_u32_t monitor_state;
	fbe_u32_t monitor_substate;
	fbe_u64_t val1;
	fbe_u64_t val2;
	fbe_u32_t check_type;
	fbe_u32_t action;
	fbe_u64_t counter;
}fbe_scheduler_debug_hook_t;

typedef fbe_status_t (* plugin_init_function_t)(void);
typedef fbe_status_t (* plugin_destroy_function_t)(void);
typedef fbe_status_t (* plugin_entry_function_t)(struct fbe_packet_s * packet);

typedef struct fbe_scheduler_plugin_s{
	plugin_init_function_t plugin_init;
	plugin_destroy_function_t plugin_destroy;
	plugin_entry_function_t plugin_entry;
}fbe_scheduler_plugin_t;

fbe_status_t fbe_scheduler_register_encryption_plugin(fbe_scheduler_plugin_t * plugin);

#endif /*FBE_SCHEDULER_INTERFACE_H*/
