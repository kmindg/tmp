#ifndef FBE_CMS_LIB_H
#define FBE_CMS_LIB_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_lib.h
 ***************************************************************************
 *
 * @brief
 *  This file contains function calls to be called by internal users of the
 *  persistent memory service
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe_cms.h"

/*the function prototypes below are needed so we can get pointers to them for testing purposes*/

typedef fbe_status_t (* fbe_cms_register_client_func_t) (fbe_cms_client_id_t            client_id,
                                                         fbe_cms_client_policy_type_t   policy,
                                                         void *                         event_callback,
                                                         fbe_u64_t                      memory_reservation,
                                                         fbe_u64_t                      memory_limit,
                                                         fbe_u32_t                      min_owner_ids,
                                                         fbe_u32_t                      max_owner_ids,
                                                         fbe_u32_t                      avg_owner_ids);

typedef fbe_status_t (* fbe_cms_unregister_client_func_t) (fbe_cms_client_id_t client_id);

typedef fbe_status_t (* fbe_cms_register_owner_func_t) (fbe_cms_client_id_t             client_id,
                                                        fbe_u64_t                       owner_id,
                                                        fbe_u64_t                       memory_reservation,
                                                        fbe_u64_t                       memory_limit,
                                                        fbe_u32_t                       shares_value);


typedef fbe_status_t (* fbe_cms_unregister_owner_func_t) (fbe_cms_client_id_t           client_id,
                                                          fbe_u64_t                     owner_id);

typedef fbe_status_t (* fbe_cms_buff_alloc_excl_func_t) (fbe_cms_buff_req_t * req_p,
														 fbe_cms_alloc_id_t alloc_id, 
														 fbe_u32_t alloc_size,
														 fbe_cms_buff_comp_func_t comp_func, 
														 void * context,
														 fbe_sg_element_t *sg_p,
														 fbe_u32_t num_sgs);

typedef fbe_status_t (* fbe_cms_buff_cont_alloc_excl_func_t) (fbe_cms_buff_req_t * req_p,
															  fbe_cms_alloc_id_t alloc_id, 
															  fbe_u32_t alloc_size,
															  fbe_cms_buff_comp_func_t comp_func,
															  void * context,
															  fbe_sg_element_t *sg_p);

typedef fbe_status_t (* fbe_cms_buff_commit_excl_func_t) (fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id,
														  fbe_u32_t offset, fbe_u32_t len, fbe_cms_buff_comp_func_t comp_func, 
														  void * context);

typedef fbe_status_t (* fbe_cms_buff_free_excl_func_t) (fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
														fbe_cms_buff_comp_func_t comp_func, void * context);

typedef fbe_status_t (* fbe_cms_buff_get_excl_lock_func_t) (fbe_cms_buff_req_t * req_p,
															fbe_cms_alloc_id_t alloc_id, 
															fbe_cms_buff_comp_func_t comp_func,
															void * context,
															fbe_sg_element_t *sg_p,
															fbe_u32_t num_sgs);

typedef fbe_status_t (* fbe_cms_buff_release_excl_lock_func_t) (fbe_cms_buff_req_t * req_p, 
																fbe_cms_alloc_id_t alloc_id);

typedef fbe_status_t (* fbe_cms_buff_free_all_unlocked_func_t) (fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
																fbe_cms_buff_comp_func_t comp_func, 
																void * context);

typedef fbe_status_t (* fbe_cms_buff_request_abort_func_t) (fbe_cms_buff_req_t * req_p);


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
                                     fbe_u32_t                      avg_owner_ids);

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-02 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_unregister_client(fbe_cms_client_id_t client_id);

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
                                    fbe_u32_t                       shares_value);

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-02 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_unregister_owner(fbe_cms_client_id_t           client_id,
                                      fbe_u64_t                     owner_id);

/*********************************************************************
 *            fbe_cms_buff_alloc_excl()
 *********************************************************************
 * Description: 
 * Allocates cluster buffer of requested size. Exclusive lock is granted.
 * Asynchronous completion. No partial allocations.
 *   
 * Inputs:
 * @param req_p - Tracking structure associated with this request.
 * @param alloc_id - unique key generated by client to identify this buff allocation. 
 * @param alloc_size - size in bytes of cluster memory requested by the client.
 * @param callback - callback function pointer provided by the client.  
 * @param context - context for the callback.
 * @param sg_p - filled by cms and passed back thru req_p in callback function.
 * @param num_sgs - number of entries in sgl. 
 *
 * Return Value: status_success if request is accepted. 
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_alloc_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                     fbe_u32_t alloc_size, fbe_cms_buff_comp_func_t comp_func, 
                                     void * context, fbe_sg_element_t *sg_p, fbe_u32_t num_sgs);
                                     
/*********************************************************************
 *            fbe_cms_buff_cont_alloc_excl()
 *********************************************************************
 * Description: 
 * Allocates contiguous cluster buffer of requested size. Exclusive lock is granted.
 * Asynchronous completion. No partial allocations.
 *   
 * Inputs:
 * @param req_p - Tracking structure associated with this request.
 * @param alloc_id - unique key generated by client to identify this buff allocation. 
 * @param alloc_size - size in bytes of cluster memory requested by the client.
 * @param callback - callback function pointer provided by the client.  
 * @param context - context for the callback. 
 * @param sg_p - filled by cms and passed back thru req_p in callback function.
 *
 * Return Value: status_success if request is accepted. 
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_cont_alloc_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                          fbe_u32_t alloc_size, fbe_cms_buff_comp_func_t comp_func,
                                          void * context, fbe_sg_element_t *sg_p);

/*********************************************************************
 *            fbe_cms_buff_commit_excl()
 *********************************************************************
 * Description: 
 * Contents of the cluster buffer are persisted. Exclusive lock is required. 
 * Asynchronous completion. Persistance is not guaranteed until callback is
 * invoked.
 *
 * Inputs:
 * @param req_p - Tracking structure associated with this request. 
 * @param alloc_id - alloc_id of buffer that needs commit.
 * @param offset - offset into buffer from where commit is required.
 * @param len - length of buffer from offset that needs commit.
 * @param callback - callback function pointer provided by the client.  
 * @param context - context for the callback.
 *
 * Return Value: status_success if request is accepted.
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_commit_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id,
                                      fbe_u32_t offset, fbe_u32_t len, fbe_cms_buff_comp_func_t comp_func, 
                                      void * context);

/*********************************************************************
 *            fbe_cms_buff_free_excl()
 *********************************************************************
 * Description: 
 * Cluster buffer associated with alloc_id is freed. Exclusive lock is   
 * required. Asynchronous completion.
 * 
 * Inputs:
 * @param req_p - Tracking structure associated with this request.
 * @param alloc_id - alloc_id of buffer that needs to be freed.
 * @param callback - callback function pointer provided by the client.  
 * @param context - context for the callback. 
 *
 * Return Value: status_success if request is accepted.
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_free_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                    fbe_cms_buff_comp_func_t comp_func, void * context);

/*********************************************************************
 *            fbe_cms_buff_request_abort()
 *********************************************************************
 * Description: 
 * Request to abort a pending buffer operation. Synchronous completion. 
 * Client should expect either an aborted operation or a successfully
 * completed operation in the callback.  
 *
 * Inputs:
 * @param req_p - Request that needs to be aborted.
 *
 * Return Value: status_success if abort request is accepted.
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_request_abort(fbe_cms_buff_req_t * req_p);


/*********************************************************************
 *            fbe_cms_buff_get_excl_lock()
 *********************************************************************
 * Description:
 * Acquires exclusive lock on cluster buffer and returns SGL describing it.
 * Asynchronous completion.   
 *
 * Inputs:
 * @param req_p - Tracking structure associated with this request.
 * @param alloc_id - alloc_id of buffer on which exclusive lock is required.
 * @param callback - callback function pointer provided by the client.  
 * @param context - context for the callback.
 * @param sg_p - filled by cms and passed back thru callback function.
 * @param num_sgs - number of entries in sgl.
 *
 * Return Value: status_success if request is accepted.
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_get_excl_lock(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                        fbe_cms_buff_comp_func_t comp_func, void * context,
                                        fbe_sg_element_t *sg_p, fbe_u32_t num_sgs);

/*********************************************************************
 *            fbe_cms_buff_get_shared_lock()
 *********************************************************************
 * Description:
 * Acquires shared lock on cluster buffer and returns SGL describing it.
 * Asynchronous completion.  
 *
 * Inputs:
 * @param req_p - Tracking structure associated with this request.
 * @param alloc_id - alloc_id of buffer on which shared lock is required.
 * @param callback - callback function pointer provided by the client.  
 * @param context - context for the callback.
 * @param sg_p - filled by cms and passed back thru callback function.
 * @param num_sgs - number of entries in sgl.
 *
 * Return Value: status_success if request is accepted.
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_get_shared_lock(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                          fbe_cms_buff_comp_func_t comp_func, 
                                          void * context, fbe_sg_element_t *sg_p, 
                                          fbe_u32_t num_sgs);

/*********************************************************************
 *            fbe_cms_buff_release_excl_lock()
 *********************************************************************
 * Description:
 * Releases exclusive lock on cluster buffer. Synchronous completion.  
 *
 * Inputs:
 * @param req_p - Tracking structure associated with this request.
 * @param alloc_id - alloc_id of buffer on which lock must be released.
 *
 * Return Value: completion status of the operation.
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_release_excl_lock(fbe_cms_buff_req_t * req_p, 
											fbe_cms_alloc_id_t alloc_id);

/*********************************************************************
 *            fbe_cms_buff_release_shared_lock()
 *********************************************************************
 * Description:
 * Releases shared lock on cluster buffer. Synchronous completion.  
 *
 * Inputs:
 * @param req_p - Tracking structure associated with this request.
 * @param alloc_id - alloc_id of buffer on which lock must be released.
 *
 * Return Value: completion status of the operation.
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_release_shared_lock(fbe_cms_buff_req_t * req_p, 
											  fbe_cms_alloc_id_t alloc_id);


/*********************************************************************
 *            fbe_cms_buff_get_first_excl()
 *********************************************************************
 * Description:
 * Returns a cluster buffer belonging to a client identified by owner_id.  
 * Asynchronous completion. Exclusive lock is granted on the buffer.
 * Client should not expect any particular order in which buffers are 
 * returned.
 *
 * Inputs:
 * @param req_p - Tracking structure associated with this request. 
 * @param owner_id - owner_id of the client.
 * @param callback - callback function pointer provided by the client.  
 * @param context - context for the callback.
 * @param sg_p - filled by cms and passed back thru callback function.
 * @param num_sgs - number of entries in sgl.
 *
 * Return Value: status_success if request is accepted.
 *
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_get_first_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                         fbe_cms_buff_comp_func_t comp_func, 
                                         void * context, fbe_sg_element_t *sg_p, 
                                         fbe_u32_t num_sgs);

/*********************************************************************
 *            fbe_cms_buff_get_next_excl()
 *********************************************************************
 * Description: 
 * Returns buffer next to buffer identified by alloc_id.   
 * Asynchronous completion. Exclusive lock is granted on the buffer.
 * Client should not expect any particular order in which buffers are 
 * returned.
 * 
 * Inputs:
 * @param req_p - Tracking structure associated with this request. 
 * @param alloc_id - alloc_id of cluster buffer returned in previous call 
                     to fbe_cms_buff_get_first_excl() or this function.
 * @param callback - callback function pointer provided by the client.  
 * @param context - context for the callback.
 * @param sg_p - filled by cms and passed back thru callback function.
 * @param num_sgs - number of entries in sgl. 
 *
 * Return Value: status_success if request is accepted.
 *
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_get_next_excl(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                        fbe_cms_buff_comp_func_t comp_func, 
                                        void * context, fbe_sg_element_t *sg_p, 
                                        fbe_u32_t num_sgs);

/*********************************************************************
 *            fbe_cms_buff_free_all_unlocked()
 *********************************************************************
 * Description:
 * Frees all cluster buffers belonging to a client identified by owner_id.  
 * Asynchronous completion. Buffer must be unlocked for free to be successful.
 *
 * Inputs:
 * @param req_p - Tracking structure associated with this request. 
 * @param owner_id - owner_id of the client.
 * @param callback - callback function pointer provided by the client.  
 * @param context - context for the callback.
 *
 * Return Value: status_success if request is accepted.
 *
 * @version
 *   2011-11-04 - Created. Vamsi V
 *********************************************************************/
fbe_status_t fbe_cms_buff_free_all_unlocked(fbe_cms_buff_req_t * req_p, fbe_cms_alloc_id_t alloc_id, 
                                            fbe_cms_buff_comp_func_t comp_func, 
                                            void * context);

#endif /* end FBE_CMS_LIB_H */
