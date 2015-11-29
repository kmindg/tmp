#ifndef _CPD_SHIM_KERNEL_PRIVATE_H_
#define _CPD_SHIM_KERNEL_PRIVATE_H_

#include "flare_sgl.h"

#define FBE_CPD_DEVICE_NAME_SIZE 64
#define FBE_CPD_PASS_THRU_BUFFER_SIZE 64

#define FBE_CPD_SHIM_MEMORY_TAG				'mihs'
#define FBE_CPD_SHIM_MAX_EVENT_QUEUE_ENTRY	150

#define FBE_CPD_SHIM_INVALID_PORT_NUMBER	0xFFFF
#define FBE_CPD_SHIM_INVALID_PORTAL_NUMBER	0xFFFF

// These definitions are placed here because they are
// new additions to the scsi spec and thus are not
// defined in the ddk.

#define SCSISTAT_ACA_ACTIVE             0x30
#define SCSISTAT_TASK_ABORTED           0x40

typedef struct cpd_port_s{    
    fbe_bool_t              is_valid;    
	fbe_u32_t				io_port_number;
	fbe_u32_t				io_portal_number;
	PEMCPAL_FILE_OBJECT			miniport_file_object;
	PEMCPAL_DEVICE_OBJECT			miniport_device_object;
	PVOID					miniport_callback_handle;
    VOID                    (*process_io)(void * miniport_context);
    CPD_DMRB_STATUS         (*enqueue_pending_dmrb)(void * miniport_context, 
                                                     CPD_DMRB * dmrb_ptr, 
                                                     UINT_32 * queue_pos);
	PVOID					enque_pending_dmrb_context;

        EMCPAL_DPC_OBJECT                       port_event_callback_dpc;
        EMCPAL_DPC_OBJECT                       device_event_callback_dpc;

	cpd_shim_port_state_t state;

	fbe_payload_ex_completion_function_t payload_completion_function;
	fbe_payload_ex_completion_context_t  payload_completion_context;

	fbe_cpd_shim_callback_function_t	callback_function;
	fbe_cpd_shim_callback_context_t		callback_context;
	fbe_cpd_shim_callback_info_t	   *port_event_callback_info_queue;
	fbe_spinlock_t                      consumer_index_lock;
	fbe_atomic_t					    consumer_index;
	fbe_atomic_t					    producer_index;
    fbe_atomic_t		                send_process_io_ioctl;
	/* Port specific Information.*/
	fbe_atomic_t						    current_generation_code;
    fbe_cpd_shim_port_info_t			    port_info;
    fbe_bool_t                              sfp_info_valid;
    fbe_cpd_shim_sfp_media_interface_info_t sfp_info;
    fbe_atomic_t						    port_reset_in_progress;
	/* Device specific information.*/
	fbe_cpd_shim_device_table_t    	   *topology_device_information_table;
	fbe_u32_t						    device_table_size;

    fbe_bool_t                          mc_core_affinity_enabled;
    fbe_u64_t                           core_affinity_proc_mask;

#ifndef ALAMOSA_WINDOWS_ENV
    csx_p_dpc_t cdb_io_completion_dpc;
    csx_p_spl_t cdb_io_completion_list_lock;
    csx_dlist_head_t cdb_io_completion_list;

    csx_p_spl_t fis_io_completion_list_lock;
    csx_p_dpc_t fis_io_completion_dpc;
    csx_dlist_head_t fis_io_completion_list;
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */
}cpd_port_t;

typedef struct cpd_dmrb_io_context_s{
	CPD_DMRB				dmrb;
#ifdef ALAMOSA_WINDOWS_ENV
	KDPC					dpc;
#else
    csx_dlist_entry_t         io_completion_list_entry;
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - BAD DPC MODEL FIX */
	fbe_u32_t				port_handle;
	fbe_cpd_device_id_t     device_id;
	fbe_payload_ex_t			*payload;	
	SGL_DESCRIPTOR			sgl_descriptor[3]; /* pre , sg and post */
}cpd_dmrb_io_context_t;

typedef struct cpd_shim_ioctl_process_io_s {
	fbe_queue_element_t queue_element;
	fbe_u32_t  port_handle;
} cpd_shim_ioctl_process_io_t;

typedef enum cpd_shim_ioctl_process_io_thread_flag_e{
    IOCTL_PROCESS_IO_THREAD_RUN,
    IOCTL_PROCESS_IO_THREAD_STOP,
    IOCTL_PROCESS_IO_THREAD_DONE
}cpd_shim_ioctl_process_io_thread_flag_t;


#endif /* _CPD_SHIM_KERNEL_PRIVATE_H_ */
