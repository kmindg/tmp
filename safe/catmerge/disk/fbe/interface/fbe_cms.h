#ifndef FBE_CMS_H
#define FBE_CMS_H

/***************************************************************************/
/** @file fbe_cms.h
***************************************************************************
*
* @brief
*  This file contains definitions of functions that are exported by the 
*  persistance service for use by internal clients
* 
***************************************************************************/
#include "fbe/fbe_service.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_sg_element.h"

#define CMS_OWNER_ID_NUMBER_OF_BITS (20)
#define CMS_OWNER_ID_MAX_VALUE ((1 << CMS_OWNER_ID_NUMBER_OF_BITS) - 1)
#define CMS_OWNER_ID_NUMBER_OF_CLIENT_BITS 6
#define CMS_OWNER_ID_NUMBER_OF_NON_CLIENT_BITS (CMS_OWNER_ID_NUMBER_OF_BITS - CMS_OWNER_ID_NUMBER_OF_CLIENT_BITS)
#define CMS_OWNER_ID_CLIENT_MASK ((1 << CMS_OWNER_ID_NUMBER_OF_CLIENT_BITS) - 1)
#define CMS_OWNER_ID_CREATE(x,y) (x + (y << CMS_OWNER_ID_NUMBER_OF_CLIENT_BITS))
#define CMS_OWNER_ID_GET_CLIENT_ID(x) (x & CMS_OWNER_ID_CLIENT_MASK)
#define CMS_OWNER_ID_GET_NON_CLIENT_PORTION(x) ((x & (~ CMS_OWNER_ID_CLIENT_MASK)) >> CMS_OWNER_ID_NUMBER_OF_CLIENT_BITS)

#define FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH 16

typedef enum fbe_cms_client_unsolicited_event_type_s
{
    FBE_CMS_CLIENT_UNSOLICITED_EVENT_TYPE_INVALID = 0,
    FBE_CMS_CLIENT_UNSOLICITED_EVENT_TYPE_DATA_LOST,
    FBE_CMS_CLIENT_UNSOLICITED_EVENT_TYPE_LAST
} fbe_cms_client_unsolicited_event_type_t;

typedef struct fbe_cms_client_unsolicited_event_s
{
    fbe_cms_client_unsolicited_event_type_t event_type;

    union {
        fbe_u32_t unused;
    } payload;
} fbe_cms_client_unsolicited_event_t;

/*! @enum fbe_cms_client_policy_type_t
 *  @brief 
 */
typedef enum fbe_cms_client_policy_type_e {
    FBE_CMS_CLIENT_POLICY_INVALID = 0,
    FBE_CMS_CLIENT_POLICY_NONE,
    FBE_CMS_CLIENT_POLICY_SIMPLE,
    FBE_CMS_CLIENT_POLICY_EXPLICIT,

    FBE_CMS_CLIENT_POLICY_LAST
} fbe_cms_client_policy_type_t;

/*! @enum fbe_cms_client_id_t
 *  @brief Identifiers for each consumer of CMS memory
 */
typedef enum fbe_cms_client_id_e {
    FBE_CMS_CLIENT_INTERNAL = 0,
    FBE_CMS_CLIENT_RAID_USER_DATA,
    FBE_CMS_CLIENT_RAID_METADATA,
    FBE_CMS_CLIENT_PAGED_METADATA,
    FBE_CMS_CLIENT_LAST
} fbe_cms_client_id_t;


/*! @enum fbe_cms_memory_persist_status_t
 *  @brief Enumerates all of the possible memory persistence statuses
 */
typedef enum fbe_cms_memory_persist_status_e
{
    FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL                      = 0x00000000,
    FBE_CMS_MEMORY_PERSIST_STATUS_STRUCT_INIT                     = 0x00000001,
    FBE_CMS_MEMORY_PERSIST_STATUS_STRUCT_CORRUPT                  = 0x00000002,
    FBE_CMS_MEMORY_PERSIST_STATUS_POWER_FAIL                      = 0x00000004,
    FBE_CMS_MEMORY_PERSIST_STATUS_SLEEP_FAIL                      = 0x00000008,
    FBE_CMS_MEMORY_PERSIST_STATUS_NORMAL_BOOT                     = 0x00000010,
    FBE_CMS_MEMORY_PERSIST_STATUS_IMPROPER_REQUEST                = 0x00000020,
    FBE_CMS_MEMORY_PERSIST_STATUS_REG_RESTORE_ERR                 = 0x00000040,
    FBE_CMS_MEMORY_PERSIST_STATUS_ECC_FAILURE                     = 0x00000080,
    FBE_CMS_MEMORY_PERSIST_STATUS_SMI_RESET                       = 0x00000100,
    FBE_CMS_MEMORY_PERSIST_STATUS_POST_ABORT                      = 0x00010000,
    FBE_CMS_MEMORY_PERSIST_STATUS_UTILITY_PARTITION_CLEARED       = 0x80000000,

} fbe_cms_memory_persist_status_t;

fbe_status_t fbe_cms_memory_get_memory_persistence_status(fbe_cms_memory_persist_status_t * status);

fbe_status_t fbe_cms_memory_request_unsafe_to_remove(fbe_cms_client_id_t client_id, 
                                                     fbe_bool_t request);

fbe_status_t fbe_cms_memory_request_persistence(fbe_cms_client_id_t client_id, 
                                                fbe_bool_t request);


/*********************************************************************/
/** @enum fbe_cms_control_code_t
*********************************************************************
* @brief enums for persistant service control codes
*********************************************************************/
typedef enum fbe_cms_control_code_e {
    FBE_CMS_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_CMS),

    FBE_CMS_CONTROL_CODE_GET_INFO,                      /**< Get generic information about the service  */
    FBE_CMS_CONTROL_CODE_GET_PERSISTENCE_STATUS,
    FBE_CMS_CONTROL_CODE_REQUEST_PERSISTENCE,
    FBE_CMS_CONTROL_CODE_GET_STATE_MACHINE_HISTORY,
	FBE_CMS_CONTROL_CODE_GET_ACCESS_FUNC_PTRS,
    FBE_CMS_CONTROL_CODE_LAST
}fbe_cms_control_code_t;

/*FBE_CMS_CONTROL_CODE_GET_INFO*/
typedef struct fbe_cms_get_info_s{
    fbe_u64_t   some_info;
}fbe_cms_get_info_t;

typedef struct fbe_cms_service_get_persistence_status_s
{
    fbe_cms_memory_persist_status_t status;
} fbe_cms_service_get_persistence_status_t;

typedef struct fbe_cms_service_request_persistence_s
{
    fbe_cms_client_id_t client_id;
    fbe_bool_t          request;
} fbe_cms_service_request_persistence_t;

typedef enum fbe_cms_env_event_e{
    FBE_CMS_ENV_EVENT_INVALID =             0x00000000,
    FBE_CMS_ENV_EVENT_AC_POWER_FAIL =   0x00000001,
}fbe_cms_env_event_t;

/************** 
* CMS Cluster 
***************/

/* Buffer Operation Opcodes */
typedef enum fbe_cms_buff_opcode_e{
    FBE_CMS_BUFF_NO_OP = 0,
    /* Start of valid opcodes */
    FBE_CMS_BUFF_ALLOC = 1,
    FBE_CMS_BUFF_CONT_ALLOC = 2,
    FBE_CMS_BUFF_COMMIT = 3,
    FBE_CMS_BUFF_FREE = 4,
    FBE_CMS_BUFF_FREE_ALL = 5,
    FBE_CMS_BUFF_ABORT = 6,
    FBE_CMS_BUFF_GET_EXCL_LOCK = 7,
    FBE_CMS_BUFF_GET_SHARED_LOCK = 8,
    FBE_CMS_BUFF_RELEASE_EXCL_LOCK = 9,
    FBE_CMS_BUFF_RELEASE_SHARED_LOCK = 10,
    FBE_CMS_BUFF_GET_FIRST = 11,
    FBE_CMS_BUFF_GET_NEXT = 12,
    /* End of Valid Opcodes */
    FBE_CMS_BUFF_INVALID = 13,
}fbe_cms_buff_opcode_t; 

/* Unique buffer allocation ID constructed by the client */
typedef struct fbe_cms_alloc_id_s
{   
    fbe_u64_t owner_id:CMS_OWNER_ID_NUMBER_OF_BITS; /* must be constructed such that its unique across all other possible owners */   
    fbe_u64_t buff_id; /* must be constructed such that its unique across all other outstanding allocs by this owner */ 
}fbe_cms_alloc_id_t;

/* Forward declaration of structure */
typedef struct fbe_cms_buff_req_s fbe_cms_buff_req_t;

/* Completion function for buffer requests */
typedef fbe_status_t (* fbe_cms_buff_comp_func_t) (fbe_cms_buff_req_t * req_p, void * context);

/* Tracking structure that client needs to pass along with every request */
struct fbe_cms_buff_req_s
{
    fbe_queue_element_t m_next;
    fbe_cms_buff_opcode_t m_opcode;
    fbe_cms_alloc_id_t m_alloc_id;
    fbe_u32_t m_alloc_size;
    fbe_u32_t m_offset;
    fbe_u32_t m_len;
    fbe_sg_element_t *m_sg_p; 
    fbe_u32_t m_num_sgs;
    fbe_cms_buff_comp_func_t m_comp_func;
    void * m_context;
    fbe_status_t m_status;
};

#endif /* FBE_CMS_H */


