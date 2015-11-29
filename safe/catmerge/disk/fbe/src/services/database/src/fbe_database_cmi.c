#include "fbe_cmm.h"
#include "fbe_cmi.h"
#include "fbe_database_cmi_interface.h"
//#include "fbe/fbe_winddk.h"
#include "fbe_database_private.h"


/*******************/
/*local definitions*/
/*******************/

typedef enum fbe_db_cmi_thread_flag_e{
    FBE_DB_CMI_THREAD_RUN,
    FBE_DB_CMI_THREAD_STOP,
    FBE_DB_CMI_THREAD_DONE
}fbe_db_cmi_thread_flag_t;

typedef struct fbe_db_cmi_memory_s{
    fbe_queue_element_t q_element;
    fbe_queue_element_t received_msg_q_element;
    fbe_cmi_event_t event_type;
    fbe_database_cmi_msg_t cmi_msg;
    void * callback_context;
}fbe_db_cmi_memory_t;

#define FBE_DB_CMI_MEM_ELEMENTS_PER_CMM_CHUNK (1048576/sizeof(fbe_db_cmi_memory_t))
#define FBE_DB_CMI_MB_ALLOCATION 4

/*******************/
/* local arguments */
/*******************/
static fbe_semaphore_t					fbe_db_cmi_semaphore;
static fbe_db_cmi_thread_flag_t			fbe_db_cmi_thread_flag;
static fbe_thread_t                 	fbe_db_cmi_thread_handle;
static fbe_queue_head_t					fbe_db_cmi_memory_queue_head;
static fbe_queue_head_t					fbe_db_outstanding_cmi_memory_queue_head;
static fbe_s32_t						fbe_db_outstanding_cmi_msg_count = 0;
static fbe_s32_t						fbe_db_cmi_msg_count_allocated = (FBE_DB_CMI_MEM_ELEMENTS_PER_CMM_CHUNK*FBE_DB_CMI_MB_ALLOCATION);
static fbe_queue_head_t					fbe_db_received_msg_queue_head;
static fbe_spinlock_t					fbe_db_cmi_memory_lock;
static fbe_spinlock_t					fbe_db_cmi_msg_queue_lock;
static CMM_POOL_ID						fbe_db_cmi_cmm_control_pool_id;
static CMM_CLIENT_ID					fbe_db_cmi_cmm_client_id;
static void *							fbe_db_cmi_cmm_chunk_addr[FBE_DB_CMI_MB_ALLOCATION];
static FBE_ALIGN(16)fbe_atomic_t        fbe_db_cmi_thread_event;

/*******************/
/* local function  */
/*******************/

static fbe_status_t fbe_db_cmi_process_contact_lost(void);
static void fbe_db_cmi_thread_func(void *context);
static void fbe_db_cmi_process_message_queue(void);
static fbe_status_t fbe_db_cmi_allocate_memory(void);
static void fbe_db_cmi_destroy_memory(void);
static fbe_status_t fbe_db_cmi_process_message_transmitted(fbe_database_cmi_msg_t *db_cmi_msg, fbe_cmi_event_callback_context_t context);
static fbe_status_t fbe_db_cmi_process_fatal_error(fbe_database_cmi_msg_t *db_cmi_msg, fbe_cmi_event_callback_context_t context);
static fbe_status_t fbe_db_cmi_process_peer_not_present(fbe_database_cmi_msg_t *db_cmi_msg, fbe_cmi_event_callback_context_t context);
static fbe_status_t fbe_db_cmi_process_peer_busy(fbe_database_cmi_msg_t *db_cmi_msg, fbe_cmi_event_callback_context_t context);
static fbe_status_t fbe_db_cmi_process_received_message(fbe_database_cmi_msg_t *db_cmi_msg);
static fbe_db_cmi_memory_t * fbe_db_cmi_copy_received_msg(fbe_database_cmi_msg_t * user_message);
static __forceinline fbe_db_cmi_memory_t * fbe_db_cmi_q_element_to_msg_memory(fbe_queue_element_t *queue_element);
static __forceinline fbe_db_cmi_memory_t * fbe_db_cmi_rcvd_q_element_to_msg_memory(fbe_queue_element_t *queue_element);
static __forceinline fbe_db_cmi_memory_t * fbe_db_cmi_msg_to_msg_memory(fbe_database_cmi_msg_t * user_message);

/*************************************************************************************************************************************************************/
static fbe_status_t fbe_db_cmi_callback(fbe_cmi_event_t event, fbe_u32_t user_message_length, fbe_cmi_message_t user_message, fbe_cmi_event_callback_context_t context)
{
    fbe_status_t				status = FBE_STATUS_OK;
    fbe_database_cmi_msg_t *	db_cmi_msg = (fbe_database_cmi_msg_t *)user_message;
    fbe_db_cmi_memory_t *		peer_msg = NULL;

    /*we don't want to process stuff on the context of the cmi callback sinct it's directly from the hardware
    at DISPATCH_LEVEL so we want to queue it to another thread to do the work*/

    switch (event) {
    case FBE_CMI_EVENT_MESSAGE_RECEIVED:
        /*for new messages which we did not generate, we get a buffer and copy the messge into*/
        peer_msg = fbe_db_cmi_copy_received_msg(user_message);
        if (peer_msg == NULL) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
    case FBE_CMI_EVENT_SP_CONTACT_LOST:
        /*get the memory for a message*/
        db_cmi_msg = fbe_database_cmi_get_msg_memory();
        /*and convert it to the data structure we really want*/
        peer_msg = fbe_db_cmi_msg_to_msg_memory(db_cmi_msg);
		
		fbe_database_cmi_disable_service_when_lost_peer();
        break;
    case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
    case FBE_CMI_EVENT_FATAL_ERROR:
    case FBE_CMI_EVENT_PEER_NOT_PRESENT:
    case FBE_CMI_EVENT_PEER_BUSY:
        /*no need to allocate since we are the ones who sent the messages so we allocated the memory before*/
        peer_msg = fbe_db_cmi_msg_to_msg_memory(user_message);
        break;
    default:
        database_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s, Invalid state %d\n", __FUNCTION__, event);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*remember the important stuff for processign by the thread*/
    peer_msg->event_type = event;
    peer_msg->callback_context = context;

    /*and queue it for processing*/
    fbe_spinlock_lock(&fbe_db_cmi_msg_queue_lock);
    fbe_queue_push(&fbe_db_received_msg_queue_head, &peer_msg->received_msg_q_element); 
    fbe_spinlock_unlock(&fbe_db_cmi_msg_queue_lock);

    /*and signal the thread to pick it up*/
    fbe_semaphore_release(&fbe_db_cmi_semaphore, 0, 1, FALSE);
    
    return status;
}

fbe_status_t fbe_database_cmi_init(void)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    EMCPAL_STATUS       	nt_status;

    /* Validate we haven't exceeded CMI size
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_database_cmi_msg_t) <= FBE_CMI_MAX_MESSAGE_SIZE);

    /*let's allocate memory so we can copy incoming messages into it*/
    status  = fbe_db_cmi_allocate_memory();
    if (status != FBE_STATUS_OK) {
        
        database_trace(FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to allocate msg memory\n", __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_cmi_register(FBE_CMI_CLIENT_ID_DATABASE, fbe_db_cmi_callback, NULL);
    if (status != FBE_STATUS_OK) {
        
        database_trace(FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to init CMI connection\n", __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&fbe_db_cmi_semaphore, 0, FBE_DB_CMI_MEM_ELEMENTS_PER_CMM_CHUNK * FBE_DB_CMI_MB_ALLOCATION);
    fbe_queue_init(&fbe_db_received_msg_queue_head);
    fbe_spinlock_init(&fbe_db_cmi_msg_queue_lock);

    fbe_db_cmi_thread_flag = FBE_DB_CMI_THREAD_RUN;
    nt_status = fbe_thread_init(&fbe_db_cmi_thread_handle, "fbe_cmi_msg_proc", fbe_db_cmi_thread_func, NULL);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: can't start message process thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 

        return FBE_STATUS_GENERIC_FAILURE;
    }


    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_cmi_destroy(void)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;

    status =  fbe_cmi_unregister(FBE_CMI_CLIENT_ID_DATABASE);
    if (status != FBE_STATUS_OK) {
        
        database_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to unregister CMI connection\n", __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_db_cmi_thread_flag = FBE_DB_CMI_THREAD_STOP;

    fbe_semaphore_release(&fbe_db_cmi_semaphore, 0, 1, FALSE);
    fbe_thread_wait(&fbe_db_cmi_thread_handle);
    fbe_thread_destroy(&fbe_db_cmi_thread_handle);

    fbe_semaphore_destroy(&fbe_db_cmi_semaphore);
    fbe_spinlock_destroy(&fbe_db_cmi_msg_queue_lock);

    fbe_db_cmi_destroy_memory();

    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_cmi_thread_set_event(fbe_u64_t event)
{
    fbe_atomic_or(&fbe_db_cmi_thread_event, event);
    /*and signal the thread to pick it up*/
    fbe_semaphore_release(&fbe_db_cmi_semaphore, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

static fbe_u64_t fbe_database_cmi_thread_get_event(void)
{
    fbe_atomic_t val;

    val = fbe_atomic_exchange(&fbe_db_cmi_thread_event, 0);
    return (fbe_u64_t)val;
}

static void fbe_db_cmi_process_event(void)
{
    fbe_u64_t event;

    event = fbe_database_cmi_thread_get_event();
    if (event & FBE_DB_CMI_EVENT_CONFIG_DONE) {

            database_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: configuration done event set, event: %llx\n",
                           __FUNCTION__, (unsigned long long)event);
            fbe_db_create_objects_passive_in_cmi_thread();
    }
}

static fbe_status_t fbe_db_cmi_process_contact_lost(void)
{
    return fbe_database_process_lost_peer();
}

static void fbe_db_cmi_thread_func(void *context)
{
    FBE_UNREFERENCED_PARAMETER(context);

    while (1) {
        fbe_semaphore_wait(&fbe_db_cmi_semaphore, NULL);
        if (fbe_db_cmi_thread_flag == FBE_DB_CMI_THREAD_RUN) {
            fbe_db_cmi_process_event();
            fbe_db_cmi_process_message_queue();
        }else{
            break;
        }
    }

    fbe_db_cmi_thread_flag = FBE_DB_CMI_THREAD_DONE;
    database_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO, "%s: done\n", __FUNCTION__); 
    
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void fbe_db_cmi_process_message_queue(void)
{
    fbe_queue_element_t *	queue_element;
    fbe_db_cmi_memory_t	*	db_cmi_memory;

    fbe_spinlock_lock(&fbe_db_cmi_msg_queue_lock);
    while (!fbe_queue_is_empty(&fbe_db_received_msg_queue_head)) {
        
        /*let go over the meeage and see how we need to be processed*/
        queue_element = fbe_queue_pop(&fbe_db_received_msg_queue_head);
        fbe_spinlock_unlock(&fbe_db_cmi_msg_queue_lock);

        db_cmi_memory = fbe_db_cmi_rcvd_q_element_to_msg_memory(queue_element);

        switch (db_cmi_memory->event_type) {
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            fbe_db_cmi_process_received_message(&db_cmi_memory->cmi_msg);
            /*since we allocate the memory to copy the peer message we also want to free it after the user is done taking care of it*/
            fbe_database_cmi_return_msg_memory(&db_cmi_memory->cmi_msg);
            break;
        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            fbe_db_cmi_process_contact_lost();
            fbe_database_cmi_return_msg_memory(&db_cmi_memory->cmi_msg);
            break;
        case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
            fbe_db_cmi_process_message_transmitted(&db_cmi_memory->cmi_msg, db_cmi_memory->callback_context);
            break;
        case FBE_CMI_EVENT_FATAL_ERROR:
            fbe_db_cmi_process_fatal_error(&db_cmi_memory->cmi_msg, db_cmi_memory->callback_context);
            break;
        case FBE_CMI_EVENT_PEER_NOT_PRESENT:
            fbe_db_cmi_process_peer_not_present(&db_cmi_memory->cmi_msg, db_cmi_memory->callback_context);
            break;
        case FBE_CMI_EVENT_PEER_BUSY:
            fbe_db_cmi_process_peer_busy(&db_cmi_memory->cmi_msg, db_cmi_memory->callback_context);
            break;
        default:
            database_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Invalid state (%d)\n", __FUNCTION__, db_cmi_memory->event_type);
        }


        fbe_spinlock_lock(&fbe_db_cmi_msg_queue_lock);
    }

    fbe_spinlock_unlock(&fbe_db_cmi_msg_queue_lock);

}

static fbe_status_t fbe_db_cmi_allocate_memory(void)
{
    CMM_ERROR 						cmm_error;
    fbe_db_cmi_memory_t *			cmi_msg_element;
    fbe_u8_t *						ptr = NULL;
    fbe_u32_t						count = 0;
    fbe_u32_t						chunk = 0;
    
    fbe_queue_init(&fbe_db_cmi_memory_queue_head);
    fbe_queue_init(&fbe_db_outstanding_cmi_memory_queue_head);

    fbe_spinlock_init(&fbe_db_cmi_memory_lock);
    
    fbe_db_cmi_cmm_control_pool_id = cmmGetLongTermControlPoolID();
    cmmRegisterClient(fbe_db_cmi_cmm_control_pool_id, 
                      NULL, 
                      0,  /* Minimum size allocation */  
                      CMM_MAXIMUM_MEMORY,   
                      "FBE DB CMI memory", 
                       &fbe_db_cmi_cmm_client_id);

    for (chunk = 0; chunk < FBE_DB_CMI_MB_ALLOCATION; chunk ++) {
        /* Allocate a 1MB chunk*/
        cmm_error = cmmGetMemoryVA(fbe_db_cmi_cmm_client_id, 1048576, &fbe_db_cmi_cmm_chunk_addr[chunk]);
        if (cmm_error != CMM_STATUS_SUCCESS) {
            database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s can't get cmm memory, err:%X\n", __FUNCTION__, cmm_error);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        ptr = fbe_db_cmi_cmm_chunk_addr[chunk];
    
        for (count = 0; count < FBE_DB_CMI_MEM_ELEMENTS_PER_CMM_CHUNK; count ++) {
            cmi_msg_element = (fbe_db_cmi_memory_t *)ptr;
            fbe_zero_memory(cmi_msg_element, sizeof(fbe_db_cmi_memory_t));
            fbe_queue_element_init(&cmi_msg_element->q_element);
            fbe_queue_push(&fbe_db_cmi_memory_queue_head, &cmi_msg_element->q_element);
    
            ptr += sizeof(fbe_db_cmi_memory_t);
        }
    
    }
    database_trace (FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s allocated %llu DB CMi msg.\n", 
                    __FUNCTION__,
		   (unsigned long long)(FBE_DB_CMI_MEM_ELEMENTS_PER_CMM_CHUNK * FBE_DB_CMI_MB_ALLOCATION));
    
    return FBE_STATUS_OK;

}

static void fbe_db_cmi_destroy_memory(void)
{
    fbe_u32_t  					chunk = 0;
    fbe_u32_t 					count = 0;
    fbe_queue_element_t *		queue_element_p = NULL;
    fbe_db_cmi_memory_t *		cmi_memory = NULL;
    
    fbe_spinlock_lock(&fbe_db_cmi_memory_lock);

    /*do we have any outstanding ones ?*/
    while (!fbe_queue_is_empty(&fbe_db_outstanding_cmi_memory_queue_head) && count < 10) {
        /*let's wait for completion of a while*/
        fbe_spinlock_unlock(&fbe_db_cmi_memory_lock);
        fbe_thread_delay(500);
        count++;
        fbe_spinlock_lock(&fbe_db_cmi_memory_lock);
    }

    if (!fbe_queue_is_empty(&fbe_db_outstanding_cmi_memory_queue_head)) {
        /*there will be a memory leak eventually but we can't stop here forever*/
        database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s:outstanding packets did not complete in 5 sec\n", __FUNCTION__);
        while (!fbe_queue_is_empty(&fbe_db_outstanding_cmi_memory_queue_head)) {
            queue_element_p = fbe_queue_pop(&fbe_db_outstanding_cmi_memory_queue_head);
            cmi_memory = fbe_db_cmi_q_element_to_msg_memory(queue_element_p);
            database_trace (FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s:outstanding:cmi_event:%d, type:%d\n", __FUNCTION__,cmi_memory->event_type, cmi_memory->cmi_msg.msg_type);


        }
    }
    
    fbe_spinlock_unlock(&fbe_db_cmi_memory_lock);
    fbe_spinlock_destroy(&fbe_db_cmi_memory_lock);

    /*free CMM memory*/
    for (chunk = 0; chunk < FBE_DB_CMI_MB_ALLOCATION; chunk++) {
        cmmFreeMemoryVA(fbe_db_cmi_cmm_client_id, fbe_db_cmi_cmm_chunk_addr[chunk]);
    }
    
    /*unregister*/
    cmmDeregisterClient(fbe_db_cmi_cmm_client_id);
}

fbe_database_cmi_msg_t * fbe_database_cmi_get_msg_memory(void)
{
    
    fbe_database_cmi_msg_t *		msg_memory = NULL;
    fbe_db_cmi_memory_t *			cmi_memory_element = NULL;
    
    fbe_spinlock_lock(&fbe_db_cmi_memory_lock);

    if (!fbe_queue_is_empty(&fbe_db_cmi_memory_queue_head)) {
        cmi_memory_element = (fbe_db_cmi_memory_t *)fbe_queue_pop(&fbe_db_cmi_memory_queue_head);
        msg_memory = &cmi_memory_element->cmi_msg;
        fbe_db_outstanding_cmi_msg_count +=1;
    }else{
        fbe_spinlock_unlock(&fbe_db_cmi_memory_lock);
        /* we don't need to panic here since all callers should handle returning of NULL */
        /* database_trace (FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s:Ran out of CMI memory for messages\n", __FUNCTION__); */
        return NULL;/*just in case*/
    }

    fbe_queue_push(&fbe_db_outstanding_cmi_memory_queue_head, &cmi_memory_element->q_element);/*move to outstanding queue*/

    fbe_spinlock_unlock(&fbe_db_cmi_memory_lock);

    return msg_memory;
}

fbe_s32_t fbe_database_cmi_get_outstanding_msg_count(void)
{
    return fbe_db_outstanding_cmi_msg_count;
}

fbe_s32_t fbe_database_cmi_get_msg_count_allocated(void)
{
    return fbe_db_cmi_msg_count_allocated;
}


void fbe_database_cmi_return_msg_memory(fbe_database_cmi_msg_t *cmi_msg_memory)
{
    fbe_queue_element_t *q_element  = NULL;
    fbe_db_cmi_memory_t * cmi_memory_element = NULL;

    /*let get back the original message memory structure*/
    cmi_memory_element = fbe_db_cmi_msg_to_msg_memory(cmi_msg_memory);
    q_element = &cmi_memory_element->q_element;

    fbe_spinlock_lock(&fbe_db_cmi_memory_lock);

    fbe_queue_remove(q_element);/*remove from outstanding queue*/
    fbe_queue_push(&fbe_db_cmi_memory_queue_head, q_element);/*push to pre allocated queue*/
    fbe_db_outstanding_cmi_msg_count -=1;

    fbe_spinlock_unlock(&fbe_db_cmi_memory_lock);
}


fbe_status_t 
fbe_database_cmi_send_mailbomb_completion(fbe_database_cmi_msg_t *returned_msg, 
                                             fbe_status_t completion_status, 
                                             void *context)
{
    /*let's return the message memory first*/
    fbe_database_cmi_return_msg_memory(returned_msg);

    if (completion_status != FBE_STATUS_OK) 
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Err: %d sending mailbomb to peer.\n", 
                        __FUNCTION__, 
                        completion_status);
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_database_cmi_send_mailbomb(void)
{
    fbe_database_cmi_msg_t *  msg_memory = NULL;

    if (!fbe_cmi_is_peer_alive()) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    msg_memory = fbe_database_cmi_get_msg_memory();
    if (msg_memory == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    msg_memory->completion_callback = fbe_database_cmi_send_mailbomb_completion;
    msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_MAILBOMB;

    fbe_database_cmi_send_message(msg_memory, NULL);

    return FBE_STATUS_OK;
}


void fbe_database_cmi_send_message(fbe_database_cmi_msg_t * db_cmi_msg, void *context)
{
	fbe_u32_t		msg_size;

	/*the biggest CMi message can be up to 10K. Since they are all in one uniton, we don't want 
	to send this 10K all the time so we'll adjust the size based on the message type*/

	msg_size = fbe_database_cmi_get_msg_size(db_cmi_msg->msg_type);

    /* initialize the CMI message */
    fbe_zero_memory(&db_cmi_msg->version_header, sizeof(db_cmi_msg->version_header));
    /* Before sending out the CMI message, initialize the size by differnt type*/
    db_cmi_msg->version_header.size = msg_size;
    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_DATABASE, 
                         sizeof(fbe_database_cmi_msg_t),
                         (fbe_cmi_message_t)db_cmi_msg,
                         (fbe_cmi_event_callback_context_t)context);
    return;
}

static fbe_status_t fbe_db_cmi_process_message_transmitted(fbe_database_cmi_msg_t *db_cmi_msg, fbe_cmi_event_callback_context_t context)
{
    if (db_cmi_msg->completion_callback != NULL) {
        db_cmi_msg->completion_callback(db_cmi_msg, FBE_STATUS_OK, context);
    }else{
        database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s:no completion pointer\n", __FUNCTION__); 	
    }

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_db_cmi_process_fatal_error(fbe_database_cmi_msg_t *db_cmi_msg, fbe_cmi_event_callback_context_t context)
{
    if (db_cmi_msg->completion_callback != NULL) {
        db_cmi_msg->completion_callback(db_cmi_msg, FBE_STATUS_GENERIC_FAILURE, context);
    }else{
        database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s:no completion pointer\n", __FUNCTION__); 	
    }

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_db_cmi_process_peer_not_present(fbe_database_cmi_msg_t *db_cmi_msg, fbe_cmi_event_callback_context_t context)
{
    if (db_cmi_msg->completion_callback != NULL) {
        db_cmi_msg->completion_callback(db_cmi_msg, FBE_STATUS_NO_DEVICE, context);
    }else{
        database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s:no completion pointer\n", __FUNCTION__); 	
    }

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_db_cmi_process_peer_busy(fbe_database_cmi_msg_t *db_cmi_msg, fbe_cmi_event_callback_context_t context)
{
    if (db_cmi_msg->completion_callback != NULL) {
        db_cmi_msg->completion_callback(db_cmi_msg, FBE_STATUS_NO_DEVICE/*FBE_STATUS_BUSY*/, context);
    }else{
        database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s:no completion pointer\n", __FUNCTION__); 	
    }

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_db_cmi_process_received_message(fbe_database_cmi_msg_t *db_cmi_msg)
{
    return fbe_database_process_received_cmi_message(db_cmi_msg);
}

/**********************************************************
 * @brief 
 *       initialize the database CMI message
 * 
 * @author gaoh1 (5/15/2012)
 * 
 * @return fbe_status_t 
 *********************************************************/
static fbe_status_t fbe_db_cmi_msg_init(fbe_database_cmi_msg_t *cmi_msg)
{
    if (cmi_msg == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s: input parameter is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* currently, initialize the cmi msg by zeroing.
     * If needed non-zero default value in the new version, initialize the CMI message here.
     */
    fbe_zero_memory(cmi_msg, sizeof(fbe_database_cmi_msg_t));

    return FBE_STATUS_OK;
}

/***********************************************************
 * @brief 
 *    copy the db transaction info from CMI message
 *    Currently, only copy the entry id for user/object/edge
 *    entry.
 *    For global info entry, copy the whole entry.
 * 
 * @author gaoh1 (5/15/2012)
 * 
 * @param cmi_msg : 
 * @param user_message : the CMI message received from Peer SP
 * 
 * @return fbe_status_t 
 ***********************************************************/
static fbe_status_t fbe_db_cmi_copy_db_transaction(fbe_database_cmi_msg_t *cmi_msg, fbe_database_cmi_msg_t *user_message)
{
    fbe_u32_t   entry_id_idx = 0;
    fbe_u8_t    *entry_ptr = NULL, *tmp_ptr = NULL;
    fbe_u32_t   entries_num = 0;

    cmi_msg->msg_type = user_message->msg_type;
    cmi_msg->completion_callback = user_message->completion_callback;
    cmi_msg->payload.db_transaction.transaction_id = user_message->payload.db_transaction.transaction_id;
    cmi_msg->payload.db_transaction.job_number = user_message->payload.db_transaction.job_number;
    cmi_msg->payload.db_transaction.transaction_type = user_message->payload.db_transaction.transaction_type;

    entry_ptr = ((fbe_u8_t *)(&user_message->payload.db_transaction)) + user_message->payload.db_transaction.user_entry_array_offset;
    /* Copy the user entry id's array*/
    if (user_message->payload.db_transaction.max_user_entries > DATABASE_TRANSACTION_MAX_USER_ENTRY) {
        database_trace (FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: received user entries %d, current SP's max user entry %d\n", 
                        __FUNCTION__, 
                        user_message->payload.db_transaction.max_user_entries,
                        DATABASE_TRANSACTION_MAX_USER_ENTRY); 
        entries_num = DATABASE_TRANSACTION_MAX_USER_ENTRY;
    } else {
        entries_num = user_message->payload.db_transaction.max_user_entries;
    }

    tmp_ptr = entry_ptr;
    for (entry_id_idx = 0; entry_id_idx < entries_num; entry_id_idx++) {
        cmi_msg->payload.db_transaction.user_entries[entry_id_idx].header.entry_id = 
            ((database_user_entry_t *)tmp_ptr)->header.entry_id;
        tmp_ptr += user_message->payload.db_transaction.user_entry_size;
    }

    /* update the pointer */
    entry_ptr = ((fbe_u8_t *)(&user_message->payload.db_transaction)) + user_message->payload.db_transaction.object_entry_array_offset;

    /* Copy the object entry id's array*/
    if (user_message->payload.db_transaction.max_object_entries > DATABASE_TRANSACTION_MAX_OBJECTS) {
        database_trace (FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: received object entries %d, current SP's max object entry %d\n", 
                        __FUNCTION__, 
                        user_message->payload.db_transaction.max_object_entries,
                        DATABASE_TRANSACTION_MAX_OBJECTS); 
        entries_num = DATABASE_TRANSACTION_MAX_OBJECTS;
    } else {
        entries_num = user_message->payload.db_transaction.max_object_entries;
    }

    tmp_ptr = entry_ptr;
    for (entry_id_idx = 0; entry_id_idx < entries_num; entry_id_idx++) {
        cmi_msg->payload.db_transaction.object_entries[entry_id_idx].header.entry_id = 
            ((database_object_entry_t *)tmp_ptr)->header.entry_id;
        tmp_ptr += user_message->payload.db_transaction.object_entry_size;
    }

    /* update the pointer */
    entry_ptr = ((fbe_u8_t *)(&user_message->payload.db_transaction)) + user_message->payload.db_transaction.edge_entry_array_offset;
    /* Copy the edge entry id's array */
    if (user_message->payload.db_transaction.max_edge_entries > DATABASE_TRANSACTION_MAX_EDGES) {
        database_trace (FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: received edge entries %d, current SP's max edge entry %d\n", 
                        __FUNCTION__, 
                        user_message->payload.db_transaction.max_edge_entries,
                        DATABASE_TRANSACTION_MAX_EDGES); 
        entries_num = DATABASE_TRANSACTION_MAX_EDGES;
    } else {
        entries_num = user_message->payload.db_transaction.max_edge_entries;
    }

    tmp_ptr = entry_ptr;
    for (entry_id_idx = 0; entry_id_idx < entries_num; entry_id_idx++) {
        cmi_msg->payload.db_transaction.edge_entries[entry_id_idx].header.entry_id = 
            ((database_edge_entry_t *)tmp_ptr)->header.entry_id;
        tmp_ptr += user_message->payload.db_transaction.edge_entry_size;
    }

    /* update the pointer */
    entry_ptr = ((fbe_u8_t *)(&user_message->payload.db_transaction)) + user_message->payload.db_transaction.global_info_entry_array_offset;
    /* Copy the global info entry id's array */
    if (user_message->payload.db_transaction.max_global_info_entries > DATABASE_TRANSACTION_MAX_GLOBAL_INFO) {
        entries_num = DATABASE_TRANSACTION_MAX_GLOBAL_INFO;
        database_trace (FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: received global info entries %d, current SP's max global info entry %d\n", 
                        __FUNCTION__, 
                        user_message->payload.db_transaction.max_global_info_entries,
                        DATABASE_TRANSACTION_MAX_GLOBAL_INFO); 
    } else {
        entries_num = user_message->payload.db_transaction.max_global_info_entries;
    }

    tmp_ptr = entry_ptr;
    for (entry_id_idx = 0; entry_id_idx < entries_num; entry_id_idx++) {
        fbe_copy_memory(&cmi_msg->payload.db_transaction.global_info_entries[entry_id_idx],
                        tmp_ptr,
                        user_message->payload.db_transaction.global_info_entry_size);
        tmp_ptr += user_message->payload.db_transaction.global_info_entry_size;
    }

    return FBE_STATUS_OK;
}

/********************************************************
 * @brief 
 *       copy the received msg by the right size  
 * 
 * @author gaoh1 (modified 5/15/2012)
 * 
 * @param user_message 
 * 
 * @return fbe_db_cmi_memory_t* 
 *********************************************************/
static fbe_db_cmi_memory_t * fbe_db_cmi_copy_received_msg(fbe_database_cmi_msg_t * user_message)
{
    fbe_db_cmi_memory_t *	msg_memory = NULL;
    fbe_database_cmi_msg_t *  cmi_msg = NULL;
    fbe_u32_t               msg_size;

    /*get the memory for a message*/
    cmi_msg = fbe_database_cmi_get_msg_memory();

    if (cmi_msg == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s: out of msg memory\n", __FUNCTION__); 
        return NULL;
    }

    /*and convert it to the data structure we really want*/
    msg_memory = fbe_db_cmi_msg_to_msg_memory(cmi_msg);

    /* Initialize the CMI message*/
    fbe_db_cmi_msg_init(cmi_msg);
    msg_size = fbe_database_cmi_get_msg_size(user_message->msg_type);
    /* If the cmi msg type is FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT, copy the entry id array by smaller number*/
    if (user_message->msg_type == FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT)
    {
        fbe_db_cmi_copy_db_transaction(cmi_msg,user_message); 
    } else {
        /*so we can finally copy and then relase the callback*/
        if (user_message->version_header.size > msg_size) {
            database_trace (FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: received cmi msg is larger(%d)\n", 
                            __FUNCTION__, user_message->version_header.size); 
            /* Cut the received message */
            fbe_copy_memory(cmi_msg, user_message, msg_size);
        } else {
            fbe_copy_memory(cmi_msg, user_message, user_message->version_header.size);
            database_trace (FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: version_header.size(%d), msg_type size (%d) (%d)\n", 
                            __FUNCTION__, user_message->version_header.size,
                            user_message->msg_type,
                            fbe_database_cmi_get_msg_size(user_message->msg_type) ); 
        }
    }
    /* initialize the version_header.size with the value from the sender */
    cmi_msg->version_header.size = user_message->version_header.size;

    return msg_memory;

}

/*get the memory of the message itself and extract from it the structure that actually contains it and the rest of what we need*/
static __forceinline fbe_db_cmi_memory_t * fbe_db_cmi_msg_to_msg_memory(fbe_database_cmi_msg_t * user_message)
{
    fbe_db_cmi_memory_t *db_cmi_memory;
    db_cmi_memory = (fbe_db_cmi_memory_t *)((fbe_u8_t *)user_message - (fbe_u8_t *)(&((fbe_db_cmi_memory_t *)0)->cmi_msg));
    return db_cmi_memory;
}

/*get the memory of the message memory based on the queu element*/
static __forceinline fbe_db_cmi_memory_t * fbe_db_cmi_q_element_to_msg_memory(fbe_queue_element_t *queue_element)
{
    fbe_db_cmi_memory_t *db_cmi_memory;
    db_cmi_memory = (fbe_db_cmi_memory_t *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_db_cmi_memory_t *)0)->q_element));
    return db_cmi_memory;
}

static __forceinline fbe_db_cmi_memory_t * fbe_db_cmi_rcvd_q_element_to_msg_memory(fbe_queue_element_t *rcvd_queue_element)
{
    fbe_db_cmi_memory_t *db_cmi_memory;
    db_cmi_memory = (fbe_db_cmi_memory_t *)((fbe_u8_t *)rcvd_queue_element - (fbe_u8_t *)(&((fbe_db_cmi_memory_t *)0)->received_msg_q_element));
    return db_cmi_memory;

}

fbe_u32_t fbe_database_cmi_get_msg_size(fbe_cmi_msg_type_t type)
{
	fbe_u32_t				msg_size;

	msg_size = (fbe_u32_t)&((fbe_database_cmi_msg_t *)0)->payload;
	switch(type) {
	case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO:
		msg_size += sizeof(fbe_database_power_save_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG:
		msg_size += sizeof(fbe_cmi_msg_update_peer_table_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_DONE:
		msg_size += sizeof(fbe_cmi_msg_update_config_done_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT:
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START:
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT:
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE:
		msg_size += sizeof(database_cmi_transaction_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_DMA:
		msg_size += sizeof(database_cmi_transaction_header_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD:
		msg_size += sizeof(fbe_database_control_update_pvd_t);
		break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE:
        msg_size += sizeof(fbe_database_control_update_pvd_block_size_t);
        break;
	case FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD:
		msg_size += sizeof(fbe_database_control_pvd_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_CREATE_VD:
		msg_size += sizeof(fbe_database_control_vd_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD:
		msg_size += sizeof(fbe_database_control_update_vd_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD: 
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD: 
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID:
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN:
	case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL:
	case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN:
		msg_size += sizeof(fbe_database_control_destroy_object_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID:
		msg_size += sizeof(fbe_database_control_raid_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID:
		msg_size += sizeof(fbe_database_control_update_raid_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN:
		msg_size += sizeof(fbe_database_control_lun_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN:
		msg_size += sizeof(fbe_database_control_update_lun_t);
		break;
    case FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT:
        msg_size += sizeof(fbe_database_control_clone_object_t);
        break;
	case FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE:
		msg_size += sizeof(fbe_database_control_create_edge_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE:
		msg_size += sizeof(fbe_database_control_destroy_edge_t);
		break;
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG:
		msg_size += sizeof(fbe_database_control_update_system_spare_config_t);
		break;
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO:
        msg_size += sizeof(fbe_database_time_threshold_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG:
        msg_size += sizeof(fbe_database_control_update_peer_system_bg_service_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_FRU_DESCRIPTOR:
        msg_size += sizeof(fbe_cmi_msg_update_peer_fru_descriptor_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SYSTEM_DB_HEADER:
        msg_size += sizeof(fbe_cmi_msg_update_peer_system_db_header_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_FRU_DESCRIPTOR_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_update_fru_descriptor_confirm_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SYSTEM_DB_HEADER_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_update_system_db_header_confirm_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_MAKE_DRIVE_ONLINE:
        msg_size += sizeof(fbe_database_control_online_planned_drive_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_MAKE_DRIVE_ONLINE_CONFIRM:
        msg_size += sizeof(fbe_database_online_planned_drive_confirm_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_GET_CONFIG:
        msg_size += sizeof(fbe_cmi_msg_get_config_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_PASSIVE_INIT_DONE:
    case FBE_DATABE_CMI_MSG_TYPE_DB_SERVICE_MODE:
    case FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_FOR_TEST:/* This type of message is used for test*/
    case FBE_DATABE_CMI_MSG_TYPE_MAILBOMB:
    case FBE_DATABE_CMI_MSG_TYPE_CLEAR_USER_MODIFIED_WWN_SEED:
        break;/*no additional size*/
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_CREATE_VD_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_CONFIRM:
	case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_CONFIG_TABLE_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_transaction_confirm_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_TYPE:
    case FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_SIZE:
        msg_size += sizeof(fbe_cmi_msg_unknown_msg_response_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SHARED_EMV_INFO:
        msg_size += sizeof(fbe_database_emv_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE:
        msg_size += sizeof(fbe_database_control_drive_connection_t);
        break;
	case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE:
		msg_size += sizeof(fbe_database_encryption_t);
		break;
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE:
        msg_size += sizeof(fbe_database_control_update_encryption_mode_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE:
        /* no payload */
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_CONFIG_TABLE:
        msg_size += sizeof(fbe_cmi_msg_update_config_table_t);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS:
    case FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS:
        msg_size += sizeof(fbe_database_control_setup_encryption_key_t);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS:
        msg_size += sizeof(fbe_database_control_update_drive_key_t);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG:
        msg_size += sizeof(fbe_global_pvd_config_t);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_GET_BE_PORT_INFO:
    case FBE_DATABASE_CMI_MSG_TYPE_GET_BE_PORT_INFO_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_database_get_be_port_info_t);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_BACKUP_STATE:
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_BACKUP_STATE_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_database_set_encryption_bakcup_state_t);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK:
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_database_setup_kek_t);
        break;

    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_database_destroy_kek_t);
        break;

    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_KEK:
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_KEK_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_database_setup_kek_kek_t);
        break;

    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_KEK:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_KEK_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_database_destroy_kek_kek_t);
        break;

    case FBE_DATABASE_CMI_MSG_TYPE_REESTABLISH_KEK_KEK:
    case FBE_DATABASE_CMI_MSG_TYPE_REESTABLISH_KEK_KEK_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_database_reestablish_kek_kek_t);
        break;

    case FBE_DATABASE_CMI_MSG_TYPE_PORT_ENCRYPTION_MODE:
    case FBE_DATABASE_CMI_MSG_TYPE_PORT_ENCRYPTION_MODE_CONFIRM:
        msg_size += sizeof(fbe_cmi_msg_database_port_encryption_mode_t);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_KILL_VAULT_DRIVE:
        msg_size += sizeof(fbe_cmi_msg_database_kill_vault_drive_t);
        break; 
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE:
        msg_size += sizeof(fbe_database_encryption_t);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL:
        msg_size += sizeof(fbe_database_control_create_ext_pool_t);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN:
        msg_size += sizeof(fbe_database_control_create_ext_pool_lun_t);
        break;
    default:
		database_trace (FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s: need to specify size of msg %d\n", __FUNCTION__, type); 
		msg_size =  sizeof(fbe_database_cmi_msg_t);
	}

	return msg_size;


}


/*!************************************************************************************************
@fn fbe_status_t fbe_database_cmi_set_peer_version(void)
***************************************************************************************************
* @brief
*  This function sets sep version in CMI service.
*
* @param peer_sep_version
*
* @return
*  fbe_status_t
*
***************************************************************************************************/
fbe_status_t fbe_database_cmi_set_peer_version(fbe_u64_t peer_sep_version)
{
    fbe_status_t status;
    fbe_metadata_control_code_t control_code;

    if (peer_sep_version <= 3300) {
        control_code = FBE_CMI_CONTROL_CODE_SET_PEER_VERSION;
    } else {
        control_code = FBE_CMI_CONTROL_CODE_CLEAR_PEER_VERSION;
    }
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s: peer sep version 0x%llx\n", __FUNCTION__, peer_sep_version);

    status = fbe_database_send_packet_to_service ( control_code,
                                                   NULL, 
                                                   0,
                                                   FBE_SERVICE_ID_CMI,
                                                   NULL, /* no sg list*/
                                                   0,    /* no sg list*/
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    return status;
}
