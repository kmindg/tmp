#ifndef FBE_CMS_EXERCISER_PRIVATE_H
#define FBE_CMS_EXERCISER_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_exerciser_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains APIs used between different exerciser files
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_cms_exerciser_interface.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe/fbe_cms_interface.h"

/*common functions*/
void cms_exerciser_trace(fbe_trace_level_t trace_level,
                         const char * fmt, ...) __attribute__((format(printf,2,3)));

fbe_status_t cms_exerciser_complete_packet(fbe_packet_t *packet, fbe_status_t packet_status);
fbe_status_t cms_exerciser_get_payload(fbe_packet_t *packet, void **payload_struct, fbe_u32_t expected_payload_len);

fbe_status_t cms_exerciser_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
													  fbe_payload_control_buffer_t buffer,
													  fbe_payload_control_buffer_length_t buffer_length,
													  fbe_service_id_t service_id,
													  fbe_package_id_t package_id);

/*main stuff*/
fbe_cms_control_get_access_func_ptrs_t * cms_exerciser_get_access_lib_func_ptrs(void);


/*test related*/
fbe_status_t fbe_cms_exerciser_control_run_exclusive_alloc_tests(fbe_packet_t * packet);
fbe_status_t fbe_cms_exerciser_control_get_interface_tests_resutls(fbe_packet_t * packet);

#endif /*FBE_CMS_EXERCISER_PRIVATE_H*/
