#ifndef FBE_METADATA_CMI_H
#define FBE_METADATA_CMI_H

#include "fbe/fbe_types.h"
//#include "fbe_metadata_stripe_lock.h"
#include "fbe/fbe_metadata_interface.h"
#include "fbe_cmi.h"

enum fbe_metadata_cmi_constants_e{
	FBE_METADATA_CMI_MEMORY_UPDATE_DATA_SIZE = 256, /* in bytes */
	FBE_METADATA_CMI_NONPAGED_WRITE_DATA_SIZE = FBE_METADATA_NONPAGED_MAX_SIZE, /* in bytes */
};


typedef struct fbe_metadata_cmi_nonpaged_write_s{
	fbe_u64_t offset;
	fbe_u8_t data[FBE_METADATA_CMI_NONPAGED_WRITE_DATA_SIZE];
	fbe_u32_t data_size;
    fbe_u64_t seq_number; // sequence number from active sp for system objects
}fbe_metadata_cmi_nonpaged_write_t;

typedef struct fbe_metadata_cmi_nonpaged_change_s{
	fbe_u64_t offset;
	fbe_u8_t data[FBE_METADATA_CMI_NONPAGED_WRITE_DATA_SIZE];
	fbe_u64_t repeat_count;
	fbe_u32_t data_size;
    fbe_u64_t second_offset;
    fbe_u64_t seq_number; // sequence number from active sp for system objects
}fbe_metadata_cmi_nonpaged_change_t;

enum fbe_metadata_cmi_lock_flags_e {
	FBE_METADATA_CMI_FLAG_RESERVE_REQUEST = 0x00000001, /* Peer requests reserve grant */
	FBE_METADATA_CMI_FLAG_RESERVE_GRANT   = 0x00000002, /* Peer provides reserve grant */
};


typedef struct fbe_metadata_cmi_message_s {
	fbe_metadata_cmi_message_header_t header;

	union {
		fbe_u8_t memory_update_data[FBE_METADATA_CMI_MEMORY_UPDATE_DATA_SIZE];
		fbe_metadata_cmi_nonpaged_write_t nonpaged_write;
		fbe_metadata_cmi_nonpaged_change_t nonpaged_change;
	}msg;

}fbe_metadata_cmi_message_t;


fbe_status_t fbe_metadata_cmi_init(void);

fbe_status_t fbe_metadata_cmi_destroy(void);

fbe_status_t fbe_metadata_cmi_send_message(fbe_metadata_cmi_message_t * metadata_cmi_message, void * context);

fbe_status_t fbe_metadata_stripe_lock_message_event(fbe_cmi_event_t event,
													fbe_metadata_cmi_message_t *metadata_cmi_msg, 
													fbe_cmi_event_callback_context_t context);
fbe_status_t fbe_ext_pool_lock_message_event(fbe_cmi_event_t event,
													fbe_metadata_cmi_message_t *metadata_cmi_msg, 
													fbe_cmi_event_callback_context_t context);

#endif /* FBE_METADATA_CMI_H */
