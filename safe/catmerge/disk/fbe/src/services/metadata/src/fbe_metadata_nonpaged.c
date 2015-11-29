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
#include "fbe_cmi.h"
#include "fbe_metadata_private.h"
#include "fbe_metadata_cmi.h"
#include "fbe_database.h"
#include "fbe_raid_library.h"
#include "fbe_raid_raw_mirror_utils.h"
#include "fbe_private_space_layout.h"

#define FBE_METADATA_NONPAGED_LOAD_READ_SIZE_BLOCKS (64 * METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER)

#define FBE_METADATA_NONPAGED_DEFAULT_BLOCKS_PER_OBJECT 8 /* for 4k system drive */

typedef enum fbe_metadata_nonpaged_state_e{
    FBE_METADATA_NONPAGED_STATE_INVALID,
    FBE_METADATA_NONPAGED_STATE_ALLOCATED,
    FBE_METADATA_NONPAGED_STATE_FREE,
    FBE_METADATA_NONPAGED_STATE_VAULT,
}fbe_metadata_nonpaged_state_t;

typedef struct fbe_metadata_nonpaged_entry_s{
    fbe_u32_t data_size;
    fbe_metadata_nonpaged_state_t state;
    fbe_u8_t data[FBE_METADATA_NONPAGED_MAX_SIZE];
}fbe_metadata_nonpaged_entry_t;

enum fbe_metadata_nonpaged_constants_e{
    METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER  = 32, /* 32 64 block chunks */
};

static fbe_metadata_nonpaged_entry_t * fbe_metadata_nonpaged_array;
fbe_metadata_raw_mirror_io_cb_t fbe_metadata_raw_mirror_io_cb;

fbe_raw_mirror_t fbe_metadata_nonpaged_raw_mirror;

static fbe_u32_t blocks_per_object = FBE_METADATA_NONPAGED_DEFAULT_BLOCKS_PER_OBJECT;

/* Forward declaration */
static fbe_status_t fbe_metadata_nonpaged_operation_write_allocation_completion(fbe_memory_request_t * memory_request, 
                                                                                fbe_memory_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_operation_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


static fbe_status_t fbe_metadata_nonpaged_operation_change_bits_allocation_completion(fbe_memory_request_t * memory_request, 
                                                                                      fbe_memory_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_operation_change_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


static fbe_status_t fbe_metadata_nonpaged_persist_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_persist_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_load_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_load_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_operation_read_persist_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_operation_read_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_operation_persist_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_operation_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_operation_change_checkpoint_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_operation_change_checkpoint_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_system_load_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_system_load_with_diskmask_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_system_load_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_operation_system_persist(fbe_packet_t * packet);
static fbe_status_t fbe_metadata_nonpaged_operation_system_persist_allocation_completion(fbe_memory_request_t * memory_request,
																						 fbe_memory_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_operation_system_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_system_clear_allocation_completion(fbe_memory_request_t * memory_request, 
																			 fbe_memory_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_system_clear_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_operation_system_write_verify(fbe_packet_t * packet);
static fbe_status_t fbe_metadata_nonpaged_operation_system_write_verify_allocation_completion(fbe_memory_request_t * memory_request,
																						 fbe_memory_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_operation_system_write_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_operation_post_persist_allocation_completion(fbe_memory_request_t * memory_request, 
                                                                                       fbe_memory_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_operation_post_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t
fbe_metadata_set_nonpaged_metadata_state_in_memory(fbe_base_config_nonpaged_metadata_state_t np_state,fbe_object_id_t in_object_id);
static fbe_status_t
fbe_metadata_set_block_seq_number(fbe_u8_t *  in_block_p,
                                                fbe_object_id_t in_object_id);

static fbe_status_t fbe_metadata_nonpaged_load_single_system_object(fbe_packet_t *packet, 
                                                                               raw_mirror_system_load_context_t*raw_mirr_op_cb);
static fbe_status_t fbe_metadata_nonpaged_load_single_block_read(fbe_packet_t * packet);
static fbe_status_t 
fbe_metadata_nonpaged_load_single_block_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_metadata_nonpaged_system_zero_and_persist_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);

static fbe_status_t fbe_metadata_nonpaged_send_packet_to_raw_mirror(fbe_packet_t * new_packet, fbe_object_id_t object_id, fbe_bool_t need_special_attribute);
static void fbe_metadata_nonpaged_save_sequence_number_for_system_object(fbe_metadata_cmi_message_header_t * cmi_msg_header, fbe_u64_t seq_number);

static __forceinline fbe_u32_t fbe_metadata_nonpaged_get_blocks_per_object(void);
static __forceinline fbe_bool_t fbe_metadata_nonpaged_is_system_drive(fbe_object_id_t start_object_id, fbe_u32_t object_count);
static __forceinline void fbe_metadata_nonpaged_calculate_checksum_for_one_object(fbe_u8_t *buffer, fbe_object_id_t object_id, fbe_u32_t blocks_per_object, fbe_bool_t is_system);
static __forceinline fbe_status_t fbe_metadata_nonpaged_get_chunk_number(fbe_u32_t object_count,
                                                                         fbe_memory_chunk_size_t *chunk_size_p,
                                                                         fbe_memory_number_of_objects_t *chunk_number_p);
static fbe_status_t fbe_metadata_nonpaged_setup_sg_list_from_memory_request(fbe_memory_request_t * memory_request,
                                                                          fbe_sg_element_t ** sg_list_pp,
                                                                          fbe_u32_t *sg_list_count_p);

static __forceinline fbe_status_t fbe_metadata_nonpaged_get_lba_block_count(fbe_object_id_t object_id, fbe_lba_t *lba_p, fbe_u32_t *block_count_p);

/*This function caculate the block count based on the chunk size
    It is used to caculate how much chunk we need
    when allocate the memory  for system nonpaged metadata load*/
static fbe_memory_chunk_size_block_count_t
chunk_size_to_block_count(fbe_memory_chunk_size_t chunk_size)
{
    switch (chunk_size) {
    case FBE_MEMORY_CHUNK_SIZE_FOR_PACKET:
        return FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET;
    case FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO:
        return FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;
    default:
        return FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET;
    }
}
void fbe_metadata_nonpaged_get_memory_use(fbe_u32_t *memory_bytes_p)
{
    fbe_u32_t memory_bytes = 0;
    memory_bytes += (FBE_METADATA_NONPAGED_MAX_OBJECTS * sizeof(fbe_metadata_nonpaged_entry_t));
    *memory_bytes_p = memory_bytes;
}

fbe_status_t
fbe_metadata_nonpaged_init(void)
{   
    fbe_status_t status;
    fbe_u32_t i;
    fbe_lba_t mirror_offset = FBE_LBA_INVALID;
    fbe_lba_t mirror_rg_offset = FBE_LBA_INVALID;
    fbe_block_count_t mirror_capacity = FBE_LBA_INVALID;

    /* Allocate memory for nonpaged array */
    metadata_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
				   "MCRMEM: NonPaged: %d \n",(int)(FBE_METADATA_NONPAGED_MAX_OBJECTS * sizeof(fbe_metadata_nonpaged_entry_t)));

    fbe_metadata_nonpaged_array = fbe_memory_allocate_required(FBE_METADATA_NONPAGED_MAX_OBJECTS * sizeof(fbe_metadata_nonpaged_entry_t));

    /* Check if we have the required memory. If not, don't proceed at all. Just PANIC. */
    if(fbe_metadata_nonpaged_array == NULL)
    {
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Could not allocate the required memory: 0x%x. PANIC..\n", 
                       __FUNCTION__, 
                       (unsigned int)(FBE_METADATA_NONPAGED_MAX_OBJECTS * sizeof(fbe_metadata_nonpaged_entry_t)));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fill array with default values. Note: we should skip this state if the memory restored from the vault */
    for(i = 0 ; i < FBE_METADATA_NONPAGED_MAX_OBJECTS; i++){
        fbe_metadata_nonpaged_array[i].data_size = 0;
        fbe_metadata_nonpaged_array[i].state = FBE_METADATA_NONPAGED_STATE_FREE;
        fbe_zero_memory(fbe_metadata_nonpaged_array[i].data, FBE_METADATA_NONPAGED_MAX_SIZE);
    }
    
    /* Get these values from the database svc. */
    fbe_database_get_non_paged_metadata_offset_capacity(&mirror_offset, &mirror_rg_offset, &mirror_capacity);
    FBE_ASSERT_AT_COMPILE_TIME(FBE_METADATA_NONPAGED_MAX_SIZE <= FBE_RAW_MIRROR_DATA_SIZE);
    status = fbe_raw_mirror_init(&fbe_metadata_nonpaged_raw_mirror, 
                                 mirror_offset, 
                                 mirror_rg_offset,
                                 mirror_capacity);
	status = fbe_metadata_raw_mirror_io_cb_init(&fbe_metadata_raw_mirror_io_cb,mirror_capacity);
	return status;
}

fbe_status_t
fbe_metadata_nonpaged_destroy(void)
{
    if(fbe_metadata_nonpaged_array != NULL)
    {
        fbe_memory_release_required(fbe_metadata_nonpaged_array);
        fbe_metadata_nonpaged_array = NULL;
    }

    fbe_raw_mirror_destroy(&fbe_metadata_nonpaged_raw_mirror);
    fbe_metadata_raw_mirror_io_cb_destroy(&fbe_metadata_raw_mirror_io_cb);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_nonpaged_operation_init(fbe_packet_t * packet)
{
    fbe_payload_ex_t                   *sep_payload = NULL;
    fbe_payload_metadata_operation_t   *metadata_operation = NULL;
    fbe_object_id_t                     object_id;
    fbe_bool_t                          b_is_nonpaged_initialized = FBE_FALSE;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_operation->metadata_element);

    if(metadata_operation->metadata_element->nonpaged_record.data_size > FBE_METADATA_NONPAGED_MAX_SIZE){
        metadata_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s Invalid nonpaged_record.data_size %d > %d \n", __FUNCTION__, 
                       metadata_operation->metadata_element->nonpaged_record.data_size,
                       FBE_METADATA_NONPAGED_MAX_SIZE);

        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_OK;
    }

    fbe_metadata_nonpaged_array[object_id].data_size = metadata_operation->metadata_element->nonpaged_record.data_size;
    fbe_metadata_nonpaged_array[object_id].state = FBE_METADATA_NONPAGED_STATE_ALLOCATED;

    /* Set pointer to memory  and check if we need to zero it or not.
     */
    metadata_operation->metadata_element->nonpaged_record.data_ptr = fbe_metadata_nonpaged_array[object_id].data;
    b_is_nonpaged_initialized = fbe_base_config_metadata_element_is_nonpaged_initialized(metadata_operation->metadata_element);

    /* Trace some debug information.
     */
    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "metadata: init obj: 0x%x size: %d is_init: %d data [0]: 0x%llx [1]: 0x%llx [2]: 0x%llx \n",
                   object_id, metadata_operation->metadata_element->nonpaged_record.data_size, b_is_nonpaged_initialized,
                   (unsigned long long)(((fbe_u64_t *)&fbe_metadata_nonpaged_array[object_id].data)[0]),
                   (unsigned long long)(((fbe_u64_t *)&fbe_metadata_nonpaged_array[object_id].data)[1]),
                   (unsigned long long)(((fbe_u64_t *)&fbe_metadata_nonpaged_array[object_id].data)[2]));

    /* Only if the non-paged is not initialized will we zero the non-paged area.
     */
    if (b_is_nonpaged_initialized == FBE_FALSE)
    {
        /* Zero the non-paged area.
         */
        fbe_zero_memory(&fbe_metadata_nonpaged_array[object_id].data, metadata_operation->metadata_element->nonpaged_record.data_size);
    }

    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_nonpaged_operation_write(fbe_packet_t * packet)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_object_id_t object_id;
    fbe_memory_request_t * memory_request = NULL;
    fbe_memory_request_priority_t memory_request_priority = 0;
    fbe_status_t status;
	fbe_metadata_element_t * metadata_element = NULL;	

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
	metadata_element = metadata_operation->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

   if(metadata_operation->u.metadata.offset + metadata_operation->u.metadata.record_data_size > fbe_metadata_nonpaged_array[object_id].data_size){
        metadata_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s Invalid write size %llu > %d \n", __FUNCTION__, 
                       (unsigned long long)(metadata_operation->u.metadata.offset + metadata_operation->u.metadata.record_data_size),
                       fbe_metadata_nonpaged_array[object_id].data_size);

        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&metadata_element->metadata_element_lock);

    fbe_copy_memory(fbe_metadata_nonpaged_array[object_id].data + metadata_operation->u.metadata.offset,
                    metadata_operation->u.metadata.record_data, 
                    metadata_operation->u.metadata.record_data_size);

	fbe_spinlock_unlock(&metadata_element->metadata_element_lock);


	if(!fbe_metadata_is_peer_object_alive(metadata_element) && fbe_metadata_element_is_active(metadata_element)){
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
	}

    /* We may want to send cmi message */
    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1, /* one chunk */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
                                   (fbe_memory_completion_function_t) fbe_metadata_nonpaged_operation_write_allocation_completion,
                                   NULL /* completion_context */);

		
	fbe_transport_memory_request_set_io_master(memory_request, packet);

    status = fbe_memory_request_entry(memory_request);
   
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_write_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_memory_header_t * master_memory_header = NULL;

    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_metadata_cmi_message_t * metadata_cmi_message;  

    packet = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    master_memory_header = memory_request->ptr;   
    metadata_cmi_message = (fbe_metadata_cmi_message_t *)master_memory_header->data;

    metadata_cmi_message->header.object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    metadata_cmi_message->header.metadata_element_receiver = metadata_element->peer_metadata_element_ptr; /* If not NULL should point to the memory of the receiver */
    metadata_cmi_message->header.metadata_element_sender = (fbe_u64_t)(csx_ptrhld_t)metadata_element;

    metadata_cmi_message->header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_NONPAGED_WRITE;
    /*metadata.record_data_size cannot be larger than FBE_PAYLOAD_METADATA_MAX_DATA_SIZE*/
    if(metadata_operation->u.metadata.record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){ 
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Invalid memory size %d > %d. Op:%d ", __FUNCTION__ , 
                       metadata_operation->u.metadata.record_data_size, 
                       FBE_METADATA_CMI_NONPAGED_WRITE_DATA_SIZE,
                       metadata_operation->opcode);

        /* Release memory */
        //fbe_memory_request_get_entry_mark_free(memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(memory_request);

        /* Complete packet */
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);


        return FBE_STATUS_OK;
    }

    fbe_copy_memory(metadata_cmi_message->msg.nonpaged_write.data,
                    metadata_operation->u.metadata.record_data, 
                    metadata_operation->u.metadata.record_data_size);

    metadata_cmi_message->msg.nonpaged_write.data_size = metadata_operation->u.metadata.record_data_size;
    metadata_cmi_message->msg.nonpaged_write.offset = metadata_operation->u.metadata.offset;
    metadata_cmi_message->header.metadata_operation_flags = metadata_operation->u.metadata.operation_flags;

    /* Include the sequence number for system objects */
    if (fbe_private_space_layout_object_id_is_system_object(metadata_cmi_message->header.object_id)) {
        metadata_cmi_message->header.metadata_operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_SEQ_NUM;
        metadata_cmi_message->msg.nonpaged_write.seq_number = fbe_metadata_raw_mirror_io_cb.seq_numbers[metadata_cmi_message->header.object_id ];
    }

    /* Set completion function */
    fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_operation_write_completion, metadata_cmi_message);

	if(fbe_metadata_element_is_peer_dead(metadata_element) || (fbe_metadata_element_is_cmi_disabled(metadata_element))) {
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    fbe_metadata_cmi_send_message(metadata_cmi_message, packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_status_t status;  
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_memory_request_t * memory_request = NULL;
    fbe_metadata_cmi_message_t * metadata_cmi_message = NULL;

    metadata_cmi_message = (fbe_metadata_cmi_message_t *)context;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    memory_request = fbe_transport_get_memory_request(packet);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    //fbe_memory_request_get_entry_mark_free(memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(memory_request);

    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    return FBE_STATUS_OK;
}

/*!**************************************************************************************
 * fbe_metadata_nonpaged_operation_post_persist()
 ****************************************************************************************
 * @brief
 *   This function starts the post persist operation
 *
 * @param  packet_p    - Pointer to the packet
 * 
 *
 * @return fbe_status_t   
 *
 * @author
 *  04/19/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************************/
fbe_status_t
fbe_metadata_nonpaged_operation_post_persist(fbe_packet_t * packet)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_object_id_t object_id;
    fbe_memory_request_t * memory_request = NULL;
    fbe_memory_request_priority_t memory_request_priority = 0;
    fbe_status_t status;
	fbe_metadata_element_t * metadata_element = NULL;	

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
	metadata_element = metadata_operation->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

   if(!fbe_metadata_is_peer_object_alive(metadata_element) && fbe_metadata_element_is_active(metadata_element)){
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
	}

    /* We may want to send cmi message */
    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1, /* one chunk */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
                                   (fbe_memory_completion_function_t) fbe_metadata_nonpaged_operation_post_persist_allocation_completion,
                                   NULL /* completion_context */);


	fbe_transport_memory_request_set_io_master(memory_request, packet);

    status = fbe_memory_request_entry(memory_request);
   
    return FBE_STATUS_OK;
}

/*!**************************************************************************************
 * fbe_metadata_nonpaged_operation_post_persist_allocation_completion()
 ****************************************************************************************
 * @brief
 *   This function starts the persist complete operation by sending a message to the peer
 * about the completion
 *
 * @param  memory_request    - Pointer to the memory that was just allocated
 * @param context      - Completion context
 *
 * @return fbe_status_t   
 *
 * @author
 *  04/19/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************************/
static fbe_status_t 
fbe_metadata_nonpaged_operation_post_persist_allocation_completion(fbe_memory_request_t * memory_request, 
                                                                   fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_memory_header_t * master_memory_header = NULL;

    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_metadata_cmi_message_t * metadata_cmi_message;  

    packet = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    master_memory_header = memory_request->ptr;   
    metadata_cmi_message = (fbe_metadata_cmi_message_t *)master_memory_header->data;

    metadata_cmi_message->header.object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    metadata_cmi_message->header.metadata_element_receiver = metadata_element->peer_metadata_element_ptr; /* If not NULL should point to the memory of the receiver */
    metadata_cmi_message->header.metadata_element_sender = (fbe_u64_t)(csx_ptrhld_t)metadata_element;

    metadata_cmi_message->header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_NONPAGED_POST_PERSIST;
    
    /* Set completion function */
    fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_operation_post_persist_completion, metadata_cmi_message);

	if(fbe_metadata_element_is_cmi_disabled(metadata_element)) {
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    fbe_metadata_cmi_send_message(metadata_cmi_message, packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************************************
 * fbe_metadata_nonpaged_operation_post_persist_completion()
 ****************************************************************************************
 * @brief
 *   This is the callback function after the CMI message is processed for the persist 
 * completion operation
 *
 * @param  packet_p    - Pointer to the packet
 * @param context      - Completion context
 *
 * @return fbe_status_t   
 *
 * @author
 *  04/19/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************************/
static fbe_status_t 
fbe_metadata_nonpaged_operation_post_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_status_t status;  
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_memory_request_t * memory_request = NULL;
    fbe_metadata_cmi_message_t * metadata_cmi_message = NULL;

    metadata_cmi_message = (fbe_metadata_cmi_message_t *)context;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    memory_request = fbe_transport_get_memory_request(packet);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    //fbe_memory_request_get_entry_mark_free(memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(memory_request);

    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    return FBE_STATUS_OK;
}


fbe_status_t
fbe_metadata_nonpaged_operation_change_bits(fbe_packet_t * packet)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_object_id_t object_id;
    fbe_memory_request_t * memory_request = NULL;
    fbe_memory_request_priority_t memory_request_priority = 0;
    fbe_status_t status;
    fbe_u64_t data_size;
    fbe_u32_t i;
    fbe_u32_t j;
    fbe_u8_t * data_ptr = NULL;
	fbe_metadata_element_t * metadata_element = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
	metadata_element = metadata_operation->metadata_element;

    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

    data_size = metadata_operation->u.metadata.offset + metadata_operation->u.metadata.record_data_size * metadata_operation->u.metadata.repeat_count;
    if(data_size > fbe_metadata_nonpaged_array[object_id].data_size){
        metadata_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s Invalid write size %llu > %d \n", __FUNCTION__, 
                       (unsigned long long)data_size,
                       fbe_metadata_nonpaged_array[object_id].data_size);

        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    data_ptr = (fbe_u8_t *)(fbe_metadata_nonpaged_array[object_id].data + metadata_operation->u.metadata.offset);

	fbe_spinlock_lock(&metadata_element->metadata_element_lock);
    for(i = 0; i < metadata_operation->u.metadata.repeat_count; i++){

        /* Make the data changes */
        if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_BITS) {
            for(j = 0; j < metadata_operation->u.metadata.record_data_size; j++) {
                data_ptr[j] |= metadata_operation->u.metadata.record_data[j];
            }
        } else {
            for(j = 0; j < metadata_operation->u.metadata.record_data_size; j++) {
                data_ptr[j] &= ~metadata_operation->u.metadata.record_data[j];
            }
        }
        data_ptr += metadata_operation->u.metadata.record_data_size;
    }
	fbe_spinlock_unlock(&metadata_element->metadata_element_lock);

	if(!fbe_metadata_is_peer_object_alive(metadata_element)){
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet_async(packet);
        return FBE_STATUS_OK;
	}

    /* We may want to send cmi message */
    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1, /* one chunk */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
                                  (fbe_memory_completion_function_t) fbe_metadata_nonpaged_operation_change_bits_allocation_completion,
                                   NULL /* completion_context */);


	fbe_transport_memory_request_set_io_master(memory_request, packet);

    status = fbe_memory_request_entry(memory_request);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_change_bits_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_memory_header_t * master_memory_header = NULL;

    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_metadata_cmi_message_t * metadata_cmi_message;  

    packet = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    master_memory_header = memory_request->ptr;   
    metadata_cmi_message = (fbe_metadata_cmi_message_t *)master_memory_header->data;

    metadata_cmi_message->header.object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    metadata_cmi_message->header.metadata_element_receiver = metadata_element->peer_metadata_element_ptr; /* If not NULL should point to the memory of the receiver */
    metadata_cmi_message->header.metadata_element_sender = (fbe_u64_t)metadata_element;

    if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_BITS){
        metadata_cmi_message->header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_SET_BITS;
    } else {
        metadata_cmi_message->header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_CLEAR_BITS;
    }
    /*metadata.record_data_size cannot be larger than FBE_PAYLOAD_METADATA_MAX_DATA_SIZE*/
    if(metadata_operation->u.metadata.record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Invalid memory size %d > %d ", __FUNCTION__ , 
                metadata_operation->u.metadata.record_data_size, 
                FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);
        
        /* Release memory */
        //fbe_memory_request_get_entry_mark_free(memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);      
		fbe_memory_free_request_entry(memory_request);

        /* Complete packet */
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_OK;
    }

	fbe_spinlock_lock(&metadata_element->metadata_element_lock);

    fbe_copy_memory(metadata_cmi_message->msg.nonpaged_change.data,
                    metadata_operation->u.metadata.record_data, 
                    metadata_operation->u.metadata.record_data_size);

    fbe_spinlock_unlock(&metadata_element->metadata_element_lock);

    metadata_cmi_message->msg.nonpaged_change.data_size = metadata_operation->u.metadata.record_data_size;
    metadata_cmi_message->msg.nonpaged_change.offset = metadata_operation->u.metadata.offset;
    metadata_cmi_message->msg.nonpaged_change.repeat_count = metadata_operation->u.metadata.repeat_count;
    metadata_cmi_message->msg.nonpaged_change.second_offset = metadata_operation->u.metadata.second_offset;
    metadata_cmi_message->header.metadata_operation_flags = metadata_operation->u.metadata.operation_flags;

    /* Include the sequence number for system objects */
    if (fbe_private_space_layout_object_id_is_system_object(metadata_cmi_message->header.object_id)) {
        metadata_cmi_message->header.metadata_operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_SEQ_NUM;
        metadata_cmi_message->msg.nonpaged_change.seq_number = fbe_metadata_raw_mirror_io_cb.seq_numbers[metadata_cmi_message->header.object_id];
    }

    /* Set completion function */
    fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_operation_change_bits_completion, metadata_cmi_message);

	if(fbe_metadata_element_is_peer_dead(metadata_element) || (fbe_metadata_element_is_cmi_disabled(metadata_element))) {
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}


    fbe_metadata_cmi_send_message(metadata_cmi_message, packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_change_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_status_t status;  
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_memory_request_t * memory_request = NULL;
    fbe_metadata_cmi_message_t * metadata_cmi_message = NULL;

    metadata_cmi_message = (fbe_metadata_cmi_message_t *)context;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    memory_request = fbe_transport_get_memory_request(packet);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    //fbe_memory_request_get_entry_mark_free(memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(memory_request);

    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_metadata_nonpaged_cmi_dispatch()
 ****************************************************************
 * @brief
 *  dispath the non-paged metadata update from peer with version check
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_status_t 
 * @author
 *  4/20/2012 - Modified. Jingcheng Zhang  for non-paged metadata versioning
 ****************************************************************/
fbe_status_t 
fbe_metadata_nonpaged_cmi_dispatch(fbe_metadata_element_t * metadata_element, fbe_metadata_cmi_message_t * metadata_cmi_msg)
{
    fbe_u8_t * data_ptr = NULL; 
    fbe_u8_t * dest_ptr = NULL;
    fbe_u8_t * src_ptr = NULL;
    fbe_u32_t data_size = 0;
    fbe_u64_t dest_size = 0;
    fbe_u32_t i;
    fbe_u32_t j;
    fbe_lba_t current_checkpoint;
    fbe_lba_t checkpoint;
    fbe_u64_t bits_update_offset = 0;

    dest_size = metadata_element->nonpaged_record.data_size;

    /* SEP upgrade may receive different version non-paged metadata update. so the update
       here should take care of the size of different version non-paged metadata in the 
       following cases
    */
    fbe_spinlock_lock(&metadata_element->metadata_element_lock);
    switch(metadata_cmi_msg->header.metadata_cmi_message_type){
        case FBE_METADATA_CMI_MESSAGE_TYPE_NONPAGED_WRITE:
            dest_ptr = metadata_element->nonpaged_record.data_ptr;
            if(metadata_cmi_msg->header.metadata_operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_PERSIST) {
                fbe_metadata_element_set_peer_persist_pending(metadata_element);
            }
            if(dest_ptr == NULL){
                /* Sometime the object (such as VD) is still in specialized state when the cmi message comes.
                   The nonpaged_record.data_ptr is not intialized and is NULL.  This causes access violation.
                   Ignore the cmi message for now */
                metadata_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s nonpaged_record.data_ptr is NULL!!!\n HACK!!!Return good status to peer.\n", __FUNCTION__);
                break;
            }

            /* version check case: update offset >= dest_size, ignore the update */
            if(metadata_cmi_msg->msg.nonpaged_write.offset >= dest_size) {
                metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "Metadata service recv new version non-paged update with offset %d > my non-paged size %d, ignore it .\n",
                       (int)metadata_cmi_msg->msg.nonpaged_write.offset, (int)dest_size);
                break;
            }
            /* version check case: update offset < dest_size, 
               but (update offset + update_size) > dest_size, apply the update partially*/
            if(metadata_cmi_msg->msg.nonpaged_write.offset + metadata_cmi_msg->msg.nonpaged_write.data_size > dest_size) {
                data_size = (fbe_u32_t)(dest_size - metadata_cmi_msg->msg.nonpaged_write.offset);
                metadata_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Metadata service recv new version non-paged update, off %d, size %d, my size %d, only apply size %d data.\n",
                               (int)metadata_cmi_msg->msg.nonpaged_write.offset, metadata_cmi_msg->msg.nonpaged_write.data_size, (int)dest_size, data_size);
            } else{
                /*no problem caused by size difference of peer, apply the whole update */
                data_size = metadata_cmi_msg->msg.nonpaged_write.data_size;
            }
                            
            dest_ptr = (fbe_u8_t *)metadata_element->nonpaged_record.data_ptr + metadata_cmi_msg->msg.nonpaged_write.offset;
            src_ptr = metadata_cmi_msg->msg.nonpaged_write.data;

            fbe_copy_memory(dest_ptr,
                            src_ptr,
                            data_size);

            /* Copy the sequence number from the active SP into memory for system objects */
            fbe_metadata_nonpaged_save_sequence_number_for_system_object(&metadata_cmi_msg->header, 
                                                                         metadata_cmi_msg->msg.nonpaged_write.seq_number); 
            if (metadata_element->cache_callback) {
                /* Inform metadata cache for the checkpoint changes */
                (metadata_element->cache_callback)(FBE_METADATA_PAGED_CACHE_ACTION_NONPAGE_CHANGED, metadata_element, NULL, 0, 0);
            }

            break;

        case FBE_METADATA_CMI_MESSAGE_TYPE_SET_BITS:
        case FBE_METADATA_CMI_MESSAGE_TYPE_CLEAR_BITS:

            data_ptr = (fbe_u8_t *)(metadata_element->nonpaged_record.data_ptr + metadata_cmi_msg->msg.nonpaged_change.offset);
            bits_update_offset = metadata_cmi_msg->msg.nonpaged_change.offset;
            for(i = 0; i < metadata_cmi_msg->msg.nonpaged_change.repeat_count; i++){
                /*version check: bits update offset shouldn't beyound my data structure size*/
                if(bits_update_offset >= dest_size) {
                    break;
                }
                /* Make the data changes */
                if(metadata_cmi_msg->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_SET_BITS) {
                    for(j = 0; j < metadata_cmi_msg->msg.nonpaged_change.data_size; j++) {
                        if(bits_update_offset >= dest_size) {
                            metadata_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Metadata service recv new version non-paged update set bits, offset beyound my structure size.\n");
                            break;
                        }
                        data_ptr[j] |= metadata_cmi_msg->msg.nonpaged_change.data[j];
                        bits_update_offset += 1;
                    }
                } else {
                    for(j = 0; j < metadata_cmi_msg->msg.nonpaged_change.data_size; j++) {
                        if(bits_update_offset >= dest_size) {
                            metadata_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Metadata service recv new version non-paged update set bits, offset beyound my structure size.\n");
                            break;
                        }
                        data_ptr[j] &= ~metadata_cmi_msg->msg.nonpaged_change.data[j];
                        bits_update_offset += 1;
                    }
                }
                data_ptr += metadata_cmi_msg->msg.nonpaged_change.data_size;
            }

            /* Copy the sequence number from the active SP into memory for system objects */
            fbe_metadata_nonpaged_save_sequence_number_for_system_object(&metadata_cmi_msg->header, 
                                                                         metadata_cmi_msg->msg.nonpaged_change.seq_number); 

            break;
        case FBE_METADATA_CMI_MESSAGE_TYPE_FORCE_SET_CHECKPOINT:
            /*version check: checkpoint shouldn't beyound my data structure size*/
            if(metadata_cmi_msg->msg.nonpaged_change.offset >= dest_size ||
               metadata_cmi_msg->msg.nonpaged_change.offset + sizeof (fbe_lba_t) > dest_size) {
                metadata_trace(FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Metadata service recv new version checkpoint set off %lld, size %d, beyound my structure size %lld.\n",
                               metadata_cmi_msg->msg.nonpaged_change.offset,
                               (int)sizeof (fbe_lba_t), dest_size);
                break;
            }
            data_ptr = (fbe_u8_t *)(metadata_element->nonpaged_record.data_ptr + metadata_cmi_msg->msg.nonpaged_change.offset);
            current_checkpoint = *(fbe_lba_t *)data_ptr;
            checkpoint = *(fbe_lba_t *)metadata_cmi_msg->msg.nonpaged_change.data;
            *(fbe_lba_t *)data_ptr = checkpoint;

            if(metadata_cmi_msg->msg.nonpaged_change.second_offset !=0)
            {
                fbe_u8_t * second_data_ptr = NULL;
                second_data_ptr = (fbe_u8_t *)(metadata_element->nonpaged_record.data_ptr + metadata_cmi_msg->msg.nonpaged_change.second_offset);                
                checkpoint = *(fbe_lba_t *)metadata_cmi_msg->msg.nonpaged_change.data;
                *(fbe_lba_t *)second_data_ptr = checkpoint;
            }

            /* Copy the sequence number from the active SP into memory for system objects */
            fbe_metadata_nonpaged_save_sequence_number_for_system_object(&metadata_cmi_msg->header, 
                                                                         metadata_cmi_msg->msg.nonpaged_change.seq_number); 

            break;

        case FBE_METADATA_CMI_MESSAGE_TYPE_SET_CHECKPOINT:
            /*version check: checkpoint shouldn't beyound my data structure size*/
            if(metadata_cmi_msg->msg.nonpaged_change.offset >= dest_size ||
               metadata_cmi_msg->msg.nonpaged_change.offset + sizeof (fbe_lba_t) > dest_size) {
                metadata_trace(FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Metadata service recv new version checkpoint set off %lld, size %d, beyound my structure size %lld.\n",
                               metadata_cmi_msg->msg.nonpaged_change.offset,
                               (int)sizeof (fbe_lba_t), dest_size);
                break;
            }
            data_ptr = (fbe_u8_t *)(metadata_element->nonpaged_record.data_ptr + metadata_cmi_msg->msg.nonpaged_change.offset);
            current_checkpoint = *(fbe_lba_t *)data_ptr;
            checkpoint = *(fbe_lba_t *)metadata_cmi_msg->msg.nonpaged_change.data;
            if(checkpoint < current_checkpoint){
                *(fbe_lba_t *)data_ptr = checkpoint;
            }

            /* Copy the sequence number from the active SP into memory for system objects */
            fbe_metadata_nonpaged_save_sequence_number_for_system_object(&metadata_cmi_msg->header, 
                                                                         metadata_cmi_msg->msg.nonpaged_change.seq_number); 

            break;

        case FBE_METADATA_CMI_MESSAGE_TYPE_INCR_CHECKPOINT:
            /*version check: checkpoint shouldn't beyound my data structure size*/
            if(metadata_cmi_msg->msg.nonpaged_change.offset >= dest_size ||
               metadata_cmi_msg->msg.nonpaged_change.offset + sizeof (fbe_lba_t) > dest_size) {
                metadata_trace(FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Metadata service recv new version checkpoint incr off %lld, size %d, beyound my structure size %lld.\n",
                               metadata_cmi_msg->msg.nonpaged_change.offset,
                               (int)sizeof (fbe_lba_t), dest_size);
                break;
            }
            data_ptr = (fbe_u8_t *)(metadata_element->nonpaged_record.data_ptr + metadata_cmi_msg->msg.nonpaged_change.offset);
            current_checkpoint = *(fbe_lba_t *)data_ptr;
            checkpoint = *(fbe_lba_t *)metadata_cmi_msg->msg.nonpaged_change.data;
            if(checkpoint == current_checkpoint){
                *(fbe_lba_t *)data_ptr = checkpoint + metadata_cmi_msg->msg.nonpaged_change.repeat_count;
                if(metadata_cmi_msg->msg.nonpaged_change.second_offset !=0)
                {
                    fbe_u8_t * second_data_ptr = NULL;
                    second_data_ptr = (fbe_u8_t *)(metadata_element->nonpaged_record.data_ptr + metadata_cmi_msg->msg.nonpaged_change.second_offset);                
                    current_checkpoint = *(fbe_lba_t *)second_data_ptr;
                    *(fbe_lba_t *)second_data_ptr = current_checkpoint + metadata_cmi_msg->msg.nonpaged_change.repeat_count;
                }
			}

            /* Copy the sequence number from the active SP into memory for system objects */
            fbe_metadata_nonpaged_save_sequence_number_for_system_object(&metadata_cmi_msg->header, 
                                                                         metadata_cmi_msg->msg.nonpaged_change.seq_number); 

			break;
        case FBE_METADATA_CMI_MESSAGE_TYPE_NONPAGED_POST_PERSIST:
            fbe_metadata_element_clear_peer_persist_pending(metadata_element);
            break;
        default:
            metadata_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Uknown message type %d \n", __FUNCTION__ , metadata_cmi_msg->header.metadata_cmi_message_type);
            break;

    }
    fbe_spinlock_unlock(&metadata_element->metadata_element_lock);
    return FBE_STATUS_OK;
}
fbe_status_t fbe_metadata_nonpaged_system_clear(fbe_packet_t * packet)
{
    fbe_memory_request_t           *memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_status_t                    status;
    fbe_lba_t offset;
    fbe_block_count_t capacity;
    fbe_u32_t blocks_per_buffer = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO / FBE_BE_BYTES_PER_BLOCK;
    fbe_u32_t total_buffers;
    fbe_database_get_non_paged_metadata_offset_capacity(&offset, NULL, &capacity);

    if ((capacity / blocks_per_buffer) > FBE_U32_MAX)
    {
        metadata_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: (capacity 0x%llx / blocks_per_buffer 0x%x) > FBE_U32_MAX\n", 
                       __FUNCTION__, (unsigned long long)capacity, blocks_per_buffer);
    }
    total_buffers = (fbe_u32_t)(capacity / blocks_per_buffer);
    if (capacity % blocks_per_buffer)
    {
        total_buffers++;
    }

    metadata_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
				   "%s: Entry\n", __FUNCTION__);

    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* Allocate memory for data */      
    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO, /* packet, sg list and 13 blocks */
                                   1 + total_buffers, /* one for sg list and the specified number for buffers. */
                                   memory_request_priority, 
                                   fbe_transport_get_io_stamp(packet),      
                                   (fbe_memory_completion_function_t)fbe_metadata_nonpaged_system_clear_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   NULL /* completion_context */);

	fbe_transport_memory_request_set_io_master(memory_request,packet);

    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;
}
/*!***************************************************************
 * @fn fbe_metadata_nonpaged_system_load
 *****************************************************************
 * @brief
 *   load system objects' NP into memory.
 *
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    5/14/2012: Yang Zhang - Modified: allocate memory 
 *                                                 accoiding to how much blocks we need
 *
 ****************************************************************/

fbe_status_t fbe_metadata_nonpaged_system_load(fbe_packet_t * packet)
{
    fbe_memory_request_t           *memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_block_count_t               block_count;
    fbe_memory_chunk_size_t         chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    fbe_memory_number_of_objects_t  chunk_number = 1;
    fbe_status_t                    status;
    //fbe_u32_t       aligned_rw_io_cb_size = 0;
    fbe_object_id_t last_object_id;
    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;
    /*caculate how much chunks we need
	    we caculate the total count for system objects
	    by last system object id adding one*/
	
	/*fbe_database_get_last_system_object_id(&last_object_id);*/
    last_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST;
	/* round up */
    block_count = (fbe_block_count_t)(last_object_id + 1); 

#if 0
    /*add extra blocks for raw mirror IO control block and raw mirror system read context
          , assuming the size smaller than 3 blocks*/
    aligned_rw_io_cb_size = (sizeof (raw_mirror_operation_cb_t) + sizeof (raw_mirror_system_load_context_t)
                             + FBE_METADATA_BLOCK_SIZE - 1)/FBE_METADATA_BLOCK_SIZE;
    block_count += aligned_rw_io_cb_size;

    if (block_count < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET) {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    } else {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }
    chunk_number = (fbe_memory_number_of_objects_t)((block_count + chunk_size_to_block_count(chunk_size) - 1) / chunk_size_to_block_count(chunk_size));
    /*TODO: we need to caculate how much chunk we need by considering
	   the packet , sg_list and other elements all together in order to save memory. 
	   For now, just add one chunk for packet and sg_list*/
    chunk_number ++;  
#endif
    fbe_metadata_nonpaged_get_chunk_number((fbe_u32_t)block_count, &chunk_size, &chunk_number);

    /* Allocate memory for data */      
    fbe_memory_build_chunk_request(memory_request, 
                                   chunk_size, 							
                                   chunk_number,
                                   memory_request_priority, 
                                   fbe_transport_get_io_stamp(packet),      
                                   (fbe_memory_completion_function_t)fbe_metadata_nonpaged_system_load_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   NULL /* completion_context */);


	fbe_transport_memory_request_set_io_master(memory_request,packet);

    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_metadata_nonpaged_system_zero_and_persist
 *****************************************************************
 * @brief
 *   Zero and persist system objects' NP into disk.
 *   If specify the object id, just persit this object's NP; 
 *   else persist all the system object NP into disk.
 *
 *   NOTE: 1. this function will persist np metadata directly to disk 
 *              by raw mirror interface.
 *             2. This function is only used on active SP during the booting time before
 *                 loading system non-paged metadata.
 *             
 * 
 * @return status - return FBE_STATUS_GENERIC_FAILURE if issue encountered
 *
 * @version
 *    8/21/2012: gaoh1 - Modified: Implement this function 
 *
 ****************************************************************/
fbe_status_t fbe_metadata_nonpaged_system_zero_and_persist(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_nonpaged_metadata_persist_system_object_context_t *persist_context = NULL;
    fbe_object_id_t                     object_id;
    fbe_block_count_t                   block_count;
    fbe_object_id_t                     last_system_object_id;
    fbe_memory_request_t                *memory_request = NULL;
    fbe_memory_request_priority_t       memory_request_priority = 0;    
    fbe_memory_chunk_size_t             chunk_size;
    fbe_memory_number_of_objects_t      chunk_number;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    metadata_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s : entry\n", __FUNCTION__);

    fbe_database_get_last_system_object_id(&last_system_object_id);
    fbe_payload_control_get_buffer(control_operation, &persist_context);
    if (persist_context == NULL)
    {
        object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
        metadata_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s Persist all the system objects' NP metadata\n", __FUNCTION__);
        block_count = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST + 1;
    } else {
        object_id = persist_context->start_object_id;
        block_count = persist_context->block_count;
        if ((object_id + block_count) > last_system_object_id) {
            metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: The last object id exceeds the system object range. obj_id = %d, count=%d\n", 
                       __FUNCTION__, object_id, (int)block_count);
            block_count = last_system_object_id + 1 - object_id;
        }
        metadata_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Persist system object (0x%x)'s NP metadata\n", 
                       __FUNCTION__, object_id);
    }

    if ((FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_FIRST >= object_id && 
         FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_FIRST < (object_id + block_count - 1)) ||
        (FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_LAST >= object_id &&
         FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_LAST < (object_id + block_count - 1)))
    {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s request invalid, cannot zero persist c4mirror and non-c4mirror in one operation\n", __FUNCTION__); 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;


    /*use the maximum chunk size*/
#if 0
    chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    chunk_number = (fbe_memory_number_of_objects_t)((block_count + chunk_size_to_block_count(chunk_size) - 1) / chunk_size_to_block_count(chunk_size));
    chunk_number ++;  /* +1 for packet and sg_list */
#endif
    fbe_metadata_nonpaged_get_chunk_number((fbe_u32_t)block_count, &chunk_size, &chunk_number);

    /* Allocate memory for data */      
    fbe_memory_build_chunk_request(memory_request, 
                                   chunk_size,
                                   chunk_number,
                                   memory_request_priority, 
                                   fbe_transport_get_io_stamp(packet),
                                   (fbe_memory_completion_function_t)fbe_metadata_nonpaged_system_zero_and_persist_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   persist_context /* completion_context */);
    fbe_transport_memory_request_set_io_master(memory_request,packet);

    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_zero_and_persist_completion
 *****************************************************************
 * @brief
 *     The complete routine of IO operation.
 *
 * @return status 
 *
 * @version
 *    8/21/2012: gaoh1 - created 
 *
 ****************************************************************/
static fbe_status_t
fbe_metadata_nonpaged_zero_and_persist_completion(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t *master_packet = NULL;
    fbe_payload_ex_t *sep_payload = NULL;
    fbe_payload_block_operation_t   *block_operation = NULL;
    fbe_payload_block_operation_status_t block_operation_status;
    fbe_payload_block_operation_qualifier_t block_operation_qualifier;
    fbe_memory_request_t    *master_memory_request = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_status_t    status;
    

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    /* Check the block opearation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);

    /* We need to  remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    /* Release block operation */
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);
    fbe_transport_destroy_packet(packet);

    fbe_memory_free_request_entry(master_memory_request);

    /* Set control operation status */
    sep_payload = fbe_transport_get_payload_ex(master_packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    /* We have to check if I/O was succesful and notify the caller via master packet */
    if (status == FBE_STATUS_OK && block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    } else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_OK;    
}
    
/*!***************************************************************
* @fn fbe_metadata_nonpaged_system_zero_and_persist_allocation_completion
*****************************************************************
* @brief
*     This function is the complete routine of memory request of system object
*     nonpaged metadata persist operation. It will copy the global nonpaged 
*     metadata by object id to memory and send IO to the np metadata raw mirror
*
* @return status 
*
* @version
*    8/21/2012: gaoh1 - created 
*
****************************************************************/
static fbe_status_t
fbe_metadata_nonpaged_system_zero_and_persist_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_u32_t       sg_list_count = 0;
    fbe_status_t status;
    fbe_memory_header_t * master_memory_header = NULL;
    //fbe_memory_header_t * memory_header = NULL;
    fbe_u8_t * buffer;
    fbe_u32_t       i = 0, j = 0;
    fbe_object_id_t start_object_id;
    fbe_object_id_t last_object_id;
    fbe_block_count_t   block_count;
    fbe_u32_t       index;
    fbe_object_id_t metadata_lun_id = FBE_OBJECT_ID_INVALID;
    fbe_lba_t lba;
    fbe_u32_t blocks_per_object;

    packet = fbe_transport_memory_request_to_packet(memory_request);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*If the context is NULL, it means all the np metadata of system objects will be persisted.
    * or only the system object from start_object_id will be persisted.
    */
    if (context != NULL) 
    {
        start_object_id = ((fbe_nonpaged_metadata_persist_system_object_context_t *)context)->start_object_id;
        block_count = ((fbe_nonpaged_metadata_persist_system_object_context_t *)context)->block_count;
    } else {
        /*All the system objects's np metatadata will be persisted in default.*/
        start_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
        /*fbe_database_get_last_system_object_id(&last_object_id);*/
        last_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST; 
        /* round up */
        block_count = (fbe_block_count_t)(last_object_id + 1);
    }


    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);

    new_packet = (fbe_packet_t *)buffer;
    //sg_list = (fbe_sg_element_t *)(buffer + sizeof(fbe_packet_t));  /* Hope packet will never exceed 4K */
    
    fbe_transport_initialize_packet(new_packet);
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation = fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_transport_propagate_expiration_time(new_packet, packet);

    /* The first chunk is for packet and sg list; while other chunks are for data buffer */
#if 0
    sg_list_count = master_memory_header->number_of_chunks - 1;
    memory_header = master_memory_header->u.hptr.next_header;
    for (i = 0; i < sg_list_count; i++)
    {
        sg_list[i].address = (fbe_u8_t *)memory_header->data;
        sg_list[i].count = 0;
        fbe_zero_memory(sg_list[i].address, FBE_METADATA_BLOCK_SIZE * 64);
        memory_header = memory_header->u.hptr.next_header;
        
    }
    sg_list[i].address = NULL;
    sg_list[i].count = 0;
#endif
    fbe_metadata_nonpaged_setup_sg_list_from_memory_request(memory_request, &sg_list, &sg_list_count);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, sg_list_count);

    fbe_metadata_nonpaged_get_lba_block_count(start_object_id, &lba, &blocks_per_object);

    /* Copy the data to 64-blocks chunk and calculate the checksum */
    index = 0;
    for (i = 0; i < sg_list_count; i++)
    {
        buffer = sg_list[i].address;
        for (j = 0; j < (64 / blocks_per_object); j++)
        {
            /*Zero the memory copy of nonpaged metadata */
            fbe_zero_memory(fbe_metadata_nonpaged_array[start_object_id + index].data, FBE_METADATA_NONPAGED_MAX_SIZE);
            fbe_copy_memory(buffer, fbe_metadata_nonpaged_array[start_object_id + index].data, FBE_METADATA_NONPAGED_MAX_SIZE);
            fbe_metadata_nonpaged_calculate_checksum_for_one_object(buffer, start_object_id + index, blocks_per_object, FBE_TRUE);

            index++;
            buffer += FBE_METADATA_BLOCK_SIZE * blocks_per_object;
            sg_list[i].count += FBE_METADATA_BLOCK_SIZE * blocks_per_object;
        
            if (index >= block_count) 
            {
                break;
            }
        }
        
    }
    
    fbe_payload_block_build_operation(block_operation,
                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,
                        lba,
                        block_count * blocks_per_object,
                        FBE_METADATA_BLOCK_SIZE,
                        1,
                        NULL);
    /*set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_zero_and_persist_completion, NULL);
    fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);

    fbe_transport_save_resource_priority(new_packet);

    status = fbe_transport_add_subpacket(packet, new_packet);

   
    if (fbe_database_is_object_c4_mirror(start_object_id))
    {
        fbe_raw_mirror_t *nonpaged_raw_mirror_p = NULL;
        /* get the c4mirror nonpaged metadata config - homewrecker_db */
        fbe_c4_mirror_get_nonpaged_metadata_config((void **)(&nonpaged_raw_mirror_p), NULL, NULL);
        fbe_payload_ex_increment_block_operation_level(sep_payload);
        status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(nonpaged_raw_mirror_p, new_packet,FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);
    }
    else if (fbe_raw_mirror_is_enabled(&fbe_metadata_nonpaged_raw_mirror))
    {
        fbe_payload_ex_increment_block_operation_level(sep_payload);
        status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_metadata_nonpaged_raw_mirror, new_packet, FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);
    } else {
        fbe_database_get_raw_mirror_metadata_lun_id(&metadata_lun_id);
        fbe_transport_set_address(new_packet,
                    FBE_PACKAGE_ID_SEP_0,
                    FBE_SERVICE_ID_TOPOLOGY,
                    FBE_CLASS_ID_INVALID,
                    metadata_lun_id);
        new_packet->base_edge = NULL;
        fbe_payload_ex_increment_block_operation_level(sep_payload);
        status = fbe_topology_send_io_packet(new_packet);
    }

    return status;

}

fbe_status_t fbe_metadata_nonpaged_system_persist(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    metadata_trace(FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s Not implemented yet \n", __FUNCTION__);


#if 0
    /*! @todo We need to allocate memory, and then 
     * allocate and build the block operation and sg lists. 
     *  
     * Then we will be able to call the raid library. 
     */
    fbe_payload_ex_increment_block_operation_level(sep_payload);
    status = fbe_raw_mirror_send_io_packet_to_raid_library(&fbe_metadata_nonpaged_raw_mirror, new_packet);
#endif

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_metadata_nonpaged_load(fbe_packet_t * packet)
{
    fbe_memory_request_t           *memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_status_t                    status;

    /* We may want to send cmi message */
    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* Allocate memory for data */      
    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER + 1, /* + 1 for packet and sg_list */
                                   memory_request_priority, 
                                   fbe_transport_get_io_stamp(packet),      
                                   (fbe_memory_completion_function_t)fbe_metadata_nonpaged_load_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   NULL /* completion_context */);


	fbe_transport_memory_request_set_io_master(memory_request, packet);

    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_metadata_nonpaged_persist(fbe_packet_t * packet)
{
    fbe_memory_request_t * memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_status_t status;

    /* We may want to send cmi message */
    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* Allocate memory for data */      
    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER + 1, /* + 1 for packet and sg_list */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),       
                                   (fbe_memory_completion_function_t)fbe_metadata_nonpaged_persist_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   NULL /* completion_context */);

		fbe_transport_memory_request_set_io_master(memory_request, packet);

    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;
}

            
static fbe_status_t
fbe_metadata_nonpaged_persist_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_status_t status;
    fbe_memory_header_t * master_memory_header = NULL;
    //fbe_memory_header_t * memory_header = NULL;
    fbe_u32_t i;
    fbe_u32_t j;
    fbe_u32_t index;
    fbe_object_id_t object_id;
    fbe_u8_t * buffer;          
    //fbe_object_id_t start_object_id;
    //fbe_u32_t  object_count;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    packet = fbe_transport_memory_request_to_packet(memory_request);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    master_memory_header = memory_request->ptr;

    /* Form SG lists */
    new_packet = (fbe_packet_t *)master_memory_header->data;
    //sg_list = (fbe_sg_element_t *)((fbe_u8_t *)master_memory_header->data + sizeof(fbe_packet_t)); /* I hope packet will never exceed 4K */

    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_transport_propagate_expiration_time(new_packet, packet);

#if 0
    memory_header = master_memory_header->u.hptr.next_header;
    for(i = 0; i < METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER; i++){
        sg_list[i].address = (fbe_u8_t *)memory_header->data;
        sg_list[i].count = FBE_METADATA_BLOCK_SIZE * 64;
        fbe_zero_memory(sg_list[i].address, sg_list[i].count);
        memory_header = memory_header->u.hptr.next_header;
    }

    sg_list[i].address = NULL;
    sg_list[i].count = 0;
#endif
    fbe_metadata_nonpaged_setup_sg_list_from_memory_request(memory_request, &sg_list, NULL);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, 1);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,
                                      0,    /* LBA */
                                      64 * METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */


    /* Copy the data */
    index = (fbe_u32_t)block_operation->lba / blocks_per_object;
    for(i = 0; i < METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER; i++){
        buffer = sg_list[i].address;
        for(j = 0; (j < (64 / blocks_per_object)) && ( index < FBE_METADATA_NONPAGED_MAX_OBJECTS); j++){
            fbe_copy_memory(buffer, fbe_metadata_nonpaged_array[index].data, FBE_METADATA_NONPAGED_MAX_SIZE);
            index++;
            buffer += (FBE_METADATA_BLOCK_SIZE * blocks_per_object);
        }
        fbe_metadata_calculate_checksum(sg_list[i].address, 64);
    }

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_persist_write_completion, NULL);
    fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);

    /* Save resource priority so that we know to which priority to unwind when packet completes Write   
     * and we need to send packet back down again for another Write operation. 
     */
	fbe_transport_save_resource_priority(new_packet);

    status = fbe_transport_add_subpacket(packet, new_packet);

    fbe_database_get_metadata_lun_id(&object_id);

    fbe_transport_set_address(  new_packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    packet->base_edge = NULL;
    fbe_payload_ex_increment_block_operation_level(sep_payload);
    status = fbe_topology_send_io_packet(new_packet);    
    return status;
}

static fbe_status_t 
fbe_metadata_nonpaged_persist_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_memory_request_t * master_memory_request = NULL;    
    fbe_u8_t * buffer = NULL;
    fbe_u32_t i,j;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_u32_t index;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    
    metadata_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s LBA %llX \n", __FUNCTION__,
		    (unsigned long long)block_operation->lba);

    /* We have to check if I/O was succesfull */

    /* Get sg list */
    fbe_payload_ex_get_sg_list(sep_payload, &sg_list, NULL);


    /* Check if we wrote the last block */
    if(((block_operation->lba + block_operation->block_count)/ blocks_per_object) >= FBE_METADATA_NONPAGED_MAX_OBJECTS){
        /* We are done ... */

        /* We need to remove packet from master queue */
        fbe_transport_remove_subpacket(packet);

        /* Release block operation */
        fbe_payload_ex_release_block_operation(sep_payload, block_operation);

        fbe_transport_destroy_packet(packet); /* This will not release the memory */

        /* Release the memory */
        //fbe_memory_request_get_entry_mark_free(master_memory_request, (fbe_memory_ptr_t *)&memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(master_memory_request);

        
        /* Set control operation status */
        sep_payload = fbe_transport_get_payload_ex(master_packet);
        control_operation = fbe_payload_ex_get_control_operation(sep_payload);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_OK;
    }

    block_operation->lba += block_operation->block_count;
    /* We are not done yet */
    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,
                                      block_operation->lba,    /* LBA */
                                      64 * METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Copy the data */
    index = (fbe_u32_t)block_operation->lba / blocks_per_object;
    for(i = 0; i < METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER; i++){
        buffer = sg_list[i].address;
        for(j = 0; (j < (64 / blocks_per_object)) && ( index < FBE_METADATA_NONPAGED_MAX_OBJECTS); j++){
            fbe_copy_memory(buffer, fbe_metadata_nonpaged_array[index].data, FBE_METADATA_NONPAGED_MAX_SIZE);
            index++;
            buffer += (FBE_METADATA_BLOCK_SIZE * blocks_per_object);
        }
        fbe_metadata_calculate_checksum(sg_list[i].address, 64);
    }

    /* Set completion function */
    fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_persist_write_completion, NULL);

    /* Since we are reusing packet for another block operation, unwind the resource priority.  
     */
    fbe_transport_restore_resource_priority(packet);

    /* Save resource priority so that we know to which priority to unwind when packet completes Write   
     * and we need to send packet back down again for another Write operation. Call to Restore func
     * above would have cleared the saved resource priority. 
     */
    fbe_transport_save_resource_priority(packet);

    fbe_database_get_metadata_lun_id(&object_id);

    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    packet->base_edge = NULL;
    status = fbe_topology_send_io_packet(packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


static fbe_status_t
fbe_metadata_nonpaged_load_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_status_t status;
    fbe_memory_header_t * master_memory_header = NULL;
    //fbe_memory_header_t * memory_header = NULL;
    //fbe_u32_t i;    
    fbe_object_id_t object_id;    

    packet = fbe_transport_memory_request_to_packet(memory_request);
    master_memory_header = memory_request->ptr;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Form SG lists */
    new_packet = (fbe_packet_t *)master_memory_header->data;
    //sg_list = (fbe_sg_element_t *)((fbe_u8_t *)master_memory_header->data + sizeof(fbe_packet_t)); 

    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_transport_propagate_expiration_time(new_packet, packet);

#if 0
    memory_header = master_memory_header->u.hptr.next_header;
    for(i = 0; i < METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER; i++){
        sg_list[i].address = (fbe_u8_t *)memory_header->data;
        sg_list[i].count = FBE_METADATA_BLOCK_SIZE * 64;
        fbe_zero_memory(sg_list[i].address, sg_list[i].count);
        memory_header = memory_header->u.hptr.next_header;
    }

    sg_list[i].address = NULL;
    sg_list[i].count = 0;
#endif
    fbe_metadata_nonpaged_setup_sg_list_from_memory_request(memory_request, &sg_list, NULL);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, 1);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      0,    /* LBA */
                                      FBE_METADATA_NONPAGED_LOAD_READ_SIZE_BLOCKS,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_load_read_completion, NULL);
    fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);

    /* Save resource priority so that we know to which priority to unwind when packet completes Read   
     * and we send it back down again for Write operation. 
     */
    fbe_transport_save_resource_priority(new_packet);

    status = fbe_transport_add_subpacket(packet, new_packet);

    fbe_database_get_metadata_lun_id(&object_id);

    fbe_transport_set_address(  new_packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    packet->base_edge = NULL;
    fbe_payload_ex_increment_block_operation_level(sep_payload);
    status = fbe_topology_send_io_packet(new_packet);    
    return status;
}

static fbe_status_t 
fbe_metadata_nonpaged_load_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_memory_request_t * master_memory_request = NULL;    
    fbe_u8_t * buffer = NULL;
    fbe_u32_t i,j;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_u32_t index;
	fbe_object_id_t last_object_id;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    
    metadata_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s LBA %llX \n", __FUNCTION__,
		    (unsigned long long)block_operation->lba);

    /* We have to check if I/O was succesfull */

    /* Get sg list */
    fbe_payload_ex_get_sg_list(sep_payload, &sg_list, NULL);

	fbe_database_get_last_system_object_id(&last_object_id);

     /* Check if we got Error on the read */
    if((block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) || (status != FBE_STATUS_OK))
    {
        /* Perform Single block read to narrow down the region */
        fbe_metadata_nonpaged_load_single_block_read(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Copy the data */
    index = (fbe_u32_t)block_operation->lba/blocks_per_object;
    for(i = 0; i < METADATA_NONPAGED_MEMORY_CHUNKS_NUMBER; i++){
        buffer = sg_list[i].address;
        for(j = 0; (j < 64/blocks_per_object) && ( index < FBE_METADATA_NONPAGED_MAX_OBJECTS); j++){
			if(index > last_object_id){
				fbe_copy_memory(fbe_metadata_nonpaged_array[index].data, buffer, FBE_METADATA_NONPAGED_MAX_SIZE);
				metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "nonpaged_load data 0x%016llX 0x%016llX 0x%016llX\n",
		    (unsigned long long)*(fbe_u64_t *)buffer,
		    (unsigned long long)*(fbe_u64_t *)&buffer[8],
		    (unsigned long long)*(fbe_u64_t *)&buffer[16]);

			}
            index++;
            buffer += FBE_METADATA_BLOCK_SIZE*blocks_per_object;
        }
    }

    
    /* Check if we wrote the last block */
    if(((block_operation->lba + block_operation->block_count)/blocks_per_object) >= FBE_METADATA_NONPAGED_MAX_OBJECTS){
        /* We are done ... */

        /* We need to remove packet from master queue */
        fbe_transport_remove_subpacket(packet);

        /* Release block operation */
        fbe_payload_ex_release_block_operation(sep_payload, block_operation);

        fbe_transport_destroy_packet(packet); /* This will not release the memory */

        /* Release the memory */
        //fbe_memory_request_get_entry_mark_free(master_memory_request, (fbe_memory_ptr_t *)&memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(master_memory_request);
        
        
        /* Set control operation status */
        sep_payload = fbe_transport_get_payload_ex(master_packet);
        control_operation = fbe_payload_ex_get_control_operation(sep_payload);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_OK;
    }

    /* We are not done yet */
    block_operation->lba += block_operation->block_count;
    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      block_operation->lba,    /* LBA */
                                      FBE_METADATA_NONPAGED_LOAD_READ_SIZE_BLOCKS,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_load_read_completion, NULL);

    /* Since we are reusing packet for another block operation, unwind the resource priority.  
     */
    fbe_transport_restore_resource_priority(packet);

    /* Save resource priority so that we know to which priority to unwind when packet completes Write   
     * and we need to send packet back down again for another Write operation. Call to Restore func
     * above would have cleared the saved resource priority. 
     */
    fbe_transport_save_resource_priority(packet);

    fbe_database_get_metadata_lun_id(&object_id);

    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    packet->base_edge = NULL;
    fbe_transport_packet_clear_callstack_depth(packet);/* reset this since we reuse the packet */
    status = fbe_topology_send_io_packet(packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 *  fbe_metadata_nonpaged_load_single_block_read
 ******************************************************************************
 *
 * @brief
 *    This function kicks of a read of single block to the Metadata LUN
 *
 * @param - packet - Pointer to the original packet
 *
 * @return  fbe_status_t
 *
 * @author
 *    06/26/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static fbe_status_t 
fbe_metadata_nonpaged_load_single_block_read(fbe_packet_t * packet)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();
    
    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    metadata_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s LBA %llX \n", __FUNCTION__, (unsigned long long)block_operation->lba);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      block_operation->lba,    /* LBA */
                                      blocks_per_object,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_load_single_block_read_completion, NULL);

    /* Since we are reusing packet for another block operation, unwind the resource priority.  
     */
    fbe_transport_restore_resource_priority(packet);

    /* Save resource priority so that we know to which priority to unwind when packet completes Write   
     * and we need to send packet back down again for another Write operation. Call to Restore func
     * above would have cleared the saved resource priority. 
     */
    fbe_transport_save_resource_priority(packet);

    fbe_database_get_metadata_lun_id(&object_id);

    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    packet->base_edge = NULL;
    status = fbe_topology_send_io_packet(packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 *  fbe_metadata_nonpaged_load_single_block_read_completion
 ******************************************************************************
 *
 * @brief
 *    Completion function for the single block read of Non-paged MD. On error
 * it marks the state to be invalid and continue to read the other blocks as
 * appropriate
 *
 * @param - packet - Original Packet
 * @param - Context - Context from the caller
 *
 * @return  void
 *
 * 
 * @author
 *    06/26/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************/
static fbe_status_t 
fbe_metadata_nonpaged_load_single_block_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_memory_request_t * master_memory_request = NULL;    
    fbe_u8_t * buffer = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_u32_t index;
	fbe_object_id_t last_object_id;
    fbe_u32_t num_blocks;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    
    /* We have to check if I/O was succesfull */

    /* Get sg list */
    fbe_payload_ex_get_sg_list(sep_payload, &sg_list, NULL);

	fbe_database_get_last_system_object_id(&last_object_id);

    index = (fbe_u32_t)block_operation->lba / blocks_per_object;
    if(index > last_object_id)
    {
        /* Check if we got Error on the read */
        if((status == FBE_STATUS_OK) && (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) )
        {
            /* Copy the data */
            buffer = sg_list[0].address;
            fbe_copy_memory(fbe_metadata_nonpaged_array[index].data, buffer, FBE_METADATA_NONPAGED_MAX_SIZE);
			metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "nonpaged_load data 0x%016llX 0x%016llX 0x%016llX\n", 
                           *(fbe_u64_t *)buffer, *(fbe_u64_t *)&buffer[8], *(fbe_u64_t *)&buffer[16]);

        }
        else
        {
            /*if the packet status is not OK or the block operation status is not ok
		     we need to set the NP state of this object to INVALID
		     We operation the memory directly when loading nonpaged metadata*/
			 fbe_metadata_set_nonpaged_metadata_state_in_memory(FBE_NONPAGED_METADATA_INVALID,index);
			 metadata_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							"fbe_md_np_single_block_read single obj read failed: status=%d, block_status=%d objID:0x%x\n",
							status, block_operation_status,index);


	     }
    }
    
    /* Check if we wrote the last block */
    if(((block_operation->lba + block_operation->block_count) / blocks_per_object) >= FBE_METADATA_NONPAGED_MAX_OBJECTS){
        /* We are done ... */

        /* We need to remove packet from master queue */
        fbe_transport_remove_subpacket(packet);

        /* Release block operation */
        fbe_payload_ex_release_block_operation(sep_payload, block_operation);

        fbe_transport_destroy_packet(packet); /* This will not release the memory */

        /* Release the memory */
        //fbe_memory_request_get_entry_mark_free(master_memory_request, (fbe_memory_ptr_t *)&memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(master_memory_request);
        
        
        /* Set control operation status */
        sep_payload = fbe_transport_get_payload_ex(master_packet);
        control_operation = fbe_payload_ex_get_control_operation(sep_payload);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet);
        return FBE_STATUS_OK;
    }

    /* We are not done yet */
    block_operation->lba += blocks_per_object;

    /* Check if we have read the blocks the original request intended to do before we
     * came into the single block mode...*/
    if(block_operation->lba % FBE_METADATA_NONPAGED_LOAD_READ_SIZE_BLOCKS) 
    {
        /* Not yet and so kick off single block mode */
        num_blocks = blocks_per_object;
        /* Set completion function */
        fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_load_single_block_read_completion, NULL);
    }
    else
    {
        /* Yes, we are done and so we can now try to kick off a full set of read and let the 
         * original completion function take it from there */
        fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_load_read_completion, NULL);
        num_blocks = FBE_METADATA_NONPAGED_LOAD_READ_SIZE_BLOCKS;
    }
    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      block_operation->lba,    /* LBA */
                                      num_blocks,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    

    /* Since we are reusing packet for another block operation, unwind the resource priority.  
     */
    fbe_transport_restore_resource_priority(packet);

    /* Save resource priority so that we know to which priority to unwind when packet completes Write   
     * and we need to send packet back down again for another Write operation. Call to Restore func
     * above would have cleared the saved resource priority. 
     */
    fbe_transport_save_resource_priority(packet);

    fbe_database_get_metadata_lun_id(&object_id);

    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                object_id);

    packet->base_edge = NULL;
    status = fbe_topology_send_io_packet(packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
fbe_status_t
fbe_metadata_nonpaged_operation_read_persist(fbe_packet_t * packet)
{
    fbe_memory_request_t * memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_status_t status;
	fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_object_id_t object_id;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);

    /* get the object information from the metadata element */
    if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_READ_PERSIST) {
        metadata_element = metadata_operation->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    } else{
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: invalid operation 0x%x.\n",
                       __FUNCTION__, metadata_operation->opcode);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    metadata_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: object_id 0x%x\n", __FUNCTION__, object_id);

    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   1, /* one chunks */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
                                   (fbe_memory_completion_function_t)fbe_metadata_nonpaged_operation_read_persist_allocation_completion, /*SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   NULL /* completion_context */);
	
	fbe_transport_memory_request_set_io_master(memory_request,packet);
	
    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;   
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_read_persist_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_object_id_t object_id;
    //fbe_object_id_t last_system_object_id;
    fbe_object_id_t metadata_lun_id;
    fbe_status_t status;
    fbe_u8_t * data_ptr = NULL;
    fbe_bool_t is_system_drive;
    fbe_lba_t lba;
    fbe_u32_t blocks_per_object;

    packet = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the object information from the metadata element */
    metadata_element = metadata_operation->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    is_system_drive = fbe_metadata_nonpaged_is_system_drive(object_id, 1);

    master_memory_header = memory_request->ptr;   

    new_packet = (fbe_packet_t *)master_memory_header->data;
    data_ptr = master_memory_header->data + FBE_MEMORY_CHUNK_SIZE;

    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_transport_propagate_expiration_time(new_packet, packet);

    fbe_metadata_nonpaged_get_lba_block_count(object_id, &lba, &blocks_per_object);

    sg_list = (fbe_sg_element_t *)(data_ptr + FBE_METADATA_BLOCK_SIZE * blocks_per_object);
    sg_list[0].address = data_ptr;
    sg_list[0].count = FBE_METADATA_BLOCK_SIZE * blocks_per_object;
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    /* Zero memory */
    fbe_zero_memory(sg_list[0].address, sg_list[0].count);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, 1);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      lba,    /* LBA */
                                      blocks_per_object,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_operation_read_persist_completion, NULL);
    fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);
    status = fbe_transport_add_subpacket(packet, new_packet);
    fbe_payload_ex_increment_block_operation_level(sep_payload);

    /* System object use the raw mirror to store the non-paged.  All other
     * objects use the 3-way mirror.
     */
    //fbe_database_get_last_system_object_id(&last_system_object_id);
    if (is_system_drive)
    {
        /* Set the request to the raw mirror */
        status = fbe_metadata_nonpaged_send_packet_to_raw_mirror(new_packet, object_id, FBE_FALSE);
    }
    else
    {
        /* Else send the request to to 3-way mirror */
        fbe_database_get_metadata_lun_id(&metadata_lun_id);
        fbe_transport_set_address(new_packet,
                                  FBE_PACKAGE_ID_SEP_0,
                                  FBE_SERVICE_ID_TOPOLOGY,
                                  FBE_CLASS_ID_INVALID,
                                  metadata_lun_id);
        packet->base_edge = NULL;
        status = fbe_topology_send_io_packet(new_packet);    
    }

    return status;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_read_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_status_t status;  
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_memory_request_t * master_memory_request = NULL;
    fbe_lba_t   lba = FBE_LBA_INVALID;
    fbe_block_count_t   blocks = 0;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t * sg_list_p = NULL;
    //fbe_u32_t  index = FBE_U32_MAX;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_object_id_t object_id;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(master_packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    /* Get sg list */
    fbe_payload_ex_get_sg_list(sep_payload, &sg_list_p, NULL);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_block_count(block_operation, &blocks);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);

	metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "nonpaged_read_persist: complete: lba: 0x%llx blks: 0x%llx status: %d qual: %d\n", 
                   lba, blocks, block_operation_status, block_operation_qualifier);

    //index = (fbe_u32_t)block_operation->lba;
    //index = index/blocks_per_object;

    /*! note Currently we don't validate the checksum.
     */

    /* Check if we got Error on the read */
    if((status == FBE_STATUS_OK) && (block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) )
    {
        /* Copy the data */
        buffer = sg_list_p->address;
        fbe_copy_memory(fbe_metadata_nonpaged_array[object_id].data, buffer, FBE_METADATA_NONPAGED_MAX_SIZE);
        metadata_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "nonpaged_read_persist data 0x%016llX 0x%016llX 0x%016llX\n", 
                       *(fbe_u64_t *)buffer, *(fbe_u64_t *)&buffer[8], *(fbe_u64_t *)&buffer[16]);

    }
    else
    {
        /*if the packet status is not OK or the block operation status is not ok
		  we need to set the NP state of this object to INVALID
		  We operation the memory directly when loading nonpaged metadata*/
        fbe_metadata_set_nonpaged_metadata_state_in_memory(FBE_NONPAGED_METADATA_INVALID, object_id);
        metadata_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "nonpaged_read_persist read failed: status=%d, block_status=%d objID:0x%x\n",
                       status, block_operation_status, object_id);
    }

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    /* Release block operation */
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    fbe_transport_destroy_packet(packet); /* This will not release the memory */

    /* Release the memory */
    //fbe_memory_request_get_entry_mark_free(master_memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(master_memory_request);

    /* Set metadata operation status */
    sep_payload = fbe_transport_get_payload_ex(master_packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_metadata_nonpaged_operation_persist(fbe_packet_t * packet)
{
    fbe_memory_request_t * memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_status_t status;
	fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_object_id_t object_id;
	fbe_object_id_t last_object_id;
    fbe_u32_t       commit_nonpaged_metadata_size = 0; 
    fbe_sep_version_header_t *ver_header = NULL;
    fbe_class_id_t  class_id;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);

    /*the FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PRESET has no valid metadata_element 
      when calling metadata service. We need pass the operated object id in u.metadata.offset*/
    if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PERSIST) {
        metadata_element = metadata_operation->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    } else if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PRESET) {
        object_id = (fbe_object_id_t)metadata_operation->u.metadata.offset; /* offset = object id*/
        fbe_copy_memory(fbe_metadata_nonpaged_array[object_id].data, metadata_operation->u.metadata.record_data,
                        metadata_operation->u.metadata.record_data_size);
} else{
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: invalid operation 0x%x.\n",
                       __FUNCTION__, metadata_operation->opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /*versioning check: check if SEP upgrade done and commit the new non-paged metadata size. update
      the version header here to persist with the latest commit size*/
    if (metadata_element != NULL) {
        /*don't handle FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PRESET here*/
        class_id = fbe_base_config_metadata_element_to_class_id(metadata_element);
        ver_header = (fbe_sep_version_header_t *)fbe_metadata_nonpaged_array[object_id].data;
        status =  fbe_database_get_committed_nonpaged_metadata_size(class_id, &commit_nonpaged_metadata_size);
        if (status == FBE_STATUS_OK && commit_nonpaged_metadata_size > ver_header->size) {
            ver_header->size = commit_nonpaged_metadata_size;
        }
    }

    fbe_database_get_last_system_object_id(&last_object_id);
    if(object_id <= last_object_id){
#ifdef C4_INTEGRATED
        /*don't persist the nonpaged metadata of the system pvds in bootflash, since we don't load cmi drivers(PCIe...) in mini-safe container, 
         *the cmi service will be disabled and both SPs are ACTIVE. In this case all system pvds are active on the both SPs.
         *
         *however, the c4mirror rg/lun are created individually for each SP, their nonpaged metadata will be persisted normally. 
         */
        if (fbe_private_space_layout_object_id_is_system_pvd(object_id) && fbe_cmi_service_disabled())
        {
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
#endif
        return fbe_metadata_nonpaged_operation_system_persist(packet);
    }

    metadata_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: object_id 0x%x\n", __FUNCTION__, object_id);

    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   1, /* one chunk */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
                                   (fbe_memory_completion_function_t)fbe_metadata_nonpaged_operation_persist_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   NULL /* completion_context */);

	
	
	fbe_transport_memory_request_set_io_master(memory_request,packet);
	
    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;   
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_persist_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_object_id_t object_id;
    fbe_object_id_t metadata_lun_id;
    fbe_status_t status;
    fbe_u8_t * data_ptr = NULL;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    packet = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*the FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PRESET has no valid metadata_element 
      when calling metadata service. We need pass the operated object id in u.metadata.offset*/
    if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PERSIST) {
        metadata_element = metadata_operation->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    } else if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PRESET) {
        object_id = (fbe_object_id_t)metadata_operation->u.metadata.offset; /* offset = object id*/
    } else{
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: invalid operation 0x%x.\n",
                       __FUNCTION__, metadata_operation->opcode);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    master_memory_header = memory_request->ptr;   

    new_packet = (fbe_packet_t *)master_memory_header->data;
    data_ptr = master_memory_header->data + FBE_MEMORY_CHUNK_SIZE;

    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_transport_propagate_expiration_time(new_packet, packet);

    sg_list = (fbe_sg_element_t *)(data_ptr + FBE_METADATA_BLOCK_SIZE * blocks_per_object);
    sg_list[0].address = data_ptr;
    sg_list[0].count = FBE_METADATA_BLOCK_SIZE * blocks_per_object;
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    /* Zero memory */
    fbe_zero_memory(sg_list[0].address, sg_list[0].count);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, 1);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,
                                      object_id * blocks_per_object,    /* LBA */
                                      blocks_per_object,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* persist nonpaged metadata */
    fbe_copy_memory(sg_list[0].address, fbe_metadata_nonpaged_array[object_id].data, FBE_METADATA_NONPAGED_MAX_SIZE);

	metadata_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "nonpaged_persist data 0x%016llX 0x%016llX 0x%016llX\n", *(fbe_u64_t *)fbe_metadata_nonpaged_array[object_id].data,
                                        *(fbe_u64_t *)&fbe_metadata_nonpaged_array[object_id].data[8],
                                        *(fbe_u64_t *)&fbe_metadata_nonpaged_array[object_id].data[16]);
    

    fbe_metadata_calculate_checksum(sg_list[0].address, blocks_per_object);

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_operation_persist_completion, NULL);
    fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);

    status = fbe_transport_add_subpacket(packet, new_packet);

    fbe_database_get_metadata_lun_id(&metadata_lun_id);

    fbe_transport_set_address(  new_packet,
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                metadata_lun_id);

    packet->base_edge = NULL;
    fbe_payload_ex_increment_block_operation_level(sep_payload);
    status = fbe_topology_send_io_packet(new_packet);    
    return status;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_status_t status;  
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_memory_request_t * master_memory_request = NULL;
    fbe_lba_t   lba = FBE_LBA_INVALID;
    fbe_block_count_t   blocks = 0;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_payload_block_operation_t * block_operation = NULL;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_block_count(block_operation, &blocks);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);

	metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "nonpaged_persist: complete: lba: 0x%llx blks: 0x%llx status: %d qual: %d\n", 
                   (unsigned long long)lba, (unsigned long long)blocks,
		   block_operation_status, block_operation_qualifier);

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    /* Release block operation */
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    fbe_transport_destroy_packet(packet); /* This will not release the memory */

    /* Release the memory */
    //fbe_memory_request_get_entry_mark_free(master_memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(master_memory_request);

    /* Set metadata operation status */
    sep_payload = fbe_transport_get_payload_ex(master_packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_OK;
}


/* Checkpoint */
fbe_status_t
fbe_metadata_nonpaged_operation_change_checkpoint(fbe_packet_t * packet)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_object_id_t object_id;
    fbe_memory_request_t * memory_request = NULL;
    fbe_memory_request_priority_t memory_request_priority = 0;
    fbe_status_t status;
    fbe_u64_t data_size;
    fbe_u8_t * data_ptr = NULL;
	fbe_metadata_element_t * metadata_element = NULL;
	fbe_lba_t checkpoint;
	fbe_lba_t current_checkpoint;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
	metadata_element = metadata_operation->metadata_element;

    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

    data_size = metadata_operation->u.metadata.offset + metadata_operation->u.metadata.record_data_size;
    if(data_size > fbe_metadata_nonpaged_array[object_id].data_size){
        metadata_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s Invalid write size %llu > %d \n", __FUNCTION__, 
                       (unsigned long long)data_size,
                       fbe_metadata_nonpaged_array[object_id].data_size);

        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    data_ptr = (fbe_u8_t *)(fbe_metadata_nonpaged_array[object_id].data + metadata_operation->u.metadata.offset);

	fbe_spinlock_lock(&metadata_element->metadata_element_lock);
	current_checkpoint = *(fbe_lba_t *)data_ptr;
	checkpoint = *(fbe_lba_t *)metadata_operation->u.metadata.record_data;
	if (metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_CHECKPOINT){
		if(checkpoint >= current_checkpoint){ /* Do nothing */
			fbe_spinlock_unlock(&metadata_element->metadata_element_lock);
			fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			fbe_transport_complete_packet_async(packet);
			return FBE_STATUS_OK;
		} else {
			*(fbe_lba_t *)data_ptr = checkpoint;
            if(metadata_operation->u.metadata.second_offset != 0)
             {
                fbe_u8_t * second_data_ptr = NULL;
                second_data_ptr = (fbe_u8_t *)(fbe_metadata_nonpaged_array[object_id].data + metadata_operation->u.metadata.second_offset);                
                *(fbe_lba_t *)second_data_ptr = checkpoint;
                metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s set first: %p 0x%llx second: %p 0x%llx\n", 
                               __FUNCTION__, data_ptr, *((unsigned long long *)data_ptr),
                               second_data_ptr, *((unsigned long long *)second_data_ptr));
            }
		}
	}
    else if (metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_FORCE_SET_CHECKPOINT){
        *(fbe_lba_t *)data_ptr = checkpoint;
        if (metadata_operation->u.metadata.second_offset != 0)
        {
            fbe_u8_t * second_data_ptr = NULL;
            second_data_ptr = (fbe_u8_t *)(fbe_metadata_nonpaged_array[object_id].data + metadata_operation->u.metadata.second_offset);                
            *(fbe_lba_t *)second_data_ptr = checkpoint;
            metadata_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s force first: %p 0x%llx second: %p 0x%llx\n", 
                           __FUNCTION__, data_ptr, *((unsigned long long *)data_ptr),
                           second_data_ptr, *((unsigned long long *)second_data_ptr));
        }
    }
    else { /* we do increment checkpoint FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INCR_CHECKPOINT */
		if(checkpoint != current_checkpoint){ /* Do nothing */
			fbe_spinlock_unlock(&metadata_element->metadata_element_lock);
			fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			fbe_transport_complete_packet_async(packet);
			return FBE_STATUS_OK;
		} else {
			*(fbe_lba_t *)data_ptr = checkpoint + metadata_operation->u.metadata.repeat_count;
            if(metadata_operation->u.metadata.second_offset != 0)
             {
                fbe_u8_t * second_data_ptr = NULL;
                second_data_ptr = (fbe_u8_t *)(fbe_metadata_nonpaged_array[object_id].data + metadata_operation->u.metadata.second_offset);                
                checkpoint = *(fbe_lba_t *)second_data_ptr;
                *(fbe_lba_t *)second_data_ptr = checkpoint + metadata_operation->u.metadata.repeat_count;
            }
		}
	}
	fbe_spinlock_unlock(&metadata_element->metadata_element_lock);

    /* We do not update the peer for certain opcodes.
     */
	if(!fbe_metadata_is_peer_object_alive(metadata_element) ||
       (metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INCR_CHECKPOINT_NO_PEER)){
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet_async(packet);
        return FBE_STATUS_OK;
	}

    /* We may want to send cmi message */
    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1, /* one chunk */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
                                  (fbe_memory_completion_function_t) fbe_metadata_nonpaged_operation_change_checkpoint_allocation_completion,
                                   NULL /* completion_context */);

		fbe_transport_memory_request_set_io_master(memory_request, packet);


    status = fbe_memory_request_entry(memory_request);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_change_checkpoint_allocation_completion(fbe_memory_request_t * memory_request, 
																		fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_memory_header_t * master_memory_header = NULL;

    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_metadata_cmi_message_t * metadata_cmi_message;  

    packet = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    master_memory_header = memory_request->ptr;   
    metadata_cmi_message = (fbe_metadata_cmi_message_t *)master_memory_header->data;

    metadata_cmi_message->header.object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    metadata_cmi_message->header.metadata_element_receiver = metadata_element->peer_metadata_element_ptr; /* If not NULL should point to the memory of the receiver */
    metadata_cmi_message->header.metadata_element_sender = (fbe_u64_t)metadata_element;

    if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_CHECKPOINT){
        metadata_cmi_message->header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_SET_CHECKPOINT;
    } else if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_FORCE_SET_CHECKPOINT){
        metadata_cmi_message->header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_FORCE_SET_CHECKPOINT;
    } else {
        metadata_cmi_message->header.metadata_cmi_message_type = FBE_METADATA_CMI_MESSAGE_TYPE_INCR_CHECKPOINT;
    }
    
    /*metadata.record_data_size cannot be larger than FBE_PAYLOAD_METADATA_MAX_DATA_SIZE*/
    if(metadata_operation->u.metadata.record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Invalid memory size %d > %d ", __FUNCTION__ , 
                metadata_operation->u.metadata.record_data_size, 
                FBE_METADATA_CMI_NONPAGED_WRITE_DATA_SIZE);
        
        /* Release memory */
        //fbe_memory_request_get_entry_mark_free(memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);      
		fbe_memory_free_request_entry(memory_request);

        /* Complete packet */
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_OK;
    }

    fbe_copy_memory(metadata_cmi_message->msg.nonpaged_change.data,
                    metadata_operation->u.metadata.record_data, 
                    metadata_operation->u.metadata.record_data_size);

    metadata_cmi_message->msg.nonpaged_change.data_size = metadata_operation->u.metadata.record_data_size;
    metadata_cmi_message->msg.nonpaged_change.offset = metadata_operation->u.metadata.offset;
    metadata_cmi_message->msg.nonpaged_change.repeat_count = metadata_operation->u.metadata.repeat_count;
    metadata_cmi_message->msg.nonpaged_change.second_offset = metadata_operation->u.metadata.second_offset;
    metadata_cmi_message->header.metadata_operation_flags = metadata_operation->u.metadata.operation_flags;

    /* Include the sequence number for system objects */
    if (fbe_private_space_layout_object_id_is_system_object(metadata_cmi_message->header.object_id)) {
        metadata_cmi_message->header.metadata_operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_SEQ_NUM;
        metadata_cmi_message->msg.nonpaged_change.seq_number = fbe_metadata_raw_mirror_io_cb.seq_numbers[metadata_cmi_message->header.object_id ];
    }

    /* Set completion function */
    fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_operation_change_checkpoint_completion, metadata_cmi_message);

	if(fbe_metadata_element_is_peer_dead(metadata_element) || (fbe_metadata_element_is_cmi_disabled(metadata_element))) {
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    fbe_metadata_cmi_send_message(metadata_cmi_message, packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_change_checkpoint_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_status_t status;  
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_memory_request_t * memory_request = NULL;
    fbe_metadata_cmi_message_t * metadata_cmi_message = NULL;

    metadata_cmi_message = (fbe_metadata_cmi_message_t *)context;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;

    memory_request = fbe_transport_get_memory_request(packet);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);

    //fbe_memory_request_get_entry_mark_free(memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(memory_request);

    fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    return FBE_STATUS_OK;
}



static fbe_status_t
fbe_metadata_nonpaged_system_load_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list_p = NULL;
    fbe_u32_t       sg_list_count = 0;
    fbe_status_t status;
    fbe_memory_header_t * master_memory_header = NULL;
    //fbe_memory_header_t * current_memory_header = NULL;
    fbe_object_id_t last_object_id;    
	fbe_u8_t * buffer;
	raw_mirror_operation_cb_t * raw_mirror_op_cb = NULL;
	raw_mirror_system_load_context_t * raw_mirror_system_load_context = NULL;
    fbe_memory_chunk_size_t memory_chunk_size;
    fbe_u32_t       sg_list_size = 0;
    fbe_u32_t       aligned_rw_io_cb_size = 0;
    //fbe_u32_t       i = 0;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    packet = fbe_transport_memory_request_to_packet(memory_request);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*memory request done, issue IO now*/
    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    memory_chunk_size = master_memory_header->memory_chunk_size; /* valid in master memory header only */

    sg_list_size = (master_memory_header->number_of_chunks) * sizeof(fbe_sg_element_t);
    /* round up the sg list size to align with 64-bit boundary */
    sg_list_size = (sg_list_size + sizeof(fbe_u64_t) - 1) & 0xfffffff8; 

#if 0 /* This check is not needed: we allocate larger memory chunks now */
    if ((master_memory_header->memory_chunk_size) < (sizeof(fbe_packet_t)+sg_list_size)) {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: chunk_size < packet+sg_lists: %d < %d \n \n",
                       __FUNCTION__,
                       memory_request->chunk_size,
                       (int)(sizeof(fbe_packet_t) + sg_list_size));

		fbe_memory_free_request_entry(memory_request);  
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
    }
#endif

	/*fbe_database_get_last_system_object_id(&last_object_id);*/
    last_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST;
	
    aligned_rw_io_cb_size = (sizeof (raw_mirror_operation_cb_t) + sizeof (raw_mirror_system_load_context_t)
                             + FBE_METADATA_BLOCK_SIZE - 1)/FBE_METADATA_BLOCK_SIZE;

    /* carve the packet */
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
    new_packet = (fbe_packet_t *)buffer;
    fbe_zero_memory(new_packet, sizeof(fbe_packet_t));
    //buffer += sizeof(fbe_packet_t);
    buffer += FBE_MEMORY_CHUNK_SIZE;

    /* carve the sg list */
    sg_list_count = master_memory_header->number_of_chunks - 1; /* minus the first 1 chunk*/
    sg_list_p = (fbe_sg_element_t *)buffer;
    fbe_zero_memory(sg_list_p, sg_list_size);
    buffer += FBE_MEMORY_CHUNK_SIZE;

    /*
      form raw-mirror IO control block and system read context.
      The second memory chunk will divided into two
      parts: raw-mirror IO cb and system load context placed at the begin, 
      the remaining memory used for IO
    */
    //fbe_memory_get_next_memory_header(master_memory_header, &current_memory_header);
    //master_memory_header = current_memory_header;

    /*the beginning part used for raw-mirror IO block*/
    //buffer = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header);
	/*allocate raw_mirror_operation_cb_t*/
	raw_mirror_op_cb = (raw_mirror_operation_cb_t*)buffer;
	buffer += sizeof(raw_mirror_operation_cb_t);
    fbe_zero_memory(raw_mirror_op_cb, sizeof(raw_mirror_operation_cb_t));
	/*allocate 
	raw_mirror_system_load_context_t*/		
	raw_mirror_system_load_context = (raw_mirror_system_load_context_t*)buffer;
    fbe_zero_memory(raw_mirror_system_load_context, sizeof(raw_mirror_system_load_context_t));

#if 0
    /*ramining memory of second memory chunk used for IO*/
    sg_list_p[0].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header) + FBE_METADATA_BLOCK_SIZE * aligned_rw_io_cb_size;
    sg_list_p[0].count = FBE_METADATA_BLOCK_SIZE * (chunk_size_to_block_count(memory_chunk_size) - aligned_rw_io_cb_size);
	fbe_zero_memory(sg_list_p[0].address, sg_list_p[0].count);

    /* Form SG lists from remaing memory chunk*/
    for (i = 1; i < sg_list_count; i++) {
        /* move to the next chunk */
        fbe_memory_get_next_memory_header(master_memory_header, &current_memory_header);
        master_memory_header = current_memory_header;

        sg_list_p[i].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header);
        sg_list_p[i].count = FBE_METADATA_BLOCK_SIZE * chunk_size_to_block_count(memory_chunk_size);
        
        fbe_zero_memory(sg_list_p[i].address, sg_list_p[i].count);
    }
    sg_list_p[sg_list_count].address = NULL;
    sg_list_p[sg_list_count].count = 0;
#endif
    fbe_metadata_nonpaged_setup_sg_list_from_memory_request(memory_request, &sg_list_p, &sg_list_count);

    /*initialize the raw-mirror IO control block.
	   It will be used in system nonpaged  metadata load error handling*/
    raw_mirror_init_operation_cb(raw_mirror_op_cb, 
    							 0,
                                 last_object_id + 1,
                                 sg_list_p,
                                 sg_list_count,
                                 &fbe_metadata_nonpaged_raw_mirror,
                                 FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);
    /*initialize the raw-mirror system load context.
	   It will be used in system nonpaged metadata load completion funtion
	   to copy the read data and issue single block mode read if neccessary*/
	raw_mirror_init_system_load_context(raw_mirror_system_load_context, 
    							        0,
                                        last_object_id + 1,
                                        sg_list_p,
                                        sg_list_count,
                                        raw_mirror_op_cb,
                                        FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);

    raw_mirror_set_error_retry(raw_mirror_op_cb,
                               FBE_TRUE);

    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);
    fbe_zero_memory(&raw_mirror_op_cb->error_verify_report, sizeof (fbe_raid_verify_raw_mirror_errors_t));
    fbe_payload_ex_set_verify_error_count_ptr(sep_payload, &raw_mirror_op_cb->error_verify_report);

    fbe_transport_propagate_expiration_time(new_packet, packet);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list_p, sg_list_count);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      (raw_mirror_op_cb->lba) * blocks_per_object,    /* LBA */
                                      (raw_mirror_op_cb->block_count) * blocks_per_object,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* This completion function is responsible for:
	     1. copy data into memory when load successfully
	     2. switch to single block mode read when there is a media error
	     3. set NP state ti invalid when single block mode read failed*/
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_system_load_read_completion, raw_mirror_system_load_context);
	/*Set complete function for error handling:
	     1. retry when load failed and caller sets retriable
	     2. issue out-of-band rebuild when seq_num/magic num error*/
    fbe_transport_set_completion_function(new_packet, fbe_raw_mirror_utils_error_handling_read_completion, raw_mirror_op_cb);

	fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);
    status = fbe_transport_add_subpacket(packet, new_packet);

    fbe_payload_ex_increment_block_operation_level(sep_payload);
    status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_metadata_nonpaged_raw_mirror, new_packet,FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);

    return status;
}
/*!***************************************************************
 * @fn fbe_metadata_nonpaged_system_load_read_completion
*****************************************************************
* @brief
*   This function is the completion function for system nonpaged metadata read.
*   it does the following things:
*   1. check whether the system NP load is songle block mode or not .
*   2. for chunk based read,
*       1) if block_operation_status is successful: copy the read data
*       2) if block_operation_status is media error:
*			 a) set the single block mode read = TRUE
*			 b) re-set this complete function
*			 c) switch to single block mode read
*       3) any other failure are handled in fbe_raw_mirror_utils_error_handling_read_completion
*   3. for single block mode read
*       1). if packet status == OK
*             and FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, copy the read data;
*	  2). any other failure status: set the NP state to INVALID
*
* @param packet   - the IO packet
* @param context  - context of the complete function run
* 
* @return status - indicating if the complete function success
*
* @version
*    5/25/2012: zhangy - created
*
****************************************************************/

static fbe_status_t 
fbe_metadata_nonpaged_system_load_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_memory_request_t * master_memory_request = NULL;    
    fbe_u8_t * buffer = NULL;
    fbe_u32_t i;
    fbe_payload_ex_t * master_sep_payload = NULL;
    fbe_payload_control_operation_t *   master_control_operation = NULL;
	fbe_object_id_t last_object_id;    
	raw_mirror_system_load_context_t * raw_mirror_system_load_context = NULL;
    fbe_object_id_t  system_object_id = 0;
    fbe_u32_t   buffer_count = 0;
    fbe_s64_t   left_block_count = 0;
    fbe_u32_t   copied_block_count = 0;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    /*retrieve the context info*/
    raw_mirror_system_load_context = (raw_mirror_system_load_context_t *)context;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);
    master_sep_payload = fbe_transport_get_payload_ex(master_packet);
    master_control_operation = fbe_payload_ex_get_control_operation(master_sep_payload);
	status = fbe_transport_get_status_code(packet);
    sep_payload = fbe_transport_get_payload_ex(packet);
	block_operation = fbe_payload_ex_get_block_operation(sep_payload);
	fbe_payload_block_get_status(block_operation, &block_operation_status);
	fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);

	/*handle chunk based system objects NP complete*/
    if(!raw_mirror_system_load_context->is_single_block_mode_read)
    {
       
	   /* Check block operation status */
       
       /*if block_operation_status is successful: copy the read data
                 if block_operation_status is media error:
                    1) set the single block mode read = TRUE
                    2) re-set this complete function
                    3) switch to single block mode read
	     */ 
	   if(status == FBE_STATUS_OK && block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
	   {
		   /* Get sg list */
		   fbe_payload_ex_get_sg_list(sep_payload, &sg_list_p, NULL);
		   
		   /* Copy the data */
		   fbe_database_get_last_system_object_id(&last_object_id);
		   /* Copy the data, one object's NP occupied one block.
		         so the object ID is the block lba.*/
		   left_block_count = raw_mirror_system_load_context->range;
		   copied_block_count = (fbe_u32_t)(raw_mirror_system_load_context->start_lba);
		   for (i = 0; i < raw_mirror_system_load_context->sg_list_count; i++) {
			   buffer = sg_list_p[i].address;
			   buffer_count = sg_list_p[i].count;
			   while ((buffer_count >= (FBE_METADATA_BLOCK_SIZE*blocks_per_object)) && (left_block_count > 0)) {
				   fbe_copy_memory(fbe_metadata_nonpaged_array[copied_block_count].data, buffer, FBE_METADATA_NONPAGED_MAX_SIZE);
				   metadata_trace(FBE_TRACE_LEVEL_INFO,
							   FBE_TRACE_MESSAGE_ID_INFO,
							   "nonpaged_system_load data 0x%016llX 0x%016llX 0x%016llX\n", *(fbe_u64_t *)buffer, *(fbe_u64_t *)&buffer[8], *(fbe_u64_t *)&buffer[16]);
				   
				   fbe_metadata_set_block_seq_number(buffer,copied_block_count);
				   left_block_count --;
				   copied_block_count++;
				   /* move forward the sg_list's pointer (520) */
				   buffer += (FBE_METADATA_BLOCK_SIZE*blocks_per_object);
				   buffer_count -= (FBE_METADATA_BLOCK_SIZE*blocks_per_object);
			   }
		   }

	   }else{
           /*If the packet status is ok but the block operation status is media error,
		        we swith to single block mode read.
		        We will reused the packet in order to keep its original control operation
		        and control operation's completion function*/
		   if(status == FBE_STATUS_OK && block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
		   {
               raw_mirror_system_load_context->current_lba = raw_mirror_system_load_context->start_lba;
			   raw_mirror_system_load_context->is_single_block_mode_read = FBE_TRUE;

               fbe_transport_packet_clear_callstack_depth(packet);/* reset this since we reuse the packet */
			   /* Release block operation */
			   fbe_payload_ex_release_block_operation(sep_payload, block_operation);
			   fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_system_load_read_completion, raw_mirror_system_load_context);
			   fbe_metadata_nonpaged_load_single_system_object(packet,raw_mirror_system_load_context);			  
			   return FBE_STATUS_MORE_PROCESSING_REQUIRED;
		   }
	   }
	}else{
	    /* a system object ocupy one block in the disk,
	            so system object id is equal to lba.
               */
	     system_object_id = (fbe_object_id_t)raw_mirror_system_load_context->current_lba;
        /*handle single block mode read complete*/
	    /*check block operation:
	           if block operation is successful, copy the data*/
	     if(status == FBE_STATUS_OK && block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
	     {   
		     /* Get sg list */
			 fbe_payload_ex_get_sg_list(sep_payload, &sg_list_p, NULL);
			 
			 /* Copy the data, one object's NP occupied one block.
				   so the object ID is the block lba.*/
			buffer = sg_list_p[0].address;
			fbe_copy_memory(fbe_metadata_nonpaged_array[system_object_id].data, buffer, FBE_METADATA_NONPAGED_MAX_SIZE);
			metadata_trace(FBE_TRACE_LEVEL_INFO,
						   FBE_TRACE_MESSAGE_ID_INFO,
						   "nonpaged_system_single_load data 0x%016llX 0x%016llX 0x%016llX\n", *(fbe_u64_t *)buffer, *(fbe_u64_t *)&buffer[8], *(fbe_u64_t *)&buffer[16]);
	        fbe_metadata_set_block_seq_number(buffer,system_object_id);				 
	     }else{
	     /*if the packet status is not OK or the block operation status is not ok
		     we need to set the NP state of this object to INVALID
		     We operation the memory directly when loading nonpaged metadata*/
			 fbe_metadata_set_nonpaged_metadata_state_in_memory(FBE_NONPAGED_METADATA_INVALID,system_object_id);
			 metadata_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
							"Single sys obj NP read failed: status =%d, block_op_status = %d objID %d\n",
							status, block_operation_status,system_object_id);


	     }
		 
	    /*check whether we finished load all the system objects NP 
	         if we don't finish read, continue to next block*/
		raw_mirror_system_load_context->current_lba ++;
                left_block_count = raw_mirror_system_load_context->range + raw_mirror_system_load_context->start_lba - raw_mirror_system_load_context->current_lba;
		if(left_block_count > 0){
            fbe_transport_packet_clear_callstack_depth(packet);/* reset this since we reuse the packet */
			/* Release block operation . We reused the packet to issue next block read*/
			fbe_payload_ex_release_block_operation(sep_payload, block_operation);
			fbe_transport_set_completion_function(packet, fbe_metadata_nonpaged_system_load_read_completion, raw_mirror_system_load_context);
			fbe_metadata_nonpaged_load_single_system_object(packet,raw_mirror_system_load_context); 		   
			return FBE_STATUS_MORE_PROCESSING_REQUIRED;
		}
	}
    /* We are done ... */

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    /* Release block operation */
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    fbe_transport_destroy_packet(packet); /* This will not release the memory */

    /* Release the memory */
	fbe_memory_free_request_entry(master_memory_request);
    
    /* Set control operation status */
	fbe_payload_control_set_status(master_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
	fbe_transport_set_status(master_packet, status, 0);
    fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_system_persist(fbe_packet_t * packet)
{
    fbe_memory_request_t * memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_status_t status;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    metadata_element = metadata_operation->metadata_element;
    object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);

        /* We may want to send cmi message */
    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   1, /* one chunks */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
                                   (fbe_memory_completion_function_t)fbe_metadata_nonpaged_operation_system_persist_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   NULL /* completion_context */);

	fbe_transport_memory_request_set_io_master(memory_request, packet);

    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;   
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_system_persist_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status;
    fbe_u8_t * data_ptr = NULL;
    fbe_lba_t lba;
    fbe_u32_t blocks_per_object;

    packet = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PERSIST) {
        metadata_element = metadata_operation->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    } else if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PRESET) {
        object_id = (fbe_object_id_t)metadata_operation->u.metadata.offset; /* offset = object id*/
        fbe_copy_memory(fbe_metadata_nonpaged_array[object_id].data,
                        metadata_operation->u.metadata.record_data,
                        metadata_operation->u.metadata.record_data_size);
    } else{
        metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: invalid operation 0x%x.\n",
                       __FUNCTION__, metadata_operation->opcode);
    }

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    metadata_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: object_id %d\n", __FUNCTION__, object_id);

    master_memory_header = memory_request->ptr;   

    new_packet = (fbe_packet_t *)master_memory_header->data;
    data_ptr = master_memory_header->data + FBE_MEMORY_CHUNK_SIZE;

    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_transport_propagate_expiration_time(new_packet, packet);

    fbe_metadata_nonpaged_get_lba_block_count(object_id, &lba, &blocks_per_object);

    sg_list = (fbe_sg_element_t *)(data_ptr + FBE_METADATA_BLOCK_SIZE * blocks_per_object);
    sg_list[0].address = data_ptr;
    sg_list[0].count = FBE_METADATA_BLOCK_SIZE * blocks_per_object;
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    /* Zero memory */
    fbe_zero_memory(sg_list[0].address, sg_list[0].count);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, 1);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      lba,    /* LBA */
                                      blocks_per_object,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Persist the new nonpaged metadata */
    fbe_copy_memory(sg_list[0].address, fbe_metadata_nonpaged_array[object_id].data, FBE_METADATA_NONPAGED_MAX_SIZE);


    //fbe_metadata_raw_mirror_calculate_checksum(sg_list[0].address,object_id,1,FBE_FALSE);
    fbe_metadata_nonpaged_calculate_checksum_for_one_object(sg_list[0].address, object_id, blocks_per_object, FBE_TRUE);

	metadata_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "nonpaged_system_persist ID 0x%x data 0x%016llX 0x%016llX 0x%016llX\n", object_id, 
			(unsigned long long)*(fbe_u64_t *)fbe_metadata_nonpaged_array[object_id].data,
			(unsigned long long)*(fbe_u64_t *)&fbe_metadata_nonpaged_array[object_id].data[8],
			(unsigned long long)*(fbe_u64_t *)&fbe_metadata_nonpaged_array[object_id].data[16]);


    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_operation_system_persist_completion, NULL);
    fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);
    status = fbe_transport_add_subpacket(packet, new_packet);
    fbe_payload_ex_increment_block_operation_level(sep_payload);
    status = fbe_metadata_nonpaged_send_packet_to_raw_mirror(new_packet, object_id, FBE_TRUE);
    return status;
}

static fbe_status_t fbe_metadata_nonpaged_send_packet_to_raw_mirror(fbe_packet_t * new_packet, fbe_object_id_t object_id, fbe_bool_t need_special_attribute)
{
    fbe_object_id_t metadata_lun_id;
    fbe_object_id_t raw_mirror_rg_object_id;
    fbe_status_t status;
   
    /* We don't allow the nonpaged operation overlaps between c4mirror object and non-c4mirror objects.
     * so if one of the objects hit the range, we can assume it is a c4mirror nonpaged operation.
     */
    if (fbe_database_is_object_c4_mirror(object_id)){
        fbe_raw_mirror_t *nonpaged_raw_mirror_p = NULL;
        metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "NP: system persist raw mirror library object_id 0x%x\n", object_id);
        /* get the c4mirror nonpaged raw mirror - homewrecker_db */
        fbe_c4_mirror_get_nonpaged_metadata_config((void **)(&nonpaged_raw_mirror_p), NULL, NULL);
        status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(nonpaged_raw_mirror_p, new_packet,FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);
    }
    else if (fbe_raw_mirror_is_enabled(&fbe_metadata_nonpaged_raw_mirror)){
        metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "NP: system persist raw mirror library object_id 0x%x\n", object_id);
        status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_metadata_nonpaged_raw_mirror, new_packet,FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);
    }
    else {
        if (need_special_attribute == FBE_TRUE) {
            fbe_database_get_raw_mirror_rg_id(&raw_mirror_rg_object_id);
            if (object_id == raw_mirror_rg_object_id){
                fbe_transport_set_packet_attr(new_packet, (FBE_PACKET_FLAG_DO_NOT_QUIESCE | FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK));
            }
            else if (fbe_database_is_object_system_pvd(object_id))
            {
                fbe_transport_set_packet_attr(new_packet, (FBE_PACKET_FLAG_DO_NOT_QUIESCE | FBE_PACKET_FLAG_DO_NOT_HOLD));
            }
        }
        fbe_database_get_raw_mirror_metadata_lun_id(&metadata_lun_id);

        metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "NP: system persist for lun: 0x%x raw mirror lun object_id 0x%x\n", 
                       object_id, metadata_lun_id);
    
        fbe_transport_set_address(new_packet,
                                  FBE_PACKAGE_ID_SEP_0,
                                  FBE_SERVICE_ID_TOPOLOGY,
                                  FBE_CLASS_ID_INVALID,
                                  metadata_lun_id);
    
        new_packet->base_edge = NULL;
        status = fbe_topology_send_io_packet(new_packet);
    }
    return status;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_system_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_ex_t * master_sep_payload = NULL;
    fbe_status_t status;  
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_memory_request_t * master_memory_request = NULL;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_payload_block_operation_t * block_operation = NULL;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    /* Release block operation */
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    fbe_transport_destroy_packet(packet); /* This will not release the memory */

    /* Release the memory */
    //fbe_memory_request_get_entry_mark_free(master_memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(master_memory_request);

    /* Set master packet status and metadata operation status */
    master_sep_payload = fbe_transport_get_payload_ex(master_packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(master_sep_payload);
	if((status == FBE_STATUS_OK)&&(block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
	{
		fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);	
		fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
	}else{	
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: non-paged md persist failed, packet status: %d block op status: %d\n", __FUNCTION__, status,block_operation_status);
		fbe_transport_set_status(master_packet, status, 0);
		fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
	}
    
    fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *  fbe_metadata_plant_sgl_with_buffer()
 ******************************************************************************
 * @brief
 *  This function is used to plant the sgl with data.
 * 
 * @param sg_list_p
 * @param sg_list_count
 * @param block_count
 * @param buffer_bytes
 * @param master_memory_header_p
 * @return
 *  fbe_status_t
 *
 * @author
 *  6/12/2012 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t
fbe_metadata_plant_sgl_with_buffer(fbe_sg_element_t * sg_list_p,
                                   fbe_u32_t sg_list_count,
                                   fbe_block_count_t block_count,
                                   fbe_u32_t buffer_bytes,
                                   fbe_memory_header_t * master_memory_header_p)
{
    fbe_status_t status;
    fbe_u32_t used_sgs = 0;
    fbe_u32_t buffer_blocks;
    fbe_u8_t *buffer_p = NULL;
    fbe_memory_header_t *memory_header_p = master_memory_header_p;
    fbe_memory_header_t *next_memory_header_p = NULL;

    /* Make sure the buffer we put into the sg is a multiple of block size.
     */
    buffer_blocks = buffer_bytes / FBE_BE_BYTES_PER_BLOCK;
    buffer_bytes -= buffer_bytes % FBE_BE_BYTES_PER_BLOCK;

    /* Fill enough blocks into the sg list for the total transfer.
     */
    while (block_count != 0)
    {
        /* Get a data buffer
         */
        status = fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
        memory_header_p = next_memory_header_p;
        if (status != FBE_STATUS_OK)
        {
            metadata_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: status 0x%x from get next memory header\n", __FUNCTION__, status);
            return status;
        }
        buffer_p = fbe_memory_header_to_data(next_memory_header_p);

        /* Fill in the sg element with this buffer.
         */
        if (used_sgs < sg_list_count)
        {
            sg_list_p->address = buffer_p;
            if(block_count >= buffer_blocks)
            {
                sg_list_p->count = buffer_bytes;
                block_count -= buffer_blocks;
            }
            else
            {
                if((block_count * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
                {
                    /* sg count should not exceed 32bit limit
                     */
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                sg_list_p->count = (fbe_u32_t)(block_count * FBE_BE_BYTES_PER_BLOCK);
                block_count = 0;
            }
            sg_list_p++;
            used_sgs++;
        }
        else
        {
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* update the last entry with null and count as zero. */
    fbe_sg_element_terminate(sg_list_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_metadata_plant_sgl_with_buffer()
 **************************************/
static fbe_status_t
fbe_metadata_nonpaged_system_clear_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_status_t status;
    fbe_memory_header_t * master_memory_header_p = NULL;
    fbe_object_id_t last_object_id;    
	fbe_u8_t * buffer;
    fbe_lba_t offset;
    fbe_block_count_t capacity;
    fbe_u32_t total_sgs;
    fbe_xor_zero_buffer_t zero_buffer;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    packet = fbe_transport_memory_request_to_packet(memory_request);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_database_get_non_paged_metadata_offset_capacity(&offset, NULL, &capacity);
	fbe_database_get_last_system_object_id(&last_object_id);

	/* Sanity check */
	if(memory_request->chunk_size * memory_request->number_of_objects < sizeof(fbe_packet_t) + 2 * sizeof(fbe_sg_element_t) + FBE_METADATA_BLOCK_SIZE * (last_object_id + 1) * blocks_per_object){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Not enought memory\n", __FUNCTION__);
        /* Release the memory */
        //fbe_memory_free_entry(memory_request->ptr);
		fbe_memory_free_request_entry(memory_request);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    /* The first buffer has the packet and the sg list.
     */
    master_memory_header_p = fbe_memory_get_first_memory_header(memory_request);
    buffer = fbe_memory_header_to_data(master_memory_header_p);
    new_packet = (fbe_packet_t *)buffer;
	buffer += sizeof(fbe_packet_t);
    sg_list = (fbe_sg_element_t *)buffer;
    total_sgs = (FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO - sizeof(fbe_packet_t)) / sizeof(fbe_sg_element_t);
    
    status = fbe_metadata_plant_sgl_with_buffer(sg_list, total_sgs, capacity, FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO, master_memory_header_p); 

    if (status != FBE_STATUS_OK)
    {
        metadata_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: status %d from plant sgls\n", __FUNCTION__, status);
		fbe_memory_free_request_entry(memory_request);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Zero buffer with the special raw magic number.
     */
    zero_buffer.disks = 1;
    zero_buffer.offset = 0;
    zero_buffer.fru[0].count = capacity;
    zero_buffer.fru[0].offset = 0;
    /* We can use a zero seed for lba stamp since we are initializing this area.
     */
    zero_buffer.fru[0].seed = FBE_LBA_INVALID; 
    zero_buffer.fru[0].sg_p = &sg_list[0];
    status = fbe_xor_lib_zero_buffer_raw_mirror(&zero_buffer);
    if (status != FBE_STATUS_OK)
    {
        metadata_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: status 0x%x from zero raw mirror\n", __FUNCTION__, status);
        return status;
    }

    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_transport_propagate_expiration_time(new_packet, packet);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, 1);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      0,    /* LBA */
                                      capacity,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_system_clear_write_completion, NULL);
    fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);
    status = fbe_transport_add_subpacket(packet, new_packet);

    fbe_payload_ex_increment_block_operation_level(sep_payload);
    status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(&fbe_metadata_nonpaged_raw_mirror, new_packet,FBE_SEND_IO_TO_RAW_MIRROR_BITMASK);

    return status;
}

static fbe_status_t 
fbe_metadata_nonpaged_system_clear_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_status_t status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_memory_request_t * master_memory_request = NULL;    
    fbe_u8_t * buffer = NULL;
    fbe_payload_ex_t * master_sep_payload = NULL;
    fbe_payload_control_operation_t *   master_control_operation = NULL;
    fbe_u32_t i ;
    fbe_object_id_t last_object_id = 0;
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    
    metadata_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s LBA %llX \n", __FUNCTION__, (unsigned long long)block_operation->lba);
    master_sep_payload = fbe_transport_get_payload_ex(master_packet);
    master_control_operation = fbe_payload_ex_get_control_operation(master_sep_payload);

    /* We have to check if I/O was succesfull */
	if(status == FBE_STATUS_OK && block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
	{
		/* Get sg list */
		fbe_payload_ex_get_sg_list(sep_payload, &sg_list, NULL);
		
		fbe_database_get_last_system_object_id(&last_object_id);
		buffer = sg_list[0].address;
		
		for(i = 0; i <= last_object_id; i++){

			fbe_metadata_raw_mirror_io_cb.seq_numbers[i]= fbe_xor_lib_raw_mirror_sector_get_seq_num((fbe_sector_t*)buffer);
			
			buffer += FBE_METADATA_BLOCK_SIZE * blocks_per_object;
		}
		fbe_payload_control_set_status(master_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
		fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
	}else{
		fbe_payload_control_set_status(master_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(master_packet, FBE_STATUS_GENERIC_FAILURE, 0);
		metadata_trace(FBE_TRACE_LEVEL_ERROR,
					FBE_TRACE_MESSAGE_ID_INFO,
					"%s:nonpaged_system_clear failed\n",__FUNCTION__);
	}

    /* We are done ... */

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    /* Release block operation */
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    fbe_transport_destroy_packet(packet); /* This will not release the memory */

    /* Release the memory */
    //fbe_memory_request_get_entry_mark_free(master_memory_request, (fbe_memory_ptr_t *)&memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(master_memory_request);
    
    fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_metadata_raw_mirror_io_cb_init
 *****************************************************************
 * @brief
 *   initialize the global metadata raw-mirror IO state 
 *   tracking structure. 
 *
 * @param rw_mirr_cb   - global metadata raw-mirror tracking structure
 * 
 * @return status - indicating if the initialization success
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_metadata_raw_mirror_io_cb_init(fbe_metadata_raw_mirror_io_cb_t *rw_mirr_cb,fbe_block_count_t capacity)
{
    fbe_object_id_t last_object_id;    

    if (!rw_mirr_cb) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
	fbe_database_get_last_system_object_id(&last_object_id);

    /*initialize raw-mirror io control block fields*/
    fbe_zero_memory(rw_mirr_cb, sizeof (fbe_metadata_raw_mirror_io_cb_t));
    rw_mirr_cb->block_count = capacity;
    rw_mirr_cb->seq_numbers = (fbe_u64_t *)fbe_memory_ex_allocate((fbe_u32_t)capacity * sizeof (fbe_u64_t));
    if (!rw_mirr_cb->seq_numbers) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(rw_mirr_cb->seq_numbers, (fbe_u32_t)capacity * sizeof (fbe_u64_t));
    fbe_spinlock_init(&rw_mirr_cb->lock);
    return FBE_STATUS_OK;
}
/*!***************************************************************
 * @fn fbe_metadata_raw_mirror_io_cb_destroy
 *****************************************************************
 * @brief
 *   interface to release the resource of global metadata raw-mirror
 *   IO state tracking structure. 
 *
 * @param rw_mirr_cb   - global metadata raw-mirror IO state tracking
 *                       structure
 * 
 * @return status - indicating if the resource released successfully
 *
 * @version
 *    3/26/2012: zhangy - created
 *
 ****************************************************************/
fbe_status_t fbe_metadata_raw_mirror_io_cb_destroy(fbe_metadata_raw_mirror_io_cb_t *rw_mirr_cb)
{
    if (!rw_mirr_cb) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    if (rw_mirr_cb->seq_numbers) {
        fbe_memory_ex_release(rw_mirr_cb->seq_numbers);
    }
    
    fbe_spinlock_destroy(&rw_mirr_cb->lock);

    return FBE_STATUS_OK;
}
fbe_status_t
fbe_metadata_nonpaged_operation_write_verify(fbe_packet_t * packet)
{
	fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_object_id_t object_id;
	fbe_object_id_t last_object_id; 

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);

    if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE_VERIFY) {
        metadata_element = metadata_operation->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    }else{
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: invalid operation 0x%x.\n",
                       __FUNCTION__, metadata_operation->opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);			
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_database_get_last_system_object_id(&last_object_id);
	if(object_id <= last_object_id){
		return fbe_metadata_nonpaged_operation_system_write_verify(packet);
	}else{
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Attemp to do write verify on a non system object 0x%x.\n",
                       __FUNCTION__, object_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);		
		fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);			
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}
  
   
}
static fbe_status_t 
fbe_metadata_nonpaged_operation_system_write_verify(fbe_packet_t * packet)
{
    fbe_memory_request_t * memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_status_t status;
    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;
    fbe_memory_build_chunk_request(memory_request, 
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   1, /* one chunks */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
		                           (fbe_memory_completion_function_t)fbe_metadata_nonpaged_operation_system_write_verify_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   NULL /* completion_context */);

	fbe_transport_memory_request_set_io_master(memory_request, packet);

    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;   
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_system_write_verify_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{ 
	fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_metadata_element_t * metadata_element = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status;
    fbe_u8_t * buffer = NULL;
    fbe_lba_t lba;
    fbe_u32_t blocks_per_object;

    packet = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    if(metadata_operation->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE_VERIFY) {
        metadata_element = metadata_operation->metadata_element;
        object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
    }else{
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: invalid operation 0x%x.\n",
                       __FUNCTION__, metadata_operation->opcode);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* carve the packet */
    master_memory_header = memory_request->ptr;   

    new_packet = (fbe_packet_t *)master_memory_header->data;
	
    /* Zero memory */
    fbe_zero_memory(new_packet, sizeof(fbe_packet_t));
    //buffer = ((fbe_memory_header_t *)(master_memory_header->u.hptr.next_header))->data;
    buffer = master_memory_header->data + FBE_MEMORY_CHUNK_SIZE;

    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_transport_propagate_expiration_time(new_packet, packet);

    fbe_metadata_nonpaged_get_lba_block_count(object_id, &lba, &blocks_per_object);

    sg_list = (fbe_sg_element_t *)(buffer + FBE_METADATA_BLOCK_SIZE * blocks_per_object);
    sg_list[0].address = buffer;
    sg_list[0].count = FBE_METADATA_BLOCK_SIZE * blocks_per_object;
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    /* Zero memory */
    fbe_zero_memory(sg_list[0].address, sg_list[0].count);
    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, 1);
    /*object id is the index in the fbe_metadata_nonpaged_array
	 also, it is the block address on MD raw mirror. 
	 So the block address and object ID are identical*/
    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,
                                      lba,    /* LBA */
                                      blocks_per_object,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    fbe_copy_memory(sg_list[0].address, fbe_metadata_nonpaged_array[object_id].data, FBE_METADATA_NONPAGED_MAX_SIZE);
    //fbe_metadata_raw_mirror_calculate_checksum(sg_list[0].address,object_id,1,FBE_FALSE);
    fbe_metadata_nonpaged_calculate_checksum_for_one_object(sg_list[0].address, object_id, blocks_per_object, FBE_TRUE);

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_operation_system_write_verify_completion,NULL);
    fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);
    status = fbe_transport_add_subpacket(packet, new_packet);

    fbe_payload_ex_increment_block_operation_level(sep_payload);
    status = fbe_metadata_nonpaged_send_packet_to_raw_mirror(new_packet, object_id, FBE_TRUE);
    return status;
}

static fbe_status_t 
fbe_metadata_nonpaged_operation_system_write_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_status_t status;  
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_memory_request_t * master_memory_request = NULL;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_payload_ex_t * master_sep_payload = NULL;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    if( master_packet == NULL )
    {
	   metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
				      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
	                  "Critical error: %s packet %p; Master packet is NULL", 
	                  __FUNCTION__, packet);	   
	   return FBE_STATUS_GENERIC_FAILURE;
    }
	master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);
	fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);


    /* Set metadata operation status */
    master_sep_payload = fbe_transport_get_payload_ex(master_packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(master_sep_payload);

	/*1. Check the sub packet status. */
	if(status != FBE_STATUS_OK )
	{
		
        fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);
		fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
		metadata_trace(FBE_TRACE_LEVEL_INFO,
					FBE_TRACE_MESSAGE_ID_INFO,
					"%s:nonpaged_system_write_verify failed\n",__FUNCTION__);
		fbe_transport_complete_packet(master_packet);
		return FBE_STATUS_OK;
	}


	fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
	/*2. check bolck operation status and qualifier*/
	/*the caller should retry when 
	 1. block operation status is FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR
	 2. block operation status is FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS
	     and the qualifier is FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED*/
	/*we set the master packet metadata operation status to FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE 
	    This will tell the caller there is correctable/recoverable error 
	    and the caller should retry write/verify operation.
	*/
    switch(block_operation_status) {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
			if(block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED)
			{
				fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE);
			}else{
				fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_OK);
			}
            break;
		
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:		
			fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE);			
            break;
				
        default:
			fbe_payload_metadata_set_status(metadata_operation, FBE_PAYLOAD_METADATA_STATUS_FAILURE);			
            break;

	}
    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    /* Release block operation */
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);

    fbe_transport_destroy_packet(packet); /* This will not release the memory */

    /* Release the memory */
    //fbe_memory_request_get_entry_mark_free(master_memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(master_memory_request);
	
    fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_restore_completion
 *****************************************************************
 * @brief
 *   The complete routine of IO operation.
 *
 * 
 * @return status - indicating if the operation successful
 *
 * @version
 *    7/30/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
static fbe_status_t 
fbe_metadata_nonpaged_restore_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    fbe_memory_request_t * master_memory_request = NULL;    
    fbe_payload_control_operation_t *   control_operation = NULL;

    master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(packet);
    master_memory_request = fbe_transport_get_memory_request(master_packet);

    sep_payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(sep_payload);

    /* Check block operation status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    
    metadata_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s LBA %llX \n", __FUNCTION__, (unsigned long long)block_operation->lba);

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    /* Release block operation */
    fbe_payload_ex_release_block_operation(sep_payload, block_operation);
    fbe_transport_destroy_packet(packet); /* This will not release the memory */

    fbe_memory_free_request_entry(master_memory_request);

        
    /* Set control operation status */
    sep_payload = fbe_transport_get_payload_ex(master_packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    /* We have to check if I/O was succesfull and notify the caller via the master packet*/
    if(status == FBE_STATUS_OK && block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    } else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    fbe_transport_set_status(master_packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_restore_allocation_completion
 *****************************************************************
 * @brief
 *   This function is the complete routine of memory request of nonpaged
 *   metadata restore operation. It will copy data to memory and send IO
 *   to the metadata raw-mirror or metadata LUN.
 * 
 * @return status - indicating if the operation successful
 *
 * @version
 *    7/30/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
static fbe_status_t
fbe_metadata_nonpaged_restore_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_memory_header_t * master_memory_header = NULL;
    //fbe_memory_header_t * memory_header = NULL;
    fbe_u32_t i = 0;
    fbe_u32_t j = 0;
    fbe_u32_t index = 0;
    fbe_nonpaged_metadata_backup_restore_t* restore_p = NULL;
    fbe_u8_t                        *buffer = NULL;
    fbe_u32_t                       object_count = 0;
    fbe_object_id_t                 start_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 last_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 metadata_lun_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                       sg_list_count = 0;
	fbe_bool_t                  is_system_restore = FBE_FALSE;
    fbe_lba_t                       lba;
    fbe_u32_t                       blocks_per_object;

    packet = fbe_transport_memory_request_to_packet(memory_request);
    restore_p = (fbe_nonpaged_metadata_backup_restore_t *)context;
    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    master_memory_header = memory_request->ptr;

    /* Form SG lists */
    new_packet = (fbe_packet_t *)master_memory_header->data;
    //sg_list = (fbe_sg_element_t *)((fbe_u8_t *)master_memory_header->data + sizeof(fbe_packet_t)); /* I hope packet will never exceed 4K */

    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);

    fbe_transport_propagate_expiration_time(new_packet, packet);

    /*the first chunk for packet and sglist, other chunks for data buffer*/
#if 0
    sg_list_count = master_memory_header->number_of_chunks - 1;
    memory_header = master_memory_header->u.hptr.next_header;
    for(i = 0; i < sg_list_count; i++){
        sg_list[i].address = (fbe_u8_t *)memory_header->data;
        /*the maximum chunk size 64 blocks used when we allocated the memory*/
        sg_list[i].count = FBE_METADATA_BLOCK_SIZE * 64;
        fbe_zero_memory(sg_list[i].address, sg_list[i].count);
        memory_header = memory_header->u.hptr.next_header;
    }

    sg_list[i].address = NULL;
    sg_list[i].count = 0;
#endif
    fbe_metadata_nonpaged_setup_sg_list_from_memory_request(memory_request, &sg_list, &sg_list_count);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list, i);

    start_object_id = restore_p->start_object_id;
    object_count = restore_p->object_number;
    is_system_restore = fbe_metadata_nonpaged_is_system_drive(start_object_id, object_count);
#if 0
    fbe_database_get_last_system_object_id(&last_object_id);

	if (last_object_id >= (start_object_id + object_count - 1)){
		is_system_restore = FBE_TRUE;
	} else {
		is_system_restore = FBE_FALSE;
    }
#endif

    fbe_metadata_nonpaged_get_lba_block_count(start_object_id, &lba, &blocks_per_object);

    /* Copy the data to the 64-blocks chunk and calculate the checksum */
    index = 0;
    for(i = 0; i < sg_list_count; i ++){
        buffer = sg_list[i].address;
        for(j = 0; (j < (64 / blocks_per_object)) && ( index < object_count); j++){
            fbe_copy_memory(buffer, restore_p->buffer + index * restore_p->nonpaged_entry_size, 
                            restore_p->nonpaged_entry_size);
            fbe_metadata_nonpaged_calculate_checksum_for_one_object(buffer, start_object_id + index, blocks_per_object, is_system_restore);

		    index++;
            buffer += (FBE_METADATA_BLOCK_SIZE * blocks_per_object);
        }

        if(index >= object_count) {
            break;
        }
    }

    metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "Restore nonpaged metadata of objects start at object id 0x%x, count %d\n", 
                   start_object_id, object_count);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,
                                      lba,    /* LBA */
                                      object_count * blocks_per_object,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */
    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_restore_completion, restore_p);
    fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);

    /* Save resource priority so that we know to which priority to unwind when packet completes Write   
    * and we need to send packet back down again for another Write operation. 
    */
    fbe_transport_save_resource_priority(new_packet);

    status = fbe_transport_add_subpacket(packet, new_packet);

    /*We already verify and don't allow the restore operation overlaps between system and 
      non-system objects. so if one system objects hit the range (start_object, count), we
      can assume it a system objects restore operation*/
    if(last_object_id >= (start_object_id + object_count - 1)) {
        status = fbe_metadata_nonpaged_send_packet_to_raw_mirror(new_packet, start_object_id, FBE_FALSE);
    } else {
        /*issue IO to the metadata LUN*/
        fbe_database_get_metadata_lun_id(&metadata_lun_id);

        fbe_transport_set_address(  new_packet,
                                    FBE_PACKAGE_ID_SEP_0,
                                    FBE_SERVICE_ID_TOPOLOGY,
                                    FBE_CLASS_ID_INVALID,
                                    metadata_lun_id);

        packet->base_edge = NULL;
        fbe_payload_ex_increment_block_operation_level(sep_payload);
        status = fbe_topology_send_io_packet(new_packet);    
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_backup_restore
 *****************************************************************
 * @brief
 *   This function is used to read nonpaged metadata array in memory for backup
 *   or restore the nonpaged metadata from a buffer to the disk 
 * 
 * @return status - indicating if the operation successful
 *
 * @version
 *    7/30/2012: Jingcheng Zhang - created
 *
 ****************************************************************/
fbe_status_t fbe_metadata_nonpaged_backup_restore(fbe_packet_t * packet)
{
    fbe_memory_request_t           *memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    //fbe_block_count_t               block_count;
    fbe_memory_chunk_size_t         chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    fbe_memory_number_of_objects_t  chunk_number = 1;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t                 last_object_id;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_nonpaged_metadata_backup_restore_t* backup_restore_p = NULL;
    fbe_u32_t                           len = 0;
    fbe_u8_t                        *buffer = NULL;
    fbe_u32_t                       object_count = 0;
    fbe_object_id_t                 start_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u64_t                       index = 0;
    fbe_nonpaged_metadata_backup_restore_op_code_t    op_code = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &backup_restore_p);
    if (backup_restore_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
		       "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
	fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
	fbe_transport_set_status(packet, status, 0);
	fbe_transport_complete_packet(packet);
	return status;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_nonpaged_metadata_backup_restore_t)){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "%s %X len invalid\n", __FUNCTION__ , len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_database_get_last_system_object_id(&last_object_id);
    start_object_id = backup_restore_p->start_object_id;
    object_count = backup_restore_p->object_number;
    buffer = backup_restore_p->buffer;
    op_code = backup_restore_p->opcode;

    if(op_code == FBE_NONPAGED_METADATA_BACKUP_OBJECTS && FBE_MAX_METADATA_BACKUP_RESTORE_SIZE < (FBE_METADATA_NONPAGED_MAX_SIZE * object_count)) {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "%s Too many objects to backup exceed buffer size, obj num %d, buffer size %d \n", 
                       __FUNCTION__, object_count, FBE_MAX_METADATA_BACKUP_RESTORE_SIZE);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*system and non-system objects' nonpaged metadata need persist to different
      aread of system drives, so restore system and non-system objects nonpaged
      in one operation is not allowed*/
    if((op_code == FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_MEMORY || 
        op_code == FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_DISK) && 
       (last_object_id >= start_object_id && last_object_id < (start_object_id + object_count - 1))) {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "Can't restore system and non-system objects in one operation \n");

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*c4mirror
     */
    if (op_code == FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_DISK && 
        ((FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_FIRST >= start_object_id && 
         FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_FIRST < (start_object_id + object_count - 1)) ||
        (FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_LAST >= start_object_id &&
         FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_LAST < (start_object_id + object_count - 1))))
    {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Can't restore c4mirror and non-c4mirror objects in one operation \n"); 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*verify the restore nonpaged entry size smaller than maximum nonpaged size*/
    if((op_code == FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_MEMORY || 
        op_code == FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_DISK) && 
       (FBE_METADATA_NONPAGED_MAX_SIZE < backup_restore_p->nonpaged_entry_size)) {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "Restore nonpaged metadata entry size %d > max nonpaged size %d \n",
                        backup_restore_p->nonpaged_entry_size, FBE_METADATA_NONPAGED_MAX_SIZE);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*verify the restore buffer size*/
    if((op_code == FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_MEMORY || 
        op_code == FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_DISK) && 
       (FBE_MAX_METADATA_BACKUP_RESTORE_SIZE < backup_restore_p->nonpaged_entry_size * object_count)) {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "Not enough buffer size for nonpaged metadata restore, count %d, entry size %d, total size %d \n",
                       object_count, backup_restore_p->nonpaged_entry_size, FBE_MAX_METADATA_BACKUP_RESTORE_SIZE);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (op_code == FBE_NONPAGED_METADATA_BACKUP_OBJECTS) {
        /*it is a backup operation, get nonpaged metadata from memory directly*/
        for(index = start_object_id; index <= (start_object_id + object_count - 1); index ++) {
            fbe_copy_memory(buffer, fbe_metadata_nonpaged_array[index].data, FBE_METADATA_NONPAGED_MAX_SIZE);
            buffer += FBE_METADATA_NONPAGED_MAX_SIZE;
        }
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
    } else if (op_code == FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_MEMORY) {
        for(index = start_object_id; index <= (start_object_id + object_count - 1); index ++) {
            fbe_copy_memory(fbe_metadata_nonpaged_array[index].data, buffer, backup_restore_p->nonpaged_entry_size);
            buffer += backup_restore_p->nonpaged_entry_size;
        }
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
    } else if (op_code == FBE_NONPAGED_METADATA_RESTORE_OBJECTS_TO_DISK) {
        /*restore nonpaged metadata to disk, allocate memory and prepare IO operations*/
        memory_request = fbe_transport_get_memory_request(packet);
        memory_request_priority = fbe_transport_get_resource_priority(packet);
        memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

        /*caculate how much chunks we need for the restore operation*/
#if 0
        block_count = (fbe_block_count_t)(object_count); 
        /*use the maximum chunk size*/
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
        chunk_number = (fbe_memory_number_of_objects_t)((block_count + chunk_size_to_block_count(chunk_size) - 1) / chunk_size_to_block_count(chunk_size));

        chunk_number ++;  /* +1 for packet and sg_list */
#endif
        fbe_metadata_nonpaged_get_chunk_number(object_count, &chunk_size, &chunk_number);


        /* Allocate memory for data */      
        fbe_memory_build_chunk_request(memory_request, 
                                       chunk_size, 							
                                       chunk_number,
                                       memory_request_priority, 
                                       fbe_transport_get_io_stamp(packet),      
                                       (fbe_memory_completion_function_t)fbe_metadata_nonpaged_restore_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                       backup_restore_p /* completion_context */);
        fbe_transport_memory_request_set_io_master(memory_request,packet);

        status = fbe_memory_request_entry(memory_request);
        if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
        {   
            metadata_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                           __FUNCTION__, memory_request, status);
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
        }

    } else {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "Invalid nonpaged metadata backup/restore op code %d \n",
                       op_code);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_system_load_with_diskmask
 *****************************************************************
 * @brief
 *   TEST USE ONLY!!!This function only used for metadata raw mirror rebuild test. 
 *   It loads nonpaged MD from the specified disk 
 * 
 * @return status - indicating if the resource released successfully
 *
 * @version
 *    4/20/2012: zhangy - created
 *
 ****************************************************************/
fbe_status_t fbe_metadata_nonpaged_system_load_with_diskmask(fbe_packet_t * packet)
{
    fbe_memory_request_t           *memory_request = NULL;
    fbe_memory_request_priority_t   memory_request_priority = 0;
    fbe_block_count_t               block_count;
    fbe_memory_chunk_size_t         chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    fbe_memory_number_of_objects_t  chunk_number = 1;
    fbe_status_t                    status;
    //fbe_u32_t       aligned_rw_io_cb_size = 0;
    fbe_object_id_t last_object_id;
	fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
	fbe_metadata_nonpaged_system_load_diskmask_t* disk_bitmask_p = NULL;
    fbe_u32_t                           len = 0;

	payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &disk_bitmask_p);
	if (disk_bitmask_p == NULL)
	{
		status = FBE_STATUS_GENERIC_FAILURE;
		metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
			           "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet, status, 0);
		fbe_transport_complete_packet(packet);
		return status;
	}
    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_metadata_nonpaged_system_load_diskmask_t)){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "%s %X len invalid\n", __FUNCTION__ , len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_request = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;
    /*caculate how much chunks we need
	    we caculate the total count for system objects
	    by last system object id adding one*/
	
	fbe_database_get_last_system_object_id(&last_object_id);
    if (disk_bitmask_p->start_object_id == FBE_OBJECT_ID_INVALID)
    {
	    /* round up */
        disk_bitmask_p->start_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST;
        disk_bitmask_p->object_count = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST + 1;
    }
    else if (disk_bitmask_p->start_object_id + disk_bitmask_p->object_count > last_object_id)
    {
        disk_bitmask_p->object_count = last_object_id + 1 - disk_bitmask_p->start_object_id; 
    }

    if ((FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_FIRST >= disk_bitmask_p->start_object_id && 
         FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_FIRST < (disk_bitmask_p->start_object_id + disk_bitmask_p->object_count - 1)) ||
        (FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_LAST >= disk_bitmask_p->start_object_id &&
         FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_LAST < (disk_bitmask_p->start_object_id + disk_bitmask_p->object_count - 1)))
    {
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s request invalid, cannot load c4mirror and non-c4mirror in one operation\n", __FUNCTION__); 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    block_count = (fbe_block_count_t)(disk_bitmask_p->object_count);
#if 0
    /*add extra blocks for raw mirror IO control block and raw mirror system read context
          , assuming the size smaller than 3 blocks*/
    aligned_rw_io_cb_size = (sizeof (raw_mirror_operation_cb_t) + sizeof (raw_mirror_system_load_context_t)
                             + FBE_METADATA_BLOCK_SIZE - 1)/FBE_METADATA_BLOCK_SIZE;
    block_count += aligned_rw_io_cb_size;

    if (block_count < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET) {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    } else {
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }
    chunk_number = (fbe_memory_number_of_objects_t)((block_count + chunk_size_to_block_count(chunk_size) - 1) / chunk_size_to_block_count(chunk_size));
    /*TODO: we need to caculate how much chunk we need by considering
	   the packet , sg_list and other elements all together in order to save memory. 
	   For now, just add one chunk for packet and sg_list*/

    chunk_number ++;  /* +1 for packet and sg_list */
#endif
    fbe_metadata_nonpaged_get_chunk_number((fbe_u32_t)block_count, &chunk_size, &chunk_number);


    /* Allocate memory for data */      
    fbe_memory_build_chunk_request(memory_request, 
                                   chunk_size, 							
                                   chunk_number,
                                   memory_request_priority, 
                                   fbe_transport_get_io_stamp(packet),      
                                   (fbe_memory_completion_function_t)fbe_metadata_nonpaged_system_load_with_diskmask_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   disk_bitmask_p /* completion_context */);
	fbe_transport_memory_request_set_io_master(memory_request,packet);

    status = fbe_memory_request_entry(memory_request);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {   
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory_request: 0x%p allocate memory failed status: 0x%x\n",
                       __FUNCTION__, memory_request, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_OK;
}


static fbe_status_t
fbe_metadata_nonpaged_system_load_with_diskmask_allocation_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_packet_t * packet = NULL;
    fbe_packet_t * new_packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_sg_element_t * sg_list_p = NULL;
    fbe_u32_t       sg_list_count = 0;
    fbe_status_t status;
    fbe_memory_header_t * master_memory_header = NULL;
    //fbe_memory_header_t * current_memory_header = NULL;
    //fbe_object_id_t last_object_id;    
	fbe_u8_t * buffer;
	raw_mirror_operation_cb_t * raw_mirror_op_cb = NULL;
	raw_mirror_system_load_context_t * raw_mirror_system_load_context = NULL;
    fbe_memory_chunk_size_t memory_chunk_size;
    fbe_u32_t       sg_list_size = 0;
    fbe_u32_t       aligned_rw_io_cb_size = 0;
    //fbe_u32_t       i = 0;
	fbe_metadata_nonpaged_system_load_diskmask_t* disk_bitmask_p = NULL;
    fbe_object_id_t start_object_id;
    fbe_u32_t object_count;
    fbe_raw_mirror_t *nonpaged_raw_mirror_p = NULL;
    fbe_lba_t lba;
    fbe_u32_t blocks_per_object;

	disk_bitmask_p = (fbe_metadata_nonpaged_system_load_diskmask_t*)context;
	packet = fbe_transport_memory_request_to_packet(memory_request);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: memory request: 0x%p state: %d failed. \n",
                       __FUNCTION__, memory_request, memory_request->request_state);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*memory request done, issue IO now*/
	master_memory_header = fbe_memory_get_first_memory_header(memory_request);
	memory_chunk_size = master_memory_header->memory_chunk_size; /* valid in master memory header only */
	
	sg_list_size = (master_memory_header->number_of_chunks) * sizeof(fbe_sg_element_t);
	/* round up the sg list size to align with 64-bit boundary */
	sg_list_size = (sg_list_size + sizeof(fbe_u64_t) - 1) & 0xfffffff8; 
	
#if 0 /* This check is not needed: we allocate larger memory chunks now */
	if ((master_memory_header->memory_chunk_size) < (sizeof(fbe_packet_t)+sg_list_size)) {
			metadata_trace(FBE_TRACE_LEVEL_ERROR,
						   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						   "%s: chunk_size < packet+sg_lists: %d < %d \n \n",
						   __FUNCTION__,
						   memory_request->chunk_size,
						   (int)(sizeof(fbe_packet_t) + sg_list_size));
	
			fbe_memory_free_request_entry(memory_request);	
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_GENERIC_FAILURE;
	}
#endif
	
	/* fbe_database_get_last_system_object_id(&last_object_id); */

    start_object_id = disk_bitmask_p->start_object_id;
    object_count = disk_bitmask_p->object_count;

	aligned_rw_io_cb_size = (sizeof (raw_mirror_operation_cb_t) + sizeof (raw_mirror_system_load_context_t)
								 + FBE_METADATA_BLOCK_SIZE - 1)/FBE_METADATA_BLOCK_SIZE;
    /* carve the packet */
	buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
	new_packet = (fbe_packet_t *)buffer;
	fbe_zero_memory(new_packet, sizeof(fbe_packet_t));
    //buffer += sizeof(fbe_packet_t);
    buffer += FBE_MEMORY_CHUNK_SIZE;
	
	/* carve the sg list */
	sg_list_count = master_memory_header->number_of_chunks - 1; /* minus the first 1 chunk*/
	sg_list_p = (fbe_sg_element_t *)buffer;
	fbe_zero_memory(sg_list_p, sg_list_size);
    buffer += FBE_MEMORY_CHUNK_SIZE;
	
	/*
		  form raw-mirror IO control block and system read context.
		  The second memory chunk will divided into two
		  parts: raw-mirror IO cb and system load context placed at the begin, 
		  the remaining memory used for IO
	*/
	//fbe_memory_get_next_memory_header(master_memory_header, &current_memory_header);
	//master_memory_header = current_memory_header;
	
	/*the beginning part used for raw-mirror IO block*/
	//buffer = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header);
	/*allocate raw_mirror_operation_cb_t*/
	raw_mirror_op_cb = (raw_mirror_operation_cb_t*)buffer;
	buffer += sizeof(raw_mirror_operation_cb_t);
	fbe_zero_memory(raw_mirror_op_cb, sizeof(raw_mirror_operation_cb_t));
	/*allocate 
		raw_mirror_system_load_context_t*/		
	raw_mirror_system_load_context = (raw_mirror_system_load_context_t*)buffer;
	fbe_zero_memory(raw_mirror_system_load_context, sizeof(raw_mirror_system_load_context_t));
	
#if 0
	/*ramining memory of second memory chunk used for IO*/
	sg_list_p[0].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header) + FBE_METADATA_BLOCK_SIZE * aligned_rw_io_cb_size;
	sg_list_p[0].count = FBE_METADATA_BLOCK_SIZE * (chunk_size_to_block_count(memory_chunk_size) - aligned_rw_io_cb_size);
	fbe_zero_memory(sg_list_p[0].address, sg_list_p[0].count);
	
		/* Form SG lists from remaing memory chunk*/
	for (i = 1; i < sg_list_count; i++) {
			/* move to the next chunk */
		fbe_memory_get_next_memory_header(master_memory_header, &current_memory_header);
		master_memory_header = current_memory_header;
	
		sg_list_p[i].address = (fbe_u8_t *)fbe_memory_header_to_data(current_memory_header);
		sg_list_p[i].count = FBE_METADATA_BLOCK_SIZE * chunk_size_to_block_count(memory_chunk_size);
			
		fbe_zero_memory(sg_list_p[i].address, sg_list_p[i].count);
	}
	
    sg_list_p[sg_list_count].address = NULL;
    sg_list_p[sg_list_count].count = 0;
#endif
    fbe_metadata_nonpaged_setup_sg_list_from_memory_request(memory_request, &sg_list_p, &sg_list_count);

    if (fbe_database_is_object_c4_mirror(start_object_id))
    {
        fbe_c4_mirror_get_nonpaged_metadata_config((void **)(&nonpaged_raw_mirror_p), NULL, NULL); 
    }
    else
    {
        nonpaged_raw_mirror_p = &fbe_metadata_nonpaged_raw_mirror;
    }
    /*initialize the raw-mirror IO control block.
	   It will be used in system nonpaged  metadata load error handling*/
    raw_mirror_init_operation_cb(raw_mirror_op_cb, 
                                 start_object_id,
                                 object_count,
                                 sg_list_p,
                                 sg_list_count,
                                 nonpaged_raw_mirror_p,
                                 disk_bitmask_p->disk_bitmask);
    /*initialize the raw-mirror system load context.
	   It will be used in system nonpaged metadata load completion funtion
	   to copy the read data and issue single block mode read if neccessary*/
	raw_mirror_init_system_load_context(raw_mirror_system_load_context, 
    					start_object_id,
                                        object_count,
                                        sg_list_p,
                                        sg_list_count,
                                        raw_mirror_op_cb,
                                        disk_bitmask_p->disk_bitmask);

    raw_mirror_set_error_retry(raw_mirror_op_cb,
                               FBE_TRUE);


    fbe_transport_initialize_sep_packet(new_packet);    
    sep_payload = fbe_transport_get_payload_ex(new_packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(sep_payload);
    fbe_zero_memory(&raw_mirror_op_cb->error_verify_report, sizeof (fbe_raid_verify_raw_mirror_errors_t));
    fbe_payload_ex_set_verify_error_count_ptr(sep_payload, &raw_mirror_op_cb->error_verify_report);

    fbe_transport_propagate_expiration_time(new_packet, packet);

    /* Propagate the io_stamp */
    fbe_transport_set_io_stamp_from_master(packet, new_packet);

    /* Set sg list */
    fbe_payload_ex_set_sg_list(sep_payload, sg_list_p, sg_list_count);

    fbe_metadata_nonpaged_get_lba_block_count(start_object_id, &lba, &blocks_per_object);

    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      lba,    /* LBA */
                                      object_count * blocks_per_object,    /* blocks */
                                      FBE_METADATA_BLOCK_SIZE,    /* block size */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet, fbe_metadata_nonpaged_system_load_read_completion, raw_mirror_system_load_context);
	/*Set complete function for error handling*/
    fbe_transport_set_completion_function(new_packet, fbe_raw_mirror_utils_error_handling_read_completion, raw_mirror_op_cb);

	fbe_transport_set_resource_priority(new_packet, memory_request->priority + FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST);
    status = fbe_transport_add_subpacket(packet, new_packet);

    fbe_payload_ex_increment_block_operation_level(sep_payload);
    status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(nonpaged_raw_mirror_p, new_packet,disk_bitmask_p->disk_bitmask);

    return status;
}

static fbe_status_t fbe_metadata_nonpaged_load_single_system_object(fbe_packet_t *packet, 
                                                                               raw_mirror_system_load_context_t *raw_mirr_system_load_context)
{
    
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t *block_operation = NULL;
    fbe_sg_element_t * sg_list_p = NULL;
	fbe_u32_t i;
	fbe_u32_t sg_list_count = 0;	
	raw_mirror_operation_cb_t* raw_mirror_cb_p = NULL;
    fbe_lba_t lba;
    fbe_u32_t blocks_per_object;

	payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
	raw_mirror_cb_p = raw_mirr_system_load_context->raw_mirror_cb_p;
    /* Zero memory */
    sg_list_p = raw_mirr_system_load_context->sg_list;
	sg_list_count = raw_mirr_system_load_context->sg_list_count;
    /* Zero memory */
    for (i = 0; i < sg_list_count; i++) {        
        fbe_zero_memory(sg_list_p[i].address, sg_list_p[i].count);
    }
    sg_list_p[sg_list_count].address = NULL;
    sg_list_p[sg_list_count].count = 0;
    fbe_zero_memory(&raw_mirr_system_load_context->error_verify_report,sizeof(fbe_raid_verify_raw_mirror_errors_t));
	/* Set sg list */
    fbe_payload_ex_set_sg_list(payload, raw_mirr_system_load_context->sg_list, 
                               raw_mirr_system_load_context->sg_list_count);
    fbe_payload_ex_set_verify_error_count_ptr(payload, &raw_mirr_system_load_context->error_verify_report);
    fbe_metadata_nonpaged_get_lba_block_count((fbe_object_id_t)(raw_mirr_system_load_context->current_lba), &lba, &blocks_per_object);
    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      lba,    /* LBA */
                                      blocks_per_object,
                                      FBE_METADATA_BLOCK_SIZE,    /* block size: 520  */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */
	
	/*single block mode read don't need retry. 
		  it will set the NP state to INVALID instead of retry*/
	raw_mirror_set_error_retry(raw_mirror_cb_p,FBE_FALSE);
	raw_mirror_cb_p->block_count = 1;
	raw_mirror_cb_p->lba = raw_mirr_system_load_context->current_lba;
	fbe_transport_set_completion_function(packet, fbe_raw_mirror_utils_error_handling_read_completion,raw_mirror_cb_p);
    fbe_payload_ex_increment_block_operation_level(payload);
    status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(raw_mirror_cb_p->raw_mirror_p, packet,raw_mirr_system_load_context->disk_bitmask);
    return status;
}

static fbe_status_t
fbe_metadata_set_nonpaged_metadata_state_in_memory(fbe_base_config_nonpaged_metadata_state_t np_state,fbe_object_id_t in_object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_u64_t    np_state_offset;
	fbe_base_config_nonpaged_metadata_state_t  np_state_data ;
	
	if(in_object_id >= FBE_METADATA_NONPAGED_MAX_OBJECTS)
	{
	  metadata_trace(FBE_TRACE_LEVEL_INFO,
				     FBE_TRACE_MESSAGE_ID_INFO,
				     "nonpaged load single object failed: invalid system object id:0x%x\n",
                     in_object_id);
	  return status;
	}
	np_state_data = np_state;
	np_state_offset = (fbe_u64_t)(&((fbe_base_config_nonpaged_metadata_t*)0)->nonpaged_metadata_state);
	fbe_copy_memory(fbe_metadata_nonpaged_array[in_object_id].data + np_state_offset,
					&np_state_data, 
					sizeof(fbe_base_config_nonpaged_metadata_state_t));
	return FBE_STATUS_OK;

}
/*!***************************************************************
 * @fn fbe_raw_mirror_utils_error_handling_read_completion
 *****************************************************************
 * @brief
 *   complete function for system NP load, it does the following thing
 *  1. check packet status, if not ok, keep retry-read until success
 *  2. if packet status == OK,check block operation status, 
 *	1) if FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, 
 *		 return to next completion function to swith to block-by-block mode read;
 *	2) if FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
 *		 check the block operation status qualifier and error report, if error exist,
 *		 issue out-of-band raw mirror rebuild request to fix the correctable error.
 *	3) any other failure status:
 *		 keep retry-read until success
 * @param packet   - the IO packet
 * @param context  - context of the complete function run
 * 
 * @return status - indicating if the complete function success
 *
 * @version
 *    3/22/2012: zhangy - created
 *
 ****************************************************************/
fbe_status_t 
fbe_raw_mirror_utils_error_handling_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_status_t packet_status;
    fbe_payload_block_operation_status_t  block_operation_status;
    fbe_payload_block_operation_qualifier_t  block_operation_qualifier;
    raw_mirror_operation_cb_t *raw_mirror_op_cb = NULL;
    fbe_raid_verify_raw_mirror_errors_t *error_verify_report = NULL;
    fbe_bool_t  flush_needed = FBE_FALSE;

    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);
    /*retrieve the context info*/
    raw_mirror_op_cb = (raw_mirror_operation_cb_t *)context;
    if (!raw_mirror_op_cb) {
		metadata_trace(FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
					   "%s: failed by no context\n", 
					   __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;/*this will call the next completion function in the in packet*/
    }


    /* Check block operation status */
    packet_status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    fbe_payload_ex_get_verify_error_count_ptr(payload, (void **)&error_verify_report);

    /*now starts the read error handling: 
      1. check packet status, 
           1) keep retry-read until success when caller set retryable.
      2. if packet status == OK,check block operation status, 
         1) if FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
              check the block operation status qualifier and error report, if error exist,
              issue out-of-band raw mirror rebuild request to fix the correctable error.
         2) any other failure status:
              keep retry-read until success when caller set retryable
      */
	if (packet_status != FBE_STATUS_OK) {
		/*Retry Read operation, we will reused the packet inorder to 
		    get its original control operation and contriol operation completion function*/
		if(raw_mirror_op_cb->is_read_retry == FBE_TRUE){
            fbe_transport_packet_clear_callstack_depth(packet);/* reset this since we reuse the packet */
			fbe_payload_ex_release_block_operation(payload, block_operation);
			raw_mirror_retry_read_blocks(packet,raw_mirror_op_cb);
			return FBE_STATUS_MORE_PROCESSING_REQUIRED;
		}
	}
	if(block_operation_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
	{
        flush_needed = FBE_FALSE;
	
         /*check whether we shuold enter read flush operation */
		 /*1. magic and sequence errors*/
		 if (error_verify_report != NULL) {
			 if (error_verify_report->raw_mirror_seq_bitmap != 0 ||
				 error_verify_report->raw_mirror_magic_bitmap != 0) {
				 metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
					 "raw_mirr_read error report:, seq_bitmap %x, magic_bitmap %x\n",
					 error_verify_report->raw_mirror_seq_bitmap,
					 error_verify_report->raw_mirror_magic_bitmap);
					 /*magic number or sequence numbers don't match, need flush*/
					 flush_needed = FBE_TRUE;
			 }
		 } 
		 
		 if (block_operation_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH) {
			 	metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
							"raw_mirr_read sequence not match");
				 flush_needed = FBE_TRUE;
		 }
		 
		 
		 /*in the read error handling case,  
		   we issue out-of-band rebuild request to fix the correctable error
		   */
		 if (flush_needed) {			 
			 /*issue out-of-band rebuild request*/
			 //fbe_metadata_notify_raw_mirror_rebuild(raw_mirror_op_cb->lba,
			 //                                       raw_mirror_op_cb->block_count);
		 }
	}else
	{/*Retry Read operation*/
		if(block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
		{
			if(raw_mirror_op_cb->is_read_retry == FBE_TRUE){
				fbe_payload_ex_release_block_operation(payload, block_operation);
				raw_mirror_retry_read_blocks(packet,raw_mirror_op_cb);
				return FBE_STATUS_MORE_PROCESSING_REQUIRED;
			}
		}
			
	}

	/*Read success will return ok to call the next completion function*/
	return FBE_STATUS_OK;
}
fbe_status_t raw_mirror_retry_read_blocks(fbe_packet_t *packet, 
                                            raw_mirror_operation_cb_t *raw_mirr_op_cb)
{
    
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t *block_operation = NULL;
    fbe_sg_element_t * sg_list_p = NULL;
    fbe_u32_t       sg_list_count = 0;
    fbe_u32_t       i = 0;
    fbe_lba_t lba;
    fbe_u32_t blocks_per_object;

	payload = fbe_transport_get_payload_ex(packet);
    block_operation =  fbe_payload_ex_allocate_block_operation(payload);
    sg_list_p = raw_mirr_op_cb->sg_list;
	sg_list_count = raw_mirr_op_cb->sg_list_count;
    /* Zero memory */
    for (i = 0; i < sg_list_count; i++) {        
        fbe_zero_memory(sg_list_p[i].address, sg_list_p[i].count);
    }
    sg_list_p[sg_list_count].address = NULL;
    sg_list_p[sg_list_count].count = 0;

    /* Set sg list */
    fbe_payload_ex_set_sg_list(payload, sg_list_p, 
                               sg_list_count);
    /*set raw-mirror error verify report to record the IO errors on 
      raw-way-mirror*/
    fbe_zero_memory(&raw_mirr_op_cb->error_verify_report, sizeof (fbe_raid_verify_raw_mirror_errors_t));
    fbe_payload_ex_set_verify_error_count_ptr(payload, &raw_mirr_op_cb->error_verify_report);
    fbe_metadata_nonpaged_get_lba_block_count((fbe_object_id_t)(raw_mirr_op_cb->lba), &lba, &blocks_per_object);
    fbe_payload_block_build_operation(block_operation,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      lba,    /* LBA */
                                      (raw_mirr_op_cb->block_count) * blocks_per_object,
                                      RAW_MIRROR_BLOCK_SIZE,    /* block size: 520  */
                                      1,    /* optimum block size */
                                      NULL);    /* preread descriptor */

    /* Set completion function */
    fbe_transport_set_completion_function(packet, fbe_raw_mirror_utils_error_handling_read_completion, raw_mirr_op_cb);
    fbe_payload_ex_increment_block_operation_level(payload);

    status = fbe_raw_mirror_ex_send_io_packet_to_raid_library(raw_mirr_op_cb->raw_mirror_p, packet,raw_mirr_op_cb->disk_bitmask);
    return status;
}
/*!***************************************************************
 * @fn fbe_metadata_set_block_seq_number
 *****************************************************************
 * @brief
 *   This function set the seq_num in global array. 
 *   The globle array record the seq_num for every block in raw mirror
 *   
 * @param in_block_p -- the pionter to the block read from raw mirror
 *               in_object_id -- It is the index for the gloable array
 * @return status - indicating if the intialization success
 *
 * @version
 *    5/25/2012: Zhangy26 - created
 *
 ****************************************************************/

static fbe_status_t
fbe_metadata_set_block_seq_number(fbe_u8_t *  in_block_p,
                                                fbe_object_id_t in_object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	
	if(in_block_p == NULL)
	{
	  metadata_trace(FBE_TRACE_LEVEL_ERROR,
				     FBE_TRACE_MESSAGE_ID_INFO,
				     "%s:failed: null pionter.\n",
				     __FUNCTION__);
	  return status;
	}
	fbe_metadata_raw_mirror_io_cb.seq_numbers[in_object_id]= fbe_xor_lib_raw_mirror_sector_get_seq_num((fbe_sector_t*)in_block_p);
	return FBE_STATUS_OK;

}
fbe_status_t fbe_metadata_nonpaged_disable_raw_mirror(void)
{
    fbe_status_t status;
    status = fbe_raw_mirror_disable(&fbe_metadata_nonpaged_raw_mirror);
    return status;
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_save_sequence_number_for_system_object
 *****************************************************************
 * @brief
 *   Given the cmi message header and sequence number, save the sequence 
 *   number into the global array if the object is a system object
 *   
 * @param cmi_msg_header -- cmi message header containing object_id and flags
 *        seq_number -- the new sequence number to save
 * @return None
 *
 * @version
 *    6/10/2013: Deanna Heng- created
 *
 ****************************************************************/
static void fbe_metadata_nonpaged_save_sequence_number_for_system_object(fbe_metadata_cmi_message_header_t * cmi_msg_header, 
                                                                         fbe_u64_t seq_number) 
{

    /* Copy the sequence number from the active SP into memory for system objects */
    if (fbe_private_space_layout_object_id_is_system_object(cmi_msg_header->object_id) &&
        cmi_msg_header->metadata_operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_SEQ_NUM) {
        fbe_metadata_raw_mirror_io_cb.seq_numbers[cmi_msg_header->object_id] = seq_number;
    }
}
/*!***************************************************************
 * @fn fbe_metadata_nonpaged_get_blocks_per_object
 *****************************************************************
 * @brief
 *   This function get the blocks per object in LUNs. 
 *   
 * @param None
 *
 * @return fbe_u32_t
 *
 * @version
 *    10/22/2014: Lili Chen - created
 *
 ****************************************************************/
static __forceinline fbe_u32_t fbe_metadata_nonpaged_get_blocks_per_object(void)
{
    /* After ICA, each object will have 8 blocks */
    return blocks_per_object;
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_is_system_drive
 *****************************************************************
 * @brief
 *   This function checks whether the drives are system drives. 
 *   
 * @param start_object_id    - start object id
 * @param object_count       - the object count
 *
 * @return fbe_bool_t
 *
 * @version
 *    10/22/2014: Lili Chen - created
 *
 ****************************************************************/
static __forceinline fbe_bool_t 
fbe_metadata_nonpaged_is_system_drive(fbe_object_id_t start_object_id, fbe_u32_t object_count)
{
    fbe_object_id_t                 last_object_id = FBE_OBJECT_ID_INVALID;
    
    fbe_database_get_last_system_object_id(&last_object_id);

	if (last_object_id >= (start_object_id + object_count - 1)) {
		return FBE_TRUE;
	} else {
		return FBE_FALSE;
    }
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_calculate_checksum_for_one_object
 *****************************************************************
 * @brief
 *   This function calculates checksum for 8 blocks.
 *   
 * @param buffer             - Pointer to the buffer
 * @param object_id          - the object id
 * @param is_system_drive    - whether it is a system drive
 *
 * @return fbe_bool_t
 *
 * @version
 *    10/22/2014: Lili Chen - created
 *
 ****************************************************************/
static __forceinline void 
fbe_metadata_nonpaged_calculate_checksum_for_one_object(fbe_u8_t *buffer, 
                                                        fbe_object_id_t object_id, 
                                                        fbe_u32_t blocks_per_object, 
                                                        fbe_bool_t is_system_drive)
{
    fbe_u32_t i;

    for(i = 0; i < blocks_per_object; i++) {
        if (is_system_drive) {
            fbe_metadata_raw_mirror_calculate_checksum(buffer, object_id, 1, FBE_FALSE);
        } else {
            fbe_metadata_calculate_checksum(buffer, 1);
        }
        buffer += FBE_METADATA_BLOCK_SIZE;
    }

    return;
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_get_chunk_number
 *****************************************************************
 * @brief
 *   This function gets the chunk number/size for a memory request.
 *   
 * @param object_count       - object count
 * @param chunk_size_p       - Pointer to the chunk size
 * @param chunk_number_p     - Pointer to the chunk number
 *
 * @return fbe_bool_t
 *
 * @version
 *    10/22/2014: Lili Chen - created
 *
 ****************************************************************/
static __forceinline fbe_status_t 
fbe_metadata_nonpaged_get_chunk_number(fbe_u32_t object_count,
                                       fbe_memory_chunk_size_t *chunk_size_p,
                                       fbe_memory_number_of_objects_t *chunk_number_p)
{
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();
    fbe_memory_chunk_size_t         chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    fbe_memory_number_of_objects_t  chunk_number;
	fbe_u32_t objects_per_chunk;
    
    objects_per_chunk = chunk_size_to_block_count(chunk_size)/blocks_per_object;
    chunk_number = (fbe_memory_number_of_objects_t)((object_count + objects_per_chunk - 1) / objects_per_chunk);
    chunk_number ++;  /* +1 for packet and sg_list */
    
    *chunk_size_p = chunk_size;
    *chunk_number_p = chunk_number;
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_metadata_nonpaged_setup_sg_list_from_memory_request
 *****************************************************************
 * @brief
 *   This function sets up the sg list after memory is allocated.
 *   
 * @param memory_request       - Pointer to memory request
 * @param sg_list_pp           - Pointer to sg list
 * @param sg_list_count_p      - Pointer to sg list count
 *
 * @return fbe_bool_t
 *
 * @version
 *    10/22/2014: Lili Chen - created
 *
 ****************************************************************/
fbe_status_t 
fbe_metadata_nonpaged_setup_sg_list_from_memory_request(fbe_memory_request_t * memory_request,
                                                      fbe_sg_element_t ** sg_list_pp,
                                                      fbe_u32_t *sg_list_count_p)
{
    fbe_sg_element_t    * sg_list = NULL;
    fbe_u32_t             sg_list_count = 0;
    fbe_memory_header_t * master_memory_header = NULL;
    fbe_memory_header_t * memory_header = NULL;
    fbe_u8_t            * buffer;
    fbe_u32_t i = 0;
    
    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
    sg_list = (fbe_sg_element_t *)(buffer + FBE_MEMORY_CHUNK_SIZE);

    /*the first chunk for packet and sglist, other chunks for data buffer*/
    sg_list_count = master_memory_header->number_of_chunks - 1;
    memory_header = master_memory_header->u.hptr.next_header;
    for(i = 0; i < sg_list_count; i++){
        sg_list[i].address = (fbe_u8_t *)memory_header->data;
        /*the maximum chunk size 64 blocks used when we allocated the memory*/
        sg_list[i].count = FBE_METADATA_BLOCK_SIZE * 64;
        fbe_zero_memory(sg_list[i].address, sg_list[i].count);
        memory_header = memory_header->u.hptr.next_header;
    }

    sg_list[i].address = NULL;
    sg_list[i].count = 0;
    
    *sg_list_pp = sg_list;
    if (sg_list_count_p) {
        *sg_list_count_p = sg_list_count;
    }
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *  fbe_metadata_nonpaged_set_blocks_per_object
 ******************************************************************************
 *
 * @brief
 *    This function set the blocks of the nonpaged metadata for each object. 
 *
 * @param - packet - Pointer to the original packet
 *
 * @return  fbe_status_t
 *
 * @author
 *    03/31/2012 - Created. Jibing Dong
 *
 *****************************************************************************/
fbe_status_t fbe_metadata_nonpaged_set_blocks_per_object(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_u32_t                           len = 0;
    fbe_u32_t * set_blocks = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &set_blocks); 
    if(set_blocks == NULL){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_u32_t)){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "%s %X len invalid\n", __FUNCTION__ , len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    blocks_per_object = *set_blocks;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *  fbe_metadata_nonpaged_get_lba_block_count
 ******************************************************************************
 *
 * @brief
 *    This function get the lba and block count for the nonpaged of the object. 
 *
 * @param - object_id - The object id 
 * @param - lba_p - Pointer to the lba
 * @param - block_count_p - Pointer to the block count 
 *
 * @return  fbe_status_t
 *
 * @author
 *    03/31/2015 - Created. Jibing Dong
 *
 *****************************************************************************/
static __forceinline fbe_status_t fbe_metadata_nonpaged_get_lba_block_count(fbe_object_id_t object_id, fbe_lba_t *lba_p, fbe_u32_t *block_count_p)
{
    fbe_u32_t blocks_per_object = fbe_metadata_nonpaged_get_blocks_per_object();

    if (fbe_database_is_object_c4_mirror(object_id))
    {
        fbe_lba_t start_lba;
        fbe_c4_mirror_get_nonpaged_metadata_config(NULL, &start_lba, NULL);
        *lba_p = (object_id - FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_FIRST) * blocks_per_object + start_lba;
        *block_count_p = blocks_per_object;
    }
    else
    {
        *lba_p = object_id * blocks_per_object;
        *block_count_p = blocks_per_object;
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_metadata_nonpaged_get_data_ptr_by_object_id
 ******************************************************************************
 *
 * @brief
 *    This function get the raid group rebuild info from its nonpaged metadata. 
 *
 * @return  fbe_status_t
 *
 * @author
 *    08/03/2015 - Created. gaoh1
 *
 *****************************************************************************/
fbe_status_t fbe_metadata_nonpaged_get_nonpaged_metadata_data_ptr(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_u32_t                           len = 0;
    fbe_database_get_nonpaged_metadata_data_ptr_t *get_nonpaged_metadata = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_nonpaged_metadata); 
    if(get_nonpaged_metadata == NULL){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_database_get_nonpaged_metadata_data_ptr_t)){
        metadata_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                        "%s %X len invalid\n", __FUNCTION__ , len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    get_nonpaged_metadata->data_ptr = fbe_metadata_nonpaged_array[get_nonpaged_metadata->object_id].data;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

