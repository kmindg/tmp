#ifndef FBE_METADATA_PRIVATE_H
#define FBE_METADATA_PRIVATE_H

#include "csx_ext.h"
#include "fbe/fbe_metadata_interface.h"
#include "fbe_metadata.h"

/* _main.c */
void metadata_trace(fbe_trace_level_t trace_level, fbe_trace_message_id_t message_id, const char * fmt, ...) __attribute__((format(__printf_func__,3,4)));
fbe_status_t fbe_metadata_switch_all_elements_to_state(fbe_metadata_element_state_t new_state);

fbe_status_t fbe_metadata_get_element_queue_head(fbe_queue_head_t ** queue_head);

void fbe_metadata_element_queue_lock(void);
void fbe_metadata_element_queue_unlock(void);
fbe_metadata_element_t * fbe_metadata_queue_element_to_metadata_element(fbe_queue_element_t * queue_element);

fbe_status_t fbe_metadata_calculate_checksum(void * metadata_block_p, fbe_u32_t size);
fbe_status_t 
	fbe_metadata_raw_mirror_calculate_checksum(void * metadata_block_p, fbe_lba_t start_lba,fbe_u32_t size,fbe_bool_t seq_reset);

fbe_status_t fbe_metadata_scan_for_cancelled_packets(fbe_bool_t is_force_scan);

/* The caller MUST hold metadata_element_queue_lock lock */
fbe_status_t fbe_metadata_get_element_by_object_id(fbe_object_id_t object_id, fbe_metadata_element_t ** metadata_element);

extern fbe_bool_t ndu_in_progress;
static __forceinline fbe_bool_t fbe_metadata_is_ndu_in_progress(void)
{
    return (ndu_in_progress);
}


/* _nonpaged.c */
fbe_status_t fbe_metadata_nonpaged_init(void);
fbe_status_t fbe_metadata_nonpaged_destroy(void);
fbe_bool_t metadata_common_cmi_is_active(void);


/*define the valid sequence number region database raw-mirror blocks use.
  avoid use the pure 0 or pure 1*/
#define METADATA_RAW_MIRROR_START_SEQ              ((fbe_u64_t)0xbeefdaed)
#define METADATA_RAW_MIRROR_END_SEQ                ((fbe_u64_t)0xFFFFFFFFFFFFdead)

fbe_status_t fbe_metadata_nonpaged_system_clear(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_system_load(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_system_load_with_diskmask(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_system_persist(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_system_zero_and_persist(fbe_packet_t *packet);

fbe_status_t fbe_metadata_nonpaged_load(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_persist(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_operation_write_verify(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_backup_restore(fbe_packet_t * packet);

struct fbe_metadata_cmi_message_s;
fbe_status_t fbe_metadata_nonpaged_operation_init(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_operation_read_persist(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_operation_write(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_operation_change_bits(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_operation_change_checkpoint(fbe_packet_t * packet);
fbe_status_t fbe_metadata_nonpaged_operation_post_persist(fbe_packet_t * packet);

fbe_status_t fbe_metadata_nonpaged_cmi_dispatch(fbe_metadata_element_t * metadata_element, struct fbe_metadata_cmi_message_s * metadata_cmi_msg);
fbe_status_t fbe_metadata_nonpaged_operation_persist(fbe_packet_t * packet);
void fbe_metadata_nonpaged_get_memory_use(fbe_u32_t *memory_mb_p);

fbe_status_t fbe_metadata_nonpaged_get_nonpaged_metadata_data_ptr(fbe_packet_t * packet);

/* _stripe_lock.c */
fbe_status_t fbe_metadata_stripe_lock_init(void);
fbe_status_t fbe_metadata_stripe_lock_destroy(void);

fbe_status_t fbe_metadata_stripe_lock_restart_element(fbe_metadata_element_t *metadata_element_p);

fbe_status_t fbe_metadata_stripe_lock_cmi_dispatch(fbe_metadata_element_t * metadata_element, 
													struct fbe_metadata_cmi_message_s * metadata_cmi_msg,
														fbe_u32_t cmi_message_length);

fbe_status_t fbe_metadata_stripe_lock_release_all_blobs(fbe_metadata_element_t * metadata_element);

fbe_status_t fbe_metadata_stripe_lock_control_entry(fbe_packet_t * packet);

fbe_status_t fbe_metadata_stripe_lock_peer_lost(void);
fbe_status_t fbe_metadata_element_stripe_lock_complete_cancelled(fbe_metadata_element_t *metadata_element);
void fbe_metadata_sl_get_memory_use(fbe_u32_t *memory_mb_p);

/* _ext_pool_lock.c */
fbe_status_t fbe_ext_pool_lock_init(void);
fbe_status_t fbe_ext_pool_lock_destroy(void);
fbe_status_t fbe_ext_pool_lock_control_entry(fbe_packet_t * packet);
fbe_status_t fbe_ext_pool_lock_release_all_blobs(fbe_metadata_element_t * metadata_element);
void fbe_metadata_ext_pool_lock_get_memory_use(fbe_u32_t *memory_bytes_p);
fbe_status_t fbe_ext_pool_lock_cmi_dispatch(fbe_metadata_element_t * metadata_element, 
									        struct fbe_metadata_cmi_message_s * metadata_cmi_msg,
									        fbe_u32_t cmi_message_length);
fbe_status_t fbe_ext_pool_lock_peer_lost_mde(fbe_metadata_element_t * mde, fbe_queue_head_t * tmp_queue, fbe_bool_t * is_cmi_required);
fbe_status_t fbe_metadata_element_ext_pool_lock_complete_cancelled(fbe_metadata_element_t * mde);
fbe_status_t fbe_ext_pool_lock_element_abort_monitor_ops(fbe_metadata_element_t * mde, fbe_bool_t abort_peer);

/* _paged.c */
fbe_status_t fbe_metadata_paged_init(void);
fbe_status_t fbe_metadata_paged_destroy(void);

fbe_status_t fbe_metadata_paged_operation_entry(fbe_packet_t * packet);
fbe_status_t fbe_metadata_paged_operation_restart_io(fbe_metadata_element_t * metadata_element);
fbe_status_t fbe_metadata_paged_abort_io(fbe_metadata_element_t * metadata_element);
fbe_status_t fbe_metadata_paged_drain_io(fbe_metadata_element_t * metadata_element);

fbe_status_t fbe_metadata_paged_init_queue(fbe_metadata_element_t* metadata_element_p);
fbe_status_t fbe_metadata_paged_destroy_queue(fbe_metadata_element_t* metadata_element_p);

fbe_status_t fbe_metadata_paged_release_blobs(fbe_metadata_element_t * metadata_element, fbe_lba_t stripe_offset, fbe_block_count_t stripe_count);
fbe_status_t fbe_metadata_element_paged_complete_cancelled(fbe_metadata_element_t *metadata_element);
void fbe_metadata_paged_get_memory_use(fbe_u32_t *memory_mb_p);

/* fbe_metadata_cancel_thread.c */
fbe_status_t fbe_metadata_cancel_thread_notify(void);
fbe_status_t fbe_metadata_cancel_thread_init(void);
fbe_status_t fbe_metadata_cancel_thread_destroy(void);
fbe_status_t fbe_metadata_cancel_function(fbe_packet_completion_context_t context);
fbe_status_t fbe_metadata_raw_mirror_io_cb_init(fbe_metadata_raw_mirror_io_cb_t *rw_mirr_cb,fbe_block_count_t capacity);
fbe_status_t fbe_metadata_raw_mirror_io_cb_destroy(fbe_metadata_raw_mirror_io_cb_t *rw_mirr_cb);

fbe_status_t fbe_metadata_nonpaged_set_blocks_per_object(fbe_packet_t * packet);
#endif /* FBE_METADATA_PRIVATE_H */
