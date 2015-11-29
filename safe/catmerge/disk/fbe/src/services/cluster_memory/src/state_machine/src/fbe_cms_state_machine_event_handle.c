/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_state_machine_event_handle.c
***************************************************************************
*
* @brief
*  This file contains CMS service handling of various states.
*  
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/

#include "fbe_cms_private.h"
#include "fbe_cms_environment.h"
#include "fbe_cms_state_machine.h"
#include "fbe_cms_sm_cmi.h"


fbe_status_t fbe_cms_sm_handle_startup(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	switch (current_state) {
	case FBE_CMS_STATE_INIT:
    case FBE_CMS_STATE_DIRTY:
	case FBE_CMS_STATE_CLEAN:
	case FBE_CMS_STATE_CLEAN_CDCA:
	case FBE_CMS_STATE_CLEANING:
	case FBE_CMS_STATE_QUIESCING:
	case FBE_CMS_STATE_QUIESCED:
	case FBE_CMS_STATE_VAULT_LOAD:
	case FBE_CMS_STATE_PEER_SYNC:
	case FBE_CMS_STATE_DUMPING:
	case FBE_CMS_STATE_DUMPED:
	case FBE_CMS_STATE_FAILED:
		break;
	case FBE_CMS_STATE_UNKNOWN:
		/*initial state is unknown and now we'll go into init state and do whatever needs do be done in it*/
		fbe_cms_sm_set_state(FBE_CMS_STATE_INIT);
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't handle state:%d\n", __FUNCTION__, current_state); 
	}
	
	
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_clean(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_dirty(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_quiesce_complete(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_sync(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_local_vault_state(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	/*let's update the state first*/
	switch (event_data->event) {
	case FBE_CMS_EVENT_LOCAL_VAULT_STATE_READY:
		fbe_cms_sm_set_vault_lun_state(FBE_CMS_VAULT_LUN_STATE_MY_SP_READY);
		break;
	case FBE_CMS_EVENT_LOCAL_VAULT_STATE_NOT_READY:
		fbe_cms_sm_set_vault_lun_state(FBE_CMS_VAULT_LUN_STATE_MY_SP_NOT_READY);
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't handle event type:%d\n", __FUNCTION__, event_data->event);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_vault_dump(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_vault_load(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	fbe_cms_sm_event_data_t		local_event_data;
    
	/*let's update the state first*/
	switch (event_data->event) {
	case FBE_CMS_EVENT_VAULT_LOAD_SUCCESS:
		local_event_data.event = FBE_CMS_EVENT_LOCAL_VAULT_STATE_READY;
		fbe_cms_sm_generate_event(&local_event_data);
		break;
	case FBE_CMS_EVENT_VAULT_LOAD_SOFT_FAIL:
		local_event_data.event = FBE_CMS_EVENT_LOCAL_VAULT_STATE_NOT_READY;
		fbe_cms_sm_generate_event(&local_event_data);
		break;
	case FBE_CMS_EVENT_VAULT_LOAD_HARD_FAIL:
		local_event_data.event = FBE_CMS_EVENT_LOCAL_VAULT_STATE_NOT_READY;
		fbe_cms_sm_generate_event(&local_event_data);
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't handle event type:%d\n", __FUNCTION__, event_data->event);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*and see what needs to be done based on our state right now*/
	switch (current_state) {
	case FBE_CMS_STATE_INIT:
		break;
    case FBE_CMS_STATE_DIRTY:
		break;
	case FBE_CMS_STATE_CLEAN:
		break;
	case FBE_CMS_STATE_CLEAN_CDCA:
		break;
	case FBE_CMS_STATE_CLEANING:
		break;
	case FBE_CMS_STATE_QUIESCING:
		break;
	case FBE_CMS_STATE_QUIESCED:
		break;
	case FBE_CMS_STATE_VAULT_LOAD:
		break;
	case FBE_CMS_STATE_PEER_SYNC:
		break;
	case FBE_CMS_STATE_DUMPING:
		break;
	case FBE_CMS_STATE_DUMPED:
		break;
	case FBE_CMS_STATE_FAILED:
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't handle state:%d\n", __FUNCTION__, current_state); 
	}

	
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_hardware_events(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	/*let's update the state first*/
	switch (event_data->event) {
	case FBE_CMS_EVENT_HARDWARE_OK:
		fbe_cms_sm_set_hardware_state(FBE_CMS_HARDWARE_STATE_OK);
		break;
	case FBE_CMS_EVENT_HARDWARE_DEGRADED:
		fbe_cms_sm_set_hardware_state(FBE_CMS_HARDWARE_STATE_DEGRADED);
		break;
	case FBE_CMS_EVENT_SHUTDOWN_IMMINENT:
		fbe_cms_sm_set_hardware_state(FBE_CMS_HARDWARE_STATE_SHUTDOWN_IMMINENT);
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't handle event type:%d\n", __FUNCTION__, event_data->event);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*and see what needs to be done based on our state right now*/
	switch (current_state) {
	case FBE_CMS_STATE_INIT:
		break;
    case FBE_CMS_STATE_DIRTY:
		break;
	case FBE_CMS_STATE_CLEAN:
		break;
	case FBE_CMS_STATE_CLEAN_CDCA:
		break;
	case FBE_CMS_STATE_CLEANING:
		break;
	case FBE_CMS_STATE_QUIESCING:
		break;
	case FBE_CMS_STATE_QUIESCED:
		break;
	case FBE_CMS_STATE_VAULT_LOAD:
		break;
	case FBE_CMS_STATE_PEER_SYNC:
		break;
	case FBE_CMS_STATE_DUMPING:
		break;
	case FBE_CMS_STATE_DUMPED:
		break;
	case FBE_CMS_STATE_FAILED:
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't handle state:%d\n", __FUNCTION__, current_state); 
	}
	

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_alive(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	/*for now we do it over here*/
	fbe_cms_sm_set_peer_state(FBE_CMS_PEER_STATE_ALIVE);

	switch (current_state) {
	case FBE_CMS_STATE_INIT:
		break;
    case FBE_CMS_STATE_DIRTY:
		break;
	case FBE_CMS_STATE_CLEAN:
		break;
	case FBE_CMS_STATE_CLEAN_CDCA:
		break;
	case FBE_CMS_STATE_CLEANING:
		break;
	case FBE_CMS_STATE_QUIESCING:
		break;
	case FBE_CMS_STATE_QUIESCED:
		break;
	case FBE_CMS_STATE_VAULT_LOAD:
		break;
	case FBE_CMS_STATE_PEER_SYNC:
		break;
	case FBE_CMS_STATE_DUMPING:
		break;
	case FBE_CMS_STATE_DUMPED:
		break;
	case FBE_CMS_STATE_FAILED:
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_INFO, "%s Can't handle state:%d\n", __FUNCTION__, current_state); 
	}
    
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_dead(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	/*for now we do it over here*/
	fbe_cms_sm_set_peer_state(FBE_CMS_PEER_STATE_DEAD);

	switch (current_state) {
	case FBE_CMS_STATE_INIT:
		break;
    case FBE_CMS_STATE_DIRTY:
		break;
	case FBE_CMS_STATE_CLEAN:
		break;
	case FBE_CMS_STATE_CLEAN_CDCA:
		break;
	case FBE_CMS_STATE_CLEANING:
		break;
	case FBE_CMS_STATE_QUIESCING:
		break;
	case FBE_CMS_STATE_QUIESCED:
		break;
	case FBE_CMS_STATE_VAULT_LOAD:
		break;
	case FBE_CMS_STATE_PEER_SYNC:
		break;
	case FBE_CMS_STATE_DUMPING:
		break;
	case FBE_CMS_STATE_DUMPED:
		break;
	case FBE_CMS_STATE_FAILED:
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_INFO, "%s Can't handle state:%d\n", __FUNCTION__, current_state); 
	}
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_write_cache_setting(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_init(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_dirty(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_clean_states(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_quiesce(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_vault_state(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_dump(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_sm_handle_peer_state_unknown(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state)
{
	return FBE_STATUS_OK;
}



