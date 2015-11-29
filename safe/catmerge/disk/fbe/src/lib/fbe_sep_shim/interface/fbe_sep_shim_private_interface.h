#ifndef FBE_SEP_SHIM_PRIVATE_INTERFACE_H
#define FBE_SEP_SHIM_PRIVATE_INTERFACE_H

#include "csx_ext.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_sep_shim.h"

typedef struct fbe_sep_shim_io_struct_s{
    fbe_queue_element_t     queue_element;/*should stay first*/
    fbe_packet_t *          packet;
    void *                  associated_io;
    void *                  associated_device;
    void *                  context;
    void *                  xor_mem_move_ptr;
    void *                  flare_dca_arg;
    void *                  dca_table;
    fbe_cpu_id_t            core_take_from;/*temporary only*/
    fbe_u32_t               coarse_time; 
    fbe_u32_t               coarse_wait_time; 
    fbe_u32_t               reserved;   /* not used */
    fbe_memory_request_t    memory_request; 
    fbe_memory_request_t    buffer; /* Memory Request for allocating IO buffers */
}fbe_sep_shim_io_struct_t;

void fbe_sep_shim_trace(fbe_trace_level_t trace_level, const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,2,3)));

/*used in asynch allocation*/
fbe_status_t fbe_sep_shim_get_io_structure(void *io_request, fbe_sep_shim_io_struct_t **io_struct);
void fbe_sep_shim_return_io_structure(fbe_sep_shim_io_struct_t *io_struct);

void fbe_sep_shim_init_io_memory(void);
void fbe_sep_shim_destroy_io_memory(void);
void fbe_sep_shim_init_background_activity_manager_interface(void);
void fbe_sep_shim_destroy_background_activity_manager_interface(void);
fbe_status_t fbe_sep_shim_ioctl_run_queue_push_packet(fbe_packet_t * packet);
fbe_u32_t fbe_sep_shim_calculate_allocation_chunks(fbe_block_count_t blocks, fbe_u32_t *chunks, fbe_memory_chunk_size_t *chunk_size);
fbe_u32_t fbe_sep_shim_calculate_dca_allocation_chunks(fbe_block_count_t blocks, fbe_u32_t *chunks, fbe_memory_chunk_size_t *chunk_size);
void sep_handle_cancel_irp_function(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp);
void fbe_sep_shim_get_io_structure_cancel_irp(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP PIrp);
void fbe_sep_shim_get_memory_cancel_irp(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP PIrp);
void fbe_sep_shim_init_waiting_request_data(void);
fbe_bool_t fbe_sep_shim_is_waiting_request_queue_empty(fbe_u32_t cpu_id);
fbe_status_t fbe_sep_shim_enqueue_request(void *io_request, fbe_u32_t cpu_id);
void fbe_sep_shim_wake_waiting_request(fbe_u32_t cpu_id);
fbe_sep_shim_io_struct_t * fbe_sep_shim_process_io_structure_request(fbe_cpu_id_t cpu_id, void *io_request);
void fbe_sep_shim_destroy_waiting_requests(void);
void fbe_sep_shim_increment_wait_stats(fbe_cpu_id_t cpu_id);
void fbe_sep_shim_decrement_wait_stats(fbe_cpu_id_t cpu_id);
void fbe_sep_shim_set_shutdown_in_progress(void);
fbe_bool_t fbe_sep_shim_is_shutdown_in_progress(void);
fbe_status_t fbe_sep_shim_display_irp(PEMCPAL_IRP PIrp, fbe_trace_level_t trace_level);
#endif


