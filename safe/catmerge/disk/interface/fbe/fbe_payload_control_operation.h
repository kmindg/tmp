#ifndef FBE_PAYLOAD_CONTROL_OPERATION_H
#define FBE_PAYLOAD_CONTROL_OPERATION_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_payload_operation_header.h"

#define FBE_PAYLOAD_CONTROL_CODE_TYPE_MASK 0x0F000000
#define FBE_PAYLOAD_CONTROL_CODE_TYPE_SHIFT 24

#define FBE_PAYLOAD_CONTROL_CODE_ID_MASK 0x00FF0000
#define FBE_PAYLOAD_CONTROL_CODE_ID_SHIFT 16

#define FBE_PAYLOAD_CONTROL_CODE_VALUE_MASK 0x0000FFFF
#define FBE_PAYLOAD_CONTROL_CODE_VALUE_SHIFT 00

enum fbe_payload_control_code_type_e{
	FBE_PAYLOAD_CONTROL_CODE_TYPE_INVALID,
	FBE_PAYLOAD_CONTROL_CODE_TYPE_SERVICE,
	FBE_PAYLOAD_CONTROL_CODE_TYPE_CLASS,
	FBE_PAYLOAD_CONTROL_CODE_TYPE_TRANSPORT,

	FBE_PAYLOAD_CONTROL_CODE_TYPE_LAST
};

typedef enum fbe_payload_control_status_e{
	FBE_PAYLOAD_CONTROL_STATUS_OK,
	FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
	FBE_PAYLOAD_CONTROL_STATUS_BUSY,
	FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES,
	FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION, /* The operation was succesfull, but additional processing requiered */
	FBE_PAYLOAD_CONTROL_STATUS_LAST
}fbe_payload_control_status_t;


typedef enum fbe_payload_control_status_qualifier_e{
	FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK, /* Nothing special */

	FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS_REQUEST,
	FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_REKEY_IN_PROGRESS_REQUEST,
	FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_ENCRYPTION_MODE_ENCRYPTED_REQUEST,

	FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_LAST
}fbe_payload_control_status_qualifier_t;


typedef	fbe_u32_t fbe_payload_control_operation_opcode_t;	
enum fbe_payload_control_operation_opcode_e {
	FBE_PAYLOAD_CONTROL_OPERATION_OPCODE_INVALID = 0xFFFFFFFF
};

typedef	void * fbe_payload_control_buffer_t;
typedef	fbe_u32_t fbe_payload_control_buffer_length_t;

#pragma pack(1)
typedef struct fbe_payload_control_operation_s{
	fbe_payload_operation_header_t			operation_header; /* Must be first */

	fbe_payload_control_operation_opcode_t	opcode;		
	// Switch the order of buffer_length and buffer to avoid the padding in 64-bit version.
	fbe_payload_control_buffer_length_t		buffer_length; 
	fbe_payload_control_buffer_t			buffer;
	fbe_payload_control_status_t			status;
	fbe_payload_control_status_qualifier_t  status_qualifier;
}fbe_payload_control_operation_t;
#pragma pack()

/* Control operation functions */
fbe_status_t fbe_payload_control_build_operation(fbe_payload_control_operation_t * control_operation,
												fbe_payload_control_operation_opcode_t	opcode,	
												fbe_payload_control_buffer_t			buffer,
												fbe_payload_control_buffer_length_t		buffer_length);


fbe_status_t fbe_payload_control_get_opcode(fbe_payload_control_operation_t * control_operation, fbe_payload_control_operation_opcode_t * opcode);

fbe_status_t fbe_payload_control_get_buffer_impl(fbe_payload_control_operation_t * control_operation, fbe_payload_control_buffer_t * buffer);
#define fbe_payload_control_get_buffer(_control_operation, _buffer) fbe_payload_control_get_buffer_impl(_control_operation, ((fbe_payload_control_buffer_t *)_buffer)) /* SAFEMESS *//* RCA_NPTF */

fbe_status_t fbe_payload_control_get_buffer_length(fbe_payload_control_operation_t * control_operation, 
												   fbe_payload_control_buffer_length_t * buffer_length);

fbe_status_t fbe_payload_control_get_status(fbe_payload_control_operation_t * control_operation, fbe_payload_control_status_t * status);
fbe_status_t fbe_payload_control_set_status(fbe_payload_control_operation_t * control_operation, fbe_payload_control_status_t status);

fbe_status_t fbe_payload_control_get_status_qualifier(fbe_payload_control_operation_t * control_operation, fbe_payload_control_status_qualifier_t * status_qualifier);
fbe_status_t fbe_payload_control_set_status_qualifier(fbe_payload_control_operation_t * control_operation, fbe_payload_control_status_qualifier_t status_qualifier);

#endif /* FBE_PAYLOAD_CONTROL_OPERATION_H */
