#ifndef FBE_API_COMMON_TRANSPORT_H
#define FBE_API_COMMON_TRANSPORT_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_package.h"

FBE_API_CPP_EXPORT_START

void * fbe_api_allocate_memory (fbe_u32_t  NumberOfBytes);
void fbe_api_free_memory (void *  memory_ptr);

PVOID fbe_api_allocate_contiguous_memory (ULONG  NumberOfBytes);
void fbe_api_free_contiguous_memory (PVOID  P);
fbe_status_t fbe_api_common_send_control_packet_to_driver (fbe_packet_t *packet);
fbe_status_t fbe_api_common_send_io_packet_to_driver (fbe_packet_t *packet);

fbe_status_t fbe_api_common_send_io_packet_to_kernel_driver(fbe_packet_t *packet);/*user to kernel*/
fbe_status_t fbe_api_common_send_control_packet_to_kernel_driver(fbe_packet_t *packet);/*user to kernel*/
fbe_status_t fbe_api_common_send_control_packet_to_simulation_pipe(fbe_packet_t *packet);/*user ro user*/
fbe_status_t fbe_api_common_send_io_packet_to_simulation_pipe(fbe_packet_t *packet);/*user ro user*/

fbe_status_t fbe_api_common_send_control_packet(fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_packet_attr_t attr,
                                                fbe_api_control_operation_status_info_t *control_status_info,
                                                fbe_package_id_t package_id);

fbe_status_t fbe_api_common_send_io_packet(fbe_packet_t *packet,
                                           fbe_object_id_t object_id,
                                           fbe_packet_completion_function_t completion_function,
                                           fbe_packet_completion_context_t completion_context,
                                           fbe_packet_cancel_function_t cancel_function,
                                           fbe_packet_cancel_context_t cancel_context,
                                           fbe_package_id_t package_id);

fbe_status_t fbe_api_common_send_control_packet_with_sg_list(fbe_payload_control_operation_opcode_t control_code,
                                                             fbe_payload_control_buffer_t buffer,
                                                             fbe_payload_control_buffer_length_t buffer_length,
                                                             fbe_object_id_t object_id,
                                                             fbe_packet_attr_t attr,
                                                             fbe_sg_element_t *sg_list,
                                                             fbe_u32_t sg_list_count,
                                                             fbe_api_control_operation_status_info_t *control_status_info,
                                                             fbe_package_id_t package_id);

fbe_status_t fbe_api_common_send_control_packet_with_sg_list_async(fbe_packet_t *packet,
                                                                   fbe_payload_control_operation_opcode_t control_code,
                                                                   fbe_payload_control_buffer_t buffer,
                                                                   fbe_payload_control_buffer_length_t buffer_length,
                                                                   fbe_object_id_t object_id,
                                                                   fbe_packet_attr_t attr,
                                                                   fbe_sg_element_t *sg_list,
                                                                   fbe_u32_t sg_list_count,
                                                                   fbe_packet_completion_function_t completion_function,
                                                                   fbe_packet_completion_context_t completion_context,
                                                                   fbe_package_id_t package_id);

fbe_status_t fbe_api_common_send_control_packet_asynch(fbe_packet_t *packet,
                                                       fbe_object_id_t object_id,
                                                       fbe_packet_completion_function_t completion_function,
                                                       fbe_packet_completion_context_t completion_context,
                                                       fbe_packet_attr_t attr,
                                                       fbe_package_id_t package_id);

fbe_status_t fbe_api_common_send_control_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
                                                           fbe_payload_control_buffer_t buffer,
                                                           fbe_payload_control_buffer_length_t buffer_length,
                                                           fbe_service_id_t service_id,
                                                           fbe_packet_attr_t attr,
                                                           fbe_api_control_operation_status_info_t *control_status_info,
                                                           fbe_package_id_t package_id);

fbe_status_t fbe_api_common_send_control_packet_to_service_asynch(fbe_packet_t *packet_p,
                                                                  fbe_payload_control_operation_opcode_t control_code,
                                                                  fbe_payload_control_buffer_t buffer,
                                                                  fbe_payload_control_buffer_length_t buffer_length,
                                                                  fbe_service_id_t service_id,
                                                                  fbe_packet_attr_t attr,
                                                                  fbe_packet_completion_function_t completion_function,
                                                                  fbe_packet_completion_context_t completion_context,
                                                                  fbe_package_id_t package_id);


fbe_status_t fbe_api_common_send_control_packet_to_class(fbe_payload_control_operation_opcode_t control_code,
                                                           fbe_payload_control_buffer_t buffer,
                                                           fbe_payload_control_buffer_length_t buffer_length,
                                                           fbe_class_id_t class_id,
                                                           fbe_packet_attr_t attr,
                                                           fbe_api_control_operation_status_info_t *control_status_info,
                                                           fbe_package_id_t package_id);

fbe_status_t fbe_api_common_send_control_packet_to_service_with_sg_list(fbe_payload_control_operation_opcode_t control_code,
                                                                        fbe_payload_control_buffer_t buffer,
                                                                        fbe_payload_control_buffer_length_t buffer_length,
                                                                        fbe_service_id_t service_id,
                                                                        fbe_packet_attr_t attr,
                                                                        fbe_sg_element_t *sg_list,
                                                                        fbe_u32_t sg_list_count,
                                                                        fbe_api_control_operation_status_info_t *control_status_info,
                                                                        fbe_package_id_t package_id);

fbe_status_t fbe_api_common_send_control_packet_to_service_with_sg_list_asynch(fbe_packet_t *packet_p,
																			   fbe_payload_control_operation_opcode_t control_code,
																			   fbe_payload_control_buffer_t buffer,
																			   fbe_payload_control_buffer_length_t buffer_length,
																			   fbe_service_id_t service_id,
																			   fbe_sg_element_t *sg_list,
																			   fbe_u32_t sg_list_count,
																			   fbe_packet_completion_function_t completion_function,
																			   fbe_packet_completion_context_t completion_context,
																			   fbe_package_id_t package_id);

fbe_packet_t * fbe_api_get_contiguous_packet(void);
void fbe_api_return_contiguous_packet (fbe_packet_t *packet);

FBE_API_CPP_EXPORT_END

#endif /*FBE_API_COMMON_TRANSPORT__H*/




