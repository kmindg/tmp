#ifndef FBE_PACKET_SERIALIZE_LIB_H
#define FBE_PACKET_SERIALIZE_LIB_H   

#include "fbe/fbe_types.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"

typedef struct fbe_serialized_control_transaction_s{
	fbe_payload_control_operation_opcode_t	user_control_code;/*IN*/
	fbe_package_id_t						packge_id;/*IN*/
	fbe_service_id_t 						service_id;/*IN*/
	fbe_class_id_t 							class_id;/*IN*/
	fbe_object_id_t 						object_id;/*IN*/
    fbe_packet_attr_t 						packet_attr;/*IN*/
	fbe_payload_control_buffer_length_t 	buffer_length;/*IN*/
	fbe_u32_t								sg_elements_count;/*IN*/
	fbe_packet_status_t						packet_status;/*OUT*/
    fbe_payload_control_status_t 			payload_control_status;/*OUT*/
	fbe_payload_control_status_qualifier_t	payload_control_qualifier;/*OUT*/
	fbe_u64_t								spare_opaque;
    fbe_key_handle_t                        key_handle;
}fbe_serialized_control_transaction_t;

fbe_status_t fbe_serialize_lib_serialize_packet(fbe_packet_t *packet_in,
												fbe_serialized_control_transaction_t **serialized_transaction_out,
												fbe_u32_t *serialized_transaction_size);

fbe_status_t fbe_serialize_lib_deserialize_transaction(fbe_serialized_control_transaction_t *serialized_transaction_in, fbe_packet_t *packet_out);
fbe_status_t fbe_serialize_lib_unpack_transaction(fbe_serialized_control_transaction_t *serialized_transaction_in, fbe_packet_t *packet_out, void **returned_sg_elements_list);
fbe_status_t fbe_serialize_lib_repack_transaction(fbe_serialized_control_transaction_t *serialized_transaction, fbe_packet_t *packet);


#endif /*FBE_PACKET_SERIALIZE_LIB_H*/

