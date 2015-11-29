/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_state_machine_main.c
***************************************************************************
*
* @brief
*  This file contains CMS service main entry points
*  
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/

#include "fbe_cms_private.h"
#include "fbe_cms_environment.h"
#include "fbe_cms_state_machine.h"
#include "fbe_cmi.h"
#include "fbe_cmm.h"
#include "fbe_cms_state_machine_queue_memory.h"
#include "fbe_cms_cdr_layout.h"
#include "fbe/fbe_cms_interface.h"

#define FBE_CMS_SM_ENUM_TO_STR(_enum){_enum, #_enum}

typedef enum fbe_cms_sm_thread_flag_e{
    FBE_CMS_SM_THREAD_RUN,
    FBE_CMS_SM_THREAD_STOP,
    FBE_CMS_SM_THREAD_DONE
}fbe_cms_sm_thread_flag_t;

typedef struct fbe_cms_sm_state_enum_to_str_s{
    fbe_u64_t		     state;      
    const fbe_u8_t *    state_str;
}fbe_cms_sm_state_enum_to_str_t;

static fbe_cms_sm_state_enum_to_str_t      fbe_cms_sm_state_string_table [] =
{
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_INIT),      
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_DIRTY),             
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_CLEAN),                
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_CLEAN_CDCA),            
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_CLEANING),             
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_QUIESCING),                 
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_QUIESCED),              
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_VAULT_LOAD),
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_PEER_SYNC),
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_DUMPING),
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_DUMPED),
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_FAILED),
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_STATE_UNKNOWN),
};

static fbe_cms_sm_state_enum_to_str_t      fbe_cms_sm_hw_state_string_table [] =
{
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_HARDWARE_STATE_OK),      
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_HARDWARE_STATE_DEGRADED),             
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_HARDWARE_STATE_SHUTDOWN_IMMINENT),                
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_HARDWARE_STATE_UNKNOWN)
};
 
static fbe_cms_sm_state_enum_to_str_t      fbe_cms_sm_vault_state_string_table [] =
{
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_VAULT_LUN_STATE_NONE_READY),      
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_VAULT_LUN_STATE_MY_SP_READY),             
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_VAULT_LUN_STATE_PEER_SP_READY),                
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_VAULT_LUN_STATE_BOTH_READY),            
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_VAULT_LUN_STATE_MY_SP_NOT_READY),             
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_VAULT_LUN_STATE_PEER_SP_NOT_READY),                 
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_VAULT_LUN_STATE_UNKNOWN)
};

static fbe_cms_sm_state_enum_to_str_t      fbe_cms_sm_peer_state_string_table [] =
{
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_PEER_STATE_DEAD),      
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_PEER_STATE_NOT_SYNCED),             
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_PEER_STATE_SYNC_REQUESTED),                
    FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_PEER_STATE_ALIVE),
	FBE_CMS_SM_ENUM_TO_STR(FBE_CMS_PEER_STATE_UNKNOWN)
};

typedef struct fbe_cms_state_machine_internal_data_s{
	fbe_cms_state_t 					current_state;/*default state to start with*/
	fbe_cms_hardware_state_t			current_hardware_state;/*until otherwise proved we should be at the worst state possible*/
	fbe_cms_peer_state_t				current_peer_state;/*we can't assume the peer is there or not*/
	fbe_cms_vault_lun_state_t			current_vault_lun_state;
	fbe_bool_t							current_write_cache_enabled;
	fbe_semaphore_t						semaphore;
	fbe_cms_sm_thread_flag_t			thread_flag;
	fbe_thread_t                 		thread_handle;
	fbe_queue_head_t					event_queue_head;
	fbe_spinlock_t						event_queue_lock;
	fbe_cms_sm_history_table_entry_t	state_history_table[FBE_CMS_MAX_HISTORY_TABLE_ENTRIES];
	fbe_u64_t							history_table_index;
	fbe_bool_t							this_sp_image_valid;
	fbe_bool_t							peer_image_valid;
	fbe_bool_t							vault_image_valid;
	fbe_cmi_sp_id_t						sp_id;
}fbe_cms_state_machine_internal_data_t;

/*this is the table that describes for each state and event combination, what needs to be done*/
static const fbe_cms_sm_table_entry_t cms_sm_table[FBE_CMS_EVENT_UNKNOWN] = 
{
    
	/*FBE_CMS_EVENT_STARTUP*/
	{fbe_cms_sm_handle_startup},
    
	/*FBE_CMS_EVENT_CLEAN*/
    {fbe_cms_sm_handle_clean},
	
	/*FBE_CMS_EVENT_DIRTY*/
	{fbe_cms_sm_handle_dirty},
	
	/*FBE_CMS_EVENT_QUIESCE_COMPLETE*/
    {fbe_cms_sm_handle_quiesce_complete},

	/*FBE_CMS_EVENT_PEER_SYNC_COMPLETE*/
	{fbe_cms_sm_handle_peer_sync},
    
	/*FBE_CMS_EVENT_LOCAL_VAULT_STATE_READY*/
    {fbe_cms_sm_handle_local_vault_state},
    
	/*FBE_CMS_EVENT_LOCAL_VAULT_STATE_NOT_READY*/
    {fbe_cms_sm_handle_local_vault_state},
    
	/*FBE_CMS_EVENT_VAULT_DUMP_SUCCESS*/
    {fbe_cms_sm_handle_vault_dump},
    
	/*FBE_CMS_EVENT_VAULT_DUMP_FAIL*/
    {fbe_cms_sm_handle_vault_dump},
    
	/*FBE_CMS_EVENT_VAULT_LOAD_SUCCESS*/
    {fbe_cms_sm_handle_vault_load},
    
	/*FBE_CMS_EVENT_VAULT_LOAD_SOFT_FAIL*/
    {fbe_cms_sm_handle_vault_load},
    
	/*FBE_CMS_EVENT_VAULT_LOAD_HARD_FAIL*/
    {fbe_cms_sm_handle_vault_load},
    
	/*FBE_CMS_EVENT_HARDWARE_OK*/
    {fbe_cms_sm_handle_hardware_events},
    
	/*FBE_CMS_EVENT_HARDWARE_DEGRADED*/
    {fbe_cms_sm_handle_hardware_events},
    
	/*FBE_CMS_EVENT_SHUTDOWN_IMMINENT*/
    {fbe_cms_sm_handle_hardware_events},

	/*FBE_CMS_EVENT_PEER_ALIVE*/
	{fbe_cms_sm_handle_peer_alive},

    /*FBE_CMS_EVENT_PEER_DEAD*/
	{fbe_cms_sm_handle_peer_dead},

    /*FBE_CMS_EVENT_DISABLE_WRITE_CACHING*/
	{fbe_cms_sm_handle_write_cache_setting},

    /*FBE_CMS_EVENT_ENABLE_WRITE_CACHING*/
	{fbe_cms_sm_handle_write_cache_setting},

    /*FBE_CMS_EVENT_PEER_STATE_INIT*/
	{fbe_cms_sm_handle_peer_init},

    /*FBE_CMS_EVENT_PEER_STATE_DIRTY*/
	{fbe_cms_sm_handle_peer_dirty},

    /*FBE_CMS_EVENT_PEER_STATE_CLEAN*/
	{fbe_cms_sm_handle_peer_clean_states},

    /*FBE_CMS_EVENT_PEER_STATE_CLEAN_CDCA*/
	{fbe_cms_sm_handle_peer_clean_states},

    /*FBE_CMS_EVENT_PEER_STATE_CLEANING*/
	{fbe_cms_sm_handle_peer_clean_states},

    /*FBE_CMS_EVENT_PEER_STATE_QUIESCING*/
	{fbe_cms_sm_handle_peer_quiesce},

    /*FBE_CMS_EVENT_PEER_STATE_QUIESCED*/
	{fbe_cms_sm_handle_peer_quiesce},

    /*FBE_CMS_EVENT_PEER_STATE_VAULT_LOAD*/
	{fbe_cms_sm_handle_peer_vault_state},

    /*FBE_CMS_EVENT_PEER_STATE_PEER_SYNC*/
	{fbe_cms_sm_handle_peer_sync},

    /*FBE_CMS_EVENT_PEER_STATE_DUMPING*/
	{fbe_cms_sm_handle_peer_dump},

    /*FBE_CMS_EVENT_PEER_STATE_DUMPED*/
	{fbe_cms_sm_handle_peer_dump},

    /*FBE_CMS_EVENT_PEER_STATE_DUMP_FAIL*/
	{fbe_cms_sm_handle_peer_dump},

    /*FBE_CMS_EVENT_PEER_STATE_VAULT_READY*/
	{fbe_cms_sm_handle_peer_vault_state},

    /*FBE_CMS_EVENT_PEER_STATE_VAULT_NOT_READY*/
	{fbe_cms_sm_handle_peer_vault_state},

    /*FBE_CMS_EVENT_PEER_STATE_SYNC_REQUEST*/
	{fbe_cms_sm_handle_peer_sync},

    /*FBE_CMS_EVENT_PEER_STATE_UNKNOWN*/
	{fbe_cms_sm_handle_peer_state_unknown},

};

static fbe_cms_state_machine_internal_data_t	fbe_cms_sm_data;

/*local functions*/
static void fbe_cms_sm_process_message_queue(void);
static void fbe_cms_sm_thread_func(void *context);
static void fbe_cms_sm_add_event_to_history(fbe_u64_t evnet_code);
static void fbe_cms_sm_update_current_state_in_history(fbe_cms_state_t new_state);

/*************************************************************************************************************************************************/
fbe_status_t fbe_cms_state_machine_process_lost_peer(void)
{
	fbe_cms_sm_event_data_t event_data;

	event_data.event = FBE_CMS_EVENT_PEER_DEAD;

	fbe_cms_sm_generate_event(&event_data);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_state_machine_init(void)
{
	fbe_status_t 	status;
	EMCPAL_STATUS       	nt_status;

	fbe_semaphore_init(&fbe_cms_sm_data.semaphore, 0, FBE_SEMAPHORE_MAX);
    fbe_queue_init(&fbe_cms_sm_data.event_queue_head);
    fbe_spinlock_init(&fbe_cms_sm_data.event_queue_lock);

	fbe_cms_sm_data.current_state = FBE_CMS_STATE_INIT;/*default state to start with*/
	fbe_cms_sm_data.current_hardware_state = FBE_CMS_HARDWARE_STATE_SHUTDOWN_IMMINENT;/*until otherwise proved we should be at the worst state possible*/
	fbe_cms_sm_data.current_peer_state = FBE_CMS_PEER_STATE_UNKNOWN;/*we can't assume the peer is there or not*/
	fbe_cms_sm_data.current_vault_lun_state = FBE_CMS_VAULT_LUN_STATE_UNKNOWN;
	fbe_cms_sm_data.current_write_cache_enabled = FBE_FALSE;
	fbe_cms_sm_data.history_table_index = 0;

    /*we need pre allocated memory to process events*/
	status = fbe_cms_sm_allocate_event_queue_memory();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: failed to init event queue\n", __FUNCTION__); 
		return status;
	}

	fbe_zero_memory(fbe_cms_sm_data.state_history_table, sizeof(fbe_cms_sm_history_table_entry_t) * FBE_CMS_MAX_HISTORY_TABLE_ENTRIES);

    fbe_cms_sm_data.thread_flag = FBE_CMS_SM_THREAD_RUN;
    nt_status = fbe_thread_init(&fbe_cms_sm_data.thread_handle, "fbe_cms_sm", fbe_cms_sm_thread_func, NULL);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: can't start state machine process thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_cmi_get_sp_id(&fbe_cms_sm_data.sp_id);

    /*status = */fbe_cms_environment_init();
	#if 0 /*enable when ESP is stable enough*/
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: failed to init ENV\n", __FUNCTION__); 
		return status;
	}
	#endif

    /*read the CDR image to see who has the latest valid image(only active side does that)*/
	if (fbe_cms_common_is_active_sp()) {
		status = fbe_cms_sm_read_image_valid_data(fbe_cms_sm_data.sp_id,
												  &fbe_cms_sm_data.this_sp_image_valid,
												  &fbe_cms_sm_data.peer_image_valid,
												  &fbe_cms_sm_data.vault_image_valid);
		if (status != FBE_STATUS_OK) {
			cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: failed to read CDR LUN\n", __FUNCTION__); 
			return status;
		}
	}

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_state_machine_destroy(void)
{
	fbe_status_t status;

        status = fbe_cms_environment_destroy();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: failed to destroy ENV\n", __FUNCTION__); 
		return status;
	}

    /*destroy resources we used*/
	fbe_cms_sm_data.thread_flag = FBE_CMS_SM_THREAD_STOP;
	fbe_semaphore_release(&fbe_cms_sm_data.semaphore, 0, 1, FALSE);
        fbe_thread_wait(&fbe_cms_sm_data.thread_handle);
        fbe_thread_destroy(&fbe_cms_sm_data.thread_handle);

    /*any chance there are some evens out there we did not handle before we went down*/
	if (!fbe_queue_is_empty(&fbe_cms_sm_data.event_queue_head)) {
		
	}

	fbe_spinlock_destroy(&fbe_cms_sm_data.event_queue_lock);

	fbe_cms_sm_destroy_event_queue_memory();

	return FBE_STATUS_OK;

}

/*this thread handles the state machine queue of events and pop them one after the other to process them*/
static void fbe_cms_sm_thread_func(void *context)
{
    fbe_cpu_id_t 					cpu_number = 1;
    fbe_cpu_affinity_mask_t 		affinity = 0x1;
    
    fbe_thread_set_affinity(&fbe_cms_sm_data.thread_handle, affinity << cpu_number);

	FBE_UNREFERENCED_PARAMETER(context);

    while (1) {
        fbe_semaphore_wait(&fbe_cms_sm_data.semaphore, NULL);
        if (fbe_cms_sm_data.thread_flag == FBE_CMS_SM_THREAD_RUN) {
            fbe_cms_sm_process_message_queue();
        }else{
            break;
        }
    }

    fbe_cms_sm_data.thread_flag = FBE_CMS_SM_THREAD_DONE;
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: done\n", __FUNCTION__); 
    
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);

}

static void fbe_cms_sm_process_message_queue(void)
{
	fbe_cms_sm_event_info_t *	event_info_ptr = NULL;
    
	fbe_spinlock_lock(&fbe_cms_sm_data.event_queue_lock);
	while (!fbe_queue_is_empty(&fbe_cms_sm_data.event_queue_head)) {
		event_info_ptr = (fbe_cms_sm_event_info_t *)fbe_queue_pop(&fbe_cms_sm_data.event_queue_head);
		fbe_spinlock_unlock(&fbe_cms_sm_data.event_queue_lock);

		cms_trace(FBE_TRACE_LEVEL_INFO, "%s:Event: %d, State:%s\n",
				   __FUNCTION__, event_info_ptr->event_data.event, fbe_cms_state_to_string(fbe_cms_sm_data.current_state)); 

		if (event_info_ptr->event_data.event < FBE_CMS_EVENT_UNKNOWN) {
		
			/*use our table to proceed with the combination of state and events*/
			if (cms_sm_table[event_info_ptr->event_data.event].state_function != NULL) {

				/*before we do anything, we record the state of all the flags in a history table for debug*/
                fbe_cms_sm_add_event_to_history(event_info_ptr->event_data.event);
                
				/*call the function that handles this state/event combination*/
				cms_sm_table[event_info_ptr->event_data.event].state_function(&cms_sm_table[event_info_ptr->event_data.event],
																			  &event_info_ptr->event_data,
																			  fbe_cms_sm_data.current_state);

				/*after we execute, we update the current state in the history table*/
				fbe_cms_sm_update_current_state_in_history(fbe_cms_sm_data.current_state);
				
			}
		}

		fbe_cms_sm_return_event_memory(event_info_ptr);
		fbe_spinlock_lock(&fbe_cms_sm_data.event_queue_lock);
	}

	fbe_spinlock_unlock(&fbe_cms_sm_data.event_queue_lock);
}

/*called by anyone who needs o alert the state machine of an event*/
fbe_status_t fbe_cms_sm_generate_event(fbe_cms_sm_event_data_t * event_data)
{
	fbe_cms_sm_event_info_t *	event_info = fbe_cms_sm_get_event_memory();/*use pre-allocated memory since we might be at DPC*/

	fbe_copy_memory(&event_info->event_data, event_data, sizeof(fbe_cms_sm_event_data_t));

	fbe_spinlock_lock(&fbe_cms_sm_data.event_queue_lock);
    fbe_queue_push(&fbe_cms_sm_data.event_queue_head, &event_info->queue_element);
	fbe_spinlock_unlock(&fbe_cms_sm_data.event_queue_lock);

	fbe_semaphore_release(&fbe_cms_sm_data.semaphore,0, 1, FALSE);


	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_set_state(fbe_cms_state_t new_state)
{
	fbe_cms_state_t	tmp = fbe_cms_sm_data.current_state;

	fbe_cms_sm_data.current_state = new_state;

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s:Old:%s, New:%s\n", __FUNCTION__, fbe_cms_state_to_string(tmp), fbe_cms_state_to_string(new_state)); 

	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_get_state(fbe_cms_state_t *state_out)
{
	if (*state_out != NULL) {
		*state_out = fbe_cms_sm_data.current_state;
		return FBE_STATUS_OK;
	}else{
		cms_trace(FBE_TRACE_LEVEL_INFO, "%s: NULL pointer\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}
}

const fbe_u8_t * fbe_cms_state_to_string(fbe_cms_state_t state)
{
	if (state <= FBE_CMS_STATE_UNKNOWN) {
		return fbe_cms_sm_state_string_table[state].state_str;
	}else{
		return NULL;
	}
}

const fbe_u8_t * fbe_cms_hw_state_to_string(fbe_cms_hardware_state_t state)
{
	if (state <= FBE_CMS_HARDWARE_STATE_UNKNOWN) {
		return fbe_cms_sm_hw_state_string_table[state].state_str;
	}else{
		return NULL;
	}
}

const fbe_u8_t * fbe_cms_peer_state_to_string(fbe_cms_peer_state_t state)
{
	if (state <= FBE_CMS_PEER_STATE_UNKNOWN) {
		return fbe_cms_sm_peer_state_string_table[state].state_str;
	}else{
		return NULL;
	}
}

const fbe_u8_t * fbe_cms_vault_state_to_string(fbe_cms_vault_lun_state_t state)
{
	if (state <= FBE_CMS_VAULT_LUN_STATE_UNKNOWN) {
		return fbe_cms_sm_vault_state_string_table[state].state_str;
	}else{
		return NULL;
	}
}

fbe_status_t fbe_cms_sm_set_hardware_state(fbe_cms_hardware_state_t new_state)
{
	fbe_cms_hardware_state_t	tmp = fbe_cms_sm_data.current_hardware_state;

	fbe_cms_sm_data.current_hardware_state = new_state;

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s:Old:%s, New:%s\n", __FUNCTION__, fbe_cms_hw_state_to_string(tmp), fbe_cms_hw_state_to_string(new_state)); 

	return FBE_STATUS_OK;

}

fbe_status_t fbe_cms_sm_get_hardware_state(fbe_cms_hardware_state_t *state_out)
{
	if (*state_out != NULL) {
		*state_out = fbe_cms_sm_data.current_hardware_state;
		return FBE_STATUS_OK;
	}else{
		cms_trace(FBE_TRACE_LEVEL_INFO, "%s: NULL pointer\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

}

fbe_status_t fbe_cms_sm_set_peer_state(fbe_cms_peer_state_t new_state)
{
	fbe_cms_peer_state_t	tmp = fbe_cms_sm_data.current_peer_state;

	fbe_cms_sm_data.current_peer_state = new_state;

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s:Old:%s, New:%s\n", __FUNCTION__, fbe_cms_peer_state_to_string(tmp), fbe_cms_peer_state_to_string(new_state)); 

	return FBE_STATUS_OK;

}

fbe_status_t fbe_cms_sm_get_peer_state(fbe_cms_peer_state_t *state_out)
{
	if (*state_out != NULL) {
		*state_out = fbe_cms_sm_data.current_peer_state;
		return FBE_STATUS_OK;
	}else{
		cms_trace(FBE_TRACE_LEVEL_INFO, "%s: NULL pointer\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

}

fbe_status_t fbe_cms_sm_set_vault_lun_state(fbe_cms_vault_lun_state_t new_state)
{
	fbe_cms_vault_lun_state_t	tmp = fbe_cms_sm_data.current_vault_lun_state;

	fbe_cms_sm_data.current_vault_lun_state = new_state;

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s:Old:%s, New:%s\n", __FUNCTION__, fbe_cms_vault_state_to_string(tmp), fbe_cms_vault_state_to_string(new_state)); 

	return FBE_STATUS_OK;

}

fbe_status_t fbe_cms_sm_get_vault_lun_state(fbe_cms_vault_lun_state_t *state_out)
{
	if (*state_out != NULL) {
		*state_out = fbe_cms_sm_data.current_vault_lun_state;
		return FBE_STATUS_OK;
	}else{
		cms_trace(FBE_TRACE_LEVEL_INFO, "%s: NULL pointer\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

}

/*called before processing the event*/
static void fbe_cms_sm_add_event_to_history(fbe_u64_t evnet_code)
{
	fbe_spinlock_lock(&fbe_cms_sm_data.event_queue_lock);
    fbe_cms_sm_data.state_history_table[fbe_cms_sm_data.history_table_index % FBE_CMS_MAX_HISTORY_TABLE_ENTRIES].event = evnet_code;
	fbe_cms_sm_data.state_history_table[fbe_cms_sm_data.history_table_index % FBE_CMS_MAX_HISTORY_TABLE_ENTRIES].old_state = fbe_cms_sm_data.current_state;
	fbe_cms_sm_data.state_history_table[fbe_cms_sm_data.history_table_index % FBE_CMS_MAX_HISTORY_TABLE_ENTRIES].hw_state = fbe_cms_sm_data.current_hardware_state;
	fbe_cms_sm_data.state_history_table[fbe_cms_sm_data.history_table_index % FBE_CMS_MAX_HISTORY_TABLE_ENTRIES].peer_alive = fbe_cmi_is_peer_alive();
	fbe_cms_sm_data.state_history_table[fbe_cms_sm_data.history_table_index % FBE_CMS_MAX_HISTORY_TABLE_ENTRIES].peer_state = fbe_cms_sm_data.current_peer_state;
    fbe_cms_sm_data.state_history_table[fbe_cms_sm_data.history_table_index % FBE_CMS_MAX_HISTORY_TABLE_ENTRIES].vault_state = fbe_cms_sm_data.current_vault_lun_state;
	fbe_cms_sm_data.state_history_table[fbe_cms_sm_data.history_table_index % FBE_CMS_MAX_HISTORY_TABLE_ENTRIES].write_cache_enabled = fbe_cms_sm_data.current_write_cache_enabled;
	fbe_get_system_time(&fbe_cms_sm_data.state_history_table[fbe_cms_sm_data.history_table_index % FBE_CMS_MAX_HISTORY_TABLE_ENTRIES].time);
	fbe_spinlock_unlock(&fbe_cms_sm_data.event_queue_lock);
    
}

/*called after processing the event*/
static void fbe_cms_sm_update_current_state_in_history(fbe_cms_state_t new_state)
{
	fbe_cms_sm_data.state_history_table[fbe_cms_sm_data.history_table_index % FBE_CMS_MAX_HISTORY_TABLE_ENTRIES].new_state = new_state;

	/*now we can increment it to the next entry*/
	fbe_cms_sm_data.history_table_index++;
}

fbe_status_t cms_sm_get_history_table(fbe_packet_t *packet)
{
	fbe_status_t                		status = FBE_STATUS_OK;
    fbe_cms_control_get_sm_history_t *	get_history = NULL;
    
    /* Verify packet contents and get pointer to the user's buffer*/
    status = cms_get_payload(packet, (void **)&get_history, sizeof(fbe_cms_control_get_sm_history_t));
    if ( status != FBE_STATUS_OK ){
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Payload failed.\n", __FUNCTION__);
        cms_complete_packet(packet, status);
        return status;
    }

	fbe_spinlock_lock(&fbe_cms_sm_data.event_queue_lock);
	/*copy the history table to the user.*/
    fbe_copy_memory(&get_history->history_table,
					&fbe_cms_sm_data.state_history_table,
					sizeof(fbe_cms_sm_history_table_entry_t) * FBE_CMS_MAX_HISTORY_TABLE_ENTRIES);

	fbe_spinlock_unlock(&fbe_cms_sm_data.event_queue_lock);

    cms_complete_packet(packet, status);
    return status;

}
