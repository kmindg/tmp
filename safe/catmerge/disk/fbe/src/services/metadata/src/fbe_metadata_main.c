#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"

#include "fbe_metadata.h"
#include "fbe/fbe_base_config.h"
#include "fbe_topology.h"
#include "fbe_metadata_private.h"
#include "fbe_cmi.h"
#include "fbe_metadata_cmi.h"
#include "EmcPAL_Misc.h"
//#include "fbe_metadata_stripe_lock.h"
#include "fbe/fbe_api_metadata_interface.h"

typedef struct fbe_metadata_service_s{
    fbe_base_service_t  base_service;
    fbe_spinlock_t      metadata_element_queue_lock; 
    fbe_queue_head_t    metadata_element_queue_head;

}fbe_metadata_service_t;

fbe_bool_t ndu_in_progress;
static fbe_u8_t * metadata_local_zero_bitbucket;
static fbe_u32_t fbe_metadata_number_of_registered_objects = 0;
extern fbe_metadata_raw_mirror_io_cb_t fbe_metadata_raw_mirror_io_cb;
/* Declare our service methods */
fbe_status_t fbe_metadata_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_metadata_service_methods = {FBE_SERVICE_ID_METADATA, fbe_metadata_control_entry};

static fbe_metadata_service_t metadata_service;

/* Forward declaration */
static fbe_status_t fbe_metadata_element_init_records_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_element_init_records_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_metadata_switch_single_element_to_state(fbe_object_id_t object_id, fbe_metadata_element_state_t new_state);

static fbe_status_t fbe_metadata_set_elements_state(struct fbe_packet_s * packet);

static fbe_status_t fbe_metadata_operation_unregister_element(fbe_packet_t * packet);

static fbe_status_t fbe_metadata_element_init_attributes(fbe_metadata_element_t* metadata_element_p);

static fbe_status_t fbe_metadata_operation_memory_update(fbe_packet_t * packet);
static fbe_status_t fbe_metadata_operation_memory_update_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_metadata_set_ndu_in_progress(struct fbe_packet_s * packet);
static fbe_status_t fbe_metadata_clear_ndu_in_progress(struct fbe_packet_s * packet);
static fbe_status_t fbe_metadata_control_get_stripe_lock_info(fbe_packet_t * packet);

/* Stripe lock queue */
static fbe_status_t fbe_metadata_element_init_stripe_lock_queue(fbe_metadata_element_t* metadata_element_p);
static fbe_status_t fbe_metadata_element_destroy_stripe_lock_queue(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_metadata_raw_mirror_io_get_seq(fbe_lba_t lba, fbe_bool_t inc,
                                                  fbe_u64_t *seq);
void
metadata_trace(fbe_trace_level_t trace_level,
                   fbe_trace_message_id_t message_id,
                   const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&metadata_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&metadata_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_METADATA,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}

static fbe_status_t 
fbe_metadata_init(fbe_packet_t * packet)
{

    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: entry\n", __FUNCTION__);

    fbe_base_service_set_service_id((fbe_base_service_t *) &metadata_service, FBE_SERVICE_ID_METADATA);
    fbe_base_service_set_trace_level((fbe_base_service_t *) &metadata_service, fbe_trace_get_default_trace_level());

    fbe_base_service_init((fbe_base_service_t *) &metadata_service);

    /* Initialize metadata_element_queue_lock */
    fbe_spinlock_init(&metadata_service.metadata_element_queue_lock);

    /* Initialize metadata_element_queue_head */
    fbe_queue_init(&metadata_service.metadata_element_queue_head);

    /* Allocate bitbuckets */
    metadata_local_zero_bitbucket = fbe_memory_native_allocate( 2 * FBE_METADATA_BLOCK_SIZE);
    fbe_zero_memory(metadata_local_zero_bitbucket, 2 * FBE_METADATA_BLOCK_SIZE);

    /* Set ndu_in_progress to TRUE */
    ndu_in_progress = FBE_TRUE;

    /*register with the CMI service*/
    status = fbe_metadata_cmi_init();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }
    status = fbe_metadata_stripe_lock_init();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    status = fbe_ext_pool_lock_init();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    status = fbe_metadata_nonpaged_init();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    status = fbe_metadata_paged_init();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }   

    status = fbe_metadata_cancel_thread_init();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_destroy(fbe_packet_t * packet)
{
    fbe_status_t status;
    metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: entry\n", __FUNCTION__);

    status = fbe_metadata_nonpaged_destroy();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    status = fbe_metadata_paged_destroy();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    status = fbe_ext_pool_lock_destroy();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    status = fbe_metadata_stripe_lock_destroy();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }


    status = fbe_metadata_cancel_thread_destroy();
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }


    //fbe_cmi_unregister(FBE_CMI_CLIENT_ID_METADATA);
    fbe_metadata_cmi_destroy();


    /* Release bitbucket */
    fbe_memory_native_release(metadata_local_zero_bitbucket);

    /* Destroy metadata_element_queue_head */
    fbe_queue_destroy(&metadata_service.metadata_element_queue_head);

    /* Destroy metadata_element_queue_lock */
    fbe_spinlock_destroy(&metadata_service.metadata_element_queue_lock);

    fbe_base_service_destroy((fbe_base_service_t *) &metadata_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_metadata_control_entry(fbe_packet_t * packet)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_metadata_init(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &metadata_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_metadata_destroy( packet);
            break;
        case FBE_METADATA_CONTROL_CODE_SET_ELEMENTS_STATE:
            status = fbe_metadata_set_elements_state(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_DEBUG_LOCK:
            status = fbe_metadata_stripe_lock_control_entry(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_DEBUG_EXT_POOL_LOCK:
            status = fbe_ext_pool_lock_control_entry(packet);
            break;

        case FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_CLEAR:
            status = fbe_metadata_nonpaged_system_clear(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD:
            status = fbe_metadata_nonpaged_system_load(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_PERSIST:
            status = fbe_metadata_nonpaged_system_persist(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_ZERO_AND_PERSIST:
            status = fbe_metadata_nonpaged_system_zero_and_persist(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_NONPAGED_LOAD:
            status = fbe_metadata_nonpaged_load(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_NONPAGED_PERSIST:
            status = fbe_metadata_nonpaged_persist(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_NONPAGED_SYSTEM_LOAD_WITH_DISKMASK:
	        status = fbe_metadata_nonpaged_system_load_with_diskmask(packet);
	        break;
        case FBE_METADATA_CONTROL_CODE_NONPAGED_BACKUP_RESTORE:
            status = fbe_metadata_nonpaged_backup_restore(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_SET_NDU_IN_PROGRESS:
            status = fbe_metadata_set_ndu_in_progress(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_CLEAR_NDU_IN_PROGRESS:
            status = fbe_metadata_clear_ndu_in_progress(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_GET_STRIPE_LOCK_INFO:
            status = fbe_metadata_control_get_stripe_lock_info(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_NONPAGED_SET_BLOCKS_PER_OBJECT:
            status = fbe_metadata_nonpaged_set_blocks_per_object(packet);
            break;
        case FBE_METADATA_CONTROL_CODE_NONPAGED_GET_NONPAGED_METADATA_PTR:
            status = fbe_metadata_nonpaged_get_nonpaged_metadata_data_ptr(packet);
            break;
        default:
            return fbe_base_service_control_entry((fbe_base_service_t *) &metadata_service, packet);
            break;
    }

    return status;
}

fbe_status_t 
fbe_metadata_register(fbe_metadata_element_t * metadata_element)
{
    /*sanity check to make sure we don't try to register the same one again*/
    if (fbe_queue_is_element_on_queue(&metadata_element->queue_element)) {
        fbe_object_id_t obj = fbe_base_config_metadata_element_to_object_id(metadata_element);

         metadata_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Attempt to register object id:%d twice\n", __FUNCTION__, obj);

         return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate all nonpaged records */
    fbe_spinlock_lock(&metadata_service.metadata_element_queue_lock);
    fbe_queue_push(&metadata_service.metadata_element_queue_head, &metadata_element->queue_element);
    metadata_element->peer_metadata_element_ptr = 0; /* We will check for peer address here */
    fbe_metadata_number_of_registered_objects++;
    fbe_spinlock_unlock(&metadata_service.metadata_element_queue_lock);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_metadata_unregister(fbe_metadata_element_t * metadata_element)
{
    /* Release all records */
    fbe_spinlock_lock(&metadata_service.metadata_element_queue_lock);
    metadata_element->peer_metadata_element_ptr = 0; 
    fbe_queue_remove(&metadata_element->queue_element);
    fbe_metadata_number_of_registered_objects--;
    fbe_spinlock_unlock(&metadata_service.metadata_element_queue_lock);
    
    return FBE_STATUS_OK;
}
fbe_status_t fbe_metadata_element_set_state(fbe_metadata_element_t *metadata_element,
                                            fbe_metadata_element_state_t state)
{
    fbe_cmi_sp_state_t sp_state;
    fbe_object_id_t object_id;
    fbe_metadata_element_state_t old_state = metadata_element->metadata_element_state;

    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    fbe_cmi_get_current_sp_state(&sp_state);

    metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s sp_state: %d mde_state: 0x%d new_mde_state: %d object_id: 0x%x\n", 
                   __FUNCTION__, sp_state, 
                   metadata_element->metadata_element_state, state, object_id);
    metadata_element->metadata_element_state = state;

    if (old_state != FBE_METADATA_ELEMENT_STATE_INVALID && state != FBE_METADATA_ELEMENT_STATE_INVALID &&
        old_state != state) {
        /* Clear the cache */
        fbe_metadata_paged_clear_cache(metadata_element);
    }
    return FBE_STATUS_OK;
}
fbe_status_t 
fbe_metadata_element_init_memory(fbe_packet_t * packet)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL; 
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_sp_state_t  sp_state = FBE_CMI_STATE_INVALID;
    
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    /*based on CMI service, we decide if this element is passive or active*/
    status = fbe_cmi_get_current_sp_state(&sp_state);
    if (status != FBE_STATUS_OK) {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Can't get SP state\n", __FUNCTION__);

        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (sp_state == FBE_CMI_STATE_BUSY) {
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s FBE_CMI_STATE_BUSY \n", __FUNCTION__);
        
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_BUSY);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    if(FBE_FALSE == fbe_metadata_element_is_element_reinited(metadata_element))
    {
        fbe_spinlock_init(&metadata_element->metadata_element_lock);
    }
    else
    {
        /*metadata element reinitialize case: spinlock is never destroyed, so no need
         *to init it here*/
    }

    if(fbe_metadata_element_is_cmi_disabled(metadata_element)) 
    {
        /* peer object is disabled, we set ourselvs to be ACTIVE */
        fbe_metadata_element_set_state(metadata_element, FBE_METADATA_ELEMENT_STATE_ACTIVE);
        fbe_metadata_element_set_peer_dead(metadata_element);
    } 
    else
    {
        /* based on what CMI tells us, we set ourselvs to be either PASSIVE or ACTIVE */
        if(sp_state == FBE_CMI_STATE_ACTIVE){
            fbe_metadata_element_set_state(metadata_element, FBE_METADATA_ELEMENT_STATE_ACTIVE);
            	fbe_metadata_element_set_peer_dead(metadata_element);
        } else {

            fbe_metadata_element_set_state(metadata_element, FBE_METADATA_ELEMENT_STATE_PASSIVE);
        }
    }

    metadata_element->nonpaged_record.data_ptr = NULL;
    metadata_element->nonpaged_record.data_size = 0;

    if(FBE_FALSE == fbe_metadata_element_is_element_reinited(metadata_element)){

        /*now we can also register this element so it's on the queue and we can do stuff with it if we want*/
        status = fbe_metadata_register(metadata_element);
        if (status != FBE_STATUS_OK) {
            metadata_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Unable to register metadata element\n", __FUNCTION__);
            fbe_spinlock_destroy(&metadata_element->metadata_element_lock);
            fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_metadata_element_init_stripe_lock_queue(metadata_element);

    fbe_metadata_paged_init_queue(metadata_element);

    fbe_metadata_element_init_attributes(metadata_element);

    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_metadata_element_reinit_memory(fbe_packet_t * packet)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL; 
    
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    /*if we have already been reinitialized, then do not do it again.*/  
    fbe_spinlock_lock(&metadata_element->metadata_element_lock);
    if(FBE_TRUE == fbe_metadata_element_is_element_reinited(metadata_element))
    {
        fbe_spinlock_unlock(&metadata_element->metadata_element_lock);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        
        return FBE_STATUS_OK;
    }
    
    fbe_metadata_element_set_element_reinited(metadata_element);
    fbe_spinlock_unlock(&metadata_element->metadata_element_lock);

    /* unregister this element from metadata service first to avoid
     * illegal access when receiving metadata cmi message from peer*/
    /*fbe_metadata_unregister(metadata_element);*/    

    /*fbe_metadata_stripe_lock_release_all_blobs(metadata_element);    

    /*fbe_metadata_element_destroy_stripe_lock_queue(metadata_element);*/

    /*fbe_metadata_paged_destroy_queue(metadata_element);*/

    /*fbe_metadata_element_set_state(metadata_element, FBE_METADATA_ELEMENT_STATE_INVALID);*/
    
    /* spinlock should not be destroyed because it will still be used to
     * avoid illegal access when receiving metadata cmi message*/
    /*fbe_spinlock_destroy(&metadata_element->metadata_element_lock);*/


    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_metadata_element_destroy(fbe_metadata_element_t * metadata_element)
{
    /* We will not touch anything if we are not initialized. */
    if (metadata_element->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID
        && FBE_FALSE == fbe_metadata_element_is_element_reinited(metadata_element))
    {
        return FBE_STATUS_OK;
    }

    fbe_metadata_element_set_state(metadata_element, FBE_METADATA_ELEMENT_STATE_INVALID);

    fbe_queue_destroy(&metadata_element->paged_record_queue_head);
        
    fbe_metadata_element_destroy_stripe_lock_queue(metadata_element);   
    fbe_metadata_paged_destroy_queue(metadata_element);
    
    fbe_spinlock_destroy(&metadata_element->metadata_element_lock);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_metadata_element_outstanding_stripe_lock_request(fbe_metadata_element_t * metadata_element)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* We will not touch anything if we are not initialized. */
    if (metadata_element->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        return status;
    }

	/* We need to check the hash queues as well */
    fbe_spinlock_lock(&metadata_element->stripe_lock_spinlock);
    if(!fbe_queue_is_empty(&metadata_element->stripe_lock_queue_head))
    {
        status = FBE_STATUS_PENDING;
    }

    if((metadata_element->stripe_lock_blob!=NULL) && (!fbe_queue_is_empty(&metadata_element->stripe_lock_blob->peer_sl_queue)))
    {
        status = FBE_STATUS_PENDING;
    }
	fbe_spinlock_unlock(&metadata_element->stripe_lock_spinlock);

    return status;
}

fbe_status_t 
fbe_metadata_element_get_state(fbe_metadata_element_t * metadata_element, fbe_metadata_element_state_t * metadata_element_state)
{
    * metadata_element_state = metadata_element->metadata_element_state;
    return FBE_STATUS_OK;
}

fbe_bool_t 
fbe_metadata_element_is_active(fbe_metadata_element_t * metadata_element)
{    
    return (metadata_element->metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);
}

fbe_status_t
fbe_metadata_element_set_paged_record_start_lba(fbe_metadata_element_t * metadata_element_p, fbe_lba_t paged_record_start_lba)
{
    metadata_element_p->paged_record_start_lba = paged_record_start_lba;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_get_paged_record_start_lba(fbe_metadata_element_t * metadata_element_p, fbe_lba_t * paged_record_start_lba_p)
{
    *paged_record_start_lba_p = metadata_element_p->paged_record_start_lba;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_set_paged_mirror_metadata_offset(fbe_metadata_element_t * metadata_element_p, fbe_lba_t paged_mirror_metadata_offset)
{
    metadata_element_p->paged_mirror_metadata_offset = paged_mirror_metadata_offset;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_get_paged_mirror_metadata_offset(fbe_metadata_element_t * metadata_element_p, fbe_lba_t * paged_mirror_metadata_offset_p)
{
    *paged_mirror_metadata_offset_p = metadata_element_p->paged_mirror_metadata_offset;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_set_paged_metadata_capacity(fbe_metadata_element_t * metadata_element_p, fbe_lba_t paged_metadata_capacity)
{
    metadata_element_p->paged_metadata_capacity = paged_metadata_capacity;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_get_paged_metadata_capacity(fbe_metadata_element_t * metadata_element_p, fbe_lba_t * paged_metadata_capacity_p)
{
    *paged_metadata_capacity_p = metadata_element_p->paged_metadata_capacity;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_init_paged_record_request_count(fbe_metadata_element_t* metadata_element_p)
{
    metadata_element_p->paged_record_request_count = 0;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_set_nonpaged_request(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_or(&metadata_element_p->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_NONPAGED_REQUEST);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_clear_nonpaged_request(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_NONPAGED_REQUEST);
    return FBE_STATUS_OK;
}

fbe_bool_t 
fbe_metadata_element_is_nonpaged_request_set(fbe_metadata_element_t* metadata_element_p)
{
	return metadata_element_p->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_NONPAGED_REQUEST;
}

static fbe_status_t
fbe_metadata_element_init_attributes(fbe_metadata_element_t* metadata_element_p)
{
    metadata_element_p->attributes = FBE_METADATA_ELEMENT_ATTRIBUTES_INVALID;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_clear_abort(fbe_metadata_element_t* metadata_element_p)
{
    /* We will not touch anything if we are not initialized.
     */
    if (metadata_element_p->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        return FBE_STATUS_OK;
    }
    fbe_spinlock_lock(&metadata_element_p->metadata_element_lock);

    fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_FAIL_PAGED);
    fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_STRIPE_LOCKS);
    
    fbe_spinlock_unlock(&metadata_element_p->metadata_element_lock);
    return FBE_STATUS_OK;
}
fbe_status_t
fbe_metadata_element_restart_io(fbe_metadata_element_t* metadata_element_p)
{
    /* We will not touch anything if we are not initialized.
     */
    if (metadata_element_p->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        return FBE_STATUS_OK;
    }

    /* Clear hold so that I/O will resume.
     */
    fbe_metadata_element_clear_abort(metadata_element_p);

    /* Restart both paged metadata and stripe lock operations.
     */
    fbe_metadata_paged_operation_restart_io(metadata_element_p);
	
	/* We may want to check that there are nothing on the SL queues */
    //fbe_metadata_stripe_lock_restart_element(metadata_element_p);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_abort_io(fbe_metadata_element_t* metadata_element_p)
{
    /* We will not touch anything if we are not initialized.
     */
    if (metadata_element_p->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        return FBE_STATUS_OK;
    }

    /* Abort all queued operations.
     */
    fbe_metadata_paged_abort_io(metadata_element_p);
    fbe_metadata_stripe_lock_element_abort(metadata_element_p);
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_metadata_element_destroy_abort_io(fbe_metadata_element_t* metadata_element_p)
{
    /* We will not touch anything if we are not initialized.
     */
    if (metadata_element_p->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        return FBE_STATUS_OK;
    }

    /* Abort all queued operations.
     */
    fbe_metadata_paged_abort_io(metadata_element_p);
    fbe_metadata_stripe_lock_element_destroy_abort(metadata_element_p);
    return FBE_STATUS_OK;
}


static fbe_status_t
fbe_metadata_element_init_stripe_lock_queue(fbe_metadata_element_t* metadata_element_p)
{
    fbe_queue_init(&metadata_element_p->stripe_lock_queue_head);

    if(metadata_element_p->stripe_lock_number_of_stripes == 0){
        /* Some arbitrary huge value */
        metadata_element_p->stripe_lock_number_of_stripes = 0x80000000; /* Objects will overwrite this value */
    }

    metadata_element_p->stripe_lock_blob = NULL;

    if(FBE_FALSE == fbe_metadata_element_is_element_reinited(metadata_element_p))
    {
        fbe_spinlock_init(&metadata_element_p->stripe_lock_spinlock);
    }
    else
    {
        /*metadata element reinitialize case: spinlock is never destroyed, so no need
         *to init it here*/
    }

	/* Init statistic counters */
	metadata_element_p->stripe_lock_count = 0;
	metadata_element_p->local_collision_count = 0;
	metadata_element_p->peer_collision_count = 0;
	metadata_element_p->cmi_message_count = 0;

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_element_destroy_stripe_lock_queue(fbe_metadata_element_t* mde)
{
    fbe_queue_destroy(&mde->stripe_lock_queue_head);

    fbe_metadata_stripe_lock_release_all_blobs(mde);    
    
    fbe_spinlock_destroy(&mde->stripe_lock_spinlock);

	if(mde->stripe_lock_hash != NULL){
		fbe_metadata_stripe_hash_destroy(mde->stripe_lock_hash);
		fbe_transport_release_buffer(mde->stripe_lock_hash);
		mde->stripe_lock_hash = NULL;
	}

	if(mde->stripe_lock_blob != NULL){
		fbe_spinlock_destroy(&mde->stripe_lock_blob->peer_sl_queue_lock);
		mde->stripe_lock_blob->flags &= ~METADATA_STRIPE_LOCK_BLOB_FLAG_STARTED;
		mde->stripe_lock_blob = NULL;
	}	

	if(mde->stripe_lock_large_io_count != 0){
		metadata_trace(FBE_TRACE_LEVEL_INFO,
							   FBE_TRACE_MESSAGE_ID_INFO,
							   "SL: HASH: stripe_lock_large_io_count %lld \n", mde->stripe_lock_large_io_count);    
	}

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_metadata_operation_entry(fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_payload_metadata_operation_opcode_t opcode;
    fbe_cpu_id_t cpu_id;
    
    /* validating the cpu_id */
    fbe_transport_get_cpu_id(packet, &cpu_id);
    if(cpu_id == FBE_CPU_ID_INVALID){
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s Invalid cpu_id in packet!\n", __FUNCTION__);
    }

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);

    fbe_payload_metadata_get_opcode(metadata_operation, &opcode);
    switch(opcode){
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INIT:
            status = fbe_metadata_nonpaged_operation_init(packet);
            break;

        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_READ_PERSIST:
            status = fbe_metadata_nonpaged_operation_read_persist(packet);
            break;

        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE:
            status = fbe_metadata_nonpaged_operation_write(packet);
            break;

        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_POST_PERSIST:
            status = fbe_metadata_nonpaged_operation_post_persist(packet);
            break;

        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_CLEAR_BITS:
        	status = fbe_metadata_nonpaged_operation_change_bits(packet);
			break;

        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_FORCE_SET_CHECKPOINT:
		case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_CHECKPOINT:
		case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INCR_CHECKPOINT:  
		case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INCR_CHECKPOINT_NO_PEER: 
        	status = fbe_metadata_nonpaged_operation_change_checkpoint(packet);
            break;

        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PERSIST:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PRESET:
            status = fbe_metadata_nonpaged_operation_persist(packet);
            break;

		case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE_VERIFY:
			status = fbe_metadata_nonpaged_operation_write_verify(packet);
			break;
			
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS:            
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE:
            status = fbe_metadata_paged_operation_entry(packet);
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_UNREGISTER_ELEMENT:
            status = fbe_metadata_operation_unregister_element(packet);
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_INIT_MEMORY:
            status = fbe_metadata_element_init_memory(packet);
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_REINIT_MEMORY:
            status = fbe_metadata_element_reinit_memory(packet);
            break;            
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_MEMORY_UPDATE:
            status = fbe_metadata_operation_memory_update(packet);
            break;
        default:
            metadata_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid opcode %d\n", __FUNCTION__, opcode);

            fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);    
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

fbe_metadata_element_t * 
fbe_metadata_queue_element_to_metadata_element(fbe_queue_element_t * queue_element)
{
    fbe_metadata_element_t * metadata_element;
    metadata_element = (fbe_metadata_element_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((fbe_metadata_element_t  *)0)->queue_element));
    return metadata_element;

}


fbe_status_t 
fbe_metadata_get_element_queue_head(fbe_queue_head_t ** queue_head)
{
    * queue_head = &metadata_service.metadata_element_queue_head;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_metadata_switch_all_elements_to_state(fbe_metadata_element_state_t new_state)
{
    fbe_queue_element_t *       queue_element_p = NULL;
    fbe_queue_element_t *       next_queue_element_p = NULL;
    fbe_metadata_element_t *    metadata_element_p = NULL;
    fbe_object_id_t             object_id;
    fbe_metadata_stripe_lock_blob_t *blob = NULL;

/*
    if(new_state != FBE_METADATA_ELEMENT_STATE_ACTIVE)
    {
        EmcpalDebugBreakPoint();
    }    
*/

    fbe_spinlock_lock(&metadata_service.metadata_element_queue_lock);

    queue_element_p = fbe_queue_front(&metadata_service.metadata_element_queue_head);
    while (queue_element_p != NULL){
        /*get the next one for next time*/
        next_queue_element_p = fbe_queue_next(&metadata_service.metadata_element_queue_head, queue_element_p);

        /*and convert the state to what the user asked*/
        metadata_element_p = fbe_metadata_queue_element_to_metadata_element(queue_element_p);

        fbe_metadata_stripe_lock_all(metadata_element_p);

        /* we defer the set of the metadata element state to the condition function, 
         * which get set in base_config_process_peer_contact_lost()
         * so that it's properly interlocked with accessor from other condition function
         */
        if (new_state != FBE_METADATA_ELEMENT_STATE_ACTIVE) {
            fbe_metadata_element_set_state(metadata_element_p, new_state);
        }
        metadata_element_p->peer_metadata_element_ptr = 0;
		fbe_metadata_element_set_peer_dead(metadata_element_p);

        /* When we see the peer die, we need to zero the memory out, 
         * this will allow us to see that it is not joined. 
         * It will also prevent us from operating on stale bits the peer previously set. 
         */
        fbe_zero_memory(metadata_element_p->metadata_memory_peer.memory_ptr,
                        metadata_element_p->metadata_memory_peer.memory_size);
        /* For now we will deal with one blob per object only */
        blob = metadata_element_p->stripe_lock_blob;

        fbe_atomic_32_or(&metadata_element_p->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST);

        /* We need to abort monitor ops when peer lost. */
        fbe_atomic_32_or(&metadata_element_p->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_MONITOR_OPS);

        fbe_metadata_stripe_unlock_all(metadata_element_p);      

        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element_p);

        fbe_topology_send_event(object_id, FBE_EVENT_TYPE_PEER_CONTACT_LOST, NULL);        

        queue_element_p = next_queue_element_p;/*go to next one*/
    }

    fbe_spinlock_unlock(&metadata_service.metadata_element_queue_lock);

    return FBE_STATUS_OK;
}


/*we have the pointer to the metadata queue element from the peer*/
fbe_bool_t
fbe_metadata_is_peer_object_alive(fbe_metadata_element_t *    metadata_element)
{   
    fbe_bool_t is_alive;
    if(metadata_element->peer_metadata_element_ptr != 0){
        is_alive = FBE_TRUE;
    } else {
        is_alive = FBE_FALSE;
    }
    return is_alive;
}

static fbe_status_t fbe_metadata_set_elements_state(struct fbe_packet_s * packet)
{
    fbe_metadata_set_elements_state_t * set_state = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &set_state); 
    if(set_state == NULL){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_metadata_set_elements_state_t)){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "%s %X len invalid\n", __FUNCTION__ , len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*do some validation*/
    if (set_state->new_state != FBE_METADATA_ELEMENT_STATE_ACTIVE && 
        set_state->new_state != FBE_METADATA_ELEMENT_STATE_PASSIVE) {

        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Illegal state to set:%d\n", __FUNCTION__ , set_state->new_state);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now check if we want to do a single object state change or all of them
    we also better trace this so if there was any issue, we know this command was sent*/
    if (set_state->object_id == FBE_OBJECT_ID_INVALID) {
        metadata_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, User request to set all elements to %s\n",
                       __FUNCTION__, set_state->new_state == FBE_METADATA_ELEMENT_STATE_ACTIVE ? "ACTIVE" : "PASSIVE");

        status = fbe_metadata_switch_all_elements_to_state(set_state->new_state);
    }else{
        metadata_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, User request to set object id 0x%X to %s\n",
                       __FUNCTION__, set_state->object_id, set_state->new_state == FBE_METADATA_ELEMENT_STATE_ACTIVE ? "ACTIVE" : "PASSIVE");

        status = fbe_metadata_switch_single_element_to_state(set_state->object_id, set_state->new_state);
    }

    if (status != FBE_STATUS_OK) {

        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s failed to set state %d\n", __FUNCTION__ , set_state->new_state);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}
static fbe_status_t fbe_metadata_switch_single_element_to_state(fbe_object_id_t requested_object_id, fbe_metadata_element_state_t new_state)
{
    fbe_queue_element_t *       queue_element_p = NULL;
    fbe_queue_element_t *       next_queue_element_p = NULL;
    fbe_metadata_element_t *    metadata_element_p = NULL;
    fbe_object_id_t             element_object_id = FBE_OBJECT_ID_INVALID;
    
    fbe_spinlock_lock(&metadata_service.metadata_element_queue_lock);

    queue_element_p = fbe_queue_front(&metadata_service.metadata_element_queue_head);
    while (queue_element_p != NULL){
        /*get the next one for next time*/
        next_queue_element_p = fbe_queue_next(&metadata_service.metadata_element_queue_head, queue_element_p);

        /*and convert the state to what the user asked*/
        metadata_element_p = fbe_metadata_queue_element_to_metadata_element(queue_element_p);
        element_object_id = fbe_base_config_metadata_element_to_object_id(metadata_element_p);
        if (element_object_id == requested_object_id) {
            fbe_metadata_element_set_state(metadata_element_p, new_state);
            fbe_spinlock_unlock(&metadata_service.metadata_element_queue_lock);
            return FBE_STATUS_OK;
        }

        queue_element_p = next_queue_element_p;/*go to next one*/
    }

    fbe_spinlock_unlock(&metadata_service.metadata_element_queue_lock);

    /*if we got here, it's bad since it means we did not find the element we want*/

    return FBE_STATUS_GENERIC_FAILURE;

}

static fbe_status_t fbe_metadata_operation_unregister_element(fbe_packet_t * packet)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation = NULL;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;
    
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation = fbe_payload_ex_get_metadata_operation(sep_payload);

    /*if for some reason someone is trying to unregister when he is not even on the queue we should catch that.
    this warning is acceptable in initial development until all objects use metadata so we don't
    return an error, otherwise, the condition will not be cleared and the object will not destroy*/
    if (metadata_operation->metadata_element->queue_element.next != NULL) {
        status = fbe_metadata_unregister(metadata_operation->metadata_element);
        if (status != FBE_STATUS_OK) {
            fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }else{
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_operation->metadata_element);
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                        "%s object id 0x%X is trying to unregister but it's not registered\n", __FUNCTION__ , object_id);
    }

    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

fbe_status_t
fbe_metadata_calculate_checksum(void * metadata_block_p, fbe_u32_t size)
{
   fbe_u32_t block_index;
   fbe_u32_t * reference_ptr; 

   /* If input pointer is null then return error. */
   if(metadata_block_p == NULL)
   {
       return FBE_STATUS_GENERIC_FAILURE;
   }
   
   /* Calculate the checksum and place it as a metadata on block. */
   for(block_index = 0; block_index < size; block_index++) {

       reference_ptr = ( fbe_u32_t * ) ((char *)metadata_block_p + FBE_METADATA_BLOCK_DATA_SIZE);
       *reference_ptr = fbe_xor_lib_calculate_checksum((fbe_u32_t*)metadata_block_p);
       reference_ptr++;

       *reference_ptr = 0;
       reference_ptr++;

       metadata_block_p = (char *) metadata_block_p + FBE_METADATA_BLOCK_SIZE;
   }

   return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_raw_mirror_calculate_checksum(void * metadata_block_p, fbe_lba_t start_lba,fbe_u32_t size,fbe_bool_t seq_reset)
{
   fbe_u32_t block_index;
   fbe_u32_t * reference_ptr; 
   fbe_u64_t seq = 0;

   /* If input pointer is null then return error. */
   if(metadata_block_p == NULL)
   {
       return FBE_STATUS_GENERIC_FAILURE;
   }
   
   /* Calculate the checksum and place it as a metadata on block. */
   for(block_index = 0; block_index < size; block_index++) {
       /* Add sequence number to raw write I/O for raw mirror error handling. */
	   
       if (!seq_reset) {
            fbe_metadata_raw_mirror_io_get_seq(start_lba + block_index,
                                               FBE_TRUE, &seq);
       } else
           seq = METADATA_RAW_MIRROR_START_SEQ;
	   
       fbe_xor_lib_raw_mirror_sector_set_seq_num(metadata_block_p,seq);

       reference_ptr = ( fbe_u32_t * ) ((char *)metadata_block_p + FBE_METADATA_BLOCK_DATA_SIZE);
       *reference_ptr = fbe_xor_lib_calculate_checksum((fbe_u32_t*)metadata_block_p);
       reference_ptr++;

       *reference_ptr = 0;
       reference_ptr++;

       metadata_block_p = (char *) metadata_block_p + FBE_METADATA_BLOCK_SIZE;
   }

   return FBE_STATUS_OK;
}



fbe_status_t 
fbe_metadata_element_get_metadata_memory(fbe_metadata_element_t * metadata_element_p,  void **memory_ptr)
{
    *memory_ptr = metadata_element_p->metadata_memory.memory_ptr;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_metadata_element_get_metadata_memory_size(fbe_metadata_element_t * metadata_element_p, fbe_u32_t * memory_size)
{
    *memory_size = metadata_element_p->metadata_memory.memory_size;

    return FBE_STATUS_OK;
}

fbe_status_t fbe_metadata_element_get_peer_metadata_memory(fbe_metadata_element_t * metadata_element_p,  void **memory_ptr)
{
    *memory_ptr = metadata_element_p->metadata_memory_peer.memory_ptr;

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_get_peer_metadata_memory_size(fbe_metadata_element_t * metadata_element_p, fbe_u32_t *memory_size)
{
    *memory_size = metadata_element_p->metadata_memory_peer.memory_size;

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_operation_memory_update(fbe_packet_t * packet)
{    
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
	fbe_payload_dmrb_operation_t * dmrb_operation;
	fbe_metadata_cmi_message_t * metadata_cmi_message;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;


	/* Allocate memory from the packet */
	dmrb_operation = fbe_payload_ex_allocate_dmrb_operation(sep_payload);
	
	fbe_payload_ex_increment_dmrb_operation_level(sep_payload);

    metadata_cmi_message = (fbe_metadata_cmi_message_t *)dmrb_operation->opaque;

    metadata_cmi_message->header.object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    metadata_cmi_message->header.metadata_element_receiver = metadata_element->peer_metadata_element_ptr; /* If not NULL should point to the memory of the receiver */
    metadata_cmi_message->header.metadata_element_sender = (fbe_u64_t)(fbe_ptrhld_t)metadata_element;

    metadata_cmi_message->header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_MEMORY_UPDATE;

    fbe_copy_memory(metadata_cmi_message->msg.memory_update_data, 
                    metadata_element->metadata_memory.memory_ptr,
                    metadata_element->metadata_memory.memory_size);

    /* Set completion function */
    fbe_transport_set_completion_function(packet, fbe_metadata_operation_memory_update_completion, metadata_cmi_message);

	if(fbe_metadata_element_is_cmi_disabled(metadata_element)) {
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    fbe_metadata_cmi_send_message(metadata_cmi_message, packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_operation_memory_update_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_status_t status;  
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_metadata_cmi_message_t * metadata_cmi_message = NULL;
	fbe_payload_dmrb_operation_t * dmrb_operation;

    metadata_cmi_message = (fbe_metadata_cmi_message_t *)context;

    sep_payload = fbe_transport_get_payload_ex(packet);

    dmrb_operation = fbe_payload_ex_get_dmrb_operation(sep_payload);
    fbe_payload_ex_release_dmrb_operation(sep_payload, dmrb_operation);

    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    return FBE_STATUS_OK;
}

void fbe_metadata_element_queue_lock(void)
{
    fbe_spinlock_lock(&metadata_service.metadata_element_queue_lock);
}

void fbe_metadata_element_queue_unlock(void)
{
    fbe_spinlock_unlock(&metadata_service.metadata_element_queue_lock);
}

/* The caller MUST hold metadata_element_queue_lock lock */
fbe_status_t 
fbe_metadata_get_element_by_object_id(fbe_object_id_t object_id, fbe_metadata_element_t ** metadata_element)
{
    fbe_queue_element_t *       queue_element_p = NULL;
    fbe_queue_element_t *       next_queue_element_p = NULL;
    fbe_metadata_element_t *    metadata_element_p = NULL;
    fbe_object_id_t             element_object_id = FBE_OBJECT_ID_INVALID;
    
    *metadata_element = NULL;

    queue_element_p = fbe_queue_front(&metadata_service.metadata_element_queue_head);
    while (queue_element_p != NULL){
        /*get the next one for next time*/
        next_queue_element_p = fbe_queue_next(&metadata_service.metadata_element_queue_head, queue_element_p);

        /*and convert the state to what the user asked*/
        metadata_element_p = fbe_metadata_queue_element_to_metadata_element(queue_element_p);
        element_object_id = fbe_base_config_metadata_element_to_object_id(metadata_element_p);
        if (element_object_id == object_id) {
            *metadata_element = metadata_element_p;
            return FBE_STATUS_OK;
        }

        queue_element_p = next_queue_element_p;/*go to next one*/
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t
fbe_metadata_element_set_number_of_stripes(fbe_metadata_element_t * metadata_element_p, fbe_u64_t number_of_stripes)
{
    /* We add one for the NP metadata lock.
     */
    metadata_element_p->stripe_lock_number_of_stripes = number_of_stripes + 1;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_get_number_of_stripes(fbe_metadata_element_t * metadata_element_p, fbe_u64_t * number_of_stripes)
{
    *number_of_stripes = metadata_element_p->stripe_lock_number_of_stripes;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_metadata_element_set_number_of_private_stripes()
 ****************************************************************
 * @brief
 *  Get number of private stripes from meta data element structure.
 *
 * @param  
 *   metadata_element_p  - pointer to the meta data element structure (in/out)
 *   number_of_stripes   - number of stripes to be set
 *
 * @return fbe_status_t
 *
 * @author
 *  7/6/2011 - Created. Naizhong Chiu
 *
 ****************************************************************/
fbe_status_t
fbe_metadata_element_set_number_of_private_stripes(fbe_metadata_element_t * metadata_element_p, fbe_u64_t number_of_stripes)
{
    metadata_element_p->stripe_lock_number_of_private_stripes = number_of_stripes;
    return FBE_STATUS_OK;
} /* end of fbe_metadata_element_set_number_of_private_stripes() */


/*!**************************************************************
 * fbe_metadata_element_get_number_of_private_stripes()
 ****************************************************************
 * @brief
 *  Get number of private stripes from meta data element structure.
 *
 * @param  
 *   metadata_element_p  - pointer to the meta data element structure
 *   number_of_stripes   - pointer to the number of stripes (output)
 *
 * @return fbe_status_t
 *
 * @author
 *  7/6/2011 - Created. Naizhong Chiu
 *
 ****************************************************************/
fbe_status_t
fbe_metadata_element_get_number_of_private_stripes(fbe_metadata_element_t * metadata_element_p, fbe_u64_t * number_of_stripes)
{
    *number_of_stripes = metadata_element_p->stripe_lock_number_of_private_stripes;
    return FBE_STATUS_OK;
}  /* end of fbe_metadata_element_get_number_of_private_stripes() */


/*!**************************************************************
 * fbe_metadata_scan_for_cancelled_packets()
 ****************************************************************
 * @brief
 *  Scan all the metadata elements for any that have the
 *  FBE_METADATA_ELEMENT_ATTRIBUTES_ABORTED_REQUESTS attribute.
 * 
 *  This attribute is set by a cancel routine for a paged or
 *  stripe lock operation.  This attribute tells the
 *  metadata element to go scan its packet queues for cancelled packets.
 *
 * @param - None.              
 *
 * @return fbe_status_t
 *
 * @author
 *  4/6/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_metadata_scan_for_cancelled_packets(fbe_bool_t is_force_scan)
{
    fbe_queue_element_t *       queue_element_p = NULL;
    fbe_queue_element_t *       next_queue_element_p = NULL;
    fbe_metadata_element_t *    metadata_element_p = NULL;

    fbe_metadata_element_queue_lock();

    /* Scan the entire queue.  If we find any then we will scan this one 
     * only for cancelled packets. 
     */
    queue_element_p = fbe_queue_front(&metadata_service.metadata_element_queue_head);
    while (queue_element_p != NULL){
        /* get the next one for next time
         */ 
        next_queue_element_p = fbe_queue_next(&metadata_service.metadata_element_queue_head, queue_element_p);

        /* Check if this element has aborted requests.
         */
        metadata_element_p = fbe_metadata_queue_element_to_metadata_element(queue_element_p);
        
        fbe_spinlock_lock(&metadata_element_p->metadata_element_lock);
        if (metadata_element_p->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_ABORTED_REQUESTS || is_force_scan)
        {
            fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_ABORTED_REQUESTS);
            fbe_spinlock_unlock(&metadata_element_p->metadata_element_lock);
			/* We found something with aborted requests, just paged and
			 * stripe lock operations for cancelled packets. 
			 */
			fbe_metadata_element_paged_complete_cancelled(metadata_element_p);
			fbe_metadata_element_stripe_lock_complete_cancelled(metadata_element_p);
			queue_element_p = next_queue_element_p;/*go to next one*/
			continue;
		}
        fbe_spinlock_unlock(&metadata_element_p->metadata_element_lock);

        queue_element_p = next_queue_element_p;/*go to next one*/
    }

    if (metadata_element_p != NULL)
    {
        /* We found something with aborted requests, just paged and
         * stripe lock operations for cancelled packets. 
         */
        fbe_metadata_element_paged_complete_cancelled(metadata_element_p);
        fbe_metadata_element_stripe_lock_complete_cancelled(metadata_element_p);
    }
    
    fbe_metadata_element_queue_unlock();

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_metadata_scan_for_cancelled_packets()
 ******************************************/

fbe_status_t 
fbe_metadata_operation_entry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_status_t status;
	status = fbe_metadata_operation_entry(packet);
	return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*we beleive the peer is at a DESTROYED state*/
fbe_status_t
fbe_metadata_element_set_peer_dead(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_or(&metadata_element_p->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_PEER_DEAD);
    return FBE_STATUS_OK;
}


/*we beleive the peer is not at a DESTROYED state*/
fbe_status_t
fbe_metadata_element_clear_peer_dead(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_PEER_DEAD);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_clear_sl_peer_lost(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_SL_PEER_LOST);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_element_clear_abort_monitor_ops(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_ABORT_MONITOR_OPS);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_metadata_element_set_element_reinited(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_or(&metadata_element_p->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_ELEMENT_REINITED);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_metadata_element_clear_element_reinited(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_ELEMENT_REINITED);
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_metadata_element_is_element_reinited(fbe_metadata_element_t* metadata_element_p)
{
    if(metadata_element_p->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_ELEMENT_REINITED)
    {
        return FBE_TRUE;
    }
    
    return FBE_FALSE;
}


/*this function checks if the peer (which may have been alive once) is at a DESTRYED state*/
fbe_bool_t
fbe_metadata_element_is_peer_dead(fbe_metadata_element_t* metadata_element_p)
{
	if(metadata_element_p->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_PEER_DEAD) {
		return FBE_TRUE;
	} else {
		return FBE_FALSE;
	}
}

/* This function just sets the attributes that indicates that peer is in the process of persisting data*/
fbe_status_t
fbe_metadata_element_set_peer_persist_pending(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_or(&metadata_element_p->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_PEER_PERSIST_PENDING);
    return FBE_STATUS_OK;
}

/* This function just clears the attributes that indicates that peer is in the process of persisting data*/
fbe_status_t
fbe_metadata_element_clear_peer_persist_pending(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_PEER_PERSIST_PENDING);
    return FBE_STATUS_OK;
}

/* This function just checks if the peer is in the process of persisting data*/
fbe_bool_t
fbe_metadata_element_is_peer_persist_pending(fbe_metadata_element_t* metadata_element_p)
{
	if(metadata_element_p->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_PEER_PERSIST_PENDING) {
		return FBE_TRUE;
	} else {
		return FBE_FALSE;
	}
}
/*we want_to_make sure we stop transmitting on CMI, just before we kill ourselvs*/
fbe_status_t
fbe_metadata_element_disable_cmi(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_or(&metadata_element_p->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_DISBALE_CMI);
    return FBE_STATUS_OK;
}

fbe_bool_t
fbe_metadata_element_is_cmi_disabled(fbe_metadata_element_t* metadata_element_p)
{
	if(metadata_element_p->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_DISBALE_CMI) {
		return FBE_TRUE;
	} else {
		return FBE_FALSE;
	}
}

fbe_status_t fbe_metadata_element_set_paged_object_cache_flag(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_or(&metadata_element_p->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_PAGED_LOCAL_CACHE);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_metadata_element_clear_paged_object_cache_flag(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_and(&metadata_element_p->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_PAGED_LOCAL_CACHE);
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_metadata_element_is_paged_object_cache(fbe_metadata_element_t* metadata_element_p)
{
    if(metadata_element_p->attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_PAGED_LOCAL_CACHE)
    {
        return FBE_TRUE;
    }
    
    return FBE_FALSE;
}

fbe_status_t fbe_metadata_element_set_paged_object_cache_function(fbe_metadata_element_t* metadata_element_p, 
																  fbe_metadata_paged_cache_callback_t callback_function)
{
    metadata_element_p->cache_callback = callback_function;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_metadata_raw_mirror_io_get_seq
 *****************************************************************
 * @brief
 *   get the sequence number of a raw-mirror block, it will check and ensure
 *   the sequence number is in a specified range
 *
 * @param lba  - the block address to get the sequence number
 * @param inc   - indicating if increase the sequence number before returning it
 * @param seq - return the got sequence number
 * 
 * @return status - indicating if we can get the sequence number successfully
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_metadata_raw_mirror_io_get_seq(fbe_lba_t lba, fbe_bool_t inc,
                                                  fbe_u64_t *seq)
{

    if (!seq || lba >= fbe_metadata_raw_mirror_io_cb.block_count) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (inc) {
        if (fbe_metadata_raw_mirror_io_cb.seq_numbers[lba] >= METADATA_RAW_MIRROR_END_SEQ) {
            fbe_metadata_raw_mirror_io_cb.seq_numbers[lba] = METADATA_RAW_MIRROR_START_SEQ;
        } else
            fbe_metadata_raw_mirror_io_cb.seq_numbers[lba] ++;
    }
    *seq = fbe_metadata_raw_mirror_io_cb.seq_numbers[lba];

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn metadata_common_cmi_is_active()
 *****************************************************************
 * @brief
 *   return the state of CMI.  If CMI is busy, wait until CMI returns a valid state.
 *
 * @param  none
 *
 * @return fbe_boot_t - FBE_TRUE for active and FBE_FALSE for passive.
 *
 * @version
 *    04/25/2012:   created
 *
 ****************************************************************/
fbe_bool_t metadata_common_cmi_is_active(void)
{
    fbe_cmi_sp_state_t sp_state;
    fbe_bool_t         is_active = FBE_TRUE;

    fbe_cmi_get_current_sp_state(&sp_state);
    while(sp_state != FBE_CMI_STATE_ACTIVE && sp_state != FBE_CMI_STATE_PASSIVE && sp_state != FBE_CMI_STATE_SERVICE_MODE)
    {
        fbe_thread_delay(100);
        fbe_cmi_get_current_sp_state(&sp_state);
    }
    if (sp_state != FBE_CMI_STATE_ACTIVE) {
        is_active = FBE_FALSE;
    }

    return is_active;
}

fbe_status_t
fbe_metadata_element_hold_peer_data_stripe_locks_on_contact_loss(fbe_metadata_element_t* metadata_element_p)
{
    fbe_atomic_32_or(&metadata_element_p->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_HOLD_PEER_DATA_STRIPE_LOCKS);
    return FBE_STATUS_OK;
}

void fbe_metadata_get_control_mem_use_mb(fbe_u32_t *memory_use_mb_p)
{
    fbe_u32_t memory_mb = 0;
    fbe_u32_t paged_mb;
    fbe_u32_t nonpaged_mb;
    fbe_u32_t sl_mb;

    fbe_metadata_paged_get_memory_use(&paged_mb);
    fbe_metadata_sl_get_memory_use(&sl_mb);
    fbe_metadata_ext_pool_lock_get_memory_use(&sl_mb);
    fbe_metadata_nonpaged_get_memory_use(&nonpaged_mb);

    memory_mb += paged_mb;
    memory_mb += sl_mb;
    memory_mb += nonpaged_mb;
    memory_mb = ((memory_mb + (1024*1024) - 1) / (1024*1024));
    *memory_use_mb_p = memory_mb;
}

/*!***************************************************************
 * @fn fbe_metadata_set_ndu_in_progress()
 *****************************************************************
 * @brief
 *   This function sets ndu_in_progress flag.
 *
 * @param  packet - pointer to usurper packet.
 *
 * @return fbe_status_t
 *
 * @version
 *    11/14/2013:   created
 *
 ****************************************************************/
static fbe_status_t fbe_metadata_set_ndu_in_progress(struct fbe_packet_s * packet)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Set the flag */
    ndu_in_progress = FBE_TRUE;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_metadata_clear_ndu_in_progress()
 *****************************************************************
 * @brief
 *   This function clears ndu_in_progress flag.
 *
 * @param  packet - pointer to usurper packet.
 *
 * @return fbe_status_t
 *
 * @version
 *    11/14/2013:   created
 *
 ****************************************************************/
static fbe_status_t fbe_metadata_clear_ndu_in_progress(struct fbe_packet_s * packet)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Clear the flag */
    ndu_in_progress = FBE_FALSE;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_metadata_element_get_sl_info(fbe_api_metadata_get_sl_info_t * sl_info)
{
    fbe_queue_element_t *       queue_element_p = NULL;
    fbe_queue_element_t *       next_queue_element_p = NULL;
    fbe_metadata_element_t *    metadata_element_p = NULL;
    fbe_object_id_t             element_object_id = FBE_OBJECT_ID_INVALID;
    
    fbe_spinlock_lock(&metadata_service.metadata_element_queue_lock);

    queue_element_p = fbe_queue_front(&metadata_service.metadata_element_queue_head);
    while (queue_element_p != NULL){
        /*get the next one for next time*/
        next_queue_element_p = fbe_queue_next(&metadata_service.metadata_element_queue_head, queue_element_p);

        metadata_element_p = fbe_metadata_queue_element_to_metadata_element(queue_element_p);
        element_object_id = fbe_base_config_metadata_element_to_object_id(metadata_element_p);
        if (element_object_id == sl_info->object_id) {
            sl_info->large_io_count = (fbe_u32_t)(metadata_element_p->stripe_lock_large_io_count);
            sl_info->blob_size = metadata_element_p->stripe_lock_blob->size;
            fbe_copy_memory(sl_info->slots, metadata_element_p->stripe_lock_blob->slot, sl_info->blob_size);
            fbe_spinlock_unlock(&metadata_service.metadata_element_queue_lock);
            return FBE_STATUS_OK;
        }

        queue_element_p = next_queue_element_p;/*go to next one*/
    }

    fbe_spinlock_unlock(&metadata_service.metadata_element_queue_lock);

    /*if we got here, it's bad since it means we did not find the element we want*/
    return FBE_STATUS_GENERIC_FAILURE;
}

static fbe_status_t fbe_metadata_control_get_stripe_lock_info(fbe_packet_t * packet)
{
    fbe_api_metadata_get_sl_info_t * get_sl_info = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_sl_info); 
    if(get_sl_info == NULL){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_api_metadata_get_sl_info_t)){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "%s %X len invalid\n", __FUNCTION__ , len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_metadata_element_get_sl_info(get_sl_info);
    if (status != FBE_STATUS_OK) {

        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s failed to get stripe lock info for obj 0x%x\n", __FUNCTION__ , get_sl_info->object_id);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
