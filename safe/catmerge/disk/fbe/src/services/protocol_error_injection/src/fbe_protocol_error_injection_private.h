#ifndef FBE_PROTOCOL_ERROR_INJECTION_PRIVATE_H
#define FBE_PROTOCOL_ERROR_INJECTION_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_protocol_error_injection_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the protocol_error_injection service.
 *
 * @version
 *   5/28/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_notification_interface.h"

#define FBE_PROTOCOL_ERROR_INJECTION_INVALID 0xFFFFFFFF

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @struct fbe_protocol_error_injection_service_t
 *********************************************************************
 * @brief
 *  This is the definition of the protocol_error_injection service, which is used to
 *  inject error for physical objects.
 *
 *********************************************************************/
typedef struct fbe_protocol_error_injection_service_s
{
    fbe_base_service_t base_service;

    /*! Lock to protect our queues.
     */
    fbe_spinlock_t   protocol_error_injection_error_queue_lock;

    /*! This is the list of error record, 
     * which track all error to be injected. 
     */
    fbe_queue_head_t protocol_error_injection_error_queue;
}fbe_protocol_error_injection_service_t;

void fbe_protocol_error_injection_service_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...)
                         __attribute__((format(__printf_func__,3,4)));
fbe_status_t protocol_error_injection_set_ssp_edge_hook (fbe_object_id_t object_id, fbe_transport_control_set_edge_tap_hook_t *hook_info);
fbe_status_t protocol_error_injection_set_stp_edge_hook (fbe_object_id_t object_id, fbe_transport_control_set_edge_tap_hook_t *hook_info);
fbe_status_t protocol_error_injection_get_object_class_id (fbe_object_id_t object_id, fbe_class_id_t *class_id, fbe_package_id_t package_id);
fbe_status_t protocol_error_injection_register_notification_element(fbe_package_id_t package_id, 
																   fbe_notification_function_t notification_function, 
																   void * context);
fbe_status_t protocol_error_injection_unregister_notification_element(fbe_package_id_t package_id, 
                                                                   fbe_notification_function_t notification_function, 
                                                                   void * context);


/*************************
 * end file fbe_protocol_error_injection_private.h
 *************************/
#endif /* end FBE_PROTOCOL_ERROR_INJECTION_PRIVATE_H */
