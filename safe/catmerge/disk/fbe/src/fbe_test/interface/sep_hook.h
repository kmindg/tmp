#ifndef SEP_HOOK_H
#define SEP_HOOK_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sep_hook.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the defines for the SEP debug hook functions.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_scheduler_interface.h"


/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/

/* debug hook function prototypes */
fbe_status_t fbe_test_add_debug_hook_active(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action);
fbe_status_t fbe_test_add_debug_hook_passive(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action);
fbe_status_t fbe_test_wait_for_debug_hook(fbe_object_id_t object_id, 
                                        fbe_u32_t state, 
                                        fbe_u32_t substate,
                                        fbe_u32_t check_type,
                                        fbe_u32_t action,
                                        fbe_u64_t val1,
                                        fbe_u64_t val2);
fbe_status_t fbe_test_wait_for_debug_hook_active(fbe_object_id_t object_id, 
                                                 fbe_u32_t state, 
                                                 fbe_u32_t substate,
                                                 fbe_u32_t check_type,
                                                 fbe_u32_t action,
                                                 fbe_u64_t val1,
                                                 fbe_u64_t val2);
fbe_status_t fbe_test_wait_for_debug_hook_passive(fbe_object_id_t object_id, 
                                                  fbe_u32_t state, 
                                                  fbe_u32_t substate,
                                                  fbe_u32_t check_type,
                                                  fbe_u32_t action,
                                                  fbe_u64_t val1,
                                                  fbe_u64_t val2);
fbe_status_t fbe_test_get_debug_hook(fbe_object_id_t rg_object_id, 
                                     fbe_u32_t state, 
                                     fbe_u32_t substate,
                                     fbe_u32_t check_type,
                                     fbe_u32_t action,
                                     fbe_u64_t val1,
                                     fbe_u64_t val2,
                                     fbe_scheduler_debug_hook_t *hook_p);
fbe_status_t fbe_test_get_debug_hook_active(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_scheduler_debug_hook_t *hook_p);
fbe_status_t fbe_test_get_debug_hook_passive(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_scheduler_debug_hook_t *hook_p);
fbe_status_t fbe_test_del_debug_hook_active(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action);
fbe_status_t fbe_test_del_debug_hook_passive(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action);
fbe_status_t fbe_test_wait_for_hook_counter(fbe_object_id_t object_id, 
                                                 fbe_u32_t state, 
                                                 fbe_u32_t substate,
                                                 fbe_u32_t check_type,
                                                 fbe_u32_t action,
                                                 fbe_u64_t val1,
                                                 fbe_u64_t val2,
                                                 fbe_u64_t counter);
fbe_status_t fbe_test_clear_all_debug_hooks_both_sps(void);
void fbe_test_validate_sniff_progress(fbe_object_id_t pvd_object_id);
void fbe_test_sniff_add_hooks(fbe_object_id_t pvd_object_id);
void fbe_test_sniff_del_hooks(fbe_object_id_t pvd_object_id);
void fbe_test_sniff_validate_no_progress(fbe_object_id_t pvd_object_id);
void fbe_test_zero_add_hooks(fbe_object_id_t pvd_object_id);
void fbe_test_zero_del_hooks(fbe_object_id_t pvd_object_id);
void fbe_test_zero_validate_no_progress(fbe_object_id_t pvd_object_id);
void fbe_test_copy_add_hooks(fbe_object_id_t vd_object_id);
void fbe_test_copy_del_hooks(fbe_object_id_t vd_object_id);
void fbe_test_copy_validate_no_progress(fbe_object_id_t vd_object_id);
void fbe_test_verify_validate_no_progress(fbe_object_id_t rg_object_id,
                                          fbe_lun_verify_type_t verify_type);

/*!*******************************************************************
 * @enum fbe_test_hook_action_t
 *********************************************************************
 * @brief This defines the type of action we should perform for the hooks.
 *
 *********************************************************************/
typedef enum fbe_test_hook_action_e
{
    FBE_TEST_HOOK_ACTION_INVALID,
    FBE_TEST_HOOK_ACTION_ADD,
    FBE_TEST_HOOK_ACTION_ADD_CURRENT,
    FBE_TEST_HOOK_ACTION_WAIT,
    FBE_TEST_HOOK_ACTION_WAIT_CURRENT,
    FBE_TEST_HOOK_ACTION_DELETE,
    FBE_TEST_HOOK_ACTION_DELETE_CURRENT,
    FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT,
    FBE_TEST_HOOK_ACTION_NEXT,
}
fbe_test_hook_action_t;

fbe_status_t fbe_test_use_rg_hooks(fbe_test_rg_configuration_t * const rg_config_p, 
                                   fbe_u32_t num_ds_objects,
                                   fbe_u32_t state, 
                                   fbe_u32_t substate,
                                   fbe_u64_t val1,
                                   fbe_u64_t val2,
                                   fbe_u32_t check_type,
                                   fbe_u32_t action,
                                   fbe_test_hook_action_t hook_action);

fbe_status_t fbe_test_rg_config_use_rg_hooks_both_sps(fbe_test_rg_configuration_t * const rg_config_p, 
                                                      fbe_u32_t raid_group_count,
                                                      fbe_u32_t num_ds_objects, 
                                                      fbe_u32_t state, 
                                                      fbe_u32_t substate,
                                                      fbe_u64_t val1,
                                                      fbe_u64_t val2,
                                                      fbe_u32_t check_type,
                                                      fbe_u32_t action,
                                                      fbe_test_hook_action_t hook_action);

fbe_status_t fbe_test_use_pvd_hooks(fbe_test_rg_configuration_t * const rg_config_p, 
                                   fbe_u32_t position,
                                   fbe_u32_t state, 
                                   fbe_u32_t substate,
                                   fbe_u64_t val1,
                                   fbe_u64_t val2,
                                   fbe_u32_t check_type,
                                   fbe_u32_t action,
                                   fbe_test_hook_action_t hook_action);

void fbe_test_sep_use_pause_rekey_hooks(fbe_test_rg_configuration_t * const rg_config_p,
                                        fbe_lba_t checkpoint,
                                        fbe_test_hook_action_t hook_action);

void fbe_test_sep_use_chkpt_hooks(fbe_test_rg_configuration_t * const rg_config_p,
                                  fbe_lba_t checkpoint,
                                  fbe_u32_t state, 
                                  fbe_u32_t substate,
                                  fbe_test_hook_action_t hook_action);

void fbe_test_sep_encryption_completing_hooks(fbe_test_rg_configuration_t * const rg_config_p, fbe_test_hook_action_t hook_action);
fbe_status_t fbe_test_sep_encryption_add_hook_and_enable_kms(fbe_test_rg_configuration_t * const rg_config_p, fbe_bool_t b_dualsp);

fbe_status_t fbe_test_add_job_service_hook(fbe_object_id_t object_id,                   
                                           fbe_job_type_t hook_type,                    
                                           fbe_job_action_state_t hook_state,           
                                           fbe_job_debug_hook_state_phase_t hook_phase, 
                                           fbe_job_debug_hook_action_t hook_action);

fbe_status_t fbe_test_wait_for_job_service_hook(fbe_object_id_t object_id,                     
                                                fbe_job_type_t hook_type,                      
                                                fbe_job_action_state_t hook_state,             
                                                fbe_job_debug_hook_state_phase_t hook_phase,   
                                                fbe_job_debug_hook_action_t hook_action);

fbe_status_t fbe_test_get_job_service_hook(fbe_object_id_t object_id,                     
                                           fbe_job_type_t hook_type,                      
                                           fbe_job_action_state_t hook_state,             
                                           fbe_job_debug_hook_state_phase_t hook_phase,   
                                           fbe_job_debug_hook_action_t hook_action,
                                           fbe_job_service_debug_hook_t *hook_p);

fbe_status_t fbe_test_del_job_service_hook(fbe_object_id_t object_id,                          
                                           fbe_job_type_t hook_type,                    
                                           fbe_job_action_state_t hook_state,           
                                           fbe_job_debug_hook_state_phase_t hook_phase, 
                                           fbe_job_debug_hook_action_t hook_action);


/* A simpler wrapper for fbe_api_scheduler_add/del_debug_hook*/
fbe_status_t fbe_test_add_scheduler_hook(const fbe_scheduler_debug_hook_t *hook);
fbe_status_t fbe_test_del_scheduler_hook(const fbe_scheduler_debug_hook_t *hook);
fbe_status_t fbe_test_wait_for_scheduler_hook(const fbe_scheduler_debug_hook_t *hook);

#endif /* SEP_HOOK_H */
