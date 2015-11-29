#ifndef FBE_CMS_STATE_MACHINE_H
#define FBE_CMS_STATE_MACHINE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_environment.h
 ***************************************************************************
 *
 * @brief
 *  This file contains state machine related interfaces.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_cms_environment.h"
#include "fbe_cms_cdr_layout.h"
#include "fbe_cms_tag_layout.h"
#include "fbe_cms_vault_layout.h"
#include "fbe/fbe_cms_interface.h"


typedef struct fbe_cms_sm_event_data_s{
	fbe_cms_event_t event;
	#if 0 /*add as needed*/
	union{

	}event_data;
	#endif

}fbe_cms_sm_event_data_t;

struct fbe_cms_sm_table_entry_s;
typedef fbe_status_t (*fbe_cms_sm_function)(const struct fbe_cms_sm_table_entry_s *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
typedef struct fbe_cms_sm_table_entry_s{
	fbe_cms_sm_function	state_function;/*the function to call in this combination of state and event*/
}fbe_cms_sm_table_entry_t;


/*main functions*/
fbe_status_t fbe_cms_state_machine_process_lost_peer(void);
fbe_status_t fbe_cms_state_machine_destroy(void);
fbe_status_t fbe_cms_state_machine_init(void);
fbe_status_t fbe_cms_sm_generate_event(fbe_cms_sm_event_data_t * event);
fbe_status_t fbe_cms_sm_set_state(fbe_cms_state_t new_state);
fbe_status_t fbe_cms_sm_get_state(fbe_cms_state_t *state_out);
fbe_status_t fbe_cms_sm_set_hardware_state(fbe_cms_hardware_state_t new_state);
fbe_status_t fbe_cms_sm_get_hardware_state(fbe_cms_hardware_state_t *state_out);
fbe_status_t fbe_cms_sm_set_peer_state(fbe_cms_peer_state_t new_state);
fbe_status_t fbe_cms_sm_get_peer_state(fbe_cms_peer_state_t *state_out);
fbe_status_t fbe_cms_sm_set_vault_lun_state(fbe_cms_vault_lun_state_t new_state);
fbe_status_t fbe_cms_sm_get_vault_lun_state(fbe_cms_vault_lun_state_t *state_out);
const fbe_u8_t * fbe_cms_state_to_string(fbe_cms_state_t state);
const fbe_u8_t * fbe_cms_hw_state_to_string(fbe_cms_hardware_state_t state);
const fbe_u8_t * fbe_cms_peer_state_to_string(fbe_cms_peer_state_t state);
const fbe_u8_t * fbe_cms_vault_state_to_string(fbe_cms_vault_lun_state_t state);
fbe_status_t cms_sm_get_history_table(fbe_packet_t *packet);


/*event handling*/
fbe_status_t fbe_cms_sm_handle_startup(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_clean(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_dirty(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_quiesce_complete(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_sync(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_local_vault_state(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_vault_dump(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_vault_load(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_hardware_events(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_alive(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_dead(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_write_cache_setting(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_init(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_dirty(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_clean_states(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_quiesce(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_vault_state(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_dump(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);
fbe_status_t fbe_cms_sm_handle_peer_state_unknown(const fbe_cms_sm_table_entry_t *rules, fbe_cms_sm_event_data_t *event_data, fbe_cms_state_t current_state);

#endif /*FBE_CMS_STATE_MACHINE_H*/
