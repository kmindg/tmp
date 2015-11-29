/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "base_object_private.h"
#include "fbe_base_config_private.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi.h"
#include "fbe_database.h"

static fbe_status_t fbe_base_config_metadata_init_memory_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_init_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_update_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_set_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_clear_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_read_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_paged_set_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_paged_clear_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_paged_get_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_paged_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t base_config_metadata_nonpaged_set_checkpoint_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_config_metadata_nonpaged_incr_checkpoint_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_config_metadata_nonpaged_write_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_config_get_np_distributed_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_config_release_np_distributed_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_post_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_start_persist(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_start_post_persist(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_metadata_nonpaged_perform_post_persist(fbe_base_config_t * base_config, fbe_packet_t * packet);

static fbe_status_t
fbe_base_config_metadata_nonpaged_perform_write(fbe_base_config_t * base_config,
                                                fbe_packet_t * packet,
                                                fbe_u64_t   metadata_offset,
                                                fbe_u8_t *  metadata_record_data,
                                                fbe_u32_t metadata_record_data_size,
                                                fbe_payload_metadata_operation_flags_t metadata_operation_flags);
static fbe_status_t
fbe_base_config_metadata_nonpaged_perform_set_bits(fbe_base_config_t * base_config,
                                                   fbe_packet_t * packet,
                                                   fbe_u64_t   metadata_offset,
                                                   fbe_u8_t *  metadata_record_data,
                                                   fbe_u32_t metadata_record_data_size,
                                                   fbe_u64_t   metadata_repeat_count,
                                                   fbe_payload_metadata_operation_flags_t flags);
static fbe_status_t
fbe_base_config_metadata_nonpaged_perform_set_checkpoint(fbe_base_config_t * base_config,
                                                         fbe_packet_t * packet,
                                                         fbe_u64_t   metadata_offset,
                                                         fbe_u64_t   second_metadata_offset,
                                                         fbe_u64_t   checkpoint,
                                                         fbe_payload_metadata_operation_flags_t flags);

static fbe_status_t base_config_ex_metadata_nonpaged_write_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_base_config_set_nonpaged_metadata_state_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_base_config_persist_nonpaged_metadata_state_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context);



fbe_status_t 
fbe_base_config_metadata_init_memory(fbe_base_config_t * base_config,
                                     fbe_packet_t * packet,
                                     void * metadata_memory_ptr,
                                     void * metadata_memory_peer_ptr, 
                                     fbe_u32_t metadata_memory_size)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_sep_version_header_t *version_header = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation = fbe_payload_ex_allocate_metadata_operation(sep_payload);

    base_config->metadata_element.metadata_memory.memory_ptr = metadata_memory_ptr;
    base_config->metadata_element.metadata_memory.memory_size = metadata_memory_size;

    base_config->metadata_element.metadata_memory_peer.memory_ptr = metadata_memory_peer_ptr;
    base_config->metadata_element.metadata_memory_peer.memory_size = metadata_memory_size;
    
    /*SEP upgrade may cause different version metadata memory sync via CMI between SP; correct
     version header need to be initialized to carry correct size. The size will be used for handling 
     version difference when dispatching metadata memory update by metadata service*/
    version_header = (fbe_sep_version_header_t *)metadata_memory_ptr;
    if (version_header != NULL) {
        fbe_zero_memory(version_header, sizeof (fbe_sep_version_header_t));
        version_header->size = metadata_memory_size;
    }

    fbe_payload_metadata_build_init_memory(metadata_operation, &base_config->metadata_element);


    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_init_memory_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status = fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_init_memory_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_metadata_status_t metadata_status;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation = fbe_payload_ex_get_metadata_operation(sep_payload);
    
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    /* Set control_status */
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    } else if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_BUSY){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_BUSY);
    }else{
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    return FBE_STATUS_OK;
}
fbe_status_t
fbe_base_config_metadata_nonpaged_zero(fbe_base_config_t * base_config_p)
{
    fbe_u32_t nonpaged_bytes;
    fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p = NULL;
    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
    if(base_config_nonpaged_metadata_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "bcfg_md_np_inited Invalid nonpaged md ptr, base_config_p:0x%p\n",
                              base_config_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_base_config_get_nonpaged_metadata_size(base_config_p, &nonpaged_bytes);

    fbe_zero_memory(base_config_nonpaged_metadata_p, nonpaged_bytes);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_nonpaged_init(fbe_base_config_t * base_config, fbe_u32_t data_size, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    base_config->metadata_element.nonpaged_record.data_ptr = NULL;
    base_config->metadata_element.nonpaged_record.data_size = data_size;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);

    fbe_payload_metadata_build_nonpaged_init(metadata_operation, &base_config->metadata_element);

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_nonpaged_init_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_nonpaged_init_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_control_operation_t * control_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    /* Get control status */
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    /* Set control_status */
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    } else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_nonpaged_is_initialized(fbe_base_config_t * base_config_p, fbe_bool_t * is_metadata_initialized_p)
{
    fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p = NULL;
    fbe_object_id_t object_id;
    fbe_generation_code_t   generation_number;

    /* initialize it as false. */
    *is_metadata_initialized_p = FBE_FALSE;

    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
    if(base_config_nonpaged_metadata_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "bcfg_md_np_inited Invalid nonpaged md ptr, base_config_p:0x%p\n",
                              base_config_p);

        return FBE_STATUS_OK;
    }

    fbe_base_object_get_object_id((fbe_base_object_t *) base_config_p, &object_id);
    fbe_base_config_get_generation_num(base_config_p, &generation_number);

    /* ig magic number of the nonpaged metadata is not valid then return false. */
    if((base_config_nonpaged_metadata_p->object_id == object_id) &&
       (base_config_nonpaged_metadata_p->generation_number == generation_number))
    {
        *is_metadata_initialized_p = FBE_TRUE;
    } else if(base_config_nonpaged_metadata_p->object_id != 0 || base_config_nonpaged_metadata_p->generation_number != 0) {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "bcfg_md_np_inited Wrong NP sig: objid 0x%x != 0x%x; gen %llX != %llX\n",
                              base_config_nonpaged_metadata_p->object_id, object_id, 
                              (unsigned long long)base_config_nonpaged_metadata_p->generation_number, (unsigned long long)generation_number);
    
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_is_initialized(fbe_base_config_t * base_config_p, fbe_bool_t * is_metadata_initialized_p)
{
    fbe_bool_t is_nonpaged_initialized = FBE_FALSE;
    fbe_bool_t is_nonpaged_state_initialize = FBE_FALSE;

    fbe_base_config_metadata_nonpaged_is_initialized(base_config_p, &is_nonpaged_initialized);
    fbe_base_config_metadata_is_nonpaged_state_initialized(base_config_p, &is_nonpaged_state_initialize);

    *is_metadata_initialized_p = FBE_FALSE;
    /* we want to make sure both nonpaged and paged data is initialized */
    if(is_nonpaged_initialized && is_nonpaged_state_initialize)
    {
        *is_metadata_initialized_p = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_is_nonpaged_state_initialized(fbe_base_config_t * base_config_p, fbe_bool_t * is_magic_num_initialized_p)
{
    fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p = NULL;

    /* initialize it as false. */
    *is_magic_num_initialized_p = FBE_FALSE;

    /* get the magic number of nonpaged metadata. */
    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
    if(base_config_nonpaged_metadata_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "bcfg_md_np_inited Invalid nonpaged md ptr, base_config_p:0x%p\n",
                              base_config_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if state of the nonpaged metadata is not valid then return false. */
    if(base_config_nonpaged_metadata_p->nonpaged_metadata_state == FBE_NONPAGED_METADATA_INITIALIZED) 
    {
        *is_magic_num_initialized_p = FBE_TRUE;
    } 

    return FBE_STATUS_OK;
}
fbe_status_t
fbe_base_config_metadata_is_nonpaged_state_valid(fbe_base_config_t * base_config_p, fbe_bool_t * is_magic_num_valid_p)
{
    fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p = NULL;

    /* initialize it as false. */
    *is_magic_num_valid_p = FBE_FALSE;

    /* get the magic number of nonpaged metadata. */
    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
    if(base_config_nonpaged_metadata_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "bcfg_md_np_inited Invalid nonpaged md ptr, base_config_p:0x%p\n",
                              base_config_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if magic number of the nonpaged metadata is not valid then return false. */
    if(base_config_nonpaged_metadata_p->nonpaged_metadata_state != FBE_NONPAGED_METADATA_INVALID)
    {
        *is_magic_num_valid_p = FBE_TRUE;
    } 

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_set_default_nonpaged_metadata(fbe_base_config_t * base_config_p, fbe_base_config_nonpaged_metadata_t *nonpaged_metadata_p)
{
    fbe_object_id_t                         object_id;
    fbe_generation_code_t                   generation_number;
    fbe_base_config_background_operation_t  background_op_bitmask;
    fbe_base_config_nonpaged_metadata_t    *base_config_nonpaged_metadata_p;
    fbe_status_t                            status;


    /*! @note Validate that area has been zeroed.
               Add nonpaged metadata state check:
               when system nonpaged MD load failed,
               Object will stuck in SPACIALIZED and its NP state is invalid
               we set the default NP to fix object stuck in SPACIALIZED, 
               the object_id,generation_number may not be 0, and the NP state is INVALID.
               At this time, we should allow the default NP setting.
     */
    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
    if (((base_config_nonpaged_metadata_p->object_id != 0)         ||
        (base_config_nonpaged_metadata_p->generation_number != 0) )&&
        (base_config_nonpaged_metadata_p->nonpaged_metadata_state != FBE_NONPAGED_METADATA_INVALID
         && base_config_nonpaged_metadata_p->nonpaged_metadata_state != FBE_NONPAGED_METADATA_UNINITIALIZED))
    {
        /* Report an error 
         */
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s obj:0x%x gen:0x%llx not 0 as expected.MD State:%d\n",
                              __FUNCTION__, base_config_nonpaged_metadata_p->object_id, 
                              base_config_nonpaged_metadata_p->generation_number,
                              base_config_nonpaged_metadata_p->nonpaged_metadata_state);

        /* return FBE_STATUS_GENERIC_FAILURE; */

        /* Remove above RETURN, because in below case, we need this function continue work
          * and the object just exactly in the case of Generation# has non-zero value
          */
          
        /* We would meet generation mismatch between NP-record and base_config under below case
          * And cannot get out from it after drive comes back. AR601723
          
          * Below are reproduction steps:
          * 1.Two SP starts normally, all objects get into Ready.
          * 2.Set PVD Garbage Collection time threshold to 1 minutes (because I do not find the cli to destroy PVD)
          * 3.Pull one drive from both SPs, I choose 0_0_12. Wait for PVD GC to destroy PVD.
          * 4.Reinsert the drive into 0_0_12, then a new PVD object with the same obj_id(0x108 here) would be 
          *     created. Due to I added that 20 seconds sleep, so this PVD would be stuck into SPEC for 20 seconds.
          * 5.Immediately pull the drive 0_0_12 out again. (in fbe_test I added a hook to let lifecycle stuck) Then 
          *     the PVD would be stuck into downstream_path_nonoptimal_cond().
          * 6.Reboot Passive SP (SPB).
          * 7.Reboot Active SP (SPA). Then on SPA side, 
          *     I see "bcfg_md_np_inited Wrong NP sig: objid 0x108 != 0x108; gen 3A != 6A" periodicity. 
         */
    }

    /*version check: ASSERT in compile time that fbe_sep_version_header_t should be first
      member of the fbe_base_config_nonpaged_metadata_t data structure */
    FBE_ASSERT_AT_COMPILE_TIME(((fbe_u32_t) &((fbe_base_config_nonpaged_metadata_t *) 0)->version_header) == 0);

    /*initialize the version header of non-paged metadata*/
    status = fbe_base_config_init_nonpaged_metadata_version_header(nonpaged_metadata_p, 
                                                          ((fbe_base_object_t *)base_config_p)->class_id);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "base config fail to initialize np metadata version header.\n");
    }
    /* get the object id and generation number from base config object */
    fbe_base_object_get_object_id((fbe_base_object_t *)base_config_p, &object_id);
    fbe_base_config_get_generation_num(base_config_p, &generation_number);

    /* get the background operation bitmask from base config object */
    fbe_base_config_nonpaged_metadata_get_background_op_bitmask(base_config_p, &background_op_bitmask);

    //  Set the object id for the raid group nonpaged metadata.
    fbe_base_config_nonpaged_metadata_set_object_id(nonpaged_metadata_p, object_id);

    //  Set the generation number for the raid group nonpaged metadata.
    fbe_base_config_nonpaged_metadata_set_generation_number(nonpaged_metadata_p, generation_number);

    // The background operation default bitmask was set by object at the time of writing default non paged
    // metadata. It is required to set it again so that default value will remain set.
    fbe_base_config_nonpaged_metadata_set_background_op_bitmask(nonpaged_metadata_p, background_op_bitmask);


    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_set_paged_record_start_lba(fbe_base_config_t * base_config_p,
                                                    fbe_lba_t paged_record_start_lba)
{
    fbe_status_t status;
    status = fbe_metadata_element_set_paged_record_start_lba(&base_config_p->metadata_element, paged_record_start_lba);
    return status;
}

fbe_status_t
fbe_base_config_metadata_get_paged_record_start_lba(fbe_base_config_t * base_config_p,
                                                    fbe_lba_t * paged_record_start_lba_p)
{
    fbe_status_t status;
    status = fbe_metadata_element_get_paged_record_start_lba(&base_config_p->metadata_element, paged_record_start_lba_p);
    return status;
}

fbe_status_t
fbe_base_config_metadata_set_paged_mirror_metadata_offset(fbe_base_config_t * base_config_p,
                                                          fbe_lba_t paged_mirror_metadata_offset)
{
    fbe_status_t status;
    status = fbe_metadata_element_set_paged_mirror_metadata_offset(&base_config_p->metadata_element, paged_mirror_metadata_offset);
    return status;
}

fbe_status_t
fbe_base_config_metadata_get_paged_mirror_metadata_offset(fbe_base_config_t * base_config_p,
                                                          fbe_lba_t * paged_mirror_metadata_offset_p)
{
    fbe_status_t status;
    status = fbe_metadata_element_get_paged_mirror_metadata_offset(&base_config_p->metadata_element, paged_mirror_metadata_offset_p);
    return status;
}

fbe_status_t
fbe_base_config_metadata_set_paged_metadata_capacity(fbe_base_config_t * base_config_p,
                                                    fbe_lba_t paged_metadata_capacity)
{
    fbe_status_t status;
    status = fbe_metadata_element_set_paged_metadata_capacity(&base_config_p->metadata_element, paged_metadata_capacity);
    return status;
}

fbe_status_t
fbe_base_config_metadata_get_paged_metadata_capacity(fbe_base_config_t * base_config_p,
                                                    fbe_lba_t * paged_metadata_capacity_p)
{
    fbe_status_t status;
    status = fbe_metadata_element_get_paged_metadata_capacity(&base_config_p->metadata_element, paged_metadata_capacity_p);
    return status;
}

fbe_object_id_t 
fbe_base_config_metadata_element_to_object_id(fbe_metadata_element_t * metadata_element)
{
    fbe_base_config_t  * base_config;

    base_config = (fbe_base_config_t  *)((fbe_u8_t *)metadata_element - (fbe_u8_t *)(&((fbe_base_config_t  *)0)->metadata_element));
    return ((fbe_base_object_t *)(base_config))->object_id;
}

fbe_class_id_t
fbe_base_config_metadata_element_to_class_id(fbe_metadata_element_t *metadata_element)
{
    fbe_base_config_t  * base_config;

    base_config = (fbe_base_config_t  *)((fbe_u8_t *)metadata_element - (fbe_u8_t *)(&((fbe_base_config_t  *)0)->metadata_element));
    return ((fbe_base_object_t *)(base_config))->class_id;
}


fbe_bool_t
fbe_base_config_metadata_element_is_nonpaged_initialized(fbe_metadata_element_t * metadata_element)
{
    fbe_base_config_t *base_config;
    fbe_bool_t         is_initialized;  

    base_config = (fbe_base_config_t  *)((fbe_u8_t *)metadata_element - (fbe_u8_t *)(&((fbe_base_config_t  *)0)->metadata_element));
    fbe_base_config_metadata_nonpaged_is_initialized(base_config, &is_initialized);
    return is_initialized;
}
fbe_bool_t
fbe_base_config_metadata_element_is_nonpaged_state_initialized(fbe_metadata_element_t * metadata_element)
{
    fbe_base_config_t *base_config;
    fbe_bool_t         is_magic_num_initialized;  

    base_config = (fbe_base_config_t  *)((fbe_u8_t *)metadata_element - (fbe_u8_t *)(&((fbe_base_config_t  *)0)->metadata_element));
    fbe_base_config_metadata_is_nonpaged_state_initialized(base_config, &is_magic_num_initialized);
    return is_magic_num_initialized;
}

fbe_status_t
fbe_base_config_metadata_nonpaged_write(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size)
{
    fbe_status_t status;
    status = fbe_base_config_metadata_nonpaged_perform_write(base_config, packet, 
                                                             metadata_offset, metadata_record_data,
                                                             metadata_record_data_size,
                                                             FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NONE);
    return status;
}

/*!**************************************************************************************
 * fbe_base_config_metadata_nonpaged_perform_write()
 ****************************************************************************************
 * @brief
 *   This function builds and sends the request to the metadata service to perform the 
 * nonpaged write operation
 *
 * @param  base_config - Pointer to the base config structure for the object
 * @param  packet_p    - Pointer to the packet
 * @param metadata_offset - Offset into the Metadata for writing
 * @param metadata_record_data - Pointer to the data that needs to be written               
 * @param metadata_record_data_size - Size of the metadata record to be written
 * @param metadata_operation_flags  - Metadata operation flags
 *
 * @return fbe_status_t   
 *
 * @author
 *  04/19/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************************/
static fbe_status_t
fbe_base_config_metadata_nonpaged_perform_write(fbe_base_config_t * base_config,
                                                fbe_packet_t * packet,
                                                fbe_u64_t   metadata_offset,
                                                fbe_u8_t *  metadata_record_data,
                                                fbe_u32_t metadata_record_data_size,
                                                fbe_payload_metadata_operation_flags_t metadata_operation_flags)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    //fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata;
    //fbe_object_id_t object_id;
    //fbe_generation_code_t   generation_number;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);

    fbe_payload_metadata_build_nonpaged_write(metadata_operation, 
                                              &base_config->metadata_element,
                                              metadata_operation_flags);
    
    
    /* Copy the data */
   if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE)
   {
        fbe_base_object_trace((fbe_base_object_t*)base_config, 
                          FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s Invalid write size %d > %d \n", __FUNCTION__, 
                          metadata_record_data_size,
                          FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

        fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

#if 0
   /* Check for signature corruption */
   if(metadata_offset == 0){
        base_config_nonpaged_metadata = (fbe_base_config_nonpaged_metadata_t *)metadata_record_data;
        fbe_base_object_get_object_id((fbe_base_object_t *) base_config, &object_id);
        fbe_base_config_get_generation_num(base_config, &generation_number);
        if(object_id != base_config_nonpaged_metadata->object_id || generation_number != base_config_nonpaged_metadata->generation_number){
            fbe_base_object_trace((fbe_base_object_t*)base_config, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s NonPaged signature \n", __FUNCTION__);

            fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
   }
#endif

    metadata_operation->u.metadata.offset = metadata_offset;
    metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

    fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_nonpaged_write_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_nonpaged_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    status = fbe_transport_get_status_code(packet);
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

	/*fbe_base_config_metadata_nonpaged_is_initialized(base_config, &is_nonpaged_initialized);
	if(is_nonpaged_initialized == FBE_FALSE) {
		fbe_base_object_trace((fbe_base_object_t*)base_config,
								FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s this is bad can't be ready and not initialized", __FUNCTION__);
	}*/

    if ((status == FBE_STATUS_OK)                           &&
        (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)    )
    {
        fbe_metadata_translate_metadata_status_to_packet_status(metadata_status, &status);
        fbe_transport_set_status(packet, status, 0);    
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************************************
 * fbe_base_config_metadata_nonpaged_write_persist()
 ****************************************************************************************
 * @brief
 *   This function setups the completion stack for write operation, Persist Operation
 * and action to be taken after the persist completes
 *
 * @param  base_config - Pointer to the base config structure for the object
 * @param  packet_p    - Pointer to the packet
 * @param metadata_offset - Offset into the Metadata for persisting
 * @param metadata_record_data - Pointer to the data that needs to be persisted               
 * @param metadata_record_data_size - Size of the metadata record to be persisted
 *
 * @return fbe_status_t   
 *
 * @author
 *  04/19/2012 - Created. Ashok Tamilarasan
 *
 *****************************************************************************************/
fbe_status_t fbe_base_config_metadata_nonpaged_write_persist(fbe_base_config_t * base_config,
                                                             fbe_packet_t * packet,
                                                             fbe_u64_t   metadata_offset,
                                                             fbe_u8_t *  metadata_record_data,
                                                             fbe_u32_t metadata_record_data_size)
{
    /* Set up the completion callback for the write persist operation in the reverse order of sequence
     */

    /* Setup a callback to start the persist operation after the write completes */
    fbe_transport_set_completion_function(packet, 
                                          fbe_base_config_metadata_nonpaged_start_persist, 
                                          base_config);

    /* Kick of the write persist operation */
    fbe_base_config_metadata_nonpaged_perform_write(base_config, packet, 
                                                    metadata_offset, metadata_record_data,
                                                    metadata_record_data_size,
                                                    FBE_PAYLOAD_METADATA_OPERATION_FLAGS_PERSIST);

    return FBE_STATUS_OK;
}

/*!**************************************************************************************
 * fbe_base_config_metadata_nonpaged_start_persist()
 ****************************************************************************************
 * @brief
 *   This function kicks starts the persist portion of the operation
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
fbe_base_config_metadata_nonpaged_start_persist(fbe_packet_t * packet, 
                                                fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = (fbe_base_config_t*)context;
    fbe_status_t status;
    
    status = fbe_transport_get_status_code(packet);

    if (status == FBE_STATUS_OK)
    {
        /* Set up a callback to perform some action after the persist completes */
        fbe_transport_set_completion_function(packet, 
                                              fbe_base_config_metadata_nonpaged_start_post_persist, 
                                              base_config);

        /* If everything is okay, kick off the persist */
        fbe_base_config_metadata_nonpaged_persist(base_config, packet);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s error pkt_st: 0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************************************
 * fbe_base_config_metadata_nonpaged_start_post_persist()
 ****************************************************************************************
 * @brief
 *   This function handle the completion of the persist portion of operation
 * and start the post persist operations
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
fbe_base_config_metadata_nonpaged_start_post_persist(fbe_packet_t * packet, 
                                                     fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = (fbe_base_config_t*)context;
    fbe_status_t status;
    
    status = fbe_transport_get_status_code(packet);

    if (status == FBE_STATUS_OK)
    {
        /* everything is okay, kicks start the post processing of the persist completion */
        fbe_base_config_metadata_nonpaged_perform_post_persist(base_config, packet);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s error pkt_st: 0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet, status, 0);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t
fbe_base_config_metadata_nonpaged_update(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_u8_t *memory_ptr;
    fbe_u32_t memory_size;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);

    fbe_payload_metadata_build_nonpaged_write(metadata_operation, 
                                              &base_config->metadata_element,
                                              FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NONE);
    
    fbe_base_config_get_nonpaged_metadata_ptr(base_config, (void **)&memory_ptr);
    fbe_base_config_get_nonpaged_metadata_size(base_config, &memory_size);

    metadata_operation->u.metadata.offset = 0;
    metadata_operation->u.metadata.record_data_size = memory_size;

    fbe_copy_memory(metadata_operation->u.metadata.record_data, memory_ptr, memory_size);

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_nonpaged_update_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_nonpaged_update_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = (fbe_base_config_t*)context;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;
	fbe_bool_t is_nonpaged_initialized;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    status = fbe_transport_get_status_code(packet);
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    fbe_metadata_element_clear_nonpaged_request(metadata_operation->metadata_element);    

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

	fbe_base_config_metadata_nonpaged_is_initialized(base_config, &is_nonpaged_initialized);
	if(is_nonpaged_initialized == FBE_FALSE) {
		fbe_base_object_trace((fbe_base_object_t*)base_config,
								FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s this is bad can't be ready and not initialized", __FUNCTION__);
	}

    if ((status == FBE_STATUS_OK)                           &&
        (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)    )
    {
        fbe_metadata_translate_metadata_status_to_packet_status(metadata_status, &status);
        fbe_transport_set_status(packet, status, 0);    
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_nonpaged_read_persist(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_u8_t *memory_ptr;
    fbe_u32_t memory_size;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);

    fbe_payload_metadata_build_nonpaged_read_persist(metadata_operation, 
                                              &base_config->metadata_element,
                                              FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NONE);
    
    fbe_base_config_get_nonpaged_metadata_ptr(base_config, (void **)&memory_ptr);
    fbe_base_config_get_nonpaged_metadata_size(base_config, &memory_size);

    metadata_operation->u.metadata.offset = 0;
    metadata_operation->u.metadata.record_data_size = memory_size;

    fbe_copy_memory(metadata_operation->u.metadata.record_data, memory_ptr, memory_size);

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_nonpaged_read_persist_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_nonpaged_read_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    status = fbe_transport_get_status_code(packet);
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    fbe_metadata_element_clear_nonpaged_request(metadata_operation->metadata_element);    

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    if ((status == FBE_STATUS_OK)                           &&
        (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)    )
    {
        fbe_metadata_translate_metadata_status_to_packet_status(metadata_status, &status);
        fbe_transport_set_status(packet, status, 0);    
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_paged_set_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_u64_t   metadata_repeat_count)

{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_paged_set_bits(metadata_operation, 
                                            &base_config->metadata_element, 
                                            metadata_offset,
                                            metadata_record_data,
                                            metadata_record_data_size,
                                            metadata_repeat_count);

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_paged_set_bits_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_paged_set_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_paged_clear_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_u64_t   metadata_repeat_count)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_paged_clear_bits(metadata_operation, 
                                            &base_config->metadata_element, 
                                            metadata_offset,
                                            metadata_record_data,
                                            metadata_record_data_size,
                                            metadata_repeat_count);

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_paged_clear_bits_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_paged_clear_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_paged_get_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_base_config_control_metadata_paged_get_bits_flags_t get_bits_flags)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_paged_get_bits(metadata_operation, 
                                              &base_config->metadata_element, 
                                              metadata_offset,
                                              metadata_record_data,
                                              metadata_record_data_size,
                                              metadata_record_data_size);

    /* If FUA is requested, set the proper flag in the metadata operation.
     */
    if ((get_bits_flags & FBE_BASE_CONFIG_CONTROL_METADATA_PAGED_GET_BITS_FLAGS_FUA_READ) != 0)
    {
        metadata_operation->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_FUA_READ;
    }

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_paged_get_bits_completion, metadata_record_data);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_paged_get_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_u8_t *  metadata_record_data = NULL;
    fbe_status_t  packet_status;
    fbe_payload_metadata_status_t metadata_status;

    packet_status = fbe_transport_get_status_code(packet);
    if (packet_status == FBE_STATUS_OK) {
        sep_payload = fbe_transport_get_payload_ex(packet);
        metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
        fbe_payload_metadata_get_status(metadata_operation, &metadata_status);
        metadata_record_data = (fbe_u8_t *)context;

        if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK) {
            fbe_copy_memory(metadata_record_data,metadata_operation->u.metadata.record_data, metadata_operation->u.metadata.record_data_size);
        }
        else 
        {
            fbe_metadata_translate_metadata_status_to_packet_status(metadata_status, &packet_status);
            fbe_transport_set_status(packet, packet_status, 0);    
        }
    }

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_paged_write(fbe_base_config_t * base_config,
                                     fbe_packet_t * packet,
                                     fbe_u64_t  metadata_offset,
                                     fbe_u8_t * metadata_record_data,
                                     fbe_u32_t metadata_record_data_size,
                                     fbe_u64_t metadata_repeat_count)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_paged_write(metadata_operation, 
                                           &base_config->metadata_element, 
                                           metadata_offset,
                                           metadata_record_data,
                                           metadata_record_data_size,
                                           metadata_repeat_count);

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_paged_write_completion, base_config);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_paged_write_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_metadata_paged_clear_cache(fbe_base_config_t * base_config)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID) {
        return FBE_STATUS_OK;
    }
    status = fbe_metadata_paged_clear_cache(&base_config->metadata_element);
    return status;
}

fbe_status_t fbe_base_config_metadata_get_power_save_state(fbe_base_config_t * base_config_p, fbe_power_save_state_t *get_sate)
{    
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;

    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);
    
    if (metadata_memory_ptr != NULL)
    {
        *get_sate = metadata_memory_ptr->power_save_state;
    }
    else
    {
        *get_sate = FBE_POWER_SAVE_STATE_INVALID;
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_set_power_save_state(fbe_base_config_t * base_config_p, fbe_power_save_state_t set_sate)
{
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;

    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);
    
    if (metadata_memory_ptr != NULL)
    {
        metadata_memory_ptr->power_save_state = set_sate;
    }
    else 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_get_peer_power_save_state(fbe_base_config_t * base_config_p, fbe_power_save_state_t *get_sate)
{
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;

    fbe_metadata_element_get_peer_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);

    if (metadata_memory_ptr != NULL)
    {
        *get_sate = metadata_memory_ptr->power_save_state;
    }
    else
    {
        *get_sate = FBE_POWER_SAVE_STATE_INVALID;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_get_peer_last_io_time(fbe_base_config_t * base_config_p, fbe_time_t *last_io_time)
{
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;

    fbe_metadata_element_get_peer_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);

    if (metadata_memory_ptr != NULL)
    {
        *last_io_time = metadata_memory_ptr->last_io_time;
    }
    else
    {
        *last_io_time = 0;
    }

    return FBE_STATUS_OK;

}

fbe_status_t fbe_base_config_metadata_get_peer_lifecycle_state(fbe_base_config_t * base_config_p, fbe_lifecycle_state_t *peer_state)
{
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;

    fbe_metadata_element_get_peer_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);

    if (metadata_memory_ptr != NULL)
    {
        *peer_state = metadata_memory_ptr->lifecycle_state;
    }
    else
    {
        *peer_state = FBE_LIFECYCLE_STATE_INVALID;
    }

    return FBE_STATUS_OK;

}

fbe_status_t fbe_base_config_metadata_set_last_io_time(fbe_base_config_t * base_config_p, fbe_time_t set_time)
{
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;

    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);
    
    metadata_memory_ptr->last_io_time = set_time;

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_get_last_io_time(fbe_base_config_t * base_config_p, fbe_time_t *get_time)
{
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;
    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);

    *get_time = metadata_memory_ptr->last_io_time;

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_get_lifecycle_state(fbe_base_config_t * base_config_p, fbe_lifecycle_state_t *my_state)
{
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;

    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);    

    *my_state = metadata_memory_ptr->lifecycle_state;

    return FBE_STATUS_OK;

}

fbe_status_t
fbe_base_config_metadata_nonpaged_set_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_u64_t   metadata_repeat_count)

{
    fbe_status_t status;

    status = fbe_base_config_metadata_nonpaged_perform_set_bits(base_config, packet, 
                                                                metadata_offset, metadata_record_data,
                                                                metadata_record_data_size,
                                                                metadata_repeat_count,
                                                                FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NONE);

    return status;
}

static fbe_status_t
fbe_base_config_metadata_nonpaged_perform_set_bits(fbe_base_config_t * base_config,
                                                   fbe_packet_t * packet,
                                                   fbe_u64_t   metadata_offset,
                                                   fbe_u8_t *  metadata_record_data,
                                                   fbe_u32_t metadata_record_data_size,
                                                   fbe_u64_t   metadata_repeat_count,
                                                   fbe_payload_metadata_operation_flags_t flags)

{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_nonpaged_set_bits(metadata_operation, 
                                                 &base_config->metadata_element, 
                                                 metadata_offset,
                                                 metadata_record_data,
                                                 metadata_record_data_size,
                                                 metadata_repeat_count,
                                                 flags);

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_nonpaged_set_bits_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_nonpaged_set_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = (fbe_base_config_t*)context;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
	fbe_bool_t is_nonpaged_initialized;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

	fbe_base_config_metadata_nonpaged_is_initialized(base_config, &is_nonpaged_initialized);
	if(is_nonpaged_initialized == FBE_FALSE) {
		fbe_base_object_trace((fbe_base_object_t*)base_config,
								FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s this is bad can't be ready and not initialized", __FUNCTION__);
	}

    return FBE_STATUS_OK;
}


fbe_status_t fbe_base_config_metadata_nonpaged_set_bits_persist(fbe_base_config_t * base_config,
                                                                fbe_packet_t * packet,
                                                                fbe_u64_t   metadata_offset,
                                                                fbe_u8_t *  metadata_record_data,
                                                                fbe_u32_t metadata_record_data_size,
                                                                fbe_u64_t   metadata_repeat_count)
{
    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_nonpaged_start_persist, base_config);
    fbe_base_config_metadata_nonpaged_perform_set_bits(base_config, packet, 
                                                       metadata_offset, metadata_record_data,
                                                       metadata_record_data_size,
                                                       metadata_repeat_count,
                                                       FBE_PAYLOAD_METADATA_OPERATION_FLAGS_PERSIST);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_nonpaged_clear_bits(fbe_base_config_t * base_config,
                                        fbe_packet_t * packet,
                                        fbe_u64_t   metadata_offset,
                                        fbe_u8_t *  metadata_record_data,
                                        fbe_u32_t metadata_record_data_size,
                                        fbe_u64_t   metadata_repeat_count)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_nonpaged_clear_bits(metadata_operation, 
                                            &base_config->metadata_element, 
                                            metadata_offset,
                                            metadata_record_data,
                                            metadata_record_data_size,
                                            metadata_repeat_count);

    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_nonpaged_clear_bits_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_nonpaged_clear_bits_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_set_quiesce_state_local_and_update_peer(fbe_base_config_t * base_config_p, 
                                                                              fbe_base_config_metadata_memory_flags_t state)
{
    
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;
    fbe_status_t                        status;


    /* Get a pointer to metadata memory */
    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);
    
    /* See if state is already set; if so, do not need to update peer again */
    if((metadata_memory_ptr->flags & FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_MASK) == state){
        return FBE_STATUS_OK;
    }
    
    /* Only one quiesce state is set at a time, so turn off quiesce bits in flags field first */
    metadata_memory_ptr->flags &= ~FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_MASK;

    /* Set quiesce state */
    metadata_memory_ptr->flags |= state;

    /* Set condition to update metadata memory; condition processing will update peer if available */
    status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                    (fbe_base_object_t*)base_config_p, 
                                    FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s cannot set condition\n",__FUNCTION__);
        return status;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_get_quiesce_state_local(fbe_base_config_t * base_config_p, 
                                                              fbe_base_config_metadata_memory_flags_t * out_state_p)
{
    
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;


    /* Get a pointer to metadata memory */
    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_ptr);
    
    
    /* Set output parameter from flags field; only one quiesce state set at a time */
    *out_state_p = metadata_memory_ptr->flags & FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_MASK;

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_get_quiesce_state_peer(fbe_base_config_t * base_config_p, 
                                                             fbe_base_config_metadata_memory_flags_t * out_state_p)
{
    fbe_base_config_metadata_memory_t * metadata_memory_peer_ptr = NULL;

    /* Get a pointer to metadata memory*/
    fbe_metadata_element_get_peer_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_peer_ptr);
    
    if (metadata_memory_peer_ptr != NULL)
    {
        /* Set output parameter from flags field; only one quiesce state set at a time */
        *out_state_p = metadata_memory_peer_ptr->flags & FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_MASK;
    }
    else
    {
        *out_state_p = FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_set_number_of_private_stripes(fbe_base_config_t * base_config_p, fbe_u64_t number_of_stripes)
{
    fbe_status_t status;
    status = fbe_metadata_element_set_number_of_private_stripes(&base_config_p->metadata_element, number_of_stripes);
    return status;
}

fbe_status_t
fbe_base_config_metadata_get_number_of_private_stripes(fbe_base_config_t * base_config_p, fbe_u64_t * number_of_stripes)
{
    fbe_status_t status;
    status = fbe_metadata_element_get_number_of_private_stripes(&base_config_p->metadata_element, number_of_stripes);
    return status;
}

fbe_status_t
fbe_base_config_metadata_set_number_of_stripes(fbe_base_config_t * base_config_p, fbe_u64_t number_of_stripes)
{
    fbe_status_t status;
    status = fbe_metadata_element_set_number_of_stripes(&base_config_p->metadata_element, number_of_stripes);
    return status;
}

fbe_status_t
fbe_base_config_metadata_get_number_of_stripes(fbe_base_config_t * base_config_p, fbe_u64_t * number_of_stripes)
{
    fbe_status_t status;
    status = fbe_metadata_element_get_number_of_stripes(&base_config_p->metadata_element, number_of_stripes);
    return status;
}


/*!**************************************************************************************
 * fbe_base_config_metadata_nonpaged_persist_sync()
 ****************************************************************************************
 * @brief
 *   This function persist nonpaged metadata.
 *   Unlike fbe_base_config_metadata_nonpaged_persist, this function
 *   will send POST PERSIST message after persist done.
 *
 * @param base_config - the object to persist
 * @param packet    - Pointer to the packet
 *
 * @return fbe_status_t
 *
 * @author
 *  12/12/2013 - Created. Jamin Kang
 *
 *****************************************************************************************/
fbe_status_t
fbe_base_config_metadata_nonpaged_persist_sync(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_status_t status;

    /* Setup a callback to send POST PERSIST message to peer */
    fbe_transport_set_completion_function(packet,
                                          fbe_base_config_metadata_nonpaged_start_post_persist,
                                          base_config);

    /* Kick off the persist */
    status = fbe_base_config_metadata_nonpaged_persist(base_config, packet);
    return status;
}

fbe_status_t
fbe_base_config_metadata_nonpaged_persist(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);

    fbe_payload_metadata_build_nonpaged_persist(metadata_operation, &base_config->metadata_element);


    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_nonpaged_persist_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_base_config_metadata_nonpaged_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    status = fbe_transport_get_status_code(packet);
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    if ((status == FBE_STATUS_OK)                           &&
        (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)    )
    {
        fbe_metadata_translate_metadata_status_to_packet_status(metadata_status, &status);
        fbe_transport_set_status(packet, status, 0);    
    }

    return FBE_STATUS_OK;
}
/*!**************************************************************************************
 * fbe_base_config_metadata_nonpaged_perform_post_persist()
 ****************************************************************************************
 * @brief
 *   This function starts the persist complete operation
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
fbe_base_config_metadata_nonpaged_perform_post_persist(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);

    fbe_payload_metadata_build_nonpaged_post_persist(metadata_operation, &base_config->metadata_element);


    fbe_transport_set_completion_function(packet, 
                                          fbe_base_config_metadata_nonpaged_post_persist_completion, 
                                          base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    if (status == FBE_STATUS_OK) 
    {
        /* we have generated a new IO, stop packet completion from here */
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    return status;
}

/*!**************************************************************************************
 * fbe_base_config_metadata_nonpaged_post_persist_completion()
 ****************************************************************************************
 * @brief
 *   This function handle the completion of the post persist operation
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
fbe_base_config_metadata_nonpaged_post_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = (fbe_base_config_t*)context;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    status = fbe_transport_get_status_code(packet);
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

    if ((status == FBE_STATUS_OK)                           &&
        (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)    )
    {
        fbe_metadata_translate_metadata_status_to_packet_status(metadata_status, &status);
        fbe_transport_set_status(packet, status, 0);    
    }

    fbe_base_config_clear_flag(base_config, FBE_BASE_CONFIG_FLAG_INITIAL_CONFIGURATION);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_get_statistics(fbe_base_config_t * base_config, fbe_packet_t * packet_p)
{
    fbe_base_config_control_get_metadata_statistics_t * 		metadata_statistics = NULL;    /* INPUT */
    fbe_payload_ex_t * 								payload = NULL;
    fbe_payload_control_operation_t * 				control_operation = NULL;
    fbe_payload_control_buffer_length_t 			len = 0;
    
    fbe_base_object_trace((fbe_base_object_t*)base_config,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &metadata_statistics);
    if (metadata_statistics == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_config,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s fbe_payload_control_get_buffer failed\n",
                                    __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_base_config_control_get_metadata_statistics_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_config,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Invalid length:%d\n",
                              __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_spinlock_lock(&base_config->metadata_element.metadata_element_lock);
    metadata_statistics->stripe_lock_count = base_config->metadata_element.stripe_lock_count;
    metadata_statistics->local_collision_count = base_config->metadata_element.local_collision_count;
    metadata_statistics->peer_collision_count = base_config->metadata_element.peer_collision_count;
    metadata_statistics->cmi_message_count = base_config->metadata_element.cmi_message_count;
    fbe_spinlock_unlock(&base_config->metadata_element.metadata_element_lock);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}

fbe_status_t
fbe_base_config_metadata_nonpaged_set_checkpoint(fbe_base_config_t * base_config,
                                                fbe_packet_t * packet,
                                                fbe_u64_t   metadata_offset,
                                                fbe_u64_t   second_metadata_offset,
                                                fbe_u64_t   checkpoint)
{
    fbe_status_t status;
    status = fbe_base_config_metadata_nonpaged_perform_set_checkpoint(base_config, packet, 
                                                                      metadata_offset,
                                                                      second_metadata_offset,
                                                                      checkpoint,
                                                                      FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NONE);
    return status;
}

fbe_status_t
fbe_base_config_metadata_nonpaged_force_set_checkpoint(fbe_base_config_t * base_config,
                                                       fbe_packet_t * packet,
                                                       fbe_u64_t   metadata_offset,
                                                       fbe_u64_t   second_metadata_offset,
                                                       fbe_u64_t   checkpoint)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_nonpaged_force_set_checkpoint(metadata_operation, 
                                                             &base_config->metadata_element, 
                                                             metadata_offset,
                                                             second_metadata_offset,
                                                             checkpoint);

    fbe_transport_set_completion_function(packet, base_config_metadata_nonpaged_set_checkpoint_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t
fbe_base_config_metadata_nonpaged_perform_set_checkpoint(fbe_base_config_t * base_config,
                                                         fbe_packet_t * packet,
                                                         fbe_u64_t   metadata_offset,
                                                         fbe_u64_t   second_metadata_offset,
                                                         fbe_u64_t   checkpoint,
                                                         fbe_payload_metadata_operation_flags_t flags)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_nonpaged_set_checkpoint(metadata_operation, 
                                                       &base_config->metadata_element, 
                                                       metadata_offset,
                                                       second_metadata_offset,
                                                       checkpoint,
                                                       flags);

    fbe_transport_set_completion_function(packet, base_config_metadata_nonpaged_set_checkpoint_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
base_config_metadata_nonpaged_set_checkpoint_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = (fbe_base_config_t*)context;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
	fbe_bool_t is_nonpaged_initialized;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);
	fbe_base_config_metadata_nonpaged_is_initialized(base_config, &is_nonpaged_initialized);
	if(is_nonpaged_initialized == FBE_FALSE) {
		fbe_base_object_trace((fbe_base_object_t*)base_config,
								FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s this is bad can't be ready and not initialized", __FUNCTION__);
	}

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_metadata_nonpaged_set_checkpoint_persist(fbe_base_config_t * base_config,
                                                                      fbe_packet_t * packet,
                                                                      fbe_u64_t   metadata_offset,
                                                                      fbe_u64_t   second_metadata_offset,                                                                      
                                                                      fbe_u64_t   checkpoint)
{
    fbe_transport_set_completion_function(packet, fbe_base_config_metadata_nonpaged_start_persist, base_config);
    fbe_base_config_metadata_nonpaged_perform_set_checkpoint(base_config, packet, 
                                                             metadata_offset,
                                                             second_metadata_offset,
                                                             checkpoint,
                                                             FBE_PAYLOAD_METADATA_OPERATION_FLAGS_PERSIST);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_metadata_nonpaged_incr_checkpoint(fbe_base_config_t * base_config,
                                                fbe_packet_t * packet,
                                                fbe_u64_t   metadata_offset,
                                                fbe_u64_t   second_metadata_offset,
                                                fbe_u64_t   checkpoint,
                                                fbe_u64_t	repeat_count)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_nonpaged_incr_checkpoint(metadata_operation, 
                                                        &base_config->metadata_element, 
                                                        metadata_offset,
                                                        second_metadata_offset,
                                                        checkpoint,
                                                        repeat_count);

    fbe_transport_set_completion_function(packet, base_config_metadata_nonpaged_incr_checkpoint_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}
fbe_status_t
fbe_base_config_metadata_nonpaged_incr_checkpoint_no_peer(fbe_base_config_t * base_config,
                                                          fbe_packet_t * packet,
                                                          fbe_u64_t   metadata_offset,
                                                          fbe_u64_t   second_metadata_offset,
                                                          fbe_u64_t   checkpoint,
                                                          fbe_u64_t	repeat_count)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_np_inc_chkpt_no_peer(metadata_operation, 
                                                    &base_config->metadata_element, 
                                                    metadata_offset,
                                                    second_metadata_offset,
                                                    checkpoint,
                                                    repeat_count);

    fbe_transport_set_completion_function(packet, base_config_metadata_nonpaged_incr_checkpoint_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
base_config_metadata_nonpaged_incr_checkpoint_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = (fbe_base_config_t*)context;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
	fbe_bool_t is_nonpaged_initialized;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

	fbe_base_config_metadata_nonpaged_is_initialized(base_config, &is_nonpaged_initialized);
	if(is_nonpaged_initialized == FBE_FALSE) {
		fbe_base_object_trace((fbe_base_object_t*)base_config,
								FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s this is bad can't be ready and not initialized", __FUNCTION__);
	}

    return FBE_STATUS_OK;
}
fbe_status_t
fbe_base_config_ex_metadata_nonpaged_write_verify(fbe_base_config_write_verify_context_t* base_config_write_verify_context,fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_payload_ex_t * sep_payload = NULL;
	fbe_payload_metadata_operation_t * metadata_operation = NULL;

	sep_payload = fbe_transport_get_payload_ex(packet);
	metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
			
	fbe_payload_metadata_build_nonpaged_write_verify(metadata_operation, &base_config_write_verify_context->base_config_p->metadata_element);
			
	fbe_transport_set_completion_function(packet, base_config_ex_metadata_nonpaged_write_verify_completion, base_config_write_verify_context);	   
	fbe_payload_ex_increment_metadata_operation_level(sep_payload);
			
	status =  fbe_metadata_operation_entry(packet);
	return status;
}
static fbe_status_t 
base_config_ex_metadata_nonpaged_write_verify_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_status_t						status;
	fbe_base_config_write_verify_context_t *				   base_config_write_verify_context = NULL;
	fbe_payload_ex_t *				  payload = NULL;
	fbe_payload_metadata_operation_t *	metadata_operation = NULL;
	fbe_payload_metadata_status_t	   metadata_status;
		   
	base_config_write_verify_context = (fbe_base_config_write_verify_context_t*)context;
		
	payload = fbe_transport_get_payload_ex(packet);
	metadata_operation = fbe_payload_ex_get_metadata_operation(payload);
		   
	/* Check packet status */
	/*
	1. both packet status and metadata operation status are ok,
		the write/verify operation is successful, and we do not need to retry the write/verify OP
	2. if the metadata operation status isn't OK, no matter the packet status is OK or not,
		the write/verify operation is failed, and if the metadata operation status is IO correctable,
		we	need to retry the write/verify OP.
					   
	*/
	status = fbe_transport_get_status_code(packet);
	fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    if ((status == FBE_STATUS_OK)                           &&
        (metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK)    )
    {  
        /*metadata write verify op is successful*/
        base_config_write_verify_context->write_verify_op_success_b = FBE_TRUE;
        base_config_write_verify_context->write_verify_operation_retryable_b = FBE_FALSE;
    }
    else if (status == FBE_STATUS_OK)
    {
        base_config_write_verify_context->write_verify_op_success_b = FBE_FALSE;
        if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE)
        {
            base_config_write_verify_context->write_verify_operation_retryable_b = FBE_TRUE;
        }
        else
        {
            base_config_write_verify_context->write_verify_operation_retryable_b = FBE_FALSE;
        }
        fbe_metadata_translate_metadata_status_to_packet_status(metadata_status, &status);
        fbe_transport_set_status(packet, status, 0);
    }
    else
    {   
        /* Else packet status was not success
         */
        base_config_write_verify_context->write_verify_op_success_b = FBE_FALSE;
        base_config_write_verify_context->write_verify_operation_retryable_b = FBE_FALSE;
    }

    /*release current MD operation*/
	fbe_payload_ex_release_metadata_operation(payload, metadata_operation);
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_base_config_is_peer_present()
 ****************************************************************
 * @brief
 *  Is the peer currently alive?
 *
 * @param base_config_p - Ptr to current object.               
 *
 * @return fbe_bool_t FBE_TRUE if the peer is present.
 *                   FBE_FALSE if the peer is not present
 *
 * @author
 *  7/22/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_base_config_is_peer_present(fbe_base_config_t *base_config_p)
{
    if (!fbe_cmi_is_peer_alive() ||
        !fbe_metadata_is_peer_object_alive(&base_config_p->metadata_element))
    {
        /* If the cmi or metadata does not believe the peer is there, then there is no 
         * need to check the clustered flags. 
         */
        return FBE_FALSE;
    }
    else
    {
       return FBE_TRUE;
    }
}
/******************************************
 * end fbe_base_config_is_peer_present()
 ******************************************/
/*!**************************************************************
 * fbe_base_config_has_peer_joined()
 ****************************************************************
 * @brief
 *  Has the peer joined with us?
 *
 * @param base_config_p - Ptr to current object.               
 *
 * @return fbe_bool_t FBE_TRUE if the peer has joined .
 *                   FBE_FALSE if the peer has not joined
 *                   
 *
 * @author
 *  7/22/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_base_config_has_peer_joined(fbe_base_config_t *base_config_p)
{
    fbe_bool_t b_peer_flag_set;

    if (!fbe_base_config_is_peer_present(base_config_p))
    {
        /* If the cmi or metadata does not believe the peer is there, then there is no 
         * need to check the clustered flags. 
         */
        return FBE_FALSE;
    }

    /* When the peer joins us (which it does in a synchronized way) it will set the joined bit.
     */
    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set(base_config_p, 
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED);
    return b_peer_flag_set;
}
/******************************************
 * end fbe_base_config_has_peer_joined()
 ******************************************/

/*!****************************************************************************
 * fbe_base_config_is_background_operation_enabled()
 ******************************************************************************
 * @brief
 *   This function checks if given background operation is enabled or not.
 *
 * @param base_config_p             - pointer to the base config
 * @param background_op             - background operation 
 * @param is_enabled                - returns FBE_TRUE if background operation is enabled 
 *                                    otherwise return FBE_FALSE
 *
 * @return  fbe_status_t  
 *
 * @author
 *   08/16/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
fbe_status_t 
fbe_base_config_is_background_operation_enabled(fbe_base_config_t * base_config_p,
                                                               fbe_base_config_background_operation_t background_op,
                                                               fbe_bool_t*  is_enabled)
{
    fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p = NULL;
    fbe_base_config_background_operation_t background_op_bitmask;

    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);

    background_op_bitmask = base_config_nonpaged_metadata_p->operation_bitmask;

    if((background_op_bitmask & background_op) == background_op)
    {
        *is_enabled = FBE_FALSE;
    }
    else
    {
        *is_enabled = FBE_TRUE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_is_background_operation_enabled()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_enable_background_operation()
 ******************************************************************************
 * @brief
 *   This function is used to enable background operation. It updates 
 *   background operation bitmask in object's non paged metadata and 
 *   send this update to metadata service. 
 *
 * @param base_config_p             - pointer to the base config
 * @param packet_p                  - pointer to a control packet.
 * @param background_op             - background operation to enable
 *
 * @return  fbe_status_t  
 *
 * @author
 *   08/16/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
fbe_status_t 
fbe_base_config_enable_background_operation(fbe_base_config_t * base_config_p,
                                                                  fbe_packet_t*         packet_p,
                                                                  fbe_base_config_background_operation_t background_op)
{
    fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p = NULL;
    fbe_base_config_background_operation_t background_op_bitmask;
    fbe_status_t    status;

    fbe_base_config_get_nonpaged_metadata_ptr( base_config_p, (void **)&base_config_nonpaged_metadata_p);

    background_op_bitmask = base_config_nonpaged_metadata_p->operation_bitmask;

    /* enable the background operation with setting the bit OFF
     */
    background_op_bitmask &= (~background_op);

    /* sending request to metadata service to enable background operation */
    status = fbe_base_config_metadata_nonpaged_write( base_config_p,
                                                     packet_p,
                                                    (fbe_u64_t)(&((fbe_base_config_nonpaged_metadata_t*)0)->operation_bitmask),
                                                    (fbe_u8_t *)&background_op_bitmask,
                                                     sizeof(fbe_base_config_background_operation_t));

    return status;
}
/******************************************************************************
 * end fbe_base_config_enable_background_operation()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_disable_background_operation()
 ******************************************************************************
 * @brief
 *   This function is used to disable background operation. It updates 
 *   background operation bitmask in object's non paged metadata and 
 *   send this update request to metadata service. 
 *
 * @param base_config_p             - pointer to the base config
 * @param packet_p                  - pointer to a control packet.
 * @param background_op             - background operation to disable
 *
 * @return  fbe_status_t  
 *
 * @author
 *   08/16/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
fbe_status_t 
fbe_base_config_disable_background_operation(fbe_base_config_t * base_config_p,
                                                                  fbe_packet_t*         packet_p,
                                                                  fbe_base_config_background_operation_t background_op)
{
    fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata = NULL;
    fbe_base_config_background_operation_t background_op_bitmask;
    fbe_status_t    status;

    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata);

    background_op_bitmask = base_config_nonpaged_metadata->operation_bitmask;

    /* disable the background operation with setting the bit ON
     */
    background_op_bitmask |= background_op;

    /* sending request to metadata service to disable background operation */
    status = fbe_base_config_metadata_nonpaged_write( base_config_p,
                                                     packet_p,
                                                    (fbe_u64_t)(&((fbe_base_config_nonpaged_metadata_t*)0)->operation_bitmask),
                                                    (fbe_u8_t *)&background_op_bitmask,
                                                     sizeof(fbe_base_config_background_operation_t));

    return status;
}
/******************************************************************************
 * end fbe_base_config_disable_background_operation()
 ******************************************************************************/

/*!**************************************************************
 * fbe_base_config_get_np_distributed_lock()
 ****************************************************************
 * @brief
 *  This function fetches the distributed lock, protecting the
 *  non-paged metadata.  This lock is always the last slot
 *  inside the blob queue.  This corresponds to the very last
 *  stripe of the object.
 *
 * @param base_config - object ptr
 * @param packet - packet ptr
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t
fbe_base_config_get_np_distributed_lock(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_u64_t num_stripes;

    sep_payload = fbe_transport_get_payload_ex(packet_p);
    stripe_lock_operation_p = fbe_payload_ex_allocate_stripe_lock_operation(sep_payload);
    fbe_metadata_element_get_number_of_stripes(&base_config_p->metadata_element, &num_stripes);

    /* The NP distributed lock is always the last stripe. */
    fbe_payload_stripe_lock_build_write_lock(stripe_lock_operation_p, 
                                             &base_config_p->metadata_element, 
                                             num_stripes - 1, 1);	

    if( 0 == num_stripes ) {
       fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                             FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "!!! Critical error: 0 stripes for object: 0x%x",
                             base_config_p->base_object.object_id);
    }

    fbe_transport_set_completion_function(packet_p, base_config_get_np_distributed_lock_completion, base_config_p);

    fbe_payload_ex_increment_stripe_lock_operation_level(sep_payload);

    status = fbe_stripe_lock_operation_entry(packet_p);
    return status;
}
/******************************************
 * end fbe_base_config_get_np_distributed_lock()
 ******************************************/

/*!**************************************************************
 * base_config_get_np_distributed_lock_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for fetching the np distributed lock.
 *  If there is a failure we release the stripe lock operation.
 * 
 *  On success we leave the stripe lock operation allocated.
 *
 * @param base_config - object ptr
 * @param packet - packet ptr
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
base_config_get_np_distributed_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config_p = (fbe_base_config_t*)context;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t * stripe_lock_operation = NULL;
    fbe_status_t status;
    fbe_payload_stripe_lock_status_t stripe_lock_status;

    sep_payload = fbe_transport_get_payload_ex(packet_p);
    stripe_lock_operation =  fbe_payload_ex_get_stripe_lock_operation(sep_payload);
    
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_stripe_lock_get_status(stripe_lock_operation, &stripe_lock_status);

    /* On a failure we release the stripe lock since this is what the caller expects.
     */
    if ((status != FBE_STATUS_OK) || (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "bc_get_np_dl_cmpl error pkt_st: 0x%x sl_st: 0x%x\n", status, stripe_lock_status);
        fbe_payload_ex_release_stripe_lock_operation(sep_payload, stripe_lock_operation);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);    
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end base_config_get_np_distributed_lock_completion()
 ******************************************/

/*!**************************************************************
 * fbe_base_config_release_np_distributed_lock()
 ****************************************************************
 * @brief
 *  This function releases the distributed lock, protecting the
 *  non-paged metadata.  This lock is always the last slot
 *  inside the blob queue.  This corresponds to the very last
 *  stripe of the object.
 *
 * @param base_config - object ptr
 * @param packet - packet ptr
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t
fbe_base_config_release_np_distributed_lock(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_u64_t num_stripes;

    sep_payload = fbe_transport_get_payload_ex(packet_p);
    stripe_lock_operation_p =  fbe_payload_ex_get_stripe_lock_operation(sep_payload);

    fbe_metadata_element_get_number_of_stripes(&base_config_p->metadata_element, &num_stripes);

    if ( (stripe_lock_operation_p->stripe.first != (num_stripes - 1)) ||
         (stripe_lock_operation_p->stripe.last != stripe_lock_operation_p->stripe.first) )
    { 
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "bc_release_np_dl_cmpl error sn: 0x%llx ns: 0x%llx sc: 0x%llx\n", 
                              (unsigned long long)stripe_lock_operation_p->stripe.first, (unsigned long long)num_stripes, 
                              (unsigned long long)stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);
    }

    /* The NP distributed lock is always the last stripe. */
    fbe_payload_stripe_lock_build_write_unlock(stripe_lock_operation_p); 

    fbe_transport_set_completion_function(packet_p, base_config_release_np_distributed_lock_completion, base_config_p);

    status = fbe_stripe_lock_operation_entry(packet_p);
    return status;
}
/******************************************
 * end fbe_base_config_release_np_distributed_lock()
 ******************************************/

/*!**************************************************************
 * base_config_release_np_distributed_lock_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for release the np distributed lock.
 *  If there is a success or failure we release the stripe lock operation.
 *
 * @param base_config - object ptr
 * @param packet - packet ptr
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
base_config_release_np_distributed_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config_p = (fbe_base_config_t*)context;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_stripe_lock_operation_t * stripe_lock_operation = NULL;
    fbe_status_t status;
    fbe_payload_stripe_lock_status_t stripe_lock_status;

    sep_payload = fbe_transport_get_payload_ex(packet_p);
    stripe_lock_operation =  fbe_payload_ex_get_stripe_lock_operation(sep_payload);
    
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_stripe_lock_get_status(stripe_lock_operation, &stripe_lock_status);
    fbe_payload_ex_release_stripe_lock_operation(sep_payload, stripe_lock_operation);

    /* On failure, we trace and return bad packet status.
     */
    if ((status != FBE_STATUS_OK) || (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "bc_rel_np_dl_cmpl error pkt_st: 0x%x sl_st: 0x%x\n", status, stripe_lock_status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);    
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* On success, we preserve the packet status.
     */

    return FBE_STATUS_OK;
}
/******************************************
 * end base_config_release_np_distributed_lock_completion()
 ******************************************/

/*!**************************************************************
 * fbe_base_config_init_nonpaged_metadata_version_header()
 ****************************************************************
 * @brief
 *  This is the function for intialize the version header of non-paged metadata.
 *
 * @param nonpaged_metadata_p - non-paged metadata ptr
 * @param class_id - Which class of object this non-paged metadata is owned by
 * @param size - the size of the non-paged metadata
 *
 * @return fbe_status_t 
 *
 * @author
 *  4/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_config_init_nonpaged_metadata_version_header(fbe_base_config_nonpaged_metadata_t *nonpaged_metadata_p, 
                                                                   fbe_class_id_t class_id)
{
    fbe_u32_t np_metadata_size = 0;
    fbe_status_t status = FBE_STATUS_OK;

    if (!nonpaged_metadata_p) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Get the non-paged metadata size from database service. It returns the correct 
      version size according to the NDU state*/
    status = fbe_database_get_committed_nonpaged_metadata_size(class_id, &np_metadata_size);
    if (status != FBE_STATUS_OK) {
        /*Fail to get the size, set it to the maximum block size*/
        np_metadata_size = FBE_METADATA_NONPAGED_MAX_SIZE;
    }

    fbe_zero_memory(&nonpaged_metadata_p->version_header, sizeof (fbe_sep_version_header_t));
    nonpaged_metadata_p->version_header.size = np_metadata_size;

    return status;
}
/******************************************
 * end fbe_base_config_init_nonpaged_metadata_version_header()
 ******************************************/


/*!**************************************************************
 * fbe_base_config_nonpaged_metadata_get_version_size()
 ****************************************************************
 * @brief
 *  This is the function for intialize the version header of non-paged metadata.
 *
 * @param nonpaged_metadata_p - non-paged metadata ptr
 * @param size ptr - return the size of the non-paged metadata
 *
 * @return fbe_status_t 
 *
 * @author
 *  5/16/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_config_nonpaged_metadata_get_version_size(fbe_base_config_nonpaged_metadata_t *nonpaged_metadata_p, fbe_u32_t *size)
{

    if (!nonpaged_metadata_p || !size) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *size = nonpaged_metadata_p->version_header.size;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_config_nonpaged_metadata_get_version_size()
 ******************************************/
fbe_status_t fbe_base_config_get_nonpaged_metadata_state(fbe_base_config_t * base_config_p, fbe_base_config_nonpaged_metadata_state_t* out_state_p)
{

    fbe_base_config_nonpaged_metadata_t *               base_config_nonpaged_metadata_p = NULL;

	fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
	if(base_config_nonpaged_metadata_p == NULL
		|| out_state_p == NULL){ 
		/* The nonpaged pointers are not initialized yet */
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s The nonpaged pointers are not initialized yet.\n",
                              __FUNCTION__);
		if(out_state_p != NULL)
		{
		   *out_state_p = FBE_NONPAGED_METADATA_UNINITIALIZED;
		}
		return FBE_STATUS_GENERIC_FAILURE;
	}
	*out_state_p = base_config_nonpaged_metadata_p->nonpaged_metadata_state;
    return FBE_STATUS_OK;

}


static fbe_status_t 
fbe_base_config_set_nonpaged_metadata_state_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{
    fbe_base_config_t *                 base_config_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lifecycle_status_t              lifecycle_status;

    base_config_p = (fbe_base_config_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set np metadata state failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
        return FBE_STATUS_OK;
    }

    /* set completion */
    fbe_transport_set_completion_function(packet_p, fbe_base_config_persist_nonpaged_metadata_state_completion, base_config_p);

    /* Simply use the base config persist method.
     */
    lifecycle_status = fbe_base_config_persist_non_paged_metadata_cond_function((fbe_base_object_t *)base_config_p, packet_p);
    if (lifecycle_status == FBE_LIFECYCLE_STATUS_PENDING) {
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_set_nonpaged_metadata_state(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p, fbe_base_config_nonpaged_metadata_state_t state)
{

    fbe_base_config_nonpaged_metadata_t *               base_config_nonpaged_metadata_p = NULL;
    fbe_base_config_nonpaged_metadata_state_t new_state  = state;
    fbe_status_t status;

	fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
	if(base_config_nonpaged_metadata_p == NULL){ 
		/* The nonpaged pointers are not initialized yet */
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s The nonpaged pointers are not initialized yet.\n",
                              __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	//base_config_nonpaged_metadata_p->nonpaged_metadata_state = state;

    /* set completion */
    fbe_transport_set_completion_function(packet_p, fbe_base_config_set_nonpaged_metadata_state_completion, base_config_p);

    status = fbe_base_config_metadata_nonpaged_write(base_config_p,
                                                     packet_p,
                                                     (fbe_u64_t)(&((fbe_base_config_nonpaged_metadata_t*)0)->nonpaged_metadata_state),
                                                     (fbe_u8_t *)&new_state,
                                                     sizeof(fbe_base_config_nonpaged_metadata_state_t)); /* size of the non paged metadata. */

    return FBE_STATUS_OK;

}


/*!**************************************************************
 * fbe_base_config_metadata_memory_read()
 ****************************************************************
 * @brief
 *  This is the function for reading metadata memory
 *
 * @param base_config - the object to read
 * @param is_peer - if true, read from peer memory; else read from local memory
 * @param buffer - buffer to store the metadata
 * @param buffer_size - sizeo of buffer
 * @param copy_size - return the size of copy data
 * @return fbe_status_t
 *
 * @author
 *  10/29/2013 - Created. Jamin Kang
 *
 ****************************************************************/
fbe_status_t
fbe_base_config_metadata_memory_read(fbe_base_config_t *base_config,
                                     fbe_bool_t is_peer,
                                     fbe_u8_t *buffer, fbe_u32_t buffer_size,
                                     fbe_u32_t *copy_size)
{
    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;
    fbe_u32_t memory_size;
    fbe_u32_t len;

    if (is_peer) {
        fbe_metadata_element_get_peer_metadata_memory(&base_config->metadata_element, (void **)&metadata_memory_ptr);
        fbe_metadata_element_get_peer_metadata_memory_size(&base_config->metadata_element, &memory_size);
    } else {
        fbe_metadata_element_get_metadata_memory(&base_config->metadata_element, (void **)&metadata_memory_ptr);
        fbe_metadata_element_get_metadata_memory_size(&base_config->metadata_element, &memory_size);
    }

    if (metadata_memory_ptr == NULL) {
        *copy_size = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (memory_size < buffer_size) {
        len = memory_size;
    } else {
        len = buffer_size;
    }

    fbe_base_config_lock(base_config);
    fbe_copy_memory(buffer, metadata_memory_ptr, len);
    fbe_base_config_unlock(base_config);

    *copy_size = len;
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_base_config_metadata_memory_upate()
 ****************************************************************
 * @brief
 *  This is the function for updating metadata memory
 *
 * @param base_config - the object to update
 * @param buffer - buffer to store the metadata
 * @param mask - mask to specific which field to update
 * @param size - sizeo of buffer/mask
 * @return fbe_status_t
 *
 * @author
 *  10/29/2013 - Created. Jamin Kang
 *
 ****************************************************************/
fbe_status_t
fbe_base_config_metadata_memory_update(fbe_base_config_t *base_config,
                                       fbe_u8_t *buffer, fbe_u8_t *mask,
                                       fbe_u32_t offset, fbe_u32_t size)
{
    fbe_u8_t * metadata_memory_ptr = NULL;
    fbe_u32_t memory_size;
    fbe_u32_t i;
    fbe_status_t status;

    fbe_metadata_element_get_metadata_memory(&base_config->metadata_element, (void **)&metadata_memory_ptr);
    fbe_metadata_element_get_metadata_memory_size(&base_config->metadata_element, &memory_size);

    if (metadata_memory_ptr == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (offset >= memory_size ||
        offset + size > memory_size) {
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: invalid range: %u:%u, metadata size: %u\n",
                              __FUNCTION__, offset, size, memory_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_config_lock(base_config);

    /* update metadata memory by bytes.
     * We use mask to control which bits to update
     * mask == '1' means this bit need to update
     */
    for (i = offset; i < offset + size; i++) {
        fbe_u8_t mask_byte = mask[i];

        if (mask_byte != 0) {
            fbe_u8_t b = metadata_memory_ptr[i];

            b &= ~mask[i];
            b |= buffer[i] & mask[i];
            metadata_memory_ptr[i] = b;
        }
    }
    fbe_base_config_unlock(base_config);

    /* It is safe to use base_config_lifecycle_const for ALL types.
     * UPDATE_METADATA_MEMORY cond only exists in base_config rotaries
     */
    status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const,
                                    (fbe_base_object_t*)base_config,
                                    FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config, FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: cannot set condition\n",__FUNCTION__);
        return status;
    }

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_base_config_persist_nonpaged_metadata_state_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{
    fbe_base_config_t *                 base_config_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                          is_non_paged_initialized_p;

    base_config_p = (fbe_base_config_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set np metadata state failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
        return FBE_STATUS_OK;
    }

    fbe_base_config_metadata_is_nonpaged_state_initialized(base_config_p, &is_non_paged_initialized_p);

    /* peer may have requested NP in the past, but we ignored it because we are not ready to answer at the time,
     * now let's go back and process again.
     */
    if ((is_non_paged_initialized_p == FBE_TRUE) &&
        (fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST)))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s peer requested non paged before, ready to answer now.\n",
                              __FUNCTION__);
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                               (fbe_base_object_t*)base_config_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;

}

/*************************
 * end file fbe_base_config_metadata.c
 *************************/
