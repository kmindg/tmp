/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cms_library.c
 ***************************************************************************
 *
 *  Description
 *      Library implementation for access of the persistent memory service.
 *      we don't want direct calls to the service since in the future, the plan is
 *      to take it out, and then the implementation will change here and the 
 *      API will stay the same
 *
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe_cms_lib_access.h"
#include "fbe_cms_lib.h"
#include "fbe_cms_operations.h"
#include "fbe_cms_main.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-02 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_register_client(fbe_cms_client_id_t            client_id,
                                     fbe_cms_client_policy_type_t   policy,
                                     void *                         event_callback,
                                     fbe_u64_t                      memory_reservation,
                                     fbe_u64_t                      memory_limit,
                                     fbe_u32_t                      min_owner_ids,
                                     fbe_u32_t                      max_owner_ids,
                                     fbe_u32_t                      avg_owner_ids)
{
    return fbe_cms_bouncer_register_client(client_id,
                                           policy,
                                           event_callback,
                                           memory_reservation,
                                           memory_limit,
                                           min_owner_ids,
                                           max_owner_ids,
                                           avg_owner_ids);

}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-02 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_unregister_client(fbe_cms_client_id_t client_id)
{
    return fbe_cms_bouncer_unregister_client(client_id);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-02 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_register_owner(fbe_cms_client_id_t             client_id,
                                    fbe_u64_t                       owner_id,
                                    fbe_u64_t                       memory_reservation,
                                    fbe_u64_t                       memory_limit,
                                    fbe_u32_t                       shares_value)
{
    return fbe_cms_bouncer_register_owner(client_id,
                                          owner_id,
                                          memory_reservation,
                                          memory_limit,
                                          shares_value);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-02 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_unregister_owner(fbe_cms_client_id_t           client_id,
                                      fbe_u64_t                     owner_id)
{
    return fbe_cms_bouncer_unregister_owner(client_id, owner_id);
}

fbe_status_t fbe_cms_buff_alloc_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                     fbe_u32_t alloc_size, fbe_cms_buff_comp_func_t comp_func, 
                                     void * context, fbe_sg_element_t *sg_p, fbe_u32_t num_sgs)
{
	/* Perform any sanity check of parameters received from client */
	req_p->m_opcode = FBE_CMS_BUFF_ALLOC; 
	req_p->m_alloc_id = alloc_id;
	req_p->m_alloc_size = alloc_size;
	req_p->m_comp_func = comp_func;
	req_p->m_context = context;
	req_p->m_sg_p = sg_p;
	req_p->m_num_sgs = num_sgs;

	return fbe_cms_bouncer_allocate(req_p);
}

fbe_status_t fbe_cms_buff_cont_alloc_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                          fbe_u32_t alloc_size, fbe_cms_buff_comp_func_t comp_func,
                                          void * context, fbe_sg_element_t *sg_p)
{
    return fbe_cms_bouncer_allocate(req_p);
}

fbe_status_t fbe_cms_buff_commit_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id,
                                      fbe_u32_t offset, fbe_u32_t len, fbe_cms_buff_comp_func_t comp_func, 
                                      void * context)
{
    return fbe_cms_operations_handle_incoming_req(req_p);
}

fbe_status_t fbe_cms_buff_free_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                    fbe_cms_buff_comp_func_t comp_func, void * context)
{
    return fbe_cms_operations_handle_incoming_req(req_p);
}

fbe_status_t fbe_cms_buff_request_abort(fbe_cms_buff_req_t * req_p)
{
    return fbe_cms_bouncer_abort(req_p);
}

fbe_status_t fbe_cms_buff_get_excl_lock(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                        fbe_cms_buff_comp_func_t comp_func, void * context,
                                        fbe_sg_element_t *sg_p, fbe_u32_t num_sgs)
{
    return fbe_cms_operations_handle_incoming_req(req_p);
}

fbe_status_t fbe_cms_buff_get_shared_lock(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                          fbe_cms_buff_comp_func_t comp_func, 
                                          void * context, fbe_sg_element_t *sg_p, 
                                          fbe_u32_t num_sgs)
{
    return fbe_cms_operations_handle_incoming_req(req_p);
}

fbe_status_t fbe_cms_buff_release_excl_lock(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id)
{
    return fbe_cms_operations_handle_incoming_req(req_p);
}

fbe_status_t fbe_cms_buff_release_shared_lock(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id)
{
    return fbe_cms_operations_handle_incoming_req(req_p);
}

fbe_status_t fbe_cms_buff_get_first_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                         fbe_cms_buff_comp_func_t comp_func, 
                                         void * context, fbe_sg_element_t *sg_p, 
                                         fbe_u32_t num_sgs)
{
    return fbe_cms_operations_handle_incoming_req(req_p);
}

fbe_status_t fbe_cms_buff_get_next_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                        fbe_cms_buff_comp_func_t comp_func, 
                                        void * context, fbe_sg_element_t *sg_p, 
                                        fbe_u32_t num_sgs)
{
    return fbe_cms_operations_handle_incoming_req(req_p);
}

fbe_status_t fbe_cms_buff_free_all_unlocked(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                            fbe_cms_buff_comp_func_t comp_func, 
                                            void * context)
{
    return fbe_cms_operations_handle_incoming_req(req_p);
}
