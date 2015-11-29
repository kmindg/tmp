/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_main.c
***************************************************************************
*
* @brief
*  This file contains persistance service functions including control entry.
*  
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_main.h"
#include "fbe_cms_private.h"
#include "fbe_cms_cmi.h"
#include "fbe_cms_environment.h"
#include "fbe_cms_state_machine.h"
#include "fbe_cms_memory.h"
#include "fbe_cms_cluster.h"
#include "fbe_cms_operations.h"
#include "fbe_cms_lun_access.h"
#include "fbe_cmm.h"
#include "fbe_cms_cdr_layout.h"


/* Declare our service methods */
fbe_status_t fbe_cms_control_entry(fbe_packet_t * packet);
fbe_service_methods_t fbe_cms_methods = {FBE_SERVICE_ID_CMS, fbe_cms_control_entry};

/* static variables */
static fbe_cms_t cms_service;/*the instance of the service that will keep service related stuff in it*/
static CMM_POOL_ID cmm_control_pool_id = 0;
static CMM_CLIENT_ID cmm_client_id = 0;
static fbe_cms_init_state_t fbe_cms_init_state;
static fbe_packet_t * fbe_cms_init_packet;
static fbe_cms_destroy_state_t fbe_cms_destroy_state;
static fbe_packet_t * fbe_cms_destroy_packet;


/* private functions */
static fbe_status_t cms_init(fbe_packet_t * packet);
static fbe_status_t cms_destroy(fbe_packet_t * packet);
static fbe_status_t cms_get_info(fbe_packet_t * packet);
static fbe_status_t cms_get_persistence_status(fbe_packet_t * packet);
static fbe_status_t cms_request_persistence(fbe_packet_t * packet);
static fbe_status_t fbe_cms_cmm_memory_init(void);
static fbe_status_t fbe_cms_cmm_memory_destroy(void);
static fbe_status_t cms_sm_get_access_func_ptrs(fbe_packet_t * packet);

/**********************************************************************************************************************************************/
fbe_status_t fbe_cms_control_entry(fbe_packet_t * packet)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_operation_opcode_t 	control_code = 0;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = cms_init(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &cms_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = cms_destroy(packet);
            break;
		case FBE_CMS_CONTROL_CODE_GET_INFO:
			status = cms_get_info(packet);
            break;
        case FBE_CMS_CONTROL_CODE_GET_PERSISTENCE_STATUS:
            status = cms_get_persistence_status(packet);
            break;
        case FBE_CMS_CONTROL_CODE_REQUEST_PERSISTENCE:
            status = cms_request_persistence(packet);
            break;
		case FBE_CMS_CONTROL_CODE_GET_STATE_MACHINE_HISTORY:
            status = cms_sm_get_history_table(packet);
            break;
		case FBE_CMS_CONTROL_CODE_GET_ACCESS_FUNC_PTRS:
			status = cms_sm_get_access_func_ptrs(packet);
            break;
        default:
            status = fbe_base_service_control_entry((fbe_base_service_t *) &cms_service, packet);
            break;
    }
    return status;
}

void cms_trace(fbe_trace_level_t trace_level,
                             const char * fmt, ...)
{
    va_list args;
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&cms_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&cms_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
        FBE_SERVICE_ID_CMS,
        trace_level,
        FBE_TRACE_MESSAGE_ID_INFO,
        fmt,
        args);
    va_end(args);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
fbe_cms_init_sm_status_t fbe_cms_init_do_synchronous_init()
{
    fbe_status_t 	status = FBE_STATUS_OK;
    
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: entry\n", __FUNCTION__);

    /* init base service */
    fbe_base_service_init((fbe_base_service_t *)&cms_service);
    fbe_base_service_set_service_id((fbe_base_service_t *)&cms_service, FBE_SERVICE_ID_CMS);
    fbe_base_service_set_trace_level((fbe_base_service_t *)&cms_service, fbe_trace_get_default_trace_level());

	/*init the run queue*/
	status = fbe_cms_run_queue_init();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init run queue\n", __FUNCTION__);
		fbe_cms_init_state = FBE_CMS_INIT_STATE_FAILED;
		return(FBE_CMS_STATUS_EXECUTING);
	}

    /* init service specific stuff */

    fbe_cms_cmm_memory_init(); /* Control pool memory */

	status = fbe_cms_cmi_init();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init CMI\n", __FUNCTION__);
		fbe_cms_init_state = FBE_CMS_INIT_STATE_FAILED;
		return(FBE_CMS_STATUS_EXECUTING);
    }

	status = fbe_cms_vault_init();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init Memory\n", __FUNCTION__);
		fbe_cms_init_state = FBE_CMS_INIT_STATE_FAILED;
		return(FBE_CMS_STATUS_EXECUTING);
    }

    status = fbe_cms_memory_init(0);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init Memory\n", __FUNCTION__);
		fbe_cms_init_state = FBE_CMS_INIT_STATE_FAILED;
		return(FBE_CMS_STATUS_EXECUTING);
    }

    status = fbe_cms_cluster_init();
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init Cluster Module\n", __FUNCTION__);
        fbe_cms_init_state = FBE_CMS_INIT_STATE_FAILED;
        return(FBE_CMS_STATUS_EXECUTING);
    }

    status = fbe_cms_operations_init();
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init Operations Module\n", __FUNCTION__);
        fbe_cms_init_state = FBE_CMS_INIT_STATE_FAILED;
        return(FBE_CMS_STATUS_EXECUTING);
    }
#if 0 /*only after reading valid invalid*/
    status = fbe_cms_memory_give_tags_to_cluster();
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to give tags to Cluster Module\n", __FUNCTION__);
        fbe_cms_init_state = FBE_CMS_INIT_STATE_FAILED;
		return(FBE_CMS_STATUS_EXECUTING);
    }
#endif
	status = fbe_cms_state_machine_init();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init State machine\n", __FUNCTION__);
		fbe_cms_init_state = FBE_CMS_INIT_STATE_FAILED;
		return(FBE_CMS_STATUS_EXECUTING);
	}
 
	status = fbe_cms_client_init();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init Client Policy\n", __FUNCTION__);
		fbe_cms_init_state = FBE_CMS_INIT_STATE_FAILED;
		return(FBE_CMS_STATUS_EXECUTING);
	}

	/*** INTER SP lock   */
	/*** READ CDR*/
	/**** Init SM with values so it cache them (make sure no events go through*/
	/* init clean dirty PLACE HOLDER ONLY*/
	/* init persistent memory (including give tags to cluster) chck interface with MAtt.*/



     
    /* service init completed */
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: init finished\n", __FUNCTION__);

	fbe_cms_init_state = FBE_CMS_INIT_STATE_DONE;
	return(FBE_CMS_STATUS_EXECUTING);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
fbe_cms_init_sm_status_t fbe_cms_init_failed()
{
	fbe_packet_t * packet = fbe_cms_init_packet;

    /* service init completed */
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: init failed\n", __FUNCTION__);

    /* complete the packet */
    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    fbe_transport_complete_packet(packet);

	return(FBE_CMS_STATUS_WAITING);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
fbe_cms_init_sm_status_t fbe_cms_init_complete()
{
	fbe_packet_t * packet = fbe_cms_init_packet;

    /* service init completed */
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: init complete\n", __FUNCTION__);

    /* complete the packet */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

	return(FBE_CMS_STATUS_WAITING);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
void fbe_cms_init_run_state_machine()
{
	fbe_cms_init_sm_status_t status = FBE_CMS_STATUS_EXECUTING;
	while(status == FBE_CMS_STATUS_EXECUTING) {
		switch(fbe_cms_init_state)
		{
		case FBE_CMS_INIT_STATE_SYNCHRONOUS_INIT:
			status = fbe_cms_init_do_synchronous_init();
			break;
		case FBE_CMS_INIT_STATE_FAILED:
			status = fbe_cms_init_failed();
			break;
		case FBE_CMS_INIT_STATE_DONE:
			status = fbe_cms_init_complete();
			break;
		default:
			/*! @todo trace error */
			status = FBE_CMS_STATUS_WAITING;
			break;
		};
	}
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
fbe_status_t cms_init(fbe_packet_t * packet)
{
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: entry\n", __FUNCTION__);

    fbe_cms_init_state  = FBE_CMS_INIT_STATE_SYNCHRONOUS_INIT;
    fbe_cms_init_packet = packet;

    fbe_cms_init_run_state_machine();

    return(FBE_STATUS_OK);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
fbe_cms_init_sm_status_t fbe_cms_init_do_synchronous_destroy()
{
    fbe_status_t status = FBE_STATUS_OK;
   
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: entry\n", __FUNCTION__);

    status = fbe_cms_state_machine_destroy();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to destroy State Machine\n", __FUNCTION__);
	}

    status = fbe_cms_client_destroy();
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to destroy Client Policy\n", __FUNCTION__);
    }

    status = fbe_cms_operations_destroy();
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to destroy operations module \n", __FUNCTION__);
    }

    status = fbe_cms_cluster_destroy();
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to destroy cluster module \n", __FUNCTION__);
    }

    status = fbe_cms_memory_destroy();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to destroy Memory\n", __FUNCTION__);
	}

	status = fbe_cms_cmi_destroy();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to destroy CMI\n", __FUNCTION__);
	}

	status = fbe_cms_run_queue_destroy();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to destroy Run queue\n", __FUNCTION__);
	}

    fbe_cms_cmm_memory_destroy(); /* Control pool memory */

    /* destroy base service */
    fbe_base_service_destroy((fbe_base_service_t *) &cms_service);

	fbe_cms_destroy_state = FBE_CMS_DESTROY_STATE_DONE;
	return(FBE_CMS_STATUS_EXECUTING);
}


/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
fbe_cms_init_sm_status_t fbe_cms_destroy_failed()
{
	fbe_packet_t * packet = fbe_cms_destroy_packet;

    /* service init completed */
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: destroy failed\n", __FUNCTION__);

    /* complete the packet */
    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    fbe_transport_complete_packet(packet);

	fbe_cms_destroy_packet = NULL;

	return(FBE_CMS_STATUS_WAITING);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
fbe_cms_init_sm_status_t fbe_cms_destroy_complete()
{
	fbe_packet_t * packet = fbe_cms_destroy_packet;

    /* service init completed */
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: destroy complete\n", __FUNCTION__);

    /* complete the packet */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

	fbe_cms_destroy_packet = NULL;

	return(FBE_CMS_STATUS_WAITING);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
void fbe_cms_destroy_run_state_machine()
{
	fbe_cms_init_sm_status_t status = FBE_CMS_STATUS_EXECUTING;
	while(status == FBE_CMS_STATUS_EXECUTING) {
		switch(fbe_cms_destroy_state)
		{
		case FBE_CMS_DESTROY_SYNCHRONOUS_DESTROY:
			status = fbe_cms_init_do_synchronous_destroy();
			break;
		case FBE_CMS_DESTROY_STATE_FAILED:
			status = fbe_cms_destroy_failed();
			break;
		case FBE_CMS_DESTROY_STATE_DONE:
			status = fbe_cms_destroy_complete();
			break;
		default:
			/*! @todo trace error */
			status = FBE_CMS_STATUS_WAITING;
			break;
		};
	}
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-12-01 - Created. Matthew Ferson
 */
fbe_status_t cms_destroy(fbe_packet_t * packet)
{
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: entry\n", __FUNCTION__);

    fbe_cms_destroy_state  = FBE_CMS_DESTROY_SYNCHRONOUS_DESTROY;
    fbe_cms_destroy_packet = packet;

    fbe_cms_destroy_run_state_machine();

    return(FBE_STATUS_OK);
}

static fbe_status_t cms_get_info(fbe_packet_t * packet)
{
	fbe_status_t                           	status = FBE_STATUS_OK;
    fbe_cms_get_info_t *		mem_persist_info = NULL;
    
    /* Verify packet contents and get pointer to the user's buffer*/
    status = cms_get_payload(packet, (void **)&mem_persist_info, sizeof(fbe_cms_get_info_t));
    if ( status != FBE_STATUS_OK ){
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Payload failed.\n", __FUNCTION__);
        cms_complete_packet(packet, status);
        return status;
    }

    mem_persist_info->some_info = 0xDEADBEEF;

    cms_complete_packet(packet, status);
    return status;

}

static fbe_status_t cms_get_persistence_status(fbe_packet_t * packet)
{
	fbe_status_t                           	    status = FBE_STATUS_OK;
    fbe_cms_service_get_persistence_status_t *  persistence_status_p = NULL;
    fbe_cms_memory_persist_status_t             persistence_status;
    
    /* Verify packet contents and get pointer to the user's buffer*/
    status = cms_get_payload(packet, 
                             (void **)&persistence_status_p, 
                             sizeof(fbe_cms_service_get_persistence_status_t));

    if(status != FBE_STATUS_OK){
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Payload failed.\n", __FUNCTION__);
        cms_complete_packet(packet, status);
        return status;
    }

    status = fbe_cms_memory_get_memory_persistence_status(&persistence_status);

    if(status != FBE_STATUS_OK){
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: call to memory module failed\n", __FUNCTION__);
        cms_complete_packet(packet, status);
        return status;
    }

    persistence_status_p->status = persistence_status;

    cms_complete_packet(packet, status);

    return status;

}

static fbe_status_t cms_request_persistence(fbe_packet_t * packet)
{
	fbe_status_t                           	    status = FBE_STATUS_OK;
    fbe_cms_service_request_persistence_t *     persistence_request = NULL;
    
    /* Verify packet contents and get pointer to the user's buffer*/
    status = cms_get_payload(packet, 
                             (void **)&persistence_request, 
                             sizeof(fbe_cms_service_request_persistence_t));

    if(status != FBE_STATUS_OK){
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Payload failed.\n", __FUNCTION__);
        cms_complete_packet(packet, status);
        return status;
    }

    status = fbe_cms_memory_request_persistence(persistence_request->client_id, 
                                                persistence_request->request);

    if(status != FBE_STATUS_OK){
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: call to memory module failed\n", __FUNCTION__);
        cms_complete_packet(packet, status);
        return status;
    }

    cms_complete_packet(packet, status);

    return status;

}

fbe_status_t fbe_cms_get_ptr(void)
{
	return FBE_STATUS_OK;
}

void cms_env_event_callback(fbe_cms_env_event_t event_type)
{
	switch (event_type) {
	case FBE_CMS_ENV_EVENT_AC_POWER_FAIL:
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:unknown event type:%d\n", __FUNCTION__, event_type);
	}

}

/* CMM memory related functions */
static fbe_status_t fbe_cms_cmm_memory_init(void)
{
   /* Register usage with the available pools in the CMM 
    * and get a small seed pool for use by CMS. 
    */
   cmm_control_pool_id = cmmGetLongTermControlPoolID();
   cmmRegisterClient(cmm_control_pool_id, 
                        NULL, 
                        0,  /* Minimum size allocation */  
                        CMM_MAXIMUM_MEMORY,   
                        "FBE CMS memory", 
                        &cmm_client_id);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cms_cmm_memory_destroy(void)
{
    cmmDeregisterClient(cmm_client_id);
    return FBE_STATUS_OK;
}

void * fbe_cms_cmm_memory_allocate(fbe_u32_t allocation_size_in_bytes)
{
    void * ptr = NULL;
    CMM_ERROR cmm_error;

    cmm_error = cmmGetMemoryVA(cmm_client_id, allocation_size_in_bytes, &ptr);
    if (cmm_error != CMM_STATUS_SUCCESS)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s entry 1 cmmGetMemoryVA fail %X\n", __FUNCTION__, cmm_error);
	}
    return ptr;
}

void fbe_cms_cmm_memory_release(void * ptr)
{
    cmmFreeMemoryVA(cmm_client_id, ptr);
    return;
}


static fbe_status_t cms_sm_get_access_func_ptrs(fbe_packet_t * packet)
{
	fbe_status_t                           	    status = FBE_STATUS_OK;
    fbe_cms_control_get_access_func_ptrs_t *    get_ptrs = NULL;
    
    /* Verify packet contents and get pointer to the user's buffer*/
    status = cms_get_payload(packet, 
                             (void **)&get_ptrs, 
                             sizeof(fbe_cms_control_get_access_func_ptrs_t));

    if(status != FBE_STATUS_OK){
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Payload failed.\n", __FUNCTION__);
        cms_complete_packet(packet, status);
        return status;
    }

	/*give the user the functioin pointers to our access function
	(we have to hope the caller is smart enough to be in the same flat address space as us)*/
    get_ptrs->register_client_func          = fbe_cms_register_client;
    get_ptrs->unregister_client_func        = fbe_cms_unregister_client;
    get_ptrs->register_owner_func           = fbe_cms_register_owner;
    get_ptrs->unregister_owner_func         = fbe_cms_unregister_owner;
    get_ptrs->allocate_cont_exclusive_func  = fbe_cms_buff_cont_alloc_excl;
	get_ptrs->allocate_execlusive_func      = fbe_cms_buff_alloc_excl;
	get_ptrs->buffer_request_abort_func     = fbe_cms_buff_request_abort;
    get_ptrs->commit_exclusive_func         = fbe_cms_buff_commit_excl;
	get_ptrs->free_all_unlocks_func         = fbe_cms_buff_free_all_unlocked;
	get_ptrs->free_exclusive_func           = fbe_cms_buff_free_excl;
    get_ptrs->get_exclusive_lock_func       = fbe_cms_buff_get_excl_lock;
	get_ptrs->get_first_exclusive_func      = fbe_cms_buff_get_first_excl;
	get_ptrs->get_next_exclusive_func       = fbe_cms_buff_get_next_excl;
    get_ptrs->get_shared_lock_func          = fbe_cms_buff_get_shared_lock;
	get_ptrs->release_exclusive_lock        = fbe_cms_buff_release_excl_lock;
	get_ptrs->release_shared_lock           = fbe_cms_buff_release_shared_lock;

	cms_complete_packet(packet, FBE_STATUS_OK);
	return FBE_STATUS_OK;
}

/***********************************************
* end file fbe_cms_main.c
***********************************************/
