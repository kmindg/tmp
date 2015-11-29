/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_transport_packet_interface_control.c
 ***************************************************************************
 *
 *  Description
 *      Takes care of control packets sent to the server
 ****************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_transport_packet_interface.h"
#include "fbe_api_transport_packet_interface_private.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_sep.h"
#include "fbe_test_package_config.h"
#include "fbe_packet_serialize_lib.h"
#include "fbe/fbe_esp.h"
#include "simulation_log.h"
#include "EmcUTIL_DllLoader_Interface.h"

/**************************************
				Local variables
**************************************/
/*maximum allowed registered applications*/
#define FBE_API_TRANSPORT_MAX_REG_APP	0xf
#define NOTIFICATION_THREAD_WAIT_TIMEOUT_MS 120000

static fbe_bool_t fbe_api_transport_control_initialized = FALSE;

static fbe_notification_element_t fbe_api_transport_control_notification_element[FBE_PACKAGE_ID_LAST];
static fbe_bool_t notification_registered[FBE_PACKAGE_ID_LAST];

typedef struct fbe_transport_control_registered_application_data_s {
    fbe_bool_t	     available;
    fbe_bool_t	     registered;
    fbe_semaphore_t  packet_arrived_semaphore;
    fbe_packet_t *   packet;
    fbe_queue_head_t notifications_data_head;
    fbe_spinlock_t   notifications_data_lock;
    fbe_semaphore_t  notification_data_arrived_semaphore;
    fbe_semaphore_t  notification_thread_semaphore;
    fbe_thread_t     notification_thread;
    fbe_transport_control_thread_flag_t notification_thread_flag;
}fbe_transport_control_registered_application_data_t;


static fbe_spinlock_t	fstc_registered_apps_table_lock;
static fbe_thread_t	fstc_event_notification_thread;
static fbe_semaphore_t  fstc_event_notification_thread_semaphore;
static fbe_semaphore_t  fstc_event_notification_semaphore;
static fbe_transport_control_thread_flag_t	fstc_event_notification_thread_flag = THREAD_NULL;
static fbe_spinlock_t	fstcl_notifications_data_lock;
static fbe_queue_head_t	fstc_notifications_data_head;

static fbe_transport_control_registered_application_data_t	fstc_registered_applications_table [FBE_PACKAGE_ID_LAST][FBE_API_TRANSPORT_MAX_REG_APP];

/*******************************************
				Local functions
********************************************/
static fbe_status_t fbe_transport_control_notification(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context);
static fbe_status_t fbe_transport_control_get_next_available_registration_table_entry (application_id_t * application_id, fbe_package_id_t package_id);
static void fbe_transport_control_initialize_registration_table(void);
static void fbe_transport_control_notification_thread_func(void * context);
static void fbe_transport_control_notify_application_func(void * context);
static fbe_bool_t fbe_transport_control_registration_table_is_empty(fbe_package_id_t package_id);
static void fbe_transport_clear_notifications_data_queue(fbe_package_id_t package_id, application_id_t application_id);
static fbe_status_t unregister_notification(fbe_package_id_t package_id, application_id_t application_id, fbe_bool_t free_slot);
/************************************************************************************************************/

fbe_status_t FBE_API_CALL fbe_api_transport_control_init_server_control (void)
{
    fbe_u32_t     r = 0;
    EMCPAL_STATUS emcpal_status = EMCPAL_STATUS_UNSUCCESSFUL;

    if (fbe_api_transport_control_initialized) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: transport control already initialized!\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (r = FBE_PACKAGE_ID_INVALID; r < FBE_PACKAGE_ID_LAST; r++) {
        notification_registered[r] = FBE_FALSE;
    }

    fbe_transport_control_initialize_registration_table();

    fbe_spinlock_init(&fstc_registered_apps_table_lock);
    fbe_queue_init(&fstc_notifications_data_head);
    fbe_spinlock_init(&fstcl_notifications_data_lock);
    fbe_semaphore_init(&fstc_event_notification_semaphore, 0, 0x7FFFFFFF);
    fbe_semaphore_init(&fstc_event_notification_thread_semaphore, 0, 0x7FFFFFFF);
    fstc_event_notification_thread_flag = THREAD_RUN;
    emcpal_status = fbe_thread_init(&fstc_event_notification_thread,
                                    "fbe_trans_notif",
                                    fbe_transport_control_notification_thread_func,
                                    NULL);

    if (EMCPAL_STATUS_SUCCESS != emcpal_status) {

        /* set flags for fbe_api_transport_control_destroy_server_control */
        fstc_event_notification_thread_flag = THREAD_NULL;
        fbe_api_transport_control_initialized = TRUE;

        fbe_api_transport_control_destroy_server_control();

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_transport_control_initialized = TRUE;

    return FBE_STATUS_OK;
}

fbe_status_t FBE_API_CALL fbe_api_transport_register_notifications(fbe_package_notification_id_mask_t package_mask)
{
    fbe_package_notification_id_mask_t  package_id = 0x1;/*will always be the first package*/
    fbe_u32_t                           package_count = 0x1;
    fbe_package_id_t                    fbe_package_id = 0x1;/*start with the first one, no matter what it is*/

    if (!fbe_api_transport_control_initialized) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: transport control not initialized!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*register to get notifications from the all requested packages*/
    while (package_count < FBE_PACKAGE_ID_LAST) {
	if ((package_id & package_mask)) {
            fbe_notification_convert_package_bitmap_to_fbe_package(package_id, &fbe_package_id);
	    if (!notification_registered[fbe_package_id]) {
		fbe_transport_control_register_notification_element(fbe_package_id, NULL, FBE_NOTIFICATION_TYPE_ALL, FBE_TOPOLOGY_OBJECT_TYPE_ALL); 
                notification_registered[fbe_package_id] = FBE_TRUE;
	    }
	}

	package_id <<= 1;/*go to next one*/
	package_count++;
    }

    return FBE_STATUS_OK;
}

fbe_bool_t fbe_api_transport_is_notification_registered(fbe_package_notification_id_mask_t package_id)
{
    return notification_registered[package_id];
}

fbe_status_t fbe_transport_control_register_notification(fbe_packet_t *packet, fbe_package_id_t package_id)
{

    fbe_api_transport_register_notification_t *fbe_package_register_app_notify = NULL;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    EMCPAL_STATUS                   emcpal_status = EMCPAL_STATUS_UNSUCCESSFUL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_u32_t                       len = 0;
    fbe_transport_application_id_t  application_id = 0;
    fbe_transport_control_registered_application_data_t *entry;
    
    fbe_api_transport_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer(control_operation,
                                   &fbe_package_register_app_notify); 
    fbe_payload_control_get_buffer_length(control_operation, &len); 

    if ((len < sizeof(fbe_api_transport_register_notification_t)) ||
        (!fbe_api_transport_control_initialized)) {

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
	fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*and allocate an entry in the table for this application*/
    status = fbe_transport_control_get_next_available_registration_table_entry(&application_id, package_id);

    if (status != FBE_STATUS_OK) {
	fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
	fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*set the return data for the user and initialize this row in the table*/
    fbe_package_register_app_notify->application_id = application_id;

    entry = &fstc_registered_applications_table[package_id][application_id];
    /* should assert entry->registered is FALSE */

    fbe_semaphore_init(&entry->packet_arrived_semaphore, 0, 2);
    fbe_queue_init(&entry->notifications_data_head);
    fbe_spinlock_init(&entry->notifications_data_lock);
    fbe_semaphore_init(&entry->notification_data_arrived_semaphore, 0, 0x0FFFFFFF);
    fbe_semaphore_init(&entry->notification_thread_semaphore, 0, 0x7FFFFFFF);
    entry->notification_thread_flag = THREAD_RUN;

    emcpal_status = fbe_thread_init(&entry->notification_thread,
                                    "fbe_trans_ctl_notif",
                                    fbe_transport_control_notify_application_func,                                  entry);

    if (EMCPAL_STATUS_SUCCESS != emcpal_status) {

        entry->notification_thread_flag = THREAD_NULL;
        unregister_notification(package_id, application_id, FBE_TRUE);

        status = FBE_STATUS_GENERIC_FAILURE;
    } else {
        entry->registered = TRUE;
        status = FBE_STATUS_OK;
    }

    /*and complete the packet*/
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t fbe_transport_control_unregister_notification(fbe_packet_t *packet, fbe_package_id_t package_id)
{
    fbe_api_transport_register_notification_t *fbe_package_register_app_notify = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_u32_t                       len = 0;
    fbe_packet_t                    *registered_packet = NULL;
    fbe_transport_application_id_t  app_id = 0;

    fbe_api_transport_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer(control_operation,
                                   &fbe_package_register_app_notify); 
    
    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if ((len < sizeof(fbe_api_transport_register_notification_t)) ||
        (!fbe_api_transport_control_initialized)) {

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
	fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    app_id = fbe_package_register_app_notify->application_id;

    /*we were supposed to get here the application id of the application who wishes to unregister
	so we just go over the queue and release this application from the queue*/
    fbe_spinlock_lock(&fstc_registered_apps_table_lock);
	
    if (!fstc_registered_applications_table[package_id][app_id].registered) {
	fbe_spinlock_unlock(&fstc_registered_apps_table_lock);
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: illegal application id sent (%d) for package %d, can't find it in table\n", __FUNCTION__, app_id, package_id); /* SAFEBUG - races cause failures unless we make this a warning - a real mess */
	/*and complete the packet*/
	fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
	fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
		
    }
    fstc_registered_applications_table[package_id][app_id].registered = FALSE;

    /*save packet for later so we can complete outside of the lock*/
    registered_packet = fstc_registered_applications_table[package_id][app_id].packet;
    fstc_registered_applications_table[package_id][app_id].packet = NULL;
    fbe_spinlock_unlock(&fstc_registered_apps_table_lock);

    unregister_notification(package_id, app_id, FBE_TRUE);

    if (registered_packet == NULL) {
	fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: registered_packet == NULL; package_id = %d; app_id = %d\n", 
		      __FUNCTION__, package_id, app_id); 
		
	/*and complete the packet*/
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }
    
    /* we complete the ioctl that was sent to us to get events with.
     * the thread that waits for it should check the status and once it sees
     * the status is cancelled, it would know to stop the thread and go out
     */
    fbe_transport_set_status(registered_packet, FBE_STATUS_CANCELED, 0);
    fbe_transport_complete_packet(registered_packet);   
	
    /*and complete the ioctl that asked as to do the undergister*/
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*
 * Caller should set the entry representing <package_id, application_id> to
 * FALSE before calling this routine.
 */
static fbe_status_t unregister_notification(fbe_package_id_t package_id, application_id_t application_id, fbe_bool_t free_slot)
{
    fbe_transport_control_registered_application_data_t *entry;

    entry = &fstc_registered_applications_table[package_id][application_id];

    /*stop the thread that waits for events*/
    entry->notification_thread_flag = THREAD_STOP;

    fbe_semaphore_release(&entry->packet_arrived_semaphore, 0, 1, FALSE);/*let the thread reach the flag*/
    fbe_semaphore_release(&entry->notification_data_arrived_semaphore, 0, 1, FALSE);/*let the thread reach the flag*/

    if (THREAD_NULL != entry->notification_thread_flag) {

        /*wait for thread to end*/
        fbe_semaphore_wait_ms(&entry->notification_thread_semaphore,
                              NOTIFICATION_THREAD_WAIT_TIMEOUT_MS);

        if (THREAD_DONE == entry->notification_thread_flag) {
            fbe_thread_wait(&entry->notification_thread);
        } else {
	    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: notficiation thread (%p) did not exit; package_id = %d; app_id = %d\n", 
		          __FUNCTION__, entry->notification_thread.thread,
                          package_id, application_id); 
            fbe_debug_break();
        }
        fbe_thread_destroy(&entry->notification_thread);
    }

    fbe_spinlock_lock(&fstc_registered_apps_table_lock);
    entry->notification_thread_flag = THREAD_NULL;
    entry->packet = NULL;
    fbe_transport_clear_notifications_data_queue(package_id, application_id);
    fbe_queue_destroy(&entry->notifications_data_head);
    fbe_spinlock_destroy(&entry->notifications_data_lock);
    fbe_semaphore_destroy(&entry->packet_arrived_semaphore);
    fbe_semaphore_destroy(&entry->notification_data_arrived_semaphore);
    fbe_semaphore_destroy(&entry->notification_thread_semaphore);
    if (free_slot) {
        entry->available = TRUE;
    }
    fbe_spinlock_unlock(&fstc_registered_apps_table_lock);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_transport_clear_all_notifications(fbe_bool_t unregister_notification_flag)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_transport_application_id_t app_id = 0;
    fbe_package_id_t package_id;
    fbe_queue_head_t packet_queue;
    fbe_u32_t packet_count = 0;

    if (!fbe_api_transport_control_initialized) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: transport control not initialized!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_queue_init(&packet_queue);

    /* Loop over all packages and application ids. 
     * If we find any registered applications, we mark them unregistered, 
     * stash the packet on our packet queue and null out the packet ptr. 
     * Down below we'll complete any  packets from this packet queue. 
     */
    fbe_spinlock_lock(&fstc_registered_apps_table_lock);
    for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++)
    {
        for (app_id = 0; app_id < FBE_API_TRANSPORT_MAX_REG_APP; app_id++)
        {
            fbe_transport_control_registered_application_data_t *entry;

            entry = &fstc_registered_applications_table[package_id][app_id];

            if (entry->registered) 
            {
                /* It is possible that packet never came back from client.
                 * This would be the case if test running on client fails.
                 */

                if (unregister_notification_flag)
                {
                    if (entry->packet != NULL)
                    {
                        /* Save our packet for completing later.
                         */
                        status = fbe_transport_enqueue_packet(entry->packet,
                                                              &packet_queue);
                        if (status != FBE_STATUS_OK) 
                        { 
                            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: failed status %d enqueuing for package_id: %d app_id: %d\n", 
                                          __FUNCTION__, status, package_id,
                                          app_id);
                        } else {
                            entry->packet = NULL;
                        }
                    }
                    /* Clear out all the fields since we are unregistered.
                     */
                    entry->registered = FALSE;
                    fbe_spinlock_unlock(&fstc_registered_apps_table_lock);
                    unregister_notification(package_id, app_id, FBE_TRUE);
                    fbe_spinlock_lock(&fstc_registered_apps_table_lock);
                }
                else
                {
                    /* if we don't need to unregister notification, just clear old notification data */
                    fbe_transport_clear_notifications_data_queue(package_id, app_id);
                }
            }
        }
    }
    fbe_spinlock_unlock(&fstc_registered_apps_table_lock);

    /* Drain our packet queue and complete each packet.
     */
    if (unregister_notification_flag)
    {
        while (!fbe_queue_is_empty(&packet_queue))
        {
            fbe_packet_t *registered_packet_p = NULL;
            fbe_queue_element_t *queue_element_p = NULL;
            queue_element_p = fbe_queue_pop(&packet_queue);
            registered_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);

            fbe_transport_set_status(registered_packet_p, FBE_STATUS_CANCELED, 0);

            fbe_transport_complete_packet(registered_packet_p);
            ++packet_count;
        }
    }

    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_transport_control_unregister_all_notifications()
 ****************************************************************
 * @brief
 *  This function clears out all registered notifications for
 *  all packages.
 *
 * @param packet_p - This is the packet for this command.
 *                   We will complete this when the function is done.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_transport_control_unregister_all_notifications(fbe_packet_t *packet)
{
    fbe_status_t status = fbe_transport_clear_all_notifications(FBE_TRUE);
    /* Complete the unregister all packet.
     */
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}
/******************************************
 * end fbe_transport_control_unregister_all_notifications()
 ******************************************/

fbe_status_t fbe_transport_control_register_notification_element(fbe_package_id_t package_id,
																void * context,
																fbe_notification_type_t	notification_type,
																fbe_topology_object_type_t object_type)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    if (!fbe_api_transport_control_initialized) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: transport control not initialized!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if(notification_registered[package_id]) {
	fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: notification registration for package %d is already done before\n", __FUNCTION__, package_id); 
	return FBE_STATUS_OK;
    }
    
    fbe_api_transport_control_notification_element[package_id].notification_function = fbe_transport_control_notification;
    fbe_api_transport_control_notification_element[package_id].notification_context = context;
    fbe_api_transport_control_notification_element[package_id].notification_type = notification_type;
    fbe_api_transport_control_notification_element[package_id].object_type = object_type;

    /* Allocate packet */
    packet = fbe_api_get_contiguous_packet();
    
    if(packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    
    fbe_payload_control_build_operation (control_operation,
                                         FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
                                         &fbe_api_transport_control_notification_element[package_id],
                                         sizeof(fbe_notification_element_t));

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                package_id,
                                FBE_SERVICE_ID_NOTIFICATION,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID); 

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_api_common_send_control_packet_to_driver(packet);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to send packet, status:%d\n", __FUNCTION__, status);
        fbe_api_return_contiguous_packet(packet);
        return status;
    }

    fbe_transport_wait_completion(packet);

	/*we don't check for the status because some packges may not exist*/
    fbe_api_return_contiguous_packet(packet);

	/*mark the fact we are registered*/
	notification_registered[package_id] = FBE_TRUE;
    
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_transport_control_notification(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context)
{
	notification_data_t	*		notification_data = NULL;

	if (!notification_registered[notification_info.source_package]) {
		return FBE_STATUS_OK;
	}

    /*should we event bother do anything (did anyone register yet ?)*/
	if (fbe_transport_control_registration_table_is_empty(notification_info.source_package) ){
		/*no one registered, just go out*/
			return FBE_STATUS_OK;
	}

	notification_data = (notification_data_t *) fbe_api_allocate_memory (sizeof (notification_data_t));

	if (notification_data == NULL) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for notification_data\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*save the data*/
    notification_data->context  = CSX_CAST_PTR_TO_PTRMAX(context);
	notification_data->object_id = object_id;
	fbe_copy_memory(&notification_data->notification_info, &notification_info, sizeof (fbe_notification_info_t));
    
    fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s: Got a notification from packageID %d Notification Type %llu \n", __FUNCTION__,
                  notification_data->notification_info.source_package,
		  (unsigned long long)notification_data->notification_info.notification_type);

	/*add the data about the notification to the queue*/
    fbe_spinlock_lock(&fstcl_notifications_data_lock);
	fbe_queue_push(&fstc_notifications_data_head, &notification_data->queue_element);
	fbe_spinlock_unlock(&fstcl_notifications_data_lock);

	/*now that we got the notification, we need to let the thread release the queued registered applications with this data*/
	fbe_semaphore_release(&fstc_event_notification_semaphore, 0, 1, FALSE);
    
    return FBE_STATUS_OK;
}

void fbe_api_transport_control_destroy_server_control(void)
{
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;
    fbe_u32_t		app_id = 0;

    if (!fbe_api_transport_control_initialized)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: transport control not initialized!\n", __FUNCTION__);
        return ;
    }
    /*stop the thread that waits for events*/
    fstc_event_notification_thread_flag = THREAD_STOP;
    fbe_semaphore_release(&fstc_event_notification_semaphore, 0, 1, FALSE);/*let the thread reach the flag*/

    /*unregister from notifications from the all packages*/
    for (package_id = FBE_PACKAGE_ID_INVALID + 1; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
        if (notification_registered[package_id]) {
            fbe_transport_control_unregister_notification_element(package_id);
	}

        fbe_spinlock_lock(&fstc_registered_apps_table_lock);
	for (app_id = 0; app_id < FBE_API_TRANSPORT_MAX_REG_APP; app_id++){
            if (fstc_registered_applications_table [package_id][app_id].registered &&
                fstc_registered_applications_table [package_id][app_id].packet != NULL){
                //fbe_semaphore_release(&fstc_registered_applications_table [package_id][app_id].packet_arrived_semaphore,0, 1, FALSE);
                fbe_spinlock_unlock(&fstc_registered_apps_table_lock);
                unregister_notification(package_id, app_id, FBE_FALSE);
                fbe_spinlock_lock(&fstc_registered_apps_table_lock);
	    }
	}
        fbe_spinlock_unlock(&fstc_registered_apps_table_lock);
    }

    if (THREAD_NULL != fstc_event_notification_thread_flag) {
        /*wait for thread to end*/
        fbe_semaphore_wait_ms(&fstc_event_notification_thread_semaphore,
                              NOTIFICATION_THREAD_WAIT_TIMEOUT_MS);

        if (THREAD_DONE == fstc_event_notification_thread_flag) {
            fbe_thread_wait(&fstc_event_notification_thread);
        } else {
	    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: event notficiation thread (%p) did not exit\n", 
		          __FUNCTION__, fstc_event_notification_thread.thread);
            fbe_debug_break();
        }
        fbe_thread_destroy(&fstc_event_notification_thread);
        fstc_event_notification_thread_flag = THREAD_NULL;
    }

    fbe_semaphore_destroy(&fstc_event_notification_thread_semaphore);
    fbe_semaphore_destroy(&fstc_event_notification_semaphore);
    fbe_queue_destroy (&fstc_notifications_data_head);
    fbe_spinlock_destroy(&fstcl_notifications_data_lock);
    fbe_spinlock_destroy(&fstc_registered_apps_table_lock);

    fbe_api_transport_control_initialized = FALSE;
}

fbe_status_t fbe_transport_control_unregister_notification_element(fbe_package_id_t package_id)
{
    fbe_packet_t *                      packet = NULL;  
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    /* Allocate packet */
    packet = fbe_api_get_contiguous_packet();
    
    if(packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,"%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,
                                         &fbe_api_transport_control_notification_element[package_id],
                                         sizeof(fbe_notification_element_t));

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                package_id,
                                FBE_SERVICE_ID_NOTIFICATION,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID); 

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_api_common_send_control_packet_to_driver(packet);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: unable to send packet, status:%d\n", __FUNCTION__, status);
        fbe_api_return_contiguous_packet(packet);
        return status;
    }


	fbe_transport_wait_completion(packet);

    /*check the packet status to make sure we have no errors*/
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: unable to unregister events, status:%d\n", __FUNCTION__, status); 
        fbe_api_return_contiguous_packet(packet);
        return status;
    }

	notification_registered[package_id] = FBE_FALSE;

    fbe_api_return_contiguous_packet(packet);
    return FBE_STATUS_OK;
}

static void fbe_transport_control_initialize_registration_table(void)
{
    fbe_u32_t            idx = 0;
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    for ( ; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
    
        for (idx = 0; idx < FBE_API_TRANSPORT_MAX_REG_APP; idx ++) {

            fbe_transport_control_registered_application_data_t *entry;

            entry = &fstc_registered_applications_table[package_id][idx];

            entry->available = TRUE;
            entry->registered = FALSE;
            entry->packet = NULL;
            entry->notification_thread_flag = THREAD_NULL;
        }
    }
}

static fbe_status_t fbe_transport_control_get_next_available_registration_table_entry(application_id_t * application_id, fbe_package_id_t package_id)
{    
    fbe_u32_t            idx = 0;

    fbe_spinlock_lock(&fstc_registered_apps_table_lock);
    for (idx = 0; idx < FBE_API_TRANSPORT_MAX_REG_APP; idx ++) {
        if (fstc_registered_applications_table[package_id][idx].available) {
            fstc_registered_applications_table[package_id][idx].available = FALSE;
            fbe_spinlock_unlock(&fstc_registered_apps_table_lock);
            *application_id = idx;
	    return FBE_STATUS_OK;
	}
    }
    fbe_spinlock_unlock(&fstc_registered_apps_table_lock);

    /*all slots are full*/
    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: all application table slots are full, can't register new application\n", __FUNCTION__);
    return FBE_STATUS_GENERIC_FAILURE;
}

static void fbe_transport_clear_notifications_data_queue(fbe_package_id_t package_id, application_id_t application_id)
{
    fbe_queue_element_t *queue_element;
    notification_data_t *data;

    queue_element = fbe_queue_pop(&fstc_registered_applications_table[package_id][application_id].notifications_data_head);
    while(queue_element != NULL){
        data = (notification_data_t *)queue_element;
        fbe_api_free_memory(data);
        queue_element = fbe_queue_pop(&fstc_registered_applications_table[package_id][application_id].notifications_data_head);
    }
}

static void fbe_transport_control_notification_thread_func(void * context)
{
	EMCPAL_STATUS 				nt_status;
	notification_data_t *	event_notification_data = NULL;
    fbe_u32_t i = 0;

	FBE_UNREFERENCED_PARAMETER(context);
	
    while(1)    
	{
		/*block until someone takes down the semaphore (which means there was an event)*/
		nt_status = fbe_semaphore_wait(&fstc_event_notification_semaphore, NULL);
		if(fstc_event_notification_thread_flag == THREAD_RUN) {
			 /*we send the event to all registered applications*/
			while (1){
                fbe_spinlock_lock(&fstcl_notifications_data_lock);
				/*get the next notification from the queue, once we get it we free the queue so the completion function will not block*/
				if(!fbe_queue_is_empty(&fstc_notifications_data_head)){
                    event_notification_data = (notification_data_t *)fbe_queue_pop(&fstc_notifications_data_head);
					fbe_spinlock_unlock(&fstcl_notifications_data_lock);

                    for(i = 0; i < FBE_API_TRANSPORT_MAX_REG_APP; ++i)
                    {
                        if(fstc_registered_applications_table[event_notification_data->notification_info.source_package][i].registered)
                        {
                            notification_data_t *data = (notification_data_t *) fbe_api_allocate_memory (sizeof(notification_data_t));
                            fbe_copy_memory(data, event_notification_data, sizeof(notification_data_t));
                            fbe_spinlock_lock(&fstc_registered_applications_table[event_notification_data->notification_info.source_package][i].notifications_data_lock);
                            fbe_queue_push(&fstc_registered_applications_table[event_notification_data->notification_info.source_package][i].notifications_data_head, &data->queue_element);
                            fbe_spinlock_unlock(&fstc_registered_applications_table[event_notification_data->notification_info.source_package][i].notifications_data_lock);
                            fbe_semaphore_release(&fstc_registered_applications_table[event_notification_data->notification_info.source_package][i].notification_data_arrived_semaphore, 0, 1, FALSE);
                        }
                    }
					/*now that we sent this event to all registered apps we don't need it anymore*/
					fbe_api_free_memory (event_notification_data);
				 } else {
					fbe_spinlock_unlock(&fstcl_notifications_data_lock);
					break;
				 }
			}
		} else {
			break;
		}
    }
    
    fstc_event_notification_thread_flag = THREAD_DONE;
    fbe_semaphore_release(&fstc_event_notification_thread_semaphore, 0, 1,
                          FALSE);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void fbe_transport_control_notify_application_func(void * context)
{

	notification_data_t *				app_buffer = NULL;
	fbe_packet_t *						registered_packet = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    notification_data_t *   event_notification_data = NULL;


    EMCPAL_STATUS                nt_status;
    fbe_transport_control_registered_application_data_t *app_data = (fbe_transport_control_registered_application_data_t *)context;

    while(app_data->notification_thread_flag == THREAD_RUN) {
         /*we send the event to registered application*/
        nt_status = fbe_semaphore_wait(&app_data->packet_arrived_semaphore, NULL);
        nt_status = fbe_semaphore_wait(&app_data->notification_data_arrived_semaphore, NULL);


        if(app_data->notification_thread_flag != THREAD_RUN)
        {
            break;
        }
        fbe_spinlock_lock(&app_data->notifications_data_lock);
        /*get the next notification from the queue, once we get it we free the queue so the completion function will not block*/
        if(!fbe_queue_is_empty(&app_data->notifications_data_head)){
            event_notification_data = (notification_data_t *)fbe_queue_pop(&app_data->notifications_data_head);
            fbe_spinlock_unlock(&app_data->notifications_data_lock);

			/*validate the irp is valid, we may have got a request to unregister and we passed the semaphore_wait
			only because the unregister did that*/
            fbe_spinlock_lock(&fstc_registered_apps_table_lock);
            
            if (app_data->packet != NULL) {

				/*transfer the data of the event to the output buffer of the Irp we will soon complete*/
                fbe_api_transport_get_control_operation(app_data->packet, &control_operation);
				fbe_payload_control_get_buffer(control_operation, &app_buffer); 
				app_buffer->object_id = event_notification_data->object_id;
				fbe_copy_memory (&app_buffer->notification_info, &event_notification_data->notification_info, sizeof (fbe_notification_info_t));
				app_buffer->context = event_notification_data->context;

                /*before we get out of the lock we make a copy of our packet because we can't complete under lock*/
                registered_packet = app_data->packet;
                app_data->packet = NULL;
                fbe_spinlock_unlock(&fstc_registered_apps_table_lock);

				/*and complete the irp, this is supposed to be handled by the application which should generate an event with the data we send*/
				fbe_transport_set_status(registered_packet, FBE_STATUS_OK, 0);
				fbe_transport_complete_packet(registered_packet);
			} else {
				fbe_spinlock_unlock(&fstc_registered_apps_table_lock);
			}


            /*now that we sent this event to registered app we don't need it anymore*/
            fbe_api_free_memory (event_notification_data);
         } else 
         {
             fbe_spinlock_unlock(&app_data->notifications_data_lock);
         }
    }

    app_data->notification_thread_flag = THREAD_DONE;
    fbe_semaphore_release(&app_data->notification_thread_semaphore, 0, 1,
                          FALSE);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static fbe_bool_t fbe_transport_control_registration_table_is_empty(fbe_package_id_t package_id)
{
	fbe_u32_t			idx = 0;

	fbe_spinlock_lock(&fstc_registered_apps_table_lock);
	for (idx = 0; idx < FBE_API_TRANSPORT_MAX_REG_APP; idx ++) {
		if (fstc_registered_applications_table[package_id][idx].registered == TRUE) {
			fbe_spinlock_unlock(&fstc_registered_apps_table_lock);
			return FBE_FALSE;
		}
	}

	fbe_spinlock_unlock(&fstc_registered_apps_table_lock);
	return FBE_TRUE;

}

fbe_status_t fbe_transport_control_get_event(fbe_packet_t *packet, fbe_package_id_t package_id)
{
    fbe_payload_ex_t *                                             payload = NULL;
    fbe_payload_control_operation_t * 							control_operation = NULL;
	fbe_u32_t													len = 0;
	notification_data_t   * 									notification_data = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &notification_data); 
    
    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len < sizeof(notification_data_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*now we have to look for the application id in the queue and place this Irp there for future completion*/
    fbe_spinlock_lock(&fstc_registered_apps_table_lock);

	if (!fstc_registered_applications_table[package_id][notification_data->application_id].registered) {
		fbe_spinlock_unlock(&fstc_registered_apps_table_lock);
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:can't match application id (%d)to existing table\n", __FUNCTION__, notification_data->application_id); /* SAFEBUG - races cause failures unless we make this a warning - a real mess */
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    
	/*once we found the registered application id, we give it the pointer to this packet*/
	fstc_registered_applications_table[package_id][notification_data->application_id].packet = packet;

    /*we signal to the sender that caused this packet to get here that the packet arrived*/
	fbe_semaphore_release(&fstc_registered_applications_table[package_id][notification_data->application_id].packet_arrived_semaphore, 0, 1, FALSE);
	
	fbe_spinlock_unlock(&fstc_registered_apps_table_lock);

    return FBE_STATUS_PENDING;

}

static fbe_status_t fbe_transport_control_load_package(fbe_packet_t * packet)
{
    fbe_status_t 	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    /*package_id can be put into the paylaod*/
    status = fbe_transport_get_package_id(packet, &package_id);
    if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to get package id\n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
    }

    switch(package_id) {
	case FBE_PACKAGE_ID_PHYSICAL:
	    status = fbe_test_load_physical_package();
	    if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load_physical_package\n", __FUNCTION__);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_OK;
	    }
	    break;
	case FBE_PACKAGE_ID_SEP_0:
        status = fbe_test_load_sep_package(packet);
	    if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load sep package\n", __FUNCTION__);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_OK;
	    }
	    break;
	case FBE_PACKAGE_ID_NEIT:
        status = fbe_test_load_neit_package(packet);
	    if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load neit package\n", __FUNCTION__);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	    }
	    break;
	case FBE_PACKAGE_ID_ESP:
    	    status = fbe_test_load_esp();
	    if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load ESP\n", __FUNCTION__);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_OK;
	    }
	    break;
	case FBE_PACKAGE_ID_KMS:
        status = fbe_test_load_kms_package(packet);
	    if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load KMS\n", __FUNCTION__);
			fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet);
			return FBE_STATUS_OK;
	    }
	    break;
	default:
	    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, package id not supported!\n", __FUNCTION__);
		fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_OK;
	}

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_api_transport_control_clear_fstc_notifications_queue(fbe_package_id_t package_id)
{
    notification_data_t *	event_notification_data = NULL;
	notification_data_t *	next_event_notification_data = NULL;

        if (!fbe_api_transport_control_initialized)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s: transport control not initialized!\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
	/*first of all, let's drain the messages that are on the queue, we don't need them anymore*/
	fbe_spinlock_lock(&fstcl_notifications_data_lock);
	if(!fbe_queue_is_empty(&fstc_notifications_data_head)){
		event_notification_data = (notification_data_t *)fbe_queue_front(&fstc_notifications_data_head);
		do{
			if (event_notification_data->notification_info.source_package == package_id) {
				next_event_notification_data = (notification_data_t *)fbe_queue_next(&fstc_notifications_data_head, &event_notification_data->queue_element);
				fbe_queue_remove(&event_notification_data->queue_element);
				fbe_api_free_memory((void *)event_notification_data);
				 event_notification_data = next_event_notification_data;
			}else{
				event_notification_data = (notification_data_t *)fbe_queue_next(&fstc_notifications_data_head, &event_notification_data->queue_element);
			}
		}while (event_notification_data != NULL);
		
	}

	fbe_spinlock_unlock(&fstcl_notifications_data_lock);
	
	return FBE_STATUS_OK;
}

/* Return the Process ID*/
fbe_status_t fbe_transport_control_get_pid (fbe_packet_t * packet)
{
    fbe_payload_ex_t *             payload = NULL;
    fbe_payload_control_operation_t * 	control_operation = NULL;
    fbe_api_transport_server_get_pid_t *	get_pid = NULL;
    fbe_u32_t		len = 0;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &get_pid); 
    
    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len < sizeof(fbe_api_transport_server_get_pid_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
	fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now we get the process id*/
    get_pid->pid =  csx_p_get_process_id();

    /* complete the packet*/
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_transport_control_suite_start_notification (fbe_packet_t * packet)
{
    fbe_payload_ex_t *             payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_api_sim_server_suite_start_notification_t *suite_start_notification = NULL;
    fbe_u32_t       len = 0;
    EMCUTIL_DLL_HANDLE simulation_log_dll_handle;
    simulation_log_get_suite_log_callback_func_func_t simulation_log_get_suite_log_callback_func_func = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &suite_start_notification);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len < sizeof(fbe_api_sim_server_suite_start_notification_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    simulation_log_dll_handle = emcutil_simple_dll_load("simulation_log");
    if(simulation_log_dll_handle == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Cannot load simulation_log.dll!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    simulation_log_get_suite_log_callback_func_func = (simulation_log_get_suite_log_callback_func_func_t)emcutil_simple_dll_lookup(simulation_log_dll_handle, "simulation_log_get_suite_log_callback_func");

    /* register the suite log callback func */
    if(simulation_log_get_suite_log_callback_func_func != NULL)
    {
        (*simulation_log_get_suite_log_callback_func_func())(suite_start_notification->log_path, suite_start_notification->log_file_name);
    }

    /* complete the packet*/
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    emcutil_simple_dll_unload(simulation_log_dll_handle); /* SAFEBUG - was missing */
    return FBE_STATUS_OK;
}

/**************************************
           Wrapper functions for sim mode
**************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_transport_control_init_server_control(void)
{
    return fbe_api_transport_control_init_server_control();
}

fbe_status_t FBE_API_CALL fbe_api_sim_transport_register_notifications(fbe_package_notification_id_mask_t package_mask)
{
    return fbe_api_transport_register_notifications(package_mask);
}


