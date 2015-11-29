#ifndef FBE_CMS_CLIENT_PRIVATE_H
#define FBE_CMS_CLIENT_PRIVATE_H

/***************************************************************************/
/** @file fbe_cms_client_private.h
***************************************************************************
*
* @brief
*  
* 
***************************************************************************/

#include "fbe_types.h"
#include "fbe_winddk.h"
#include "fbe_cms.h"


/*! @struct
 *  @brief 
 */
typedef struct fbe_cms_client_owner_data_s
{
    fbe_bool_t              registered;

    /* Static info provided during registration*/
    fbe_cms_client_id_t     client_id;
    fbe_u64_t               owner_id;
    fbe_u64_t               memory_reservation;
    fbe_u64_t               memory_limit;
    fbe_u32_t               shares_value;

    /* Dynamic info used by the policy engine */
    fbe_u64_t               number_of_bytes_allocated;
    fbe_u64_t               number_of_allocations;

    fbe_queue_head_t        queued_requests;

} fbe_cms_client_owner_data_t;

/*! @struct
 *  @brief 
 */
typedef struct fbe_cms_client_data_s
{
    fbe_bool_t                      registered;

    /* Static info provided during registration*/
    fbe_cms_client_id_t             client_id;
    fbe_cms_client_policy_type_t    policy;
    void *                          event_callback;
    fbe_u64_t                       memory_reservation;
    fbe_u64_t                       memory_limit;
    fbe_u32_t                       min_owner_ids;
    fbe_u32_t                       max_owner_ids;
    fbe_u32_t                       avg_owner_ids;

    /* Dynamic info used by the policy engine */
    fbe_u32_t                       number_of_owner_ids;
    fbe_u64_t                       number_of_bytes_allocated;
    fbe_u64_t                       number_of_allocations;
    fbe_u64_t                       number_of_shares;

    fbe_queue_head_t                queued_requests;

    fbe_cms_client_owner_data_t *   owner_data;

} fbe_cms_client_data_t;

/*! @struct
 *  @brief 
 */
typedef struct fbe_cms_client_table_s
{
    /* Lock to protect this structure */
    fbe_spinlock_t          lock;
    fbe_u32_t               number_of_clients;
    fbe_u64_t               number_of_bytes_allocated;
    fbe_u64_t               number_of_allocations;
    fbe_cms_client_data_t   client_data[FBE_CMS_CLIENT_LAST];

} fbe_cms_client_table_t;




#endif  /* FBE_CMS_CLIENT_PRIVATE_H */
