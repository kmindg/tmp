#ifndef BGSL_PAYLOAD_CONTROL_OPERATION_H
#define BGSL_PAYLOAD_CONTROL_OPERATION_H

#include "fbe/bgsl_types.h"
#include "fbe/bgsl_payload_operation_header.h"

#define BGSL_PAYLOAD_CONTROL_CODE_TYPE_MASK 0x0F000000
#define BGSL_PAYLOAD_CONTROL_CODE_TYPE_SHIFT 24

#define BGSL_PAYLOAD_CONTROL_CODE_ID_MASK 0x00FF0000
#define BGSL_PAYLOAD_CONTROL_CODE_ID_SHIFT 16

#define BGSL_PAYLOAD_CONTROL_CODE_VALUE_MASK 0x0000FFFF
#define BGSL_PAYLOAD_CONTROL_CODE_VALUE_SHIFT 00

enum bgsl_payload_control_code_type_e{
	BGSL_PAYLOAD_CONTROL_CODE_TYPE_INVALID,
	BGSL_PAYLOAD_CONTROL_CODE_TYPE_SERVICE,
	BGSL_PAYLOAD_CONTROL_CODE_TYPE_CLASS,
	BGSL_PAYLOAD_CONTROL_CODE_TYPE_TRANSPORT,

	BGSL_PAYLOAD_CONTROL_CODE_TYPE_LAST
};

typedef enum bgsl_payload_control_status_e{
	BGSL_PAYLOAD_CONTROL_STATUS_OK,
	BGSL_PAYLOAD_CONTROL_STATUS_FAILURE,
	
	BGSL_PAYLOAD_CONTROL_STATUS_LAST
}bgsl_payload_control_status_t;

typedef	bgsl_u32_t bgsl_payload_control_operation_opcode_t;	
enum bgsl_payload_control_operation_opcode_e {
	BGSL_PAYLOAD_CONTROL_OPERATION_OPCODE_INVALID = 0xFFFFFFFF
};

typedef	void * bgsl_payload_control_buffer_t;
typedef	bgsl_u32_t bgsl_payload_control_buffer_length_t;
typedef bgsl_u32_t bgsl_payload_control_status_qualifier_t;

typedef struct bgsl_payload_control_operation_s{
	bgsl_payload_operation_header_t			operation_header; /* Must be first */

	bgsl_payload_control_operation_opcode_t	opcode;		
	// Switch the order of buffer_length and buffer to avoid the padding in 64-bit version.
	bgsl_payload_control_buffer_length_t		buffer_length; 
	bgsl_payload_control_buffer_t			buffer;
	bgsl_payload_control_status_t			status;
	bgsl_payload_control_status_qualifier_t  status_qualifier;
}bgsl_payload_control_operation_t;

/* Control operation functions */
bgsl_status_t bgsl_payload_control_build_operation(bgsl_payload_control_operation_t * control_operation,
												bgsl_payload_control_operation_opcode_t	opcode,	
												bgsl_payload_control_buffer_t			buffer,
												bgsl_payload_control_buffer_length_t		buffer_length);


bgsl_status_t bgsl_payload_control_get_opcode(bgsl_payload_control_operation_t * control_operation, bgsl_payload_control_operation_opcode_t * opcode);

bgsl_status_t bgsl_payload_control_get_buffer(bgsl_payload_control_operation_t * control_operation, bgsl_payload_control_buffer_t * buffer);

bgsl_status_t bgsl_payload_control_get_buffer_length(bgsl_payload_control_operation_t * control_operation, 
												   bgsl_payload_control_buffer_length_t * buffer_length);

bgsl_status_t bgsl_payload_control_get_status(bgsl_payload_control_operation_t * control_operation, bgsl_payload_control_status_t * status);
bgsl_status_t bgsl_payload_control_set_status(bgsl_payload_control_operation_t * control_operation, bgsl_payload_control_status_t status);

bgsl_status_t bgsl_payload_control_get_status_qualifier(bgsl_payload_control_operation_t * control_operation, bgsl_payload_control_status_qualifier_t * status_qualifier);
bgsl_status_t bgsl_payload_control_set_status_qualifier(bgsl_payload_control_operation_t * control_operation, bgsl_payload_control_status_qualifier_t status_qualifier);

#endif /* BGSL_PAYLOAD_CONTROL_OPERATION_H */
