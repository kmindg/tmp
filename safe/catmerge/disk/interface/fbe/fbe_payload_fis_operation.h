#ifndef FBE_PAYLOAD_FIS_OPERATION_H
#define FBE_PAYLOAD_FIS_OPERATION_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_payload_sg_descriptor.h"
#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_port.h"

typedef enum fbe_payload_fis_constants_e {
	FBE_PAYLOAD_FIS_SIZE = 20,
	FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE = 20
}fbe_payload_fis_constants_t;

/* SCSI relates definitions from SPC-3 and SAM-3 and from CPD_TAG_TYPE_TAG definition in cpd_generic.h */
typedef enum fbe_payload_fis_task_attribute_e {
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_INVALID,

    /* Note TCD uses bool tests for untagged - so must be 0
		CPD_UNTAGGED                        = 0x00,
	*/
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_UNTAGGED,

    /* These values are taken from the SAM spec.
		CPD_SIMPLE_QUEUE_TAG                = 0x20,
		CPD_HEAD_OF_QUEUE_TAG               = 0x21,
		CPD_ORDERED_QUEUE_TAG               = 0x22,
		CPD_ACA_QUEUE_TAG                   = 0x2f,
	*/
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_SIMPLE,			/* windows SRB_SIMPLE_TAG_REQUEST */
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_ORDERED,			/* windows SRB_ORDERED_QUEUE_TAG_REQUEST */
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_HEAD_OF_QUEUE,	/* windows SRB_HEAD_OF_QUEUE_TAG_REQUEST */
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_ACA,				/* ??? */

    /* These values below are NOT from the SAM spec. (They are randomly chosen. :-)
		CPD_SATA_NCQ_TAG                    = 0x80,
		CPD_SATA_DMA_UNTAGGED               = 0x81,
		CPD_SATA_PIO_UNTAGGED               = 0x82,
		CPD_SATA_NON_DATA                   = 0x83
	*/
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NCQ,				/* ??? */
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_DMA,				/* ??? */
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_PIO,				/* ??? */
	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_NON_DATA,		/* ??? */

	FBE_PAYLOAD_FIS_TASK_ATTRIBUTE_LAST
}fbe_payload_fis_task_attribute_t;

typedef fbe_u32_t fbe_payload_fis_queue_tag_t;

typedef enum fbe_payload_fis_flags_e {
    FBE_PAYLOAD_FIS_FLAGS_DATA_IN                   = 0x00000040,
    FBE_PAYLOAD_FIS_FLAGS_DATA_OUT                  = 0x00000080,
    FBE_PAYLOAD_FIS_FLAGS_NO_DATA_TRANSFER          = 0x00000000,
    FBE_PAYLOAD_FIS_FLAGS_UNSPECIFIED_DIRECTION     = 0x000000C0, /* (FBE_PAYLOAD_FIS_FLAGS_DATA_IN | FBE_PAYLOAD_FIS_FLAGS_DATA_OUT) */
    FBE_PAYLOAD_FIS_FLAGS_DRIVE_FW_UPGRADE          = 0x00000100, /* Used to inform the miniport that we're doing a drive FW upgrade */
}fbe_payload_fis_flags_t;

typedef struct fbe_payload_fis_operation_s{
	fbe_payload_operation_header_t		operation_header; /* Must be first */

	fbe_u8_t							fis[FBE_PAYLOAD_FIS_SIZE];
	fbe_u8_t							fis_length;
	
	fbe_payload_fis_task_attribute_t	payload_fis_task_attribute;	/* IN scsi task attributes ( queueing type) */
	fbe_payload_fis_flags_t				payload_fis_flags;
	fbe_port_request_status_t			port_request_status;
    fbe_port_recovery_status_t			port_recovery_status;


	fbe_u8_t							response_buffer[FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE];
    fbe_u32_t							response_buffer_length;
	fbe_time_t							timeout;
    fbe_payload_sg_descriptor_t			payload_sg_descriptor; /* describes sg list passed in. */
	fbe_u32_t							transfer_count;
	fbe_u32_t                           transferred_count; /* Actual number of bytes transfered.*/
}fbe_payload_fis_operation_t;

/*!**************************************************************
 * @fn fbe_payload_fis_get_sg_descriptor(
 *          fbe_payload_fis_operation_t *payload_fis_operation,
 *          fbe_payload_sg_descriptor_t ** sg_descriptor)
 ****************************************************************
 * @brief Returns the pointer to the sg descriptor of the fis operation
 * payload.
 *
 * @param payload_fis_operation - Pointer to the fis payload struct.
 * @param sg_descriptor - Pointer to the sg descriptor pointer to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_fis_operation_t 
 * @see fbe_payload_fis_operation_t::sg_descriptor
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_fis_get_sg_descriptor(fbe_payload_fis_operation_t * payload_fis_operation,
									fbe_payload_sg_descriptor_t ** sg_descriptor)
{
	* sg_descriptor = &payload_fis_operation->payload_sg_descriptor;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_payload_fis_build_read_fpdma_queued(fbe_payload_fis_operation_t * payload_fis_operation,
											fbe_lba_t lba,
											fbe_block_count_t block_count,
											fbe_time_t timeout);

fbe_status_t 
fbe_payload_fis_build_write_fpdma_queued(fbe_payload_fis_operation_t * payload_fis_operation,
								fbe_lba_t lba,
								fbe_block_count_t block_count,
								fbe_time_t timeout);

fbe_status_t fbe_payload_fis_set_tag(fbe_payload_fis_operation_t * fis_operation, fbe_u8_t tag);
fbe_status_t fbe_payload_fis_get_tag(fbe_payload_fis_operation_t * fis_operation, fbe_u8_t * tag);

fbe_status_t fbe_payload_fis_build_identify_device(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_read_native_max_address_ext(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_read_inscription(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout, fbe_u64_t lba);
fbe_status_t fbe_payload_fis_build_check_power_mode(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_execute_device_diag(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_flush_write_cache(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_disable_write_cache(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_enable_rla(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_set_pio_mode(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout, fbe_u8_t PIOMode);
fbe_status_t fbe_payload_fis_build_set_udma_mode(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout, fbe_u8_t UDMAMode);
fbe_status_t fbe_payload_fis_build_enable_smart(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_enable_smart_autosave(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_smart_return_status(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_smart_read_attributes(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_sct_set_timer(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_firmware_download(fbe_payload_fis_operation_t * fis_operation, fbe_block_count_t block_count, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_standby(fbe_payload_fis_operation_t * fis_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_fis_build_write_uncorrectable(fbe_payload_fis_operation_t * fis_operation, fbe_lba_t lba, fbe_time_t timeout);

fbe_status_t fbe_payload_fis_set_transfer_count(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t transfer_count);
fbe_status_t fbe_payload_fis_get_transfer_count(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t * transfer_count);

fbe_status_t fbe_payload_fis_set_transferred_count(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t transferred_count);
fbe_status_t fbe_payload_fis_get_transferred_count(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t * transferred_count);

fbe_status_t fbe_payload_fis_set_response_buffer_length(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t response_buffer_length);
fbe_status_t fbe_payload_fis_get_response_buffer_length(fbe_payload_fis_operation_t * fis_operation, fbe_u32_t * response_buffer_length);
fbe_status_t fbe_payload_fis_get_lba(fbe_payload_fis_operation_t * fis_operation, fbe_lba_t * lba);
fbe_status_t fbe_payload_fis_get_block_count(fbe_payload_fis_operation_t * fis_operation, fbe_block_count_t * block_count);

fbe_status_t fbe_payload_fis_set_request_status(fbe_payload_fis_operation_t * fis_operation, fbe_port_request_status_t request_status);
fbe_status_t fbe_payload_fis_get_request_status(fbe_payload_fis_operation_t * fis_operation, fbe_port_request_status_t * request_status);

fbe_status_t fbe_payload_fis_set_recovery_status(fbe_payload_fis_operation_t * fis_operation, fbe_port_recovery_status_t recovery_status);
fbe_status_t fbe_payload_fis_get_recovery_status(fbe_payload_fis_operation_t * fis_operation, fbe_port_recovery_status_t * recovery_status);

void fbe_payload_fis_trace_fis_buff(fbe_payload_fis_operation_t * fis_operation);
void fbe_payload_fis_trace_response_buff(fbe_payload_fis_operation_t * fis_operation);

#endif /* FBE_PAYLOAD_FIS_OPERATION_H */
