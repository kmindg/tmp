#ifndef FBE_CMS_MAIN_H
#define FBE_CMS_MAIN_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_main.h
 ***************************************************************************
 *
 * @brief
 *  This file contains cms driver related interfaces.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_cms.h"

/*functions*/
void * fbe_cms_cmm_memory_allocate(fbe_u32_t allocation_size_in_bytes);
void fbe_cms_cmm_memory_release(void * ptr);
fbe_status_t fbe_cms_client_init();
fbe_status_t fbe_cms_client_destroy();

fbe_status_t fbe_cms_bouncer_abort(fbe_cms_buff_req_t * buf_req);

fbe_status_t fbe_cms_bouncer_allocate(fbe_cms_buff_req_t * buf_req);

fbe_status_t fbe_cms_bouncer_register_client(fbe_cms_client_id_t            client_id,
                                             fbe_cms_client_policy_type_t   policy,
                                             void *                         event_callback,
                                             fbe_u64_t                      memory_reservation,
                                             fbe_u64_t                      memory_limit,
                                             fbe_u32_t                      min_owner_ids,
                                             fbe_u32_t                      max_owner_ids,
                                             fbe_u32_t                      avg_owner_ids);

fbe_status_t fbe_cms_bouncer_unregister_client(fbe_cms_client_id_t client_id);

fbe_status_t fbe_cms_bouncer_register_owner(fbe_cms_client_id_t             client_id,
                                            fbe_u64_t                       owner_id,
                                            fbe_u64_t                       memory_reservation,
                                            fbe_u64_t                       memory_limit,
                                            fbe_u32_t                       shares_value);

fbe_status_t fbe_cms_bouncer_unregister_owner(fbe_cms_client_id_t           client_id,
                                              fbe_u64_t                     owner_id);

typedef enum fbe_cms_init_state_e
{
    FBE_CMS_INIT_STATE_INVALID = 0,
    FBE_CMS_INIT_STATE_SYNCHRONOUS_INIT,
    FBE_CMS_INIT_GET_INTER_SP_LOCK,
    FBE_CMS_INIT_ISSUE_CDR_READ,
    FBE_CMS_INIT_CDR_READ_COMPLETE,
    FBE_CMS_INIT_SEND_MEMORY_MAP,
    FBE_CMS_INIT_MEMORY_MAP_SENT,
    FBE_CMS_INIT_RECEIVE_MEMORY_MAP,
    FBE_CMS_INIT_CARVE_MEMORY,
    FBE_CMS_INIT_RELEASE_INTER_SP_LOCK,
    FBE_CMS_INIT_STATE_FAILED,
    FBE_CMS_INIT_STATE_DONE
} fbe_cms_init_state_t;

typedef enum fbe_cms_destroy_state_e
{
    FBE_CMS_DESTROY_STATE_INVALID = 0,
    FBE_CMS_DESTROY_SYNCHRONOUS_DESTROY,
    FBE_CMS_DESTROY_STATE_FAILED,
    FBE_CMS_DESTROY_STATE_DONE
} fbe_cms_destroy_state_t;

typedef enum fbe_cms_init_sm_status_e
{
    FBE_CMS_STATUS_WAITING = 0,
    FBE_CMS_STATUS_EXECUTING
} fbe_cms_init_sm_status_t;

#endif /*FBE_CMS_MAIN_H*/
