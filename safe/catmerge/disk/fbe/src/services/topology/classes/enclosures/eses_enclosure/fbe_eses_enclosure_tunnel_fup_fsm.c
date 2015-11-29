/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_tunnel_fup_fsm.c
 ***************************************************************************
 *
 * @brief
 *  This file contains download and activate FSMs to upgrade the firmware on 
 *  peer devices through the ESES tunneling protocol.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   08 Mar 2011:  Created. Kenny Huang
 *
 ***************************************************************************/

#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_eses_enclosure_private.h"
#include "edal_eses_enclosure_data.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_enclosure.h"

static fbe_eses_tunnel_fup_state_t
fbe_eses_fup_invalid(fbe_eses_tunnel_fup_context_t* context_p,
                     fbe_eses_tunnel_fup_event_t event,
                     fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_tunnel_send_get_config(fbe_eses_tunnel_fup_context_t* context_p,
                                       fbe_eses_tunnel_fup_event_t event,
                                       fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_failure_tunnel_send_get_config(fbe_eses_tunnel_fup_context_t* context_p,
                                               fbe_eses_tunnel_fup_event_t event,
                                               fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_busy_tunnel_send_get_config(fbe_eses_tunnel_fup_context_t* context_p,
                                            fbe_eses_tunnel_fup_event_t event,
                                            fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_tunnel_send_dl_ctrl_page(fbe_eses_tunnel_fup_context_t* context_p,
                                  fbe_eses_tunnel_fup_event_t event,
                                  fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_failure_tunnel_send_dl_ctrl_page(fbe_eses_tunnel_fup_context_t* context_p,
                                                 fbe_eses_tunnel_fup_event_t event,
                                                 fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_busy_tunnel_send_dl_ctrl_page(fbe_eses_tunnel_fup_context_t* context_p,
                                              fbe_eses_tunnel_fup_event_t event,
                                              fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                               fbe_eses_tunnel_fup_event_t event,
                               fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_processing_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                                                 fbe_eses_tunnel_fup_event_t event,
                                                 fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_failure_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                                              fbe_eses_tunnel_fup_event_t event,
                                              fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_busy_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                                           fbe_eses_tunnel_fup_event_t event,
                                           fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_tunnel_receive_dl_status_page(fbe_eses_tunnel_fup_context_t* context_p,
                                       fbe_eses_tunnel_fup_event_t event,
                                       fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_failure_tunnel_receive_dl_status_page(fbe_eses_tunnel_fup_context_t* context_p,
                                                      fbe_eses_tunnel_fup_event_t event,
                                                      fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_busy_tunnel_receive_dl_status_page(fbe_eses_tunnel_fup_context_t* context_p,
                                                   fbe_eses_tunnel_fup_event_t event,
                                                   fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_tunnel_handle_dl_complete(fbe_eses_tunnel_fup_context_t* context_p,
                                   fbe_eses_tunnel_fup_event_t event,
                                   fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_activate_check_rev_and_tunnel_receive_dl_status_page(fbe_eses_tunnel_fup_context_t* context_p,
                                                              fbe_eses_tunnel_fup_event_t event,
                                                              fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_activate_check_rev_and_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                                                      fbe_eses_tunnel_fup_event_t event,
                                                      fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_activate_check_rev(fbe_eses_tunnel_fup_context_t* context_p,
                            fbe_eses_tunnel_fup_event_t event,
                            fbe_eses_tunnel_fup_state_t next_state);

static fbe_eses_tunnel_fup_state_t
fbe_eses_fup_wait_for_reset(fbe_eses_tunnel_fup_context_t* context_p,
                            fbe_eses_tunnel_fup_event_t event,
                            fbe_eses_tunnel_fup_state_t next_state);

static fbe_bool_t 
fbe_eses_enclosure_is_peer_rev_changed(fbe_eses_tunnel_fup_context_t* fup_context_p);

static fbe_bool_t
fbe_eses_enclosure_tunnel_cmd_timed_out(fbe_eses_tunnel_fup_context_t* fup_context_p);

static void
fbe_eses_enclosure_tunnel_set_expected_status_page_code(fbe_eses_tunnel_fup_context_t* fup_context_p,
                                                        ses_pg_code_enum page_code);


static ses_pg_code_enum
fbe_eses_enclosure_tunnel_get_expected_status_page_code(fbe_eses_tunnel_fup_context_t* fup_context_p);

static fbe_enclosure_status_t
fbe_eses_enclosure_process_tunnel_download_ucode_status(fbe_eses_enclosure_t *eses_enclosure,
                                                        fbe_eses_download_status_page_t *dl_status_page_p);

static char *
fbe_eses_tunnel_fup_state_to_string(fbe_eses_tunnel_fup_state_t state);

static char *
fbe_eses_tunnel_fup_event_to_string(fbe_eses_tunnel_fup_event_t event);

static void
fbe_eses_enclosure_reset_peer_lcc(fbe_eses_enclosure_t *eses_enclosure);

// Tunnel Download FSM
static fbe_eses_tunnel_fup_fsm_table_entry_t
fbe_eses_tunnel_download_fsm_table[FBE_ESES_ST_TUNNEL_FUP_LAST][FBE_ESES_EV_TUNNEL_FUP_LAST] =
{
    { // state == FBE_ESES_ST_TUNNEL_FUP_INIT
      {fbe_eses_tunnel_send_get_config, FBE_ESES_ST_TUNNEL_FUP_SEND_GET_CONFIG},  // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                        // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                        // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                        // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                        // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                        // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}                         // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_SEND_GET_CONFIG
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                      // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_READY},                           // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                      // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_handle_failure_tunnel_send_get_config, FBE_ESES_ST_TUNNEL_FUP_SEND_GET_CONFIG}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_handle_busy_tunnel_send_get_config, FBE_ESES_ST_TUNNEL_FUP_SEND_GET_CONFIG},    // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                      // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}                                       // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_READY
      {fbe_eses_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG},       // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG},       // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_handle_processing_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_READY},         // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_handle_failure_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_READY},            // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_handle_busy_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_READY},               // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_handle_failure_tunnel_send_get_config, FBE_ESES_ST_TUNNEL_FUP_SEND_GET_CONFIG}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}                                       // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                               // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS},        // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                               // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_handle_failure_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_handle_busy_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG},    // event == FBE_ESES_EV_TUNNEL_FUP_BUSY
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                               // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}                                                // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                          // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_tunnel_receive_dl_status_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG},                 // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_handle_processing_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS}, // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_handle_failure_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS},    // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_handle_busy_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS},       // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_handle_failure_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG},            // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED
      {fbe_eses_handle_failure_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG}             // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                         // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS},             // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                         // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_handle_failure_tunnel_receive_dl_status_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_handle_busy_tunnel_receive_dl_status_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG},    // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                         // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}                                                          // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                               // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_tunnel_handle_dl_complete, FBE_ESES_ST_TUNNEL_FUP_READY},                                                // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_handle_processing_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS}, // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_handle_failure_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS},    // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_handle_busy_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS},       // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_handle_failure_tunnel_receive_dl_status_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG},       // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_handle_failure_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG}                  // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}  // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED
    }
};

// Tunnel Activate FSM
static fbe_eses_tunnel_fup_fsm_table_entry_t
fbe_eses_tunnel_activate_fsm_table[FBE_ESES_ST_TUNNEL_FUP_LAST][FBE_ESES_EV_TUNNEL_FUP_LAST] =
{
    { // state == FBE_ESES_ST_TUNNEL_FUP_INIT
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}  // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_SEND_GET_CONFIG
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}  // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_READY
      {fbe_eses_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}                                 // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                               // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS},        // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                               // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_handle_failure_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_handle_busy_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG},    // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                               // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}                                                // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                               // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_tunnel_receive_dl_status_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG},                      // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_activate_check_rev_and_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS}, // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_activate_check_rev_and_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_activate_check_rev_and_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS}, // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_handle_failure_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG},                 // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_handle_failure_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG}                  // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                                 // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS},                     // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                                 // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_activate_check_rev_and_tunnel_receive_dl_status_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_activate_check_rev_and_tunnel_receive_dl_status_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG}, // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                                 // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST}                                                                  // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS
      {fbe_eses_fup_invalid, FBE_ESES_ST_TUNNEL_FUP_LAST},                                                                    // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_activate_check_rev, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS},                                // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_activate_check_rev_and_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS}, // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_activate_check_rev_and_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_activate_check_rev_and_get_tunnel_cmd_status, FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS}, // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_activate_check_rev_and_tunnel_receive_dl_status_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG},    // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_handle_failure_tunnel_send_dl_ctrl_page, FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG}                       // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED  
    },
    { // state == FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL 
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_PROCESSING  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_FAILED  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_BUSY  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}, // event == FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED  
      {fbe_eses_fup_wait_for_reset, FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET}  // event == FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED
    }
};

/*!**************************************************************
 * @fn fbe_eses_tunnel_download_fsm
 ***************************************************************
 * @brief
 *   The function runs the FBE download FSM  
 *
 * @param fsm_table - The pointer to the FSM table to run.
 * @param context_p - The pointer to the FSM context.
 * @param event - The event.  
 *
 * @return void - None.
 *
 * @version:
 *  08-Mar-2011 Kenny Huang - Created. 
 *
 ****************************************************************/
fbe_eses_tunnel_fup_schedule_op_t
fbe_eses_tunnel_run_fsm(fbe_eses_tunnel_fup_fsm_table_entry_t fsm_table[][FBE_ESES_EV_TUNNEL_FUP_LAST], 
                        fbe_eses_tunnel_fup_context_t *context_p, 
                        fbe_eses_tunnel_fup_event_t event)
{
    fbe_eses_tunnel_fup_state_t current_state;
    fbe_eses_tunnel_fup_state_t next_state;

    current_state = context_p->current_state;
    next_state = fsm_table[context_p->current_state][event].next_state;

    // Log the first download state transitions or the retried state transitions.
    if ((0 == context_p->eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred) &&
        (FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD == context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperation) &&
         context_p->eses_enclosure->enclCurrentFupInfo.enclFupUseTunnelling == TRUE)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "FBE FUP FSM curr_st %s evt %s next_st %s\n",
                           fbe_eses_tunnel_fup_state_to_string(current_state),
                           fbe_eses_tunnel_fup_event_to_string(event),
                           fbe_eses_tunnel_fup_state_to_string(next_state));
    }
    else
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "FBE FUP FSM curr_state %s event %s next_state %s\n",
                           fbe_eses_tunnel_fup_state_to_string(current_state),
                           fbe_eses_tunnel_fup_event_to_string(event),
                           fbe_eses_tunnel_fup_state_to_string(next_state));
    }

    // Run the transition function.
    next_state = fsm_table[current_state][event].transition_func(context_p, event, next_state);
  
    if (FBE_ESES_ST_TUNNEL_FUP_LAST == next_state)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "FBE FUP %s FSM done curr_state %s event %s next_state %s\n",
                            __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(current_state),
                           fbe_eses_tunnel_fup_event_to_string(event),
                           fbe_eses_tunnel_fup_state_to_string(next_state));
    }
    else if(next_state != current_state)
    {
        context_p->previous_state = current_state;
        context_p->current_state = next_state;
    }

    return(context_p->schedule_op);
} //fbe_eses_tunnel_run_fsm

/*!**************************************************************
 * @fn fbe_eses_tunnel_download_fsm
 ***************************************************************
 * @brief
 *   The function runs the FBE Download FSM  
 *
 * @param context_p - The pointer to the FSM context.
 * @param event - The event.  
 *
 * @return void - None.
 *
 * @version:
 *  08-Mar-2011 Kenny Huang - Created. 
 *
 ****************************************************************/
fbe_eses_tunnel_fup_schedule_op_t
fbe_eses_tunnel_download_fsm(fbe_eses_tunnel_fup_context_t *context_p, fbe_eses_tunnel_fup_event_t event)
{
    return (fbe_eses_tunnel_run_fsm(fbe_eses_tunnel_download_fsm_table, context_p, event));
}

/*!**************************************************************
 * @fn fbe_eses_tunnel_activate_fsm
 ***************************************************************
 * @brief
 *   The function runs the FBE Activate FSM  
 *
 * @param context_p - The pointer to the FSM context.
 * @param event - The event.  
 *
 * @return void - None.
 *
 * @version:
 *  08-Mar-2011 Kenny Huang - Created. 
 *
 ****************************************************************/
fbe_eses_tunnel_fup_schedule_op_t
fbe_eses_tunnel_activate_fsm(fbe_eses_tunnel_fup_context_t *context_p, fbe_eses_tunnel_fup_event_t event)
{
    return (fbe_eses_tunnel_run_fsm(fbe_eses_tunnel_activate_fsm_table, context_p, event));
}

/*!*************************************************************************
* @fn fbe_eses_fup_invalid 
***************************************************************************
* @brief
*   It is a SW bug to to get here.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*
* @return    next state 
*
* NOTES
*
* HISTORY
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_fup_invalid(fbe_eses_tunnel_fup_context_t *context_p,
                     fbe_eses_tunnel_fup_event_t event,
                     fbe_eses_tunnel_fup_state_t next_state)
{
    KvTracex(TRC_K_STD, "FBE: %s, event:0x%x, prev_state 0x%x cur_state 0x%x next_state 0x%x.\n",
            __FUNCTION__,
            event,
            context_p->previous_state,
            context_p->current_state,
            next_state);

    context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_send_get_config 
***************************************************************************
* @brief
*   Tunnel a get configuration command.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*
* @return    next state 
*
* NOTES
*
* HISTORY
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_tunnel_send_get_config(fbe_eses_tunnel_fup_context_t* context_p,
                                fbe_eses_tunnel_fup_event_t event,
                                fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;
    // Initialize the retry counts and send a tunnel command with download control page. 
    context_p->failure_retry_count = 0;
    context_p->busy_retry_count = 0;
    context_p->tunnel_cmd_start = fbe_get_time();
    fbe_eses_enclosure_tunnel_set_expected_status_page_code(context_p, SES_PG_CODE_CONFIG);

    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_GET_CONFIGURATION);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_handle_failure_tunnel_send_get_config 
***************************************************************************
* @brief
*   The tunnel get configuration command failed.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*
* @return    next state 
*
* NOTES
*
* HISTORY
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_failure_tunnel_send_get_config(fbe_eses_tunnel_fup_context_t* context_p,
                                               fbe_eses_tunnel_fup_event_t event,
                                               fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // If the max retry count has reached, abort the operation. Otherwise, retry the operation.
    if (context_p->failure_retry_count > context_p->max_failure_retry_count)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s error retry failed: curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       // fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_TUNNEL_GET_WAIT_TIME);
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_TUNNEL_CMD_WAIT_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    if (fbe_eses_enclosure_tunnel_cmd_timed_out(context_p)) 
    {
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    context_p->failure_retry_count++;
    fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s error retry %d: curr_st %s evt %s\n",
                           __FUNCTION__,
                           context_p->failure_retry_count,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_GET_CONFIGURATION);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_handle_busy_tunnel_send_get_config 
***************************************************************************
* @brief                                       
*   The tunnel get configuration command got a device busy response.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_busy_tunnel_send_get_config(fbe_eses_tunnel_fup_context_t* context_p,
                                            fbe_eses_tunnel_fup_event_t event,
                                            fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // If the max retry count has reached, abort the operation. Otherwise, retry the operation.
    if (context_p->busy_retry_count > context_p->max_busy_retry_count)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s busy retry failed: curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    if (fbe_eses_enclosure_tunnel_cmd_timed_out(context_p)) 
    {
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    context_p->busy_retry_count++;
    fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s busy retry %d: curr_st %s evt %s\n",
                           __FUNCTION__,
                           context_p->busy_retry_count,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_GET_CONFIGURATION);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }


    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_send_dl_ctrl_page 
***************************************************************************
* @brief                                       
*   Tunnel a mcode download command.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_tunnel_send_dl_ctrl_page(fbe_eses_tunnel_fup_context_t* context_p,
                                  fbe_eses_tunnel_fup_event_t event,
                                  fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // Initialize the retry counts and send a tunnel command with download control page. 
    context_p->failure_retry_count = 0;
    context_p->busy_retry_count = 0;
    context_p->tunnel_cmd_start = fbe_get_time();
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_DOWNLOAD_FIRMWARE);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_handle_failure_tunnel_send_dl_ctrl_page 
***************************************************************************
* @brief                                       
*   The tunnel mcode download command failed.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_failure_tunnel_send_dl_ctrl_page(fbe_eses_tunnel_fup_context_t* context_p,
                                                 fbe_eses_tunnel_fup_event_t event,
                                                 fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // The tunnel command with download control page failed.
    // If the max retry count has reached, abort the operation. Otherwise, retry the operation.
    if (context_p->failure_retry_count > context_p->max_failure_retry_count)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s error retry failed: curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    if (fbe_eses_enclosure_tunnel_cmd_timed_out(context_p))
    {
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    context_p->failure_retry_count++;
    fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s error retry %d: curr_st %s evt %s\n",
                           __FUNCTION__,
                           context_p->failure_retry_count,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_DOWNLOAD_FIRMWARE);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_handle_busy_tunnel_send_dl_ctrl_page 
***************************************************************************
* @brief                                       
*   The tunnel mcode download command got a device busy response.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_busy_tunnel_send_dl_ctrl_page(fbe_eses_tunnel_fup_context_t* context_p,
                                              fbe_eses_tunnel_fup_event_t event,
                                              fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // Too busy to process the tunnel command with download control page.
    // If the max retry count has reached, abort the operation. Otherwise, retry the operation.
    if (context_p->busy_retry_count > context_p->max_busy_retry_count)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s busy retry failed: curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    if (fbe_eses_enclosure_tunnel_cmd_timed_out(context_p))
    {
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }
   
    context_p->busy_retry_count++;
    fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s busy retry %d: curr_st %s evt %s\n",
                           __FUNCTION__,
                           context_p->busy_retry_count,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_DOWNLOAD_FIRMWARE);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_get_tunnel_cmd_status 
***************************************************************************
* @brief                                       
*   Send receive diagnostic page 0x83.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                               fbe_eses_tunnel_fup_event_t event,
                               fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // Initialize the retry counts and send a receive diagnostic status page 0x83 command
    context_p->failure_retry_count = 0;
    context_p->busy_retry_count = 0;
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_handle_processing_get_tunnel_cmd_status 
***************************************************************************
* @brief                                       
*   The diagnotic page 0x83 returned a processing tunnel command status.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_processing_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                                                 fbe_eses_tunnel_fup_event_t event,
                                                 fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // Peer is hasn't responded yet. Get the status again if tunnel commannd has not timed out yet. 
    if (fbe_eses_enclosure_tunnel_cmd_timed_out(context_p))
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s tunnel cmd processing too long: curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s processing curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_handle_failure_get_tunnel_cmd_status 
***************************************************************************
* @brief                                       
*   Send receive diagnotic page 0x83 failed.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_failure_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                                              fbe_eses_tunnel_fup_event_t event,
                                              fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // The receive diagnostic status page 0x83 command failed.
    // If the max retry count has reached, abort the operation. Otherwise, retry the operation.
    if (context_p->failure_retry_count > context_p->max_failure_retry_count)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s error retry failed: curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    if (fbe_eses_enclosure_tunnel_cmd_timed_out(context_p))
    {
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    context_p->failure_retry_count++;
    fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s error retry %d: curr_st %s evt %s\n",
                           __FUNCTION__,
                           context_p->failure_retry_count,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS);


    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_handle_busy_get_tunnel_cmd_status 
***************************************************************************
* @brief                                       
*   Send receive diagnotic page 0x83 got a device busy response.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_busy_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                                           fbe_eses_tunnel_fup_event_t event,
                                           fbe_eses_tunnel_fup_state_t next_state)
{
     fbe_status_t status = FBE_STATUS_OK;

    // Too busy to process the receive diagnostic status page 0x83 command.
    // If the max retry count has reached, abort the operation. Otherwise, retry the operation.
    if (context_p->busy_retry_count > context_p->max_busy_retry_count)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s busy retry failed: curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    if (fbe_eses_enclosure_tunnel_cmd_timed_out(context_p))
    {
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    context_p->busy_retry_count++;
    fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s busy retry %d: curr_st %s evt %s\n",
                           __FUNCTION__,
                           context_p->busy_retry_count,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_receive_dl_status_page 
***************************************************************************
* @brief                                       
*   Tunnel a receive diagnostic mcode download status page.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_tunnel_receive_dl_status_page(fbe_eses_tunnel_fup_context_t* context_p,
                                       fbe_eses_tunnel_fup_event_t event,
                                       fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // Initialize the retry counts and send a tunnel command with receive download status page. 
    context_p->failure_retry_count = 0;
    context_p->busy_retry_count = 0;
    context_p->tunnel_cmd_start = fbe_get_time();
    fbe_eses_enclosure_tunnel_set_expected_status_page_code(context_p, SES_PG_CODE_DOWNLOAD_MICROCODE_STAT);

    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_GET_DOWNLOAD_STATUS);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_handle_failure_tunnel_receive_dl_status_page 
***************************************************************************
* @brief                                       
*   The tunnel receive diagnostic mcode download status page failed.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_failure_tunnel_receive_dl_status_page(fbe_eses_tunnel_fup_context_t* context_p,
                                                      fbe_eses_tunnel_fup_event_t event,
                                                      fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // The tunnel command with receive download status page has failed.  
    // If the max retry count has reached, abort the operation. Otherwise, retry the operation.
    if (context_p->failure_retry_count > context_p->max_failure_retry_count)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s error retry failed: curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    if (fbe_eses_enclosure_tunnel_cmd_timed_out(context_p))
    {
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    context_p->failure_retry_count++;
    fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s error retry %d: curr_st %s evt %s\n",
                           __FUNCTION__,
                           context_p->failure_retry_count,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_GET_DOWNLOAD_STATUS);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_handle_busy_tunnel_receive_dl_status_page 
***************************************************************************
* @brief                                              
*   The tunnel receive diagnostic mcode download status page got a device busy response.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_handle_busy_tunnel_receive_dl_status_page(fbe_eses_tunnel_fup_context_t* context_p,
                                                   fbe_eses_tunnel_fup_event_t event,
                                                   fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    // Too busy to process the tunnel command with receive download status page. 
    // If the max retry count has reached, abort the operation. Otherwise, retry the operation.
    if (context_p->busy_retry_count > context_p->max_busy_retry_count)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s busy retry failed: curr_st %s evt %s\n",
                           __FUNCTION__,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    if (fbe_eses_enclosure_tunnel_cmd_timed_out(context_p))
    {
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    context_p->busy_retry_count++;
    fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "%s busy retry %d: curr_st %s evt %s\n",
                           __FUNCTION__,
                           context_p->busy_retry_count,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(event));
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_GET_DOWNLOAD_STATUS);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_handle_dl_complete 
***************************************************************************
* @brief                                              
*   Microcode download command succeeded.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_tunnel_handle_dl_complete(fbe_eses_tunnel_fup_context_t* context_p,
                                   fbe_eses_tunnel_fup_event_t event,
                                   fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_time_t current_time = fbe_get_time();
    fbe_u32_t download_time; // in minutes
    fbe_eses_enclosure_t *eses_enclosure = context_p->eses_enclosure;


    // check if byte count is not increasing, this indicates
    // something is wrong and we could be stuck in a loop 
    // going through this path
    if (eses_enclosure->enclCurrentFupInfo.enclFupCurrTransferSize == 0)
    {
        eses_enclosure->enclCurrentFupInfo.enclFupNoBytesSent++;
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s 0 Bytes Transferred, Iterations:%d\n",
                            __FUNCTION__,
                            eses_enclosure->enclCurrentFupInfo.enclFupNoBytesSent);
    }
    else
    {
        eses_enclosure->enclCurrentFupInfo.enclFupNoBytesSent = 0;
    }

    if (eses_enclosure->enclCurrentFupInfo.enclFupNoBytesSent > 3)
    {
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    // A chuck of image has been successfully downloaded; 
    // The fbe_eses_enclosure_fup_check_for_more_bytes function uppdates the
    // eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred and sets the 
    // eses_enclosure->enclCurrentFupInfo.enclFupOperation to FBE_ENCLOSURE_FIRMWARE_OP_NONE
    // if the entire image has been transferred; otherwise, it reschedules the download 
    // condition right away and the fbe_eses_tunnel_download_fsm will restart with next 
    // chuck of image.
    eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred +=
        eses_enclosure->enclCurrentFupInfo.enclFupCurrTransferSize;
    eses_enclosure->enclCurrentFupInfo.enclFupCurrTransferSize = 0;

    if ((current_time - context_p->time_marker) >= FBE_ESES_ENCLOSURE_DL_LOG_TIME)
    {
       download_time = (fbe_get_elapsed_seconds(eses_enclosure->enclCurrentFupInfo.enclFupStartTime))/60; 
       fbe_base_object_customizable_trace((fbe_base_object_t*)(eses_enclosure),
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "FBE FUP %d bytes (of %d) transferred in %d minutes.\n",
                            eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred,
                            eses_enclosure->enclCurrentFupInfo.enclFupImageSize,
                            download_time);
       context_p->time_marker = current_time;
    }
        
    if (context_p->eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred >=
        context_p->eses_enclosure->enclCurrentFupInfo.enclFupImageSize)
    {
       // We have completed the transfer.
       download_time = (fbe_get_elapsed_seconds(context_p->eses_enclosure->enclCurrentFupInfo.enclFupStartTime))/60;
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "FBE FUP %d bytes (of %d) transferred in %d minutes - download completed.\n",
                            eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred,
                            eses_enclosure->enclCurrentFupInfo.enclFupImageSize,
                            download_time);

       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DONE;
       return FBE_ESES_EV_TUNNEL_FUP_LAST;
    }

    fbe_eses_enclosure_tunnel_set_delay(context_p, 0);
    context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;

    context_p->new_event = FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL;
    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_activate_check_rev_and_tunnel_receive_dl_status_page 
***************************************************************************
* @brief                                              
*   Tunnel a get download status command to check the activation status.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_activate_check_rev_and_tunnel_receive_dl_status_page(fbe_eses_tunnel_fup_context_t* context_p,
                                                              fbe_eses_tunnel_fup_event_t event,
                                                              fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t current_time;
    fbe_u32_t activation_time;
    fbe_eses_enclosure_t *eses_enclosure_p = context_p->eses_enclosure;

    // if activation in progress check for rev change or timeout exceeded
    if (fbe_eses_enclosure_is_peer_rev_changed(context_p))
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DONE;
       return FBE_ESES_ST_TUNNEL_FUP_LAST;
    }
    else if (context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_NONE &&
             context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_NONE)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DONE;
       return FBE_ESES_ST_TUNNEL_FUP_LAST;
    }
    else if (fbe_eses_enclosure_is_activate_timeout(context_p->eses_enclosure))
    {
       current_time = fbe_get_time();
       activation_time = fbe_get_elapsed_seconds(eses_enclosure_p->enclCurrentFupInfo.enclFupStartTime);
       fbe_base_object_customizable_trace((fbe_base_object_t*)(eses_enclosure_p),
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(eses_enclosure_p)),
                            "FBE FUP %s activation timed out after %d seconds.\n",
                            __FUNCTION__, activation_time);

       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_TUNNEL_GET_DOWNLOAD_STATUS);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_activate_check_rev_and_get_tunnel_cmd_status 
***************************************************************************
* @brief                                              
*   Send receive diagnostic page 0x83 to get the activation status.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_activate_check_rev_and_get_tunnel_cmd_status(fbe_eses_tunnel_fup_context_t* context_p,
                                                      fbe_eses_tunnel_fup_event_t event,
                                                      fbe_eses_tunnel_fup_state_t next_state)
{                                        
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t current_time;
    fbe_u32_t activation_time;
    fbe_eses_enclosure_t *eses_enclosure_p = context_p->eses_enclosure;

    // if activation in progress check for rev change or timeout exceeded
    if (fbe_eses_enclosure_is_peer_rev_changed(context_p))
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DONE;
       return FBE_ESES_ST_TUNNEL_FUP_LAST;
    }
    else if (context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_NONE &&
             context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_NONE)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DONE;
       return FBE_ESES_ST_TUNNEL_FUP_LAST;
    }
    else if (fbe_eses_enclosure_is_activate_timeout(context_p->eses_enclosure))
    {
       current_time = fbe_get_time();
       activation_time = fbe_get_elapsed_seconds(eses_enclosure_p->enclCurrentFupInfo.enclFupStartTime);
       fbe_base_object_customizable_trace((fbe_base_object_t*)(eses_enclosure_p),
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(eses_enclosure_p)),
                            "FBE FUP %s activation timed out after %d seconds.\n",
                            __FUNCTION__, activation_time);

       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }
 
    status = fbe_eses_enclosure_fup_send_funtional_packet(context_p->eses_enclosure,
                                                      context_p->packet,
                                                      FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS);

    if (FBE_STATUS_OK == status)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_PEND;
    }
    else
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_activate_check_rev 
***************************************************************************
* @brief                                              
*   Check if the firmware rev has changed.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*      
* @return    next state    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_activate_check_rev(fbe_eses_tunnel_fup_context_t* context_p,
                            fbe_eses_tunnel_fup_event_t event,
                            fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_eses_enclosure_t *eses_enclosure_p = context_p->eses_enclosure;
    fbe_time_t current_time;
    fbe_u32_t activation_time;
    
    // if activation in progress check for rev change or timeout exceeded
    if (fbe_eses_enclosure_is_peer_rev_changed(context_p))
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DONE;
       return FBE_ESES_ST_TUNNEL_FUP_LAST;
    }
    else if (context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_NONE &&
             context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_NONE)
    {
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DONE;
       return FBE_ESES_ST_TUNNEL_FUP_LAST;
    }
    else if (fbe_eses_enclosure_is_activate_timeout(eses_enclosure_p))
    {
       current_time = fbe_get_time();
       activation_time = fbe_get_elapsed_seconds(eses_enclosure_p->enclCurrentFupInfo.enclFupStartTime);
       fbe_base_object_customizable_trace((fbe_base_object_t*)(eses_enclosure_p),
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(eses_enclosure_p)),
                            "FBE FUP %s activation timed out after %d seconds.\n",
                            __FUNCTION__, activation_time);
       fbe_eses_enclosure_reset_peer_lcc(context_p->eses_enclosure);
       context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;
       fbe_eses_enclosure_tunnel_set_delay(context_p, FBE_ESES_PEER_LCC_RESET_POLL_TIME);
       return FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET;
    }

    // It takes about 22 seconds for LCC and 80 seconds for PS to activate.  
    // delay 3 seconds 
    fbe_eses_enclosure_tunnel_set_delay(context_p, 3000); 
    context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DELAY;

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_fup_wait_for_reset 
***************************************************************************
* @brief
*   After resetting the peer LCC, wait for it to come back online.
*
* @param     context_p  - tunnel fup conext
*            event      - event
*            next_state - default next state
*
* @return    next state 
*
* NOTES
*
* HISTORY
*   4-Apr-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_eses_tunnel_fup_state_t
fbe_eses_fup_wait_for_reset(fbe_eses_tunnel_fup_context_t* context_p,
                            fbe_eses_tunnel_fup_event_t event,
                            fbe_eses_tunnel_fup_state_t next_state)
{
    fbe_time_t current_time = fbe_get_time();
    fbe_time_t elapsed_time = 0;

    elapsed_time = current_time - context_p->time_marker;
    if (elapsed_time > FBE_ESES_LCC_RESET_TIME)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "FBE FUP reset completed in %ds: curr_st %s evt %s\n",
                           (int)elapsed_time/1000,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(fbe_eses_tunnel_fup_context_get_event(context_p)));

       // Sometimes peer PS upgrade takes a reset for the local see the rev change. 
       if ((FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE == context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperation) &&
            (context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperation == TRUE)&&
            (FBE_FW_TARGET_PS ==  context_p->eses_enclosure->enclCurrentFupInfo.enclFupComponent) &&
            (fbe_eses_enclosure_is_peer_rev_changed(context_p) || 
             (context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_NONE &&
              context_p->eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_NONE)))
       {
          context_p->schedule_op = FBE_ESES_TUNNEL_FUP_DONE;
       }
       else
       {
          context_p->schedule_op = FBE_ESES_TUNNEL_FUP_FAIL;
       }

       return (FBE_ESES_ST_TUNNEL_FUP_LAST);
    }

    return next_state;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_is_peer_rev_changed 
***************************************************************************
* @brief                                              
*   Utility function to check if peer device rev has changed.
*
* @param     context_p  - tunnel fup conext
*      
* @return    TRUE if rev has changed.    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
fbe_bool_t 
fbe_eses_enclosure_is_peer_rev_changed(fbe_eses_tunnel_fup_context_t* context_p)
{
    fbe_enclosure_fw_target_t fw_target;
    fbe_u32_t status;
    fbe_u8_t index = 0;
    fbe_edal_general_comp_handle_t generalDataPtr=NULL;
    char newrev[FBE_ESES_FW_REVISION_SIZE];
    fbe_bool_t rc = FBE_FALSE;
    fbe_eses_enclosure_t *eses_enclosure_p = context_p->eses_enclosure;
    fbe_time_t current_time = fbe_get_time();
    fbe_u32_t activation_time;
    fbe_enclosure_component_types_t fw_comp_type; 
    fbe_base_enclosure_attributes_t fw_comp_attr;
    fbe_edal_fw_info_t              fw_info;

    
    for (fw_target = FBE_FW_TARGET_LCC_EXPANDER; fw_target != FBE_FW_TARGET_MAX; fw_target++)
    {

        fbe_edal_get_fw_target_component_type_attr(eses_enclosure_p->enclCurrentFupInfo.enclFupComponent,
                                               &fw_comp_type,
                                               &fw_comp_attr);

        // check if the activation is for this fw component
        if ( eses_enclosure_p->enclCurrentFupInfo.enclFupComponent != fw_target)
        {
            continue;
        }

        status = (fbe_u32_t)fbe_eses_enclosure_map_fw_target_type_to_component_index(eses_enclosure_p, 
                                        fw_comp_type, 
                                        eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide, 
                                        0, // Overall Rev.
                                        &index);
    
        if (status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                           "is_rev_change: Failed to get component index for %s, side %d.\n", 
                           fbe_eses_enclosure_fw_targ_to_text(eses_enclosure_p->enclCurrentFupInfo.enclFupComponent),
                           eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide);
    
            return FBE_FALSE;
        }


        // the revision and identifier are in the public area, so need different access method
        status = (fbe_u32_t)fbe_base_enclosure_access_specific_component((fbe_base_enclosure_t *)eses_enclosure_p,
                                                           fw_comp_type,
                                                           index,
                                                           &generalDataPtr);
        if(FBE_STATUS_OK == status)
        {
            // copy the fw revision
            status = (fbe_u32_t)eses_enclosure_access_getStr(generalDataPtr,
                                      fw_comp_attr,
                                      fw_comp_type,
                                      sizeof(fw_info),
                                      (char *)&fw_info);    // dest of the copy

            if (FBE_EDAL_STATUS_OK == status)
            {
                // compare the old with the current, using memcmp in case null char encountered
                if (fbe_equal_memory(fw_info.fwRevision, 
                                      eses_enclosure_p->enclFwInfo->subencl_fw_info[fw_target].fwEsesFwRevision, 
                                      FBE_ESES_FW_REVISION_SIZE))
                {
                  if (FBE_ENCL_POWER_SUPPLY == fw_comp_type)
                  {
                     if ((current_time - context_p->time_marker) >= FBE_ESES_ENCLOSURE_PS_ACTIVATION_LOG_TIME)
                     {
                        activation_time = (fbe_get_elapsed_seconds(eses_enclosure_p->enclCurrentFupInfo.enclFupStartTime))/60;
                        context_p->time_marker = current_time;

                        fbe_base_object_customizable_trace((fbe_base_object_t*)(eses_enclosure_p),
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(eses_enclosure_p)),
                            "FBE FUP PS activation elasped %dm. Rev not changed new %s old %s side %d.\n",
                            activation_time, 
                            newrev, 
                            eses_enclosure_p->enclFwInfo->subencl_fw_info[fw_target].fwEsesFwRevision,
                            eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide);
                     }
                  }
                  else
                  {
                     if ((current_time - context_p->time_marker) >= FBE_ESES_ENCLOSURE_LCC_ACTIVATION_LOG_TIME)
                     {
                        activation_time = fbe_get_elapsed_seconds(eses_enclosure_p->enclCurrentFupInfo.enclFupStartTime);
                        context_p->time_marker = current_time;

                        fbe_base_object_customizable_trace((fbe_base_object_t*)(eses_enclosure_p),
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(eses_enclosure_p)),
                            "FBE FUP LCC activation elapsed %ds. Rev not changed new %s old %s side %d.\n",
                            activation_time, 
                            newrev, 
                            eses_enclosure_p->enclFwInfo->subencl_fw_info[fw_target].fwEsesFwRevision, 
                            eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide);
                     }
                  }
                }
                else
                {
                    activation_time = fbe_get_elapsed_seconds(eses_enclosure_p->enclCurrentFupInfo.enclFupStartTime);

                    fbe_base_object_customizable_trace((fbe_base_object_t*)(eses_enclosure_p),
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(eses_enclosure_p)),
                            "FBE FUP activation elapsed %ds. Rev changed new %s old %s side %d.\n",
                            activation_time, 
                            newrev, 
                            eses_enclosure_p->enclFwInfo->subencl_fw_info[fw_target].fwEsesFwRevision,
                            eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide);

                    rc = FBE_TRUE;
                    break;
                }
            }
        }
    }

    return rc;
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_fup_context_init 
***************************************************************************
* @brief                                              
*   Initialize the context.
*
* @param     context_p  - tunnel fup conext
*            eses_enclosure_p - pointer to enclosure
*            state - initial state
*            event - initial event
*            max_failure_retry - failure retry limit
*            max_busy_retry - busy retry limit
*      
* @return    NONE.    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
void
fbe_eses_tunnel_fup_context_init(fbe_eses_tunnel_fup_context_t *context_p,
                                 fbe_eses_enclosure_t *eses_enclosure_p,
                                 fbe_eses_tunnel_fup_state_t state,
                                 fbe_eses_tunnel_fup_event_t event,
                                 fbe_u8_t max_failure_retry,
                                 fbe_u8_t max_busy_retry)
{
    if (NULL != context_p)
    {
       context_p->current_state = state;
       context_p->previous_state = FBE_ESES_ST_TUNNEL_FUP_LAST;
       context_p->new_event = event;
       context_p->max_failure_retry_count = max_failure_retry;
       context_p->max_busy_retry_count = max_busy_retry;
       context_p->failure_retry_count = 0;
       context_p->busy_retry_count = 0;
       context_p->delay = 0;
       context_p->time_marker = fbe_get_time();
       context_p->eses_enclosure = eses_enclosure_p;
       context_p->packet = NULL;
    }
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_fup_state_to_string 
***************************************************************************
* @brief                                              
*   Reture the state string.
*
* @param     state - state to translate
*      
* @return    state string.    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
char *
fbe_eses_tunnel_fup_state_to_string(fbe_eses_tunnel_fup_state_t state)
{
    switch (state)
    {
        case FBE_ESES_ST_TUNNEL_FUP_INIT:
            return "INIT";
            break;
        case FBE_ESES_ST_TUNNEL_FUP_SEND_GET_CONFIG:
            return "SEND_GET_CONFIG";
            break;
        case FBE_ESES_ST_TUNNEL_FUP_READY:
            return "READY";
            break;
        case FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG:
            return "SEND_DL_CTRL_PG";
            break;
        case FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS:
            return "GET_TUNNEL_SEND_DL_CTRL_PG_STATUS";
            break;
        case FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG:
            return "TUNNEL_RECEIVE_DL_STATUS_PG";
            break;
        case FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS:
            return "GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS";
            break;
        case FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET:
            return "WAIT_FOR_RESET";
            break;
        case FBE_ESES_ST_TUNNEL_FUP_LAST:
            return "LAST";
            break;
        default:
            return "UNKNOWN";
    }
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_fup_event_to_string 
***************************************************************************
* @brief                                              
*   Return the event string.
*
* @param     event - event to translate
*      
* @return    event string.    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
char *
fbe_eses_tunnel_fup_event_to_string(fbe_eses_tunnel_fup_event_t event)
{
    switch (event)
    {
        case FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL:
            return "TUNNEL_DL";
            break;
        case FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED:
            return "SUCCEEDED";
            break;
        case FBE_ESES_EV_TUNNEL_FUP_PROCESSING:
            return "PROCESSING";
            break;
        case FBE_ESES_EV_TUNNEL_FUP_FAILED:
            return "FAILED";
            break;
        case FBE_ESES_EV_TUNNEL_FUP_BUSY:
            return "BUSY";
            break;
        case FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED:
            return "TUNNELED CMD FAILED";
            break;
        case FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED:
            return "DL FAILED";
            break;
        case FBE_ESES_EV_TUNNEL_FUP_LAST:
            return "LAST";
            break;
        default:
            return "UNKNOWN";
    }
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_fup_context_set_packet 
***************************************************************************
* @brief                                              
*   Save the packet pointer in the context.
*
* @param     context_p - the context 
*            packet_p - packet to save 
*      
* @return    NONE.    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
void
fbe_eses_tunnel_fup_context_set_packet(fbe_eses_tunnel_fup_context_t *context_p, fbe_packet_t *packet_p)
{
    context_p->packet = packet_p;
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_fup_context_set_event 
***************************************************************************
* @brief                                              
*   Save the next event to run in the context.
*
* @param     context_p - the context 
*            event - next event to run
*      
* @return    NONE.    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
void
fbe_eses_tunnel_fup_context_set_event(fbe_eses_tunnel_fup_context_t *context_p, fbe_eses_tunnel_fup_event_t event)
{
    context_p->new_event = event;
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_fup_context_get_event 
***************************************************************************
* @brief                                              
*   Return the next event to run.
*
* @param     context_p - the context 
*      
* @return    next event to run.    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
fbe_eses_tunnel_fup_event_t
fbe_eses_tunnel_fup_context_get_event(fbe_eses_tunnel_fup_context_t *context_p)
{
    return context_p->new_event;
}

/*!*************************************************************************
* @fn fbe_eses_tunnel_fup_encl_status_to_event 
***************************************************************************
* @brief                                              
*   Translate the encl_status to the next event to run.
*
* @param     encl_status - enclosure status 
*      
* @return    next event to run.    
*                          
* NOTES                    
*                          
* HISTORY                  
*   08-Mar-2011  - Kenny Huang Created.
***************************************************************************/
fbe_eses_tunnel_fup_event_t
fbe_eses_tunnel_fup_encl_status_to_event(fbe_enclosure_status_t encl_status)
{
    fbe_eses_tunnel_fup_event_t event;

    // See fbe_eses_enclosure_handle_scsi_cmd_response and fbe_enclosure.h
    switch (encl_status)
    {
    case FBE_ENCLOSURE_STATUS_OK:
        event = FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED;
        break;
    case FBE_ENCLOSURE_STATUS_PROCESSING_TUNNEL_CMD:
        event = FBE_ESES_EV_TUNNEL_FUP_PROCESSING;
        break;
    case FBE_ENCLOSURE_STATUS_BUSY:
        event = FBE_ESES_EV_TUNNEL_FUP_BUSY;
        break;
    case FBE_ENCLOSURE_STATUS_TUNNELED_CMD_FAILED:
        event = FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED;
        break;
    case FBE_ENCLOSURE_STATUS_TUNNEL_DL_FAILED:
        event = FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED;
        break;
    default:
        // We are not distinguish other types of errors to keep things simple. Note that 
        // because we are transferring lots of data across a I2C bus, we are subject 
        // to high probability of corrupted data (Kenny Huang).
        event = FBE_ESES_EV_TUNNEL_FUP_FAILED;
    }
    return event;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_tunnel_set_expected_status_page_code 
***************************************************************************
* @brief
*   Set the expected page code to be checked against data field of page 0x83.  
*
* @param     fup_context_p
*
* @return    ses_pg_code_enum
*
* NOTES
*
* HISTORY
*   23-Mar-2011  - Kenny Huang Created.
***************************************************************************/
void
fbe_eses_enclosure_tunnel_set_expected_status_page_code(fbe_eses_tunnel_fup_context_t* fup_context_p,
                                                        ses_pg_code_enum page_code) 
{
    fup_context_p->expected_pg_code = page_code;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_tunnel_get_expected_status_page_code 
***************************************************************************
* @brief
*   Return the expected page code in the data field of page 0x83.  
*
* @param     fup_context_p
*
* @return    ses_pg_code_enum
*
* NOTES
*
* HISTORY
*   23-Mar-2011  - Kenny Huang Created.
***************************************************************************/
ses_pg_code_enum
fbe_eses_enclosure_tunnel_get_expected_status_page_code(fbe_eses_tunnel_fup_context_t* fup_context_p) 
{
    return (fup_context_p->expected_pg_code);
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_tunnel_set_delay 
***************************************************************************
* @brief
*   Set the delay timer (in milliseconds) for next event processing.
*
* @param     fup_context_p
*
* @return   
*
* NOTES
*
* HISTORY
*   23-Mar-2011  - Kenny Huang Created.
***************************************************************************/
void
fbe_eses_enclosure_tunnel_set_delay(fbe_eses_tunnel_fup_context_t* fup_context_p,
                                    fbe_lifecycle_timer_msec_t delay)
{
    fup_context_p->delay = delay;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_tunnel_get_delay 
***************************************************************************
* @brief
*   Get the delay time (in milliseconds) for next event processing.
*
* @param     fup_context_p
*
* @return    delay in milliseconds
*
* NOTES
*
* HISTORY
*   23-Mar-2011  - Kenny Huang Created.
***************************************************************************/
fbe_lifecycle_timer_msec_t
fbe_eses_enclosure_tunnel_get_delay(fbe_eses_tunnel_fup_context_t* fup_context_p)
{
    return (fup_context_p->delay);
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_tunnel_cmd_timed_out 
***************************************************************************
* @brief
*   Get the delay time (in milliseconds) for next event processing.
*
* @param     fup_context_p
*
* @return    delay in milliseconds
*
* NOTES
*
* HISTORY
*   23-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_bool_t
fbe_eses_enclosure_tunnel_cmd_timed_out(fbe_eses_tunnel_fup_context_t* context_p)
{
    fbe_time_t current_time = fbe_get_time();
    fbe_time_t elapsed_time = 0;

    elapsed_time = current_time - context_p->tunnel_cmd_start;
    if (elapsed_time > FBE_ESES_TUNNEL_CMD_TIME_LIMIT)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)(context_p->eses_enclosure),
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)(context_p->eses_enclosure)),
                           "FBE FUP tunnel command timed out in %dms: curr_st %s evt %s\n",
                           (int)elapsed_time,
                           fbe_eses_tunnel_fup_state_to_string(context_p->current_state),
                           fbe_eses_tunnel_fup_event_to_string(fbe_eses_tunnel_fup_context_get_event(context_p)));
       return (FBE_TRUE); 
    }
 
    return (FBE_FALSE);
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_tunnel_configuration_page 
***************************************************************************
* @brief
*   Process the microcode download status bytes from the Download Microcode
*   Status page for tunnel download case. 
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       dl_status_page - The pointer to the returned download status.
*
* @return    fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   11-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_process_tunnel_configuration_page(fbe_eses_enclosure_t *eses_enclosure, 
                                       ses_common_pg_hdr_struct *pg_hdr)
{
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
     
    eses_enclosure->tunnel_fup_context_p.generation_code = 
                      fbe_eses_get_pg_gen_code(pg_hdr);

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_INFO,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s, new generation_code 0x%x\n",
                            __FUNCTION__,
                            eses_enclosure->tunnel_fup_context_p.generation_code);
  
    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_tunnel_download_ucode_status
***************************************************************************
* @brief
*   Process the microcode download status bytes from the Download Microcode
*   Status page for tunnel download case. 
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       dl_status_page - The pointer to the returned download status.
*
* @return    fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   11-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_process_tunnel_download_ucode_status(fbe_eses_enclosure_t *eses_enclosure, 
                                             fbe_eses_download_status_page_t *dl_status_page_p)
{
    fbe_enclosure_status_t encl_status;

    if (NULL == eses_enclosure)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_ESES_ENCLOSURE,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, Null pointer eses_enclosure\n", 
                            __FUNCTION__);

        return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
    }

    if (NULL == dl_status_page_p)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_ERROR,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s, Null pointer dl_status_page_p\n", 
                            __FUNCTION__);

        return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
    }

    // The following status is returned from the download microcode status diagnostic page
    switch(dl_status_page_p->dl_status.status)
    {
    case ESES_DOWNLOAD_STATUS_NONE :            // no download in  progress
        eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
        encl_status = FBE_ENCLOSURE_STATUS_OK;
        break;
    case ESES_DOWNLOAD_STATUS_IMAGE_IN_USE :    // Complete - subenclosure will attempt to use image
    case ESES_DOWNLOAD_STATUS_IN_PROGRESS :     // Interim - download in progress
    case ESES_DOWNLOAD_STATUS_UPDATING_FLASH :  // Interim - transfer complete, updating flash
    case ESES_DOWNLOAD_STATUS_UPDATING_NONVOL : // Interim - updating non-vol with image
    case ESES_DOWNLOAD_STATUS_ACTIVATE_IN_PROGRESS: // adding temporary support until expander fw changes
        encl_status = FBE_ENCLOSURE_STATUS_OK;
        break;
    case ESES_DOWNLOAD_STATUS_NEEDS_ACTIVATE :  // Complete - needs activation or hard reset
        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED;
        encl_status = FBE_ENCLOSURE_STATUS_OK;
        break;
    default :
        // ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD
        // ESES_DOWNLOAD_STATUS_ERROR_CHECKSUM 
        // ESES_DOWNLOAD_STATUS_ERROR_IMAGE
        // ESES_DOWNLOAD_STATUS_ERROR_BACKUP
        // ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED
        // ESES_DOWNLOAD_STATUS_NO_IMAGE
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                              FBE_TRACE_LEVEL_ERROR,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "FBE FUP Peer download_ucode_status page 0x%x error dlstatus0x%x addlstatus:0x%x\n",
                              dl_status_page_p->page_code,
                              dl_status_page_p->dl_status.status, 
                              dl_status_page_p->dl_status.addl_status);
        
        encl_status = FBE_ENCLOSURE_STATUS_TUNNEL_DL_FAILED;
        break;
    }

    if (FBE_ENCLOSURE_STATUS_OK == encl_status)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                          "FBE FUP Peer download_ucode_status page 0x%x dlstatus0x%x addlstatus:0x%x\n",
                          dl_status_page_p->page_code,
                          dl_status_page_p->dl_status.status, 
                          dl_status_page_p->dl_status.addl_status);
    }

    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_tunnel_status_page
***************************************************************************
* @brief
*   Process the microcode download status bytes from the Download Microcode
*   Status page for tunnel download case. 
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       dl_status_page - The pointer to the returned download status.
*
* @return    fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   11-Mar-2011  - Kenny Huang Created.
***************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_process_tunnel_status_page(fbe_eses_enclosure_t *eses_enclosure, 
                                              ses_common_pg_hdr_struct *pg_hdr)
{
    fbe_enclosure_status_t encl_status;
    ses_pg_code_enum expected_page_code;

    if (NULL == eses_enclosure)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_ESES_ENCLOSURE,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, Null pointer eses_enclosure\n", 
                            __FUNCTION__);

        return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
    }

    if (NULL == pg_hdr)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_ERROR,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "%s, Null pointer pg_hdr\n", 
                            __FUNCTION__);

        return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
    }
    
    expected_page_code = fbe_eses_enclosure_tunnel_get_expected_status_page_code(&(eses_enclosure->tunnel_fup_context_p));

    // for receive diag page must check for enclosure busy page
    if (SES_PG_CODE_ENCL_BUSY == pg_hdr->pg_code)
    {
        // enclosure is busy
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_ERROR,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "FBE FUP %s, busy page code\n", 
                             __FUNCTION__);

       encl_status = FBE_ENCLOSURE_STATUS_TUNNELED_CMD_FAILED;
    }
    else if (pg_hdr->pg_code != expected_page_code) 
    {
       // Work around of the rev 113 LCC firwmare bug: you may get the status page of previous command 
       // if you retrieves the status too quickly. Some test shows the wait time is 100 ms, but it is
       // not guarenteed.
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                             FBE_TRACE_LEVEL_ERROR,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                             "FBE FUP %s: wrong page code returned 0x%x expected 0x%x\n", 
                             __FUNCTION__,
                             pg_hdr->pg_code,
                             expected_page_code);

       encl_status = FBE_ENCLOSURE_STATUS_PROCESSING_TUNNEL_CMD;

    }
    else if (SES_PG_CODE_CONFIG == pg_hdr->pg_code) 
    {
       encl_status = fbe_eses_enclosure_process_tunnel_configuration_page(eses_enclosure, pg_hdr);
    }
    else if (SES_PG_CODE_DOWNLOAD_MICROCODE_STAT == pg_hdr->pg_code)
    {
       encl_status = fbe_eses_enclosure_process_tunnel_download_ucode_status(eses_enclosure,
                                                           (fbe_eses_download_status_page_t *)pg_hdr);
    }
    else
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                              FBE_TRACE_LEVEL_ERROR,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s, Invalid page code %d\n", 
                            __FUNCTION__, pg_hdr->pg_code);
       encl_status = FBE_ENCLOSURE_STATUS_TUNNELED_CMD_FAILED;
    }

    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_status_page_83h 
***************************************************************************
* @brief
*   This function handles received diagnostic status page 0x83
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*            sg_list - Contains the status page 0x83.
*
* @return    fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   11-Mar-2011  - Kenny Huang Created.
***************************************************************************/
fbe_enclosure_status_t
fbe_eses_enclosure_handle_status_page_83h(fbe_eses_enclosure_t *eses_enclosure,
                                          fbe_sg_element_t *sg_list)
{
    fbe_u32_t page_size = 0;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_eses_tunnel_cmd_status_page_header_t *pg_hdr;
    ses_common_pg_hdr_struct *data_p;
    fbe_u16_t data_len = 0;
    fbe_u16_t resp_len = 0;
    fbe_payload_cdb_scsi_status_t scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;
    fbe_u8_t *scsi_status_p;
    fbe_u8_t *sense_data;
    fbe_u8_t sense_key = 0;
    fbe_u8_t asc = 0;
    fbe_u8_t ascq = 0;

    pg_hdr = (fbe_eses_tunnel_cmd_status_page_header_t *)(sg_list[0].address);

    // Check page pointer.
    if (pg_hdr == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s,  NULL pointer for page 0x83.\n",
                           __FUNCTION__);

        eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN;
        return FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID;
    }
    if (pg_hdr->page_code == SES_PG_CODE_ENCL_BUSY)
    {
       eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_BUSY;
       return FBE_ENCLOSURE_STATUS_BUSY;
    }

    if (pg_hdr->page_code != SES_PG_CODE_TUNNEL_DIAG_STATUS)
    {
       eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_ERR_PAGE;
       return FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID;
    }

    // Check page size.
    page_size = fbe_eses_get_pg_size((ses_common_pg_hdr_struct*)pg_hdr);

    if (page_size > FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE)
    {
       eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_ERR_PAGE;
       return FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT;
    }

    // We need to have at least the status byte.
    if (page_size < 1)
    {
       eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_ERR_PAGE;
       return FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID;
    }

    // Parse the status field
    if (FBE_ESES_TUNNEL_DIAG_STATUS_SUCCESS == pg_hdr->status)
    {
       data_len = swap16(pg_hdr->data_len);
       resp_len = swap16(pg_hdr->resp_len);

       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "FBE FUP tunnel_cmd_status (successful): page 0x%x data_len %d resp_len %d.\n",
                                pg_hdr->page_code, data_len, resp_len);

       if (resp_len > 0)
       {
          // We got a scsi error. This is a bug in rev 1.13 LCC firmware that the scsi status is not included in
          // the response.
          if (resp_len <= 18)
          {
             scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
             sense_data = (fbe_u8_t*)((fbe_u8_t*)pg_hdr+sizeof(fbe_eses_tunnel_cmd_status_page_header_t)+data_len);
          }
          else
          {
              scsi_status_p = (fbe_u8_t*)((fbe_u8_t*)pg_hdr+sizeof(fbe_eses_tunnel_cmd_status_page_header_t)+data_len);
              scsi_status = *scsi_status_p;
              sense_data = scsi_status_p+1;
          }

          if (FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD != scsi_status)
          {
             sense_key = sense_data[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] & FBE_SCSI_SENSE_DATA_SENSE_KEY_MASK;
             asc = sense_data[FBE_SCSI_SENSE_DATA_ASC_OFFSET];
             ascq = sense_data[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET];
             fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_ERROR,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "FBE FUP tunnel cmd resp (check cond): sense_key 0x%x asc 0x%x ascq 0x%x resp_code 0x%x.\n",
                       sense_key, asc, ascq, *sense_data);

             encl_status = FBE_ENCLOSURE_STATUS_TUNNELED_CMD_FAILED;
          }
          else
          {
             encl_status = FBE_ENCLOSURE_STATUS_OK;
          }
       }

       // Process data and response fields.
       if ((encl_status == FBE_ENCLOSURE_STATUS_OK) && (data_len > 0))
       {
          data_p = (ses_common_pg_hdr_struct*)((fbe_u8_t*)pg_hdr + sizeof(fbe_eses_tunnel_cmd_status_page_header_t));
          encl_status = fbe_eses_enclosure_process_tunnel_status_page(eses_enclosure, data_p);
       }
    }
    else if ((FBE_ESES_TUNNEL_DIAG_STATUS_PROC_LOCAL == pg_hdr->status) ||
             (FBE_ESES_TUNNEL_DIAG_STATUS_PROC_PEER == pg_hdr->status))
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "FBE FUP tunnel_download_status_resp (processing): page 0x%x status 0x%x.\n",
                                pg_hdr->page_code, pg_hdr->status);
       eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_TUNNEL_PROCESSING;
       encl_status = FBE_ENCLOSURE_STATUS_PROCESSING_TUNNEL_CMD;
    }
    else
    {
            // FBE_ESES_TUNNEL_DIAG_STATUS_ABORTED:
            // FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_RESOURCES:
            // FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_COMM:
            // FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_COLLISION:
            // FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_REQ_INV:
            // FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_BUSY
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "FBE FUP tunnel_download_status_resp (failed): page 0x%x status 0x%x.\n",
                                pg_hdr->page_code, pg_hdr->status);
            encl_status = FBE_ENCLOSURE_STATUS_TUNNELED_CMD_FAILED;
    }

    return encl_status;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_reset_peer_lcc 
***************************************************************************
* @brief
*   This function resets peer lcc 
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*
* @return    NONE.
*
* NOTES
*
* HISTORY
*   01-Apr-2011  - Kenny Huang Created.
***************************************************************************/
static void
fbe_eses_enclosure_reset_peer_lcc(fbe_eses_enclosure_t *eses_enclosure)
{
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK; 
    fbe_status_t           status = FBE_STATUS_OK;
    fbe_u32_t              index;

    index = fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)eses_enclosure,
                                                          FBE_ENCL_COMP_IS_LOCAL,  //attribute
                                                          FBE_ENCL_LCC,  // Component type
                                                          0, //starting index
                                                          TRUE);
    // Peer lcc is the one behind the local lcc.
    index++;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "FBE FUP %s lcc index %d\n", __FUNCTION__, index);

    enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_LCC_POWER_CYCLE_REQUEST,
                                                    FBE_ENCL_LCC,
                                                    index,
                                                    FBE_ESES_ENCL_POWER_CYCLE_REQ_BEGIN);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "FBE FUP %s failed to set power cycle request.\n", __FUNCTION__);
    }

    enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_LCC_POWER_CYCLE_DURATION,
                                                    FBE_ENCL_LCC,
                                                    index,
                                                    0);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "FBE FUP %s failed to set power cycle duration.\n", __FUNCTION__);
    }

    enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_LCC_POWER_CYCLE_DELAY,
                                                    FBE_ENCL_LCC,
                                                    index,
                                                    0);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
       fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_ERROR,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "FBE FUP %s failed to set power cycle delay.\n", __FUNCTION__);
    }

    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const,
                                        (fbe_base_object_t*)eses_enclosure,
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, can't set ControlNeeded condition, status: 0x%x.\n",
                                __FUNCTION__, status);
    }
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_handle_schedule_op 
***************************************************************************
* @brief
*   This function handles download schedule operations returned by tunnel fsm. 
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*            schedule_op - schedule operation returned by the tunnel fsm.
*
* @return    fbe_lifecycle_status_t 
*
* NOTES
*
* HISTORY
*   01-Apr-2011  - Kenny Huang Created.
***************************************************************************/
fbe_lifecycle_status_t
fbe_eses_enclosure_handle_schedule_op(fbe_eses_enclosure_t *eses_enclosure, fbe_eses_tunnel_fup_schedule_op_t schedule_op)
{
    fbe_eses_tunnel_fup_context_t *tunnel_fup_context_p = &(eses_enclosure->tunnel_fup_context_p);
    fbe_lifecycle_status_t ret_status;
    fbe_lifecycle_timer_msec_t delay;
    fbe_status_t status;
    
    switch (schedule_op)
    {
        case FBE_ESES_TUNNEL_FUP_PEND:
            ret_status = FBE_LIFECYCLE_STATUS_PENDING;
            break;
        case FBE_ESES_TUNNEL_FUP_DELAY:
            delay = fbe_eses_enclosure_tunnel_get_delay(tunnel_fup_context_p);
            if (0 != delay)
            {
               // Reschedule after the delay.
               fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const,
                             (fbe_base_object_t*)eses_enclosure,
                             delay);

               ret_status = FBE_ESES_TUNNEL_FUP_DONE;
            }
            else
            {
               // Reschedule without delay. 
               ret_status = FBE_LIFECYCLE_STATUS_RESCHEDULE;
            }
            break;
        case FBE_ESES_TUNNEL_FUP_FAIL:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "FBE FUP %s op %d failed.\n",
                                        __FUNCTION__, eses_enclosure->enclCurrentFupInfo.enclFupOperation);

            eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;

            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                        (fbe_base_object_t*)eses_enclosure, 
                        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "FUP: Can't set FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE, status: 0x%x.\n",
                                status);
       
            }

            status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                            (fbe_base_object_t*)eses_enclosure,  
                                            0);

            ret_status = FBE_LIFECYCLE_STATUS_DONE;
            break;
        case FBE_ESES_TUNNEL_FUP_DONE:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "FBE FUP %s op %d completed.\n",
                                        __FUNCTION__, eses_enclosure->enclCurrentFupInfo.enclFupOperation);

            eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;

            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                    (fbe_base_object_t*)eses_enclosure, 
                     FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);
     
             if (status != FBE_STATUS_OK) 
             {
                 fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                 FBE_TRACE_LEVEL_ERROR,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                 "FUP: Can't set FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE, status: 0x%x.\n",
                                 status);
            
             }
     
             status = fbe_lifecycle_reschedule(&fbe_eses_enclosure_lifecycle_const, 
                                             (fbe_base_object_t*)eses_enclosure,  
                                             0);

            ret_status = FBE_LIFECYCLE_STATUS_DONE;
            break;
           
        default:
            eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
            eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)eses_enclosure);

            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s can't clear current condition, status: 0x%X",
                                        __FUNCTION__, status);

            }

            ret_status = FBE_LIFECYCLE_STATUS_DONE;
    }

    return ret_status;
}
