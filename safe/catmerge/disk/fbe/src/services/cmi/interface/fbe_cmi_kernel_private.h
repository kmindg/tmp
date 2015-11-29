#ifndef FBE_CMI_KERNEL_PRIVATE_H
#define FBE_CMI_KERNEL_PRIVATE_H

#include "fbe/fbe_types.h"
#include "CmiUpperInterface.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"
#include "fbe/fbe_winddk.h"

typedef struct fbe_cmi_kernel_message_info_s{
	PEMCPAL_IRP								pirp;
    CMI_TRANSMIT_MESSAGE_IOCTL_INFO		cmid_ioctl;
	fbe_cmi_client_id_t					client_id;
	fbe_cmi_event_callback_function_t 	callback;
	fbe_cmi_event_callback_context_t 	context;
	fbe_cmi_message_t 					user_msg;
	CMI_PHYSICAL_SG_ELEMENT				cmi_sgl[2];
}fbe_cmi_kernel_message_info_t;

typedef struct fbe_cmi_driver_s{
    PEMCPAL_FILE_OBJECT			cmid_file_object;
}fbe_cmi_driver_t;


fbe_status_t fbe_cmi_kernel_allocate_message_pool(void);
void fbe_cmi_kernel_destroy_message_pool(void);
fbe_cmi_kernel_message_info_t * fbe_cmi_kernel_get_message_info_memory(void);
void fbe_cmi_kernel_return_message_info_memory(fbe_cmi_kernel_message_info_t *returned_msg);
fbe_status_t fbe_cmi_kernel_io_alloc_memory(void);
void fbe_cmi_kernel_io_destroy_memory(void);
void fbe_cmi_kernel_mark_irp_completed(fbe_cmi_kernel_message_info_t *returned_msg);


#endif/*FBE_CMI_KERNEL_PRIVATE_H*/
