/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_common_notification.c
 ***************************************************************************
 *
 * @brief
 *   Notification registration and handling.
 *
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_common_notification_interface
 *
 * @version
 *   07/22/09    sharel - Created
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"


/**************************************
                Local variables
**************************************/
static fbe_notification_element_t notification_element[FBE_PACKAGE_ID_LAST];
static fbe_u64_t fbe_api_notification_current_reg_id = 0;
/*!********************************************************************* 
 * @enum fbe_api_notification_thread_flag_t 
 *  
 * @brief 
 *  This contains the data info for Notification thread flag.
 *
 * @ingroup fbe_api_common_notification_interface
 * @ingroup fbe_api_notification_thread_flag
 **********************************************************************/
typedef enum fbe_api_notification_thread_flag_t{
    FBE_API_NOTIFICATION_THREAD_RUN,   /*!< Run Thread Flag */
    FBE_API_NOTIFICATION_THREAD_STOP,  /*!< Stop Thread Flag */
    FBE_API_NOTIFICATION_THREAD_DONE   /*!< Done Thread Flag */
}fbe_api_notification_thread_flag_t;

/*!********************************************************************* 
 * @struct notification_info_t 
 *  
 * @brief 
 *  This contains the data info for Notification info.
 *
 * @ingroup fbe_api_common_notification_interface
 * @ingroup notification_info
 **********************************************************************/
typedef struct notification_info_s{
    fbe_queue_element_t                         queue_element;         /*!< Queue Element */
    fbe_notification_type_t                   	state_mask;            /*!< State Mask */
    fbe_package_notification_id_mask_t          package_mask;          /*!< Packet ID Mask */
    fbe_api_notification_callback_function_t    callback_func;         /*!< Callback Function */
    void *                                      notification_context;  /*!< Notification Context Data */
    fbe_topology_object_type_t                  object_type_mask;      /*!< Topoloty Object Type Mask */
	fbe_u64_t									registraction_id;		/*!< Unique ID we will use for unregistration */
	fbe_topology_object_type_t 					object_type;			/*!< what kind of object generated the notification */
	fbe_notification_type_t 					notification_type;		/*!< what kind of a notification is it */
	fbe_atomic_t			 					ref_count;				/*!< protect queue */
}notification_info_t;

#if 0 /*depracated*/
/*!********************************************************************* 
 * @struct fbe2api_state_map_t 
 *  
 * @brief 
 *  This contains the data info for Notification state map.
 *
 * @ingroup fbe_api_common_notification_interface
 * @ingroup fbe2api_state_map
 **********************************************************************/
typedef struct fbe2api_state_map_s{
    fbe_lifecycle_state_t           fbe_state;  /*!< FBE State */
    fbe_notification_type_t       api_state;  /*!< Life Cycle State */
}fbe2api_state_map_t;

/*!********************************************************************* 
 * @struct fbe2api_package_map_t 
 *  
 * @brief 
 *  This contains the data info for Notification package map.
 *
 * @ingroup fbe_api_common_notification_interface
 * @ingroup fbe2api_package_map
 **********************************************************************/
typedef struct fbe2api_package_map_s{
    fbe_package_id_t                fbe_package;  /*!< FBE Package ID */
    fbe_package_notification_id_mask_t            api_package;  /*!< API Package ID */
}fbe2api_package_map_t;

static fbe2api_state_map_t      state_map [] = 
{
    {FBE_LIFECYCLE_STATE_SPECIALIZE, FBE_API_STATE_SPECIALIZE},      
    {FBE_LIFECYCLE_STATE_ACTIVATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE},            
    {FBE_LIFECYCLE_STATE_READY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY},               
    {FBE_LIFECYCLE_STATE_HIBERNATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE} ,           
    {FBE_LIFECYCLE_STATE_OFFLINE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_OFFLINE},             
    {FBE_LIFECYCLE_STATE_FAIL, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL},                
    {FBE_LIFECYCLE_STATE_DESTROY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY},
    {FBE_LIFECYCLE_STATE_PENDING_READY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_READY}, 
    {FBE_LIFECYCLE_STATE_PENDING_ACTIVATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_ACTIVATE},
    {FBE_LIFECYCLE_STATE_PENDING_HIBERNATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_HIBERNATE} ,
    {FBE_LIFECYCLE_STATE_PENDING_OFFLINE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_OFFLINE},
    {FBE_LIFECYCLE_STATE_PENDING_FAIL, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_FAIL},
    {FBE_LIFECYCLE_STATE_PENDING_DESTROY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY},
    {FBE_LIFECYCLE_STATE_INVALID, FBE_NOTIFICATION_TYPE_INVALID} 
};

static fbe2api_package_map_t      package_map [] = 
{
    {FBE_PACKAGE_ID_PHYSICAL, FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL},  
    {FBE_PACKAGE_ID_NEIT, FBE_PACKAGE_NOTIFICATION_ID_NEIT},            
    {FBE_PACKAGE_ID_SEP_0, FBE_PACKAGE_NOTIFICATION_ID_SEP_0},
    {FBE_PACKAGE_ID_ESP, FBE_PACKAGE_NOTIFICATION_ID_ESP},
    {FBE_PACKAGE_ID_KMS, FBE_PACKAGE_NOTIFICATION_ID_KMS},
    {FBE_PACKAGE_ID_INVALID, NULL},  

    
};
#endif/*depracated*/

static fbe_bool_t                   notification_registered[FBE_PACKAGE_ID_LAST];
static fbe_thread_t                 fbe_api_notification_thread_handle;
static fbe_api_notification_thread_flag_t     fbe_api_notification_thread_flag;
static fbe_semaphore_t              fbe_api_notification_thread_semaphore;
static fbe_semaphore_t              fbe_api_notification_semaphore;
static fbe_spinlock_t               fbe_api_notification_queue_lock;
static fbe_queue_head_t             fbe_api_notification_queue_head;
static fbe_queue_head_t             fbe_api_notification_event_registration_queue;
static fbe_spinlock_t               fbe_api_notification_event_registration_queue_lock;
static fbe_bool_t                   notification_interface_initialized = FBE_FALSE;
#define FBE_COMMON_NOTIFICATION_MAX_SEMAPHORE_COUNT 0x7FFFFFFF 

/*******************************************
                Local functions
********************************************/
static void fbe_api_notification_thread_func(void * context);
static void fbe_api_notification_dispatch_queue(void);

static fbe_status_t fbe_api_notification_register_notification_element(fbe_package_id_t package_id,
																	   void * context, 
																	   fbe_notification_type_t notification_type,
																	   fbe_topology_object_type_t object_type);

static fbe_status_t fbe_api_notification_unregister_notification_element(fbe_package_id_t package_id);
static void fbe_api_notification_send_notification (update_object_msg_t * update_request);
fbe_status_t fbe_api_notification_function(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context);
static fbe_status_t fbe_api_notification_element_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static void empty_event_registration_queue(void);
//static fbe_notification_type_t fbe_api_notification_translate_fbe_state_to_api_state (update_object_msg_t * update_request);
//static fbe_package_notification_id_mask_t fbe_api_notification_translate_fbe_package_to_api_package (update_object_msg_t * update_request);
static void fbe_api_notification_interface_unregister_packages(notification_info_t *  notification_info);

/*!***************************************************************
 * @fn fbe_api_notification_interface_init()
 *****************************************************************
 * @brief
 *   This function is used for initializing the notification interface
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_notification_interface_init (void)
{
    EMCPAL_STATUS        nt_status;
    fbe_package_id_t     package_id = FBE_PACKAGE_ID_INVALID;

    if (notification_interface_initialized == FBE_FALSE) {
        
        /*initialize a queue of notifications and the lock to it*/
        fbe_queue_init(&fbe_api_notification_queue_head);
        fbe_spinlock_init(&fbe_api_notification_queue_lock);
    
        /*init event registration*/
        fbe_queue_init(&fbe_api_notification_event_registration_queue);
        fbe_spinlock_init(&fbe_api_notification_event_registration_queue_lock);
    
        /*initialize the semaphore that will control the thread*/
        fbe_semaphore_init(&fbe_api_notification_semaphore, 0, FBE_COMMON_NOTIFICATION_MAX_SEMAPHORE_COUNT);
        fbe_semaphore_init(&fbe_api_notification_thread_semaphore, 0, 1);
    
        fbe_api_notification_thread_flag = FBE_API_NOTIFICATION_THREAD_RUN;
    
        /*start the thread that will execute updates from the queue*/
        nt_status = fbe_thread_init(&fbe_api_notification_thread_handle, "fbe_api_notif", fbe_api_notification_thread_func, NULL);
    
        if (nt_status != EMCPAL_STATUS_SUCCESS) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't start notification thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id ++) {
            notification_registered[package_id] = FBE_FALSE;
			notification_element[package_id].notification_type = FBE_NOTIFICATION_TYPE_INVALID;
			notification_element[package_id].object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
        }

        notification_interface_initialized = TRUE;
    }

    return FBE_STATUS_OK;
    

}

/*!***************************************************************
 * @fn fbe_api_notification_interface_destroy()
 *****************************************************************
 * @brief
 *   This function frees used resources
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_notification_interface_destroy ()
{
    fbe_package_id_t        package_id = FBE_PACKAGE_ID_INVALID;

    if (notification_interface_initialized == FBE_TRUE){

        
         /*unregister for notification from package*/
        for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
            if (notification_registered[package_id]) {
                fbe_api_notification_unregister_notification_element(package_id);
            }
			
        }
        

        /*flag the theread we should stop*/
        fbe_api_notification_thread_flag = FBE_API_NOTIFICATION_THREAD_STOP;
        
        /*change the state of the sempahore so the thread will not block*/
        fbe_semaphore_release(&fbe_api_notification_semaphore, 0, 1, FALSE);
        
        /*wait for it to end*/
        fbe_semaphore_wait_ms(&fbe_api_notification_thread_semaphore, 20000);

        if (FBE_API_NOTIFICATION_THREAD_DONE == fbe_api_notification_thread_flag) {
            fbe_thread_wait(&fbe_api_notification_thread_handle);
        } else {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: notification thread (%p) did not exit!\n",
                          __FUNCTION__,
                          fbe_api_notification_thread_handle.thread);
        }
        fbe_thread_destroy(&fbe_api_notification_thread_handle);
        fbe_semaphore_destroy(&fbe_api_notification_thread_semaphore);
        
        /*destroy all other resources*/
        fbe_semaphore_destroy(&fbe_api_notification_semaphore);
        
        fbe_spinlock_destroy(&fbe_api_notification_queue_lock);
        fbe_queue_destroy (&fbe_api_notification_queue_head);

        /*check if someone is still registered to get notifications*/
        if (!fbe_queue_is_empty(&fbe_api_notification_event_registration_queue)){
            fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s: Some elements are still registered for events, emptying queueu\n", __FUNCTION__); 
            empty_event_registration_queue();
        }
        
        fbe_queue_destroy(&fbe_api_notification_event_registration_queue);
        fbe_spinlock_destroy(&fbe_api_notification_event_registration_queue_lock);

        notification_interface_initialized = FALSE;
        return FBE_STATUS_OK;
    
    }else{
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: notification interface not initialized, can't destroy\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
}

/*!***************************************************************
 * @fn fbe_api_notification_register_notification_element(
 *       fbe_package_id_t package_id, 
 *       void * context)
 *****************************************************************
 * @brief
 *   register for getting notification on object changes   
 *
 * @param package_id - package ID
 * @param context - pointer to context info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  8/07/07: sharel created
 *
 ****************************************************************/
static fbe_status_t fbe_api_notification_register_notification_element(fbe_package_id_t package_id,
																	   void * context, 
																	   fbe_notification_type_t notification_type,
																	   fbe_topology_object_type_t object_type)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    notification_element[package_id].targe_package = package_id;
    notification_element[package_id].notification_function = fbe_api_notification_function;
    notification_element[package_id].notification_context = context;
    notification_element[package_id].notification_type |= notification_type;
	notification_element[package_id].object_type |= object_type;
    
	if (FBE_IS_TRUE(notification_registered[package_id])) {
		return FBE_STATUS_OK;/*no need to re-send*/
	}

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
                                         &notification_element[package_id],
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
    }

    fbe_transport_wait_completion(packet);

    status = fbe_transport_get_status_code(packet);

    /*we don't check for the status because some packges may not exist*/
    fbe_api_return_contiguous_packet(packet);

	if (status == FBE_STATUS_OK) {
		notification_registered[package_id] = FBE_TRUE;
	}
    
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_api_notification_function(
 *        fbe_object_id_t object_id, 
 *        fbe_notification_info_t notification_info, 
 *        fbe_notification_context_t context)
 *****************************************************************
 * @brief
 *   register for getting notification on object changes   
 *
 * @param object_id - object that had the event
 * @param notification_info - notification info
 * @param context - any context we saved when we registered
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  8/07/07: sharel created
 *  9/10/07:sharel changes to support object state instead of notification type
 *
 ****************************************************************/
fbe_status_t fbe_api_notification_function(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context)
{
    update_object_msg_t *   update_msg = NULL;

   /*we allocate here and free after the message is used from the queue*/
    update_msg = (update_object_msg_t *) fbe_api_allocate_memory (sizeof (update_object_msg_t));

    if (update_msg == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for map update\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*copy theinformation we have now*/
    update_msg->object_id = object_id;
    fbe_copy_memory(&update_msg->notification_info, &notification_info, sizeof (fbe_notification_info_t));

    fbe_spinlock_lock (&fbe_api_notification_queue_lock);
    fbe_queue_push(&fbe_api_notification_queue_head, &update_msg->queue_element);
    fbe_spinlock_unlock (&fbe_api_notification_queue_lock);
    fbe_semaphore_release(&fbe_api_notification_semaphore, 0, 1, FALSE);
        
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_notification_unregister_notification_element(
 *       fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   remove ourselves from events notification 
 *
 * @param package_id - package ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  8/07/07: sharel created
 *
 ****************************************************************/
static fbe_status_t fbe_api_notification_unregister_notification_element(fbe_package_id_t package_id)
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
                                         &notification_element[package_id],
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
    }

    fbe_transport_wait_completion(packet);

    /*check the packet status to make sure we have no errors*/
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to unregister events, status:%d\n", __FUNCTION__, status); 
        fbe_api_return_contiguous_packet(packet);
        return status;
    }
    
    fbe_api_return_contiguous_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_notification_thread_func(
 *       void * context)
 *****************************************************************
 * @brief
 *   the thread that will dispatch the state changes from the queue
 *
 * @param context - pointer to the context info
 *
 * @return
 *
 * @version
 *  8/07/07: sharel created
 *
 ****************************************************************/
static void fbe_api_notification_thread_func(void * context)
{
    EMCPAL_STATUS           nt_status;

    FBE_UNREFERENCED_PARAMETER(context);
    
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:\n", __FUNCTION__); 

    while(1)    
    {
        /*block until someone takes down the semaphore*/
        nt_status = fbe_semaphore_wait(&fbe_api_notification_semaphore, NULL);
        if(fbe_api_notification_thread_flag == FBE_API_NOTIFICATION_THREAD_RUN) {
            /*check if there are any notifictions waiting on the queue*/
            fbe_api_notification_dispatch_queue();
        } else {
            break;
        }
    }

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:\n", __FUNCTION__); 

    fbe_api_notification_thread_flag = FBE_API_NOTIFICATION_THREAD_DONE;
    fbe_semaphore_release(&fbe_api_notification_thread_semaphore, 0, 1, FALSE);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}


/*!***************************************************************
 * @fn fbe_api_notification_dispatch_queue()
 *****************************************************************
 * @brief
 *   deque objects and call the function to change their state
 *
 * @return
 *
 * @version
 *  8/07/07: sharel created
 *
 ****************************************************************/
static void fbe_api_notification_dispatch_queue(void)
{
    update_object_msg_t         * update_request = NULL;
    
    while (1) {
         fbe_spinlock_lock(&fbe_api_notification_queue_lock);
        /*wo we have something on the queue ?*/
        if (!fbe_queue_is_empty(&fbe_api_notification_queue_head)) {
            /*get the next item from the queue*/
            update_request = (update_object_msg_t *) fbe_queue_pop(&fbe_api_notification_queue_head);
            fbe_spinlock_unlock(&fbe_api_notification_queue_lock); 

            fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                           "%s: Dispatching notification. Obj:%d Obj type: 0x%llX\n",
                           __FUNCTION__, update_request->object_id,
			   (unsigned long long)update_request->notification_info.notification_type); 

            fbe_api_notification_send_notification (update_request);
            
            /*and delete it, we don't need it anymore*/
            fbe_api_free_memory (update_request);

        } else {
            fbe_spinlock_unlock(&fbe_api_notification_queue_lock); 
            break;
        }
    }
       
}

/*!***************************************************************
 * @fn fbe_api_notification_interface_register_notification(fbe_notification_type_t state_mask,
                                                            fbe_package_notification_id_mask_t package_mask,
                                                            fbe_topology_object_type_t object_type_mask,
                                                            fbe_api_notification_callback_function_t callback_func,
                                                            void * context,
															fbe_notification_registration_id_t *user_registration_id)
 *****************************************************************
 *@brief
 *  register a function that will alert external component of state changes
 *  or any other notification.
 *
 *  @param notification_type - what kind of notification you want
 *  @param package_mask - the packet we want to register with
 *  @param object_type_mask - which objects we want to get notification from
 *  @param callback_func - the function to call
 *  @param context - context to get back to user upon callback
 *  @param user_registration_id - returned to the user and used later for unregistration
 *           
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_notification_interface_register_notification(fbe_notification_type_t notification_type,
                                                                                fbe_package_notification_id_mask_t package_mask,
                                                                                fbe_topology_object_type_t object_type_mask,
                                                                                fbe_api_notification_callback_function_t callback_func,
                                                                                void * context,
																			    fbe_notification_registration_id_t * registration_id)
{
    notification_info_t *       			notification_info = NULL;
    fbe_u32_t                   			package_count = 0x1;
    fbe_package_id_t            			fbe_package_id = 0x1;/*start with the first one, no matter what it is*/
	fbe_package_notification_id_mask_t		package_id = 0x1;

    if (callback_func == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:callback function is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

	if (FBE_IS_FALSE(notification_interface_initialized)) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: you did not initialize notification inrerface\n", __FUNCTION__); 
       return FBE_STATUS_GENERIC_FAILURE;
	}

	notification_info = (notification_info_t *)fbe_api_allocate_memory (sizeof (notification_info_t));

	if (notification_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to get memory to register even callback\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*stick this info in out queue for later use*/
    notification_info->state_mask = notification_type;
    notification_info->callback_func = callback_func;
    notification_info->package_mask = package_mask;
    notification_info->notification_context = context;
    notification_info->object_type = object_type_mask;
	notification_info->notification_type = notification_type;
	notification_info->ref_count = 0;/*reset*/
    
    fbe_spinlock_lock(&fbe_api_notification_event_registration_queue_lock);
	*registration_id = notification_info->registraction_id = fbe_api_notification_current_reg_id;
	fbe_api_notification_current_reg_id++;
    fbe_queue_push (&fbe_api_notification_event_registration_queue, &notification_info->queue_element);
    fbe_spinlock_unlock(&fbe_api_notification_event_registration_queue_lock);

    /*now we want to make sure we already have entry point for this package*/
    while (package_count < FBE_PACKAGE_ID_LAST) {
		fbe_notification_convert_package_bitmap_to_fbe_package(package_id, &fbe_package_id);
        if ((package_id & package_mask) /*&& !notification_registered[fbe_package_id]*/) {
                fbe_api_notification_register_notification_element(fbe_package_id, context, notification_type, object_type_mask);
        }

        package_id <<= 1;/*go to next one*/
        package_count++;
    }

    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_notification_send_notification(
 *        update_object_msg_t * update_request)
 *****************************************************************
 * @brief
 *   send notification to registered Flare modules
 *
 * @param update_request - pointer to the update request
 *
 * @return
 *
 * @version
 *  10/03/08: sharel    created
 *
 ****************************************************************/
static void fbe_api_notification_send_notification (update_object_msg_t * update_request)
{
    notification_info_t *       		notification_info = NULL;
	fbe_package_notification_id_mask_t	package_bitmap;

    fbe_spinlock_lock(&fbe_api_notification_event_registration_queue_lock);
    notification_info = (notification_info_t *)fbe_queue_front(&fbe_api_notification_event_registration_queue);
    while (notification_info != NULL) {

		fbe_notification_convert_fbe_package_to_package_bitmap(update_request->notification_info.source_package, &package_bitmap);
		
		if ((update_request->notification_info.notification_type & notification_info->notification_type) &&
			(package_bitmap & notification_info->package_mask) &&
			(update_request->notification_info.object_type & notification_info->object_type)) {

			fbe_atomic_increment(&notification_info->ref_count);/*so none can unregister until we are done*/

			fbe_spinlock_unlock(&fbe_api_notification_event_registration_queue_lock);

            fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH,
						   "%s: Invoking App Callback. App notify_type: %d, reg_obj_type: %d \n",
							__FUNCTION__, (int)update_request->notification_info.notification_type, 
						   (int)update_request->notification_info.object_type); 
	
			notification_info->callback_func (update_request, notification_info->notification_context);

			fbe_spinlock_lock(&fbe_api_notification_event_registration_queue_lock);
			fbe_atomic_decrement(&notification_info->ref_count);/*so none can unregister until we are done*/
		}
        
        /*and go to the next one*/
        notification_info = (notification_info_t *)fbe_queue_next(&fbe_api_notification_event_registration_queue, &notification_info->queue_element);
    }

     fbe_spinlock_unlock(&fbe_api_notification_event_registration_queue_lock);
}

/*!***************************************************************
 * @fn fbe_api_notification_interface_unregister_notification(
 *        fbe_api_notification_callback_function_t callback_func,
		  fbe_api_registration_id user_registration_id)
 *****************************************************************
 * @brief
 *  unregister from notifications
 *
 * @param callback_func - the callback used for registration
 * @param user_registration_id - the same ID the user got for unregistration
 *           
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_notification_interface_unregister_notification(fbe_api_notification_callback_function_t callback_func,
																				 fbe_notification_registration_id_t user_registration_id)
{
    notification_info_t *       notification_info = NULL;
    notification_info_t *       next_notification_info = NULL;
	fbe_atomic_t				unregister_lock = 0;

	if (FBE_IS_FALSE(notification_interface_initialized)) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: you did not initialize notification inrerface\n", __FUNCTION__); 
       return FBE_STATUS_GENERIC_FAILURE;
	}
    
    /*we need to walk the queue and look for the registered function and pop it out*/

    fbe_spinlock_lock(&fbe_api_notification_event_registration_queue_lock);
    next_notification_info = (notification_info_t *)fbe_queue_front(&fbe_api_notification_event_registration_queue);
    while (next_notification_info != NULL) {
        notification_info = next_notification_info;
        next_notification_info = (notification_info_t *)fbe_queue_next(&fbe_api_notification_event_registration_queue, &next_notification_info->queue_element);   
        if ((notification_info->callback_func == callback_func) && (user_registration_id == notification_info->registraction_id )){

            /*can we unregister(protect against unregistring whiole we still get notifications*/
			fbe_spinlock_unlock(&fbe_api_notification_event_registration_queue_lock); 

			do {
				fbe_spinlock_lock(&fbe_api_notification_event_registration_queue_lock);
				/*before we do anything,let's check the ref count*/
				fbe_atomic_exchange(&unregister_lock, notification_info->ref_count);
				if (!unregister_lock) {
					/*we are good*/
					fbe_queue_remove((fbe_queue_element_t *)notification_info);
				}

				fbe_spinlock_unlock(&fbe_api_notification_event_registration_queue_lock); 

				if (unregister_lock) {
					fbe_thread_delay(200);
				}
				
			} while (unregister_lock);

            
            /*at this stage we need to unregister from the specific pacakges that were on this notification,
            but do it only if there is no other notification that did that*/
            fbe_api_notification_interface_unregister_packages(notification_info);
            fbe_api_free_memory (notification_info);

            return FBE_STATUS_OK;
        }
        
    }

    /*if we got here we did not find the function to unregister*/
    fbe_spinlock_unlock(&fbe_api_notification_event_registration_queue_lock);     
    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: did not find function %p, and ID %llu, to unregister\n",
		    __FUNCTION__, (void *)callback_func,
		    (unsigned long long)user_registration_id); 
    return FBE_STATUS_GENERIC_FAILURE;  

}

/*!***************************************************************
 * @fn empty_event_registration_queue()
 *****************************************************************
 * @brief
 *   unregister any previously registered function that forgot to unregister itself
 *
 * @return
 *
 * @version
 *  1/02/09: sharel created
 *
 ****************************************************************/
static void empty_event_registration_queue(void)
{
    notification_info_t *       notification_info = NULL;
    
    /*we need to walk the queue and look for the registered function and pop it out*/
    fbe_spinlock_lock(&fbe_api_notification_event_registration_queue_lock);
    notification_info = (notification_info_t *)fbe_queue_front(&fbe_api_notification_event_registration_queue);
    if (notification_info != NULL) {
        notification_info = (notification_info_t *)fbe_queue_pop(&fbe_api_notification_event_registration_queue); 
        fbe_api_free_memory (notification_info);        
    }

    /*if we got here we did not find the function to unregister*/
    fbe_spinlock_unlock(&fbe_api_notification_event_registration_queue_lock);     
    return; 
}

/*!***************************************************************
 * @fn fbe_api_notification_translate_fbe_state_to_api_state(
 *        update_object_msg_t * update_request)
 *****************************************************************
 * @brief
 *  map between the fbe state and the shim state.
 *  this is needed because FBE state (fbe_lifecycle_state_t)is serving as index
 *  and we can't do a bit map with it
 *
 * @param update_request - pointer to the update request data
 *
 * @return fbe_notification_type_t - lifecycle state
 *
 * @version
 *  1/02/09: sharel created
 *
 ****************************************************************/
#if 0/*depreacted*/
static fbe_notification_type_t fbe_api_notification_translate_fbe_state_to_api_state (update_object_msg_t * update_request)
{
    fbe2api_state_map_t *       map  = state_map;

    /*if we have to translate a lifecycle state we pick the lifecycle state path*/
    if (update_request->notification_info.notification_type == NOTIFICATION_TYPE_LIFECYCLE_STATE_CHANGED) {
        while (map->fbe_state != FBE_LIFECYCLE_STATE_INVALID) {
            if (map->fbe_state == update_request->notification_info.notification_data.lifecycle_state) {
                return map->api_state;
            }
    
            map++;
        }
    
    }else {
        /*if it's not a lifecycle state, it might be one of the other special cases*/
        switch (update_request->notification_info.notification_type) {
        case FBE_NOTIFICATION_TYPE_END_OF_LIFE:
            return FBE_NOTIFICATION_TYPE_END_OF_LIFE;
            break;
        case FBE_NOTIFICATION_TYPE_RECOVERY:
            return FBE_NOTIFICATION_TYPE_RECOVERY;
            break;
        case FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED:
            return FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
            break;
        case FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED:
            return FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
            break;
		case FBE_NOTIFICATION_TYPE_CHECK_QUEUED_IO:/*FBE3, this is temporary and should really removed, it was invented to resolve some Flare issues*/
			return FBE_API_STATE_OBJECT_IO_TIMER_CHECK_4_ACTIVATE;
        case FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED:
            return  FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED;
			break;
        case FBE_NOTIFICATION_TYPE_SWAP_INFO:
            return FBE_NOTIFICATION_TYPE_SWAP_INFO;
            break;
        case FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED:
            return FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED;
            break;
		case FBE_NOTIFICATION_TYPE_FBE_ERROR_TRACE:
			return FBE_NOTIFICATION_TYPE_FBE_ERROR_TRACE;
			break;
        default:
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't map notification type:%d, notification lost !!!\n", __FUNCTION__,  update_request->notification_info.notification_type); 
            return FBE_NOTIFICATION_TYPE_INVALID;
            break;
        }
    }

    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't map notification state:%d, notification lost !!!\n", __FUNCTION__,  update_request->notification_info.notification_data.lifecycle_state); 
    return FBE_NOTIFICATION_TYPE_INVALID;
}
#endif

/*!***************************************************************
 * @fn fbe_api_notification_translate_fbe_package_to_api_package(
 *       update_object_msg_t * update_request)
 *****************************************************************
 * @brief
 *  translate fbe package to api package
 *
 * @param update_request - pointer to the update request data
 *
 * @return fbe_package_notification_id_mask_t - package ID
 *
 * @version
 *  1/02/09: sharel created
 *
 ****************************************************************/
#if 0 /*depracated*/
static fbe_package_notification_id_mask_t fbe_api_notification_translate_fbe_package_to_api_package (update_object_msg_t * update_request)
{
    fbe2api_package_map_t *       map  = package_map;

    
     while (map->fbe_package != FBE_PACKAGE_ID_INVALID) {
        if (map->fbe_package == update_request->notification_info.source_package) {
            return map->api_package;
        }

        map++;
    }

    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't map package ID:0x%X to nothing, notification Lost !!!!!\n", __FUNCTION__,  update_request->notification_info.source_package); 
    return FBE_PACKAGE_NOTIFICATION_ID_INVALID;

}
#endif


/*!***************************************************************
 * @fn fbe_api_notification_translate_api_package_to_fbe_package(
 *        fbe_package_notification_id_mask_t package_id)
 *****************************************************************
 * @brief
 *  translate api package to fbe package
 *
 * @param package_id - api package ID
 *
 * @return fbe_package_id_t - package ID
 *
 * @version
 *  1/02/09: sharel created
 *
 ****************************************************************/
#if 0/*depracated*/
fbe_package_id_t FBE_API_CALL fbe_api_notification_translate_api_package_to_fbe_package (fbe_package_notification_id_mask_t package_id)
{
    fbe2api_package_map_t *       map  = package_map;

    
     while (map->fbe_package != FBE_PACKAGE_ID_INVALID) {
        if (map->api_package == package_id) {
            return map->fbe_package;
        }

        map++;
    }

    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't map package ID:0x%X to nothing, notification Lost !!!!!\n", __FUNCTION__,  package_id); 
    return FBE_PACKAGE_ID_INVALID;

}

#endif /*depracated*/
/*!***************************************************************
 * @fn fbe_api_notification_interface_unregister_packages(
 *       notification_info_t *  notification_info_in)
 *****************************************************************
 * @brief
 *  unregister package 
 *
 * @param notification_info_in - notification info 
 *
 * @return
 *
 * @version
 *  1/02/09: sharel created
 *
 ****************************************************************/
static void fbe_api_notification_interface_unregister_packages(notification_info_t *  notification_info_in)
{
    notification_info_t *       			notification_info = NULL;
    notification_info_t *       			next_notification_info = NULL;
    fbe_package_notification_id_mask_t      package_id = 0x1;/*will always be the first package*/
    fbe_u32_t                   			package_count = 0x1;
    fbe_package_id_t            			fbe_package_id = 0x1;

    fbe_spinlock_lock(&fbe_api_notification_event_registration_queue_lock);
    next_notification_info = (notification_info_t *)fbe_queue_front(&fbe_api_notification_event_registration_queue);
    while (next_notification_info != NULL) {
        notification_info = next_notification_info;
        next_notification_info = (notification_info_t *)fbe_queue_next(&fbe_api_notification_event_registration_queue, &next_notification_info->queue_element);   
        if (notification_info_in->package_mask & notification_info->package_mask) {
            package_count = 0x1;
            package_id = 0x1;
            /*this notification info has at least one of the packages we want to unregister from so it means we can't unregister from it
            We start to walk over the bit to find where the match is and mark our bits as zero until the next iteration*/
            while (package_count < FBE_PACKAGE_ID_LAST) {
                if ((package_id & notification_info_in->package_mask) && (package_id & notification_info->package_mask)) {
                    notification_info_in->package_mask &= ~package_id;
                }
        
                package_id <<= 1;/*go to next one*/
                package_count++;
            }

            /*now we want to point again to the start of the queue head and see if we have a match again or not*/
            next_notification_info = (notification_info_t *)fbe_queue_front(&fbe_api_notification_event_registration_queue);
        }
    }
    fbe_spinlock_unlock(&fbe_api_notification_event_registration_queue_lock);    

    package_count = 0x1;
    package_id = 0x1;

    
    /*now we can unregister all packages left in this mask*/
    while (package_count < FBE_PACKAGE_ID_LAST) {
	fbe_notification_convert_package_bitmap_to_fbe_package(package_id, &fbe_package_id);
        if ((package_id & notification_info_in->package_mask) && notification_registered[fbe_package_id]) {
            fbe_api_notification_unregister_notification_element(fbe_package_id);
            notification_registered[fbe_package_id] = FBE_FALSE;
        }

        package_id <<= 1;/*go to next one*/
        package_count++;
    }


}
