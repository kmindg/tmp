
#include <ntddk.h>

#include "ktrace.h"
#include "fbe_base_package.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_driver_notification_interface.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe_panic_sp.h"


/***************************************************************************
 *  fbe_driver_notification.c
 ***************************************************************************
 *
 *  Description
 *      manage notifications for user space
 *      
 *
 *  History:
 *      7/22/09	sharel	Created
 *    
 ***************************************************************************/


/**************************************
				Local variables
**************************************/

/*maximum allowed registered applications*/
#define MAX_REG_APP						0x20
#define SEM_MISS_COUNT					10

/* number of notifications to allocate memory */
#define NUM_NOTIFICATIONS 400


/*we don't want to consume more than 90MB of notifications, this should give us roughly 100K of pending notifications*/
#define FBE_DRIVER_NOTIFICATION_MAX_QUEUED_NOTIFICATIONS ((1048576 * 90) / sizeof(fbe_notification_info_t))

typedef enum thread_flag_e{
    THREAD_RUN,
    THREAD_STOP,
    THREAD_DONE
}thread_flag_t;

typedef enum app_register_status_e{
    APP_REGISTER_STATUS_NOT_REGISTERED,
    APP_REGISTER_STATUS_IN_USE,
    APP_REGISTER_STATUS_REGISTERED,
    APP_REGISTER_STATUS_UN_REGISTER_IN_PROGRESS
}app_register_status_t;

typedef struct registered_application_data_s {
	app_register_status_t				registered;
	fbe_semaphore_t  		packet_arrived_semaphore;
	PEMCPAL_IRP 			PIrp;
	fbe_u32_t				semaphore_miss_count;/*how many times did the app did not send get_next*/
    volatile fbe_u32_t		waiting;
	fbe_char_t				name[APP_NAME_LENGTH];
}registered_application_data_t;

/* used in pre-allocation of memory for notifications */
typedef struct notification_mem_packet_q_element_s {
   fbe_queue_element_t q_element;
   fbe_bool_t dynamically_allocated;
   notification_data_t mem;
} notification_mem_packet_q_element_t;


static fbe_notification_element_t 			driver_notification_element;
static fbe_spinlock_t						registered_apps_table_lock;
static fbe_thread_t							event_notification_thread;
static fbe_semaphore_t      				event_notification_semaphore;
static thread_flag_t						event_notification_thread_flag;
static fbe_spinlock_t						notifications_data_lock;
static fbe_queue_head_t	   					quick_notifications_data_head;
static fbe_queue_head_t	   					background_notifications_data_head;
static fbe_package_id_t						destination_package = FBE_PACKAGE_ID_INVALID;
registered_application_data_t				registered_applications_table [MAX_REG_APP];
static fbe_atomic_t							outstanding_notifications = 0;
static fbe_u32_t							registered_applications = 0;
static fbe_queue_head_t				pre_allocated_queue_head;
static fbe_queue_head_t				outstanding_queue_head;
static fbe_spinlock_t				notification_mem_spinlock;
static fbe_u8_t *                         notif_mem_pool = NULL;
/*******************************************
				Local functions
********************************************/
static fbe_status_t register_notification_element(void);
static fbe_status_t	unregister_notification_element(void);
static fbe_status_t fbe_driver_notification_function(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context);
static void notify_application_of_event_thread_func(void * context);
static void notify_all_registered_applications(notification_data_t *	event_notification_data);
static fbe_status_t get_next_available_registration_table_entry (application_id_t * application_id);
static void initialize_registration_table(void);
static void remove_application_from_table(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp);
static notification_data_t * get_notification_mem(void);
static void release_notification_mem(notification_data_t * notif_mem);

/*********************************************************************
 *            fbe_driver_notification_init ()
 *********************************************************************
 *
 *  Description: do the first registration to get notifications from the
 *				 notification service, once we get them we will propagate
 *				 them to the registered user applications
 *
 *
 *  Return Value: none
 *
 *  History:
 *    10/16/07: sharel	created
 *    
 *********************************************************************/
void fbe_driver_notification_init (fbe_package_id_t package_id)
{
	EMCPAL_STATUS 				nt_status;
        fbe_u32_t                               count = 0;
        notification_mem_packet_q_element_t *           mem_packet_q_element;
        fbe_u8_t * notif_mem_pool_copy = NULL;

        
	initialize_registration_table();

        fbe_queue_init(&pre_allocated_queue_head);
        fbe_spinlock_init(&notification_mem_spinlock);
        fbe_queue_init(&outstanding_queue_head);

	fbe_spinlock_init(&registered_apps_table_lock);
	fbe_queue_init (&quick_notifications_data_head);
	fbe_queue_init (&background_notifications_data_head);
	fbe_spinlock_init(&notifications_data_lock);

        /* Pre allocating memory for notificatioins*/
        notif_mem_pool = (fbe_u8_t *) fbe_allocate_nonpaged_pool_with_tag((NUM_NOTIFICATIONS*sizeof(notification_mem_packet_q_element_t)), 'qmem');

        notif_mem_pool_copy = notif_mem_pool;

        if (notif_mem_pool != NULL) {
           for (count = 0; count < NUM_NOTIFICATIONS; count++) {
              mem_packet_q_element = (notification_mem_packet_q_element_t *) notif_mem_pool;
              fbe_zero_memory(mem_packet_q_element, sizeof(notification_mem_packet_q_element_t));
              fbe_queue_element_init(&mem_packet_q_element->q_element);
              mem_packet_q_element->dynamically_allocated = FBE_FALSE;
              /* add to pre-allocated queue */
              fbe_queue_push(&pre_allocated_queue_head, &mem_packet_q_element->q_element);
              notif_mem_pool += sizeof(notification_mem_packet_q_element_t);
           }

           /* to make notfi_mem_pool point to the start of pre allocated memory */
           notif_mem_pool = notif_mem_pool_copy;
        }

        else {
           KvTrace("%s: Failed to pre-allocate memory for notifications\n",
			__FUNCTION__);
        }

        
	/*to make sure no one abusese the notification to pass big structures across we do some sanity checks here*/
	FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_notification_info_t) < (1024 /2))
    
	/*start the thread that will get notifications and handle them, we need a limit of FBE_MAX_OBJECTS
	because in theory we might have a system collapsing and all objects sending notifications before we even have
	time to run the release thread*/
	fbe_semaphore_init(&event_notification_semaphore, 0 ,FBE_SEMAPHORE_MAX);
	event_notification_thread_flag = THREAD_RUN;
	nt_status = fbe_thread_init(&event_notification_thread, "fbe_notif_evt", notify_application_of_event_thread_func, NULL);

	if (nt_status != EMCPAL_STATUS_SUCCESS) {
	   	KvTrace("%s: can't start notification queue thread %X\n",
			__FUNCTION__, nt_status);
        return;
    }

	destination_package = package_id;

    /*register to get notifications from the service*/
	register_notification_element();

}


/*********************************************************************
 *            fbe_driver_notification_destroy ()
 *********************************************************************
 *
 *  Description: remove all allocated resources
 *
 *
 *  Return Value: none
 *
 *  History:
 *    10/16/07: sharel	created
 *    
 *********************************************************************/
void fbe_driver_notification_destroy (void)
{
        
        KvTrace("%s: entry\n", __FUNCTION__);

        fbe_spinlock_lock(&notification_mem_spinlock);
        fbe_queue_destroy (&outstanding_queue_head);
        fbe_queue_destroy (&pre_allocated_queue_head);
        if (notif_mem_pool != NULL) {
           fbe_release_nonpaged_pool_with_tag(notif_mem_pool, 'qmem');
        }
        fbe_spinlock_unlock(&notification_mem_spinlock);
        fbe_spinlock_destroy(&notification_mem_spinlock);

	/*unregister from the notification service*/
	unregister_notification_element();

	/*stop the thread that waits for events*/
    event_notification_thread_flag = THREAD_STOP;
	fbe_semaphore_release(&event_notification_semaphore, 0, 1, FALSE);/*let the thread reach the flag*/

	/*wait for thread to end*/
	fbe_thread_wait(&event_notification_thread);
	fbe_thread_destroy(&event_notification_thread);

	fbe_semaphore_destroy(&event_notification_semaphore);
	fbe_queue_destroy (&quick_notifications_data_head);
	fbe_queue_destroy (&background_notifications_data_head);
	fbe_spinlock_destroy(&notifications_data_lock);

    fbe_spinlock_destroy(&registered_apps_table_lock);

    
	KvTrace("%s: completed\n", __FUNCTION__);

}

/*********************************************************************
 *            register_notification_element ()
 *********************************************************************
 *
 *  Description: register for getting notification on object changes   
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    10/16/07: sharel	created
 *    
 *********************************************************************/
static fbe_status_t register_notification_element(void)
{
	fbe_packet_t * 				packet = NULL;	
    fbe_status_t				status = FBE_STATUS_GENERIC_FAILURE;
	fbe_payload_ex_t *			payload = NULL;
	fbe_payload_control_operation_t * control_operation = NULL; 

    driver_notification_element.notification_function = fbe_driver_notification_function;
	driver_notification_element.notification_context = NULL;
	driver_notification_element.notification_type = FBE_NOTIFICATION_TYPE_ALL_WITHOUT_PENDING_STATES;/*for user space we want to reduce traffic and don't support pending states*/
	driver_notification_element.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    
    /* Allocate packet */
	packet = (fbe_packet_t *) fbe_allocate_nonpaged_pool_with_tag ( sizeof (fbe_packet_t), 'knpf');
	
	if(packet == NULL) {
		KvTrace("%s: failed to allocate memory for packet\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    if (destination_package == FBE_PACKAGE_ID_PHYSICAL) {
		fbe_transport_initialize_packet(packet);
	}else{
		fbe_transport_initialize_sep_packet(packet);
	}

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation (control_operation,
										 FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
										 &driver_notification_element,
										  sizeof(fbe_notification_element_t));

    /* Set packet address */
	fbe_transport_set_address(	packet,
								destination_package,
								FBE_SERVICE_ID_NOTIFICATION,
								FBE_CLASS_ID_INVALID,
								FBE_OBJECT_ID_INVALID); 

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
	fbe_service_manager_send_control_packet(packet);
    fbe_transport_wait_completion(packet);
    
	/*check the packet status to make sure we have no errors*/
	status = fbe_transport_get_status_code (packet);
	if (status != FBE_STATUS_OK){
		KvTrace ( "%s: unable to register for events %X\n",
			  __FUNCTION__, status);	
		fbe_release_nonpaged_pool_with_tag(packet, 'knpf');
		return status;
	}
	
   /* need to destroy the packet before release its memory. This is especially true for semaphore using CSX implementation */
    fbe_transport_destroy_packet(packet);


	fbe_release_nonpaged_pool_with_tag(packet, 'knpf');
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_driver_notification_function ()
 *********************************************************************
 *
 *  Description: function that is called every time there is an event in any package   
 *
 *  Inputs: object_id - the object that had the event
 *  		fbe_lifecycle_state_t what is the new state of the object
 *			context - any context we saved when we registered
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    10/15/07: sharel	created
 *    
 *********************************************************************/
static fbe_status_t 
fbe_driver_notification_function(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context)
{
	notification_data_t	*		notification_data = NULL;

    /*should we event bother do anything (did anyone register yet ?)
	no need to call under lock, since we just check it and if someone just registered and we missed it, not a big
	deal, he would have missed this anyway*/
	if (registered_applications == 0){
		/*no one registered, just go out*/
		return FBE_STATUS_OK;
	}

	/*another check to make sure a rogue object/s do not send a stream of notification that will take time to 
	clear and eventually chew up all the non paged pool */
	fbe_atomic_increment(&outstanding_notifications);

        if (outstanding_notifications > FBE_DRIVER_NOTIFICATION_MAX_QUEUED_NOTIFICATIONS) {
		KvTrace ("\n\n%s:ERR Over %d of outstanding notification, can't send more\n\n", __FUNCTION__, (int)FBE_DRIVER_NOTIFICATION_MAX_QUEUED_NOTIFICATIONS);
		#if DBG
		FBE_PANIC_SP(FBE_PANIC_SP_BASE, FBE_STATUS_OK);
		#endif
		return FBE_STATUS_OK;
	}

        /* get memory for a notification */
        notification_data = get_notification_mem();

	if (notification_data == NULL) {
		KvTrace ("%s: unable to allocate memory for notification_data\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*save the data*/
	notification_data->context  = (fbe_u64_t)context;
	notification_data->object_id = object_id;
	fbe_copy_memory(&notification_data->notification_info, &notification_info, sizeof (fbe_notification_info_t));
    
	/*add the data about the notification to the queue*/
	fbe_spinlock_lock(&notifications_data_lock);

	/*if these are the zeroing notifications there could be many of them on a very big system.
	We don't want them to come before state change notifications so we push them to the back*/
	if (notification_data->notification_info.notification_type == FBE_NOTIFICATION_TYPE_ZEROING) {
		fbe_queue_push(&background_notifications_data_head, &notification_data->queue_element);
	}else{
		fbe_queue_push(&quick_notifications_data_head, &notification_data->queue_element);
	}

	fbe_spinlock_unlock(&notifications_data_lock);

	/*now that we got the notification, we need to let the thread release the queued registered applications with this data*/
	fbe_semaphore_release(&event_notification_semaphore, 0, 1, FALSE);
    
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_driver_notification_register_application ()
 *********************************************************************
 *
 *  Description: user space application asks us to get events
 *
 *  Input: Irp from user space   
 *
 *  Return Value:  NT status
 *
 *  History:
 *    10/17/07: sharel	created
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_driver_notification_register_application (PEMCPAL_IRP PIrp)
{
	PEMCPAL_IO_STACK_LOCATION  							irpStack;
    fbe_u8_t * 										ioBuffer = NULL;
    fbe_package_register_app_notify_t *				fbe_package_register_app_notify = NULL;
	ULONG 											outLength = 0;
	application_id_t								application_id = 0;
	fbe_status_t									status;
    fbe_package_id_t								package_id;

	fbe_get_package_id(&package_id);
	
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);
	ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
	outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

	/*some sanity checking*/
	if (outLength != sizeof(fbe_package_register_app_notify_t)) {
		KvTrace("%s Illegal length %d \n", __FUNCTION__, outLength);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
		EmcpalExtIrpInformationFieldSet(PIrp, sizeof(fbe_package_register_app_notify_t));
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_SUCCESS;
	}

	fbe_package_register_app_notify = (fbe_package_register_app_notify_t *)ioBuffer;

	if (fbe_package_register_app_notify == NULL) {
		KvTrace("%s NULL buffer \n", __FUNCTION__);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
		EmcpalExtIrpInformationFieldSet(PIrp, sizeof(fbe_package_register_app_notify_t));
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_SUCCESS;
	}

    /*and allocate an entry in the table for this application*/
	status = get_next_available_registration_table_entry (&application_id);

	if (status != FBE_STATUS_OK) {
		KvTrace("%s No more apps allowed to register\n", __FUNCTION__);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
		EmcpalExtIrpInformationFieldSet(PIrp, sizeof(fbe_package_register_app_notify_t));
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_SUCCESS;
	}

	    /*remember the name for tracing*/
	fbe_copy_memory(&registered_applications_table[application_id].name, fbe_package_register_app_notify->registered_app_name, APP_NAME_LENGTH); 
	/*to make sure the user space did not forget to add terminating NULL to the string, we'll do so*/
    registered_applications_table[application_id].name[APP_NAME_LENGTH-1] =  0;

	KvTrace("%s:Pkg:%d, Registed new App with ID:%d\n", __FUNCTION__, package_id, application_id);
	KvTrace("App name:%s\n", fbe_package_register_app_notify->registered_app_name );


	/*set the return data for the user and initialize this row in the table*/
	fbe_package_register_app_notify->application_id = application_id;

	fbe_spinlock_lock(&registered_apps_table_lock);
	fbe_semaphore_init (&registered_applications_table[application_id].packet_arrived_semaphore, 0, 1);
	registered_applications_table[application_id].registered = APP_REGISTER_STATUS_REGISTERED;
	registered_applications_table[application_id].semaphore_miss_count = 0;
	registered_applications++;
	fbe_spinlock_unlock(&registered_apps_table_lock);


	/*and complete the packet*/
	EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
	EmcpalExtIrpInformationFieldSet(PIrp, sizeof(fbe_package_register_app_notify_t));

	EmcpalIrpCompleteRequest(PIrp);

	return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
 *            unregister_notification_element ()
 *********************************************************************
 *
 *  Description: remove ourselvs from events notification    
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    10/16/07: sharel	created
 *    
 *********************************************************************/
static fbe_status_t unregister_notification_element(void)
{
	fbe_packet_t * 			packet = NULL;	
    fbe_status_t					status = FBE_STATUS_GENERIC_FAILURE;
	fbe_payload_ex_t *				payload = NULL;
	fbe_payload_control_operation_t * control_operation = NULL; 

    /* Allocate packet */
	packet = (fbe_packet_t *) fbe_allocate_nonpaged_pool_with_tag (
															   sizeof (fbe_packet_t),
															   'knpf');
	
	if(packet == NULL) {
		KvTrace("%s: failed to allocate memory for packet\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    if (destination_package == FBE_PACKAGE_ID_PHYSICAL) {
		fbe_transport_initialize_packet(packet);
	}else{
		fbe_transport_initialize_sep_packet(packet);
	}
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation (control_operation,
										 FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,
										 &driver_notification_element,
										  sizeof(fbe_notification_element_t));

    /* Set packet address */
	fbe_transport_set_address(	packet,
								destination_package,
								FBE_SERVICE_ID_NOTIFICATION,
								FBE_CLASS_ID_INVALID,
								FBE_OBJECT_ID_INVALID); 

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
	fbe_service_manager_send_control_packet(packet);
    fbe_transport_wait_completion(packet);

	/*check the packet status to make sure we have no errors*/
	status = fbe_transport_get_status_code (packet);
	if (status != FBE_STATUS_OK){
		KvTrace ( "%s: unable to unregister events %X\n",
			  __FUNCTION__, status);	
		fbe_release_nonpaged_pool_with_tag(packet, 'knpf');
		return status;
	}
	
	fbe_release_nonpaged_pool_with_tag(packet, 'knpf');
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            notify_application_of_event_thread_func ()
 *********************************************************************
 *
 *  Description: the thread that will dispatch the event propagation to the applications
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    10/16/07: sharel	created
 *    
 *********************************************************************/
static void notify_application_of_event_thread_func(void * context)
{
	EMCPAL_STATUS 				nt_status;
	notification_data_t *	event_notification_data = NULL;

	FBE_UNREFERENCED_PARAMETER(context);
	
	KvTrace("%s started\n", __FUNCTION__);

    while(1)    
	{
		/*block until someone takes down the semaphore (which means there was an event)*/
		nt_status = fbe_semaphore_wait(&event_notification_semaphore, NULL);
		if(event_notification_thread_flag == THREAD_RUN) {
			 /*we send the event to all registered applications*/
			while (1){
				fbe_spinlock_lock(&notifications_data_lock);
				/*get the next notification from the quick queue first, once we get it we free the queue so the completion function will not block*/
				if(!fbe_queue_is_empty(&quick_notifications_data_head)){
                    event_notification_data = (notification_data_t *)fbe_queue_pop(&quick_notifications_data_head);
					fbe_spinlock_unlock(&notifications_data_lock);
					/*and send it to all registered applications*/
					notify_all_registered_applications(event_notification_data);
					/*now that we sent this event to all registered apps we don't need it anymore*/
                                        release_notification_mem(event_notification_data);
					fbe_atomic_decrement(&outstanding_notifications);/*count for the notifiations we sent*/
				 } else if (!fbe_queue_is_empty(&background_notifications_data_head)){
					event_notification_data = (notification_data_t *)fbe_queue_pop(&background_notifications_data_head);
					fbe_spinlock_unlock(&notifications_data_lock);
					/*and send it to all registered applications*/
					notify_all_registered_applications(event_notification_data);
					/*now that we sent this event to all registered apps we don't need it anymore*/
                                        release_notification_mem(event_notification_data);
					fbe_atomic_decrement(&outstanding_notifications);/*count for the notifiations we sent*/

				 }else{
					fbe_spinlock_unlock(&notifications_data_lock);
					break;
				 }
			}
		} else {
			break;
		}
    }

	KvTrace("%s finished\n", __FUNCTION__);

    event_notification_thread_flag = THREAD_DONE;
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/*********************************************************************
 *            notify_all_registered_applications ()
 *********************************************************************
 *
 *  Description: complete the IRPs to indicate events happened
 *
 *
 *  Return Value: none
 *
 *  History:
 *    10/16/07: sharel	created
 *    
 *********************************************************************/
static void notify_all_registered_applications(notification_data_t *	event_notification_data)
{
    
	notification_data_t *			app_buffer = NULL;
	fbe_u32_t						app_id = 0;
	PEMCPAL_IRP							registered_irp = NULL;
	fbe_status_t					status;
	
    /*go over all the registered applications and send them this event*/
	for (app_id = 0; app_id < MAX_REG_APP; app_id ++) {
		/*get a registered application from the table*/
		fbe_spinlock_lock(&registered_apps_table_lock);
		if (registered_applications_table[app_id].registered == APP_REGISTER_STATUS_REGISTERED) {
            registered_applications_table[app_id].waiting = 1;
			fbe_spinlock_unlock(&registered_apps_table_lock);

            /*blocking here will verify that the client received the previuous event.
			whenever the client receives an event, it is supposed to send the next IOCTL down
			this IOCTL is in the fbe_driver_notification_get_next_event function and it will 
			release the semaphore, until he send it we block here. we wait 3 seconds only so we will not hold other clients*/
			if (registered_applications_table[app_id].semaphore_miss_count < SEM_MISS_COUNT) {
			
				status = fbe_semaphore_wait_ms(&registered_applications_table[app_id].packet_arrived_semaphore, 3000);
				if (status == FBE_STATUS_TIMEOUT) {
                    registered_applications_table[app_id].semaphore_miss_count++;

					KvTrace ("App. id:%d didn't send get_next in 3 sec. %d times,loose next event. Pkg: %d. Notif. Type:0x%llX\n", 
							 app_id, registered_applications_table[app_id].semaphore_miss_count,
							 event_notification_data->notification_info.source_package, (unsigned long long)event_notification_data->notification_info.notification_type);    

					registered_applications_table[app_id].waiting = 0;

                    continue;
				}
			}else if (registered_applications_table[app_id].semaphore_miss_count >= SEM_MISS_COUNT) {
				if (registered_applications_table[app_id].semaphore_miss_count == SEM_MISS_COUNT) {
					KvTrace ("Pkg:%d,App. id:%d Irp0x%X, didn't send get_next in 3 sec. %d times, never sending\n",
							 event_notification_data->notification_info.source_package, app_id, (unsigned int)registered_applications_table[app_id].PIrp,
							 registered_applications_table[app_id].semaphore_miss_count);
	
					KvTrace("App name:%s\n", registered_applications_table[app_id].name);
				}

				registered_applications_table[app_id].waiting = 0;
				registered_applications_table[app_id].semaphore_miss_count = SEM_MISS_COUNT + 1;/*so the trace will stop showing up*/
				continue;
			}

			registered_applications_table[app_id].waiting = 0;
			registered_applications_table[app_id].semaphore_miss_count = 0;/*reset just in case it missed one*/

			/*validate the irp is valid, we may have got a request to unregister and we passed the semaphore_wait
			only because the unregister did that*/
			fbe_spinlock_lock(&registered_apps_table_lock);
			if (registered_applications_table[app_id].PIrp != NULL) {
				/*transfer the data of the event to the output buffer of the Irp we will soon complete*/
				app_buffer = (notification_data_t *)EmcpalExtIrpSystemBufferGet(registered_applications_table[app_id].PIrp);
				app_buffer->object_id = event_notification_data->object_id;
				fbe_copy_memory (&app_buffer->notification_info, &event_notification_data->notification_info, sizeof (fbe_notification_info_t));
				app_buffer->context = event_notification_data->context;
                
				/*before we get out of the lock we make a copy of our Irp because we can't complete under lock*/
				registered_irp = registered_applications_table[app_id].PIrp;
				registered_applications_table[app_id].PIrp = NULL;

				fbe_spinlock_unlock(&registered_apps_table_lock);

				if (CSX_NOT_NULL(EmcpalIrpCancelRoutineSet(registered_irp, NULL))){
					/*and complete the irp, this is supposed to be handled by the application which should generate an event with the data we send*/
					EmcpalExtIrpStatusFieldSet(registered_irp, EMCPAL_STATUS_SUCCESS);
					EmcpalExtIrpInformationFieldSet(registered_irp, sizeof(notification_data_t));
					EmcpalIrpCompleteRequest(registered_irp);
				}
				
			} else {
				fbe_spinlock_unlock(&registered_apps_table_lock);
			}

		} else {
            fbe_spinlock_unlock(&registered_apps_table_lock);
		}
	}

}



/*********************************************************************
 *            fbe_driver_notification_get_next_event ()
 *********************************************************************
 *
 *  Description: user space application Irp that would be completed with
 *				 some data once we have an event, this Irp would sit in the table
 *
 *  Input: Irp from user space   
 *
 *  Return Value:  NT status
 *
 *  History:
 *    10/17/07: sharel	created
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_driver_notification_get_next_event(PEMCPAL_IRP PIrp)
{
    fbe_u8_t * 						ioBuffer = NULL;
	notification_data_t   * 		notification_data = NULL;
    
    ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
	notification_data = (notification_data_t *)ioBuffer;

	if (notification_data->application_id < 0 || notification_data->application_id >= MAX_REG_APP) {
		KvTrace ("%s:application id (%d) is out of range\n", __FUNCTION__, notification_data->application_id);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_UNSUCCESSFUL;

	}
	/*now we have to look for the application id in the queue and place this Irp there for future completion*/
	fbe_spinlock_lock(&registered_apps_table_lock);

	if (registered_applications_table[notification_data->application_id].registered != APP_REGISTER_STATUS_REGISTERED) {
		fbe_spinlock_unlock(&registered_apps_table_lock);
		KvTrace ("%s:can't match application id (%d)to existing table or unregistering. Status:%d\n", 
			 __FUNCTION__, notification_data->application_id,
			 registered_applications_table[notification_data->application_id].registered);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_UNSUCCESSFUL);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_UNSUCCESSFUL;
	}
    
    
	/*once we found the registered application id, we give it the pointer to this IRP*/
	registered_applications_table[notification_data->application_id].PIrp = PIrp;

	/*set up the cancle routine*/
	EmcpalIrpCancelRoutineSet (PIrp, remove_application_from_table);

	/*make sure the cancle was not already called*/
	if (EmcpalIrpIsCancelInProgress(PIrp) && EmcpalIrpCancelRoutineSet (PIrp, NULL)) {
		KvTrace ("%s:cancel called for app id %d\n", __FUNCTION__, notification_data->application_id);
		registered_applications_table[notification_data->application_id].PIrp = NULL;
		registered_applications_table[notification_data->application_id].registered = APP_REGISTER_STATUS_NOT_REGISTERED;
		fbe_spinlock_unlock(&registered_apps_table_lock);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_CANCELLED);
		EmcpalExtIrpInformationFieldSet(PIrp, 0);
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_CANCELLED;
		
	}

    /*and let the application wait*/
	EmcpalIrpMarkPending (PIrp);

	/*we signal to the sender that caused this packet to get here that the packet arrived*/
	fbe_semaphore_release(&registered_applications_table[notification_data->application_id].packet_arrived_semaphore, 0, 1, FALSE);

	fbe_spinlock_unlock(&registered_apps_table_lock);

    return EMCPAL_STATUS_PENDING;
}

/*********************************************************************
 *            fbe_driver_notification_register_application ()
 *********************************************************************
 *
 *  Description: user space application asks us to stop getting events
 *
 *  Input: Irp from user space   
 *
 *  Return Value:  NT status
 *
 *  History:
 *    10/17/07: sharel	created
 *    
 *********************************************************************/
EMCPAL_STATUS fbe_driver_notification_unregister_application (PEMCPAL_IRP PIrp)
{
	PEMCPAL_IO_STACK_LOCATION  							irpStack;
    fbe_u8_t * 										ioBuffer = NULL;
    fbe_package_register_app_notify_t *				fbe_package_register_app_notify = NULL;
	ULONG 											outLength = 0;
	PEMCPAL_IRP											registered_irp = NULL;
    fbe_package_id_t								package_id;

	fbe_get_package_id(&package_id);
	
    irpStack = EmcpalIrpGetCurrentStackLocation(PIrp);
	ioBuffer  = EmcpalExtIrpSystemBufferGet(PIrp);
	outLength = EmcpalExtIrpStackParamIoctlOutputSizeGet(irpStack);

	/*some sanity checking*/
	if (outLength != sizeof(fbe_package_register_app_notify_t)) {
		/*and complete the packet*/
		KvTrace("%s: size mismatch o:%d\n", __FUNCTION__, outLength);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
		EmcpalExtIrpInformationFieldSet(PIrp, sizeof(fbe_package_register_app_notify_t));
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_SUCCESS;
	}

	fbe_package_register_app_notify = (fbe_package_register_app_notify_t *)ioBuffer;

	if (fbe_package_register_app_notify == NULL) {
		/*and complete the packet*/
		KvTrace("%s: NUL buffer\n", __FUNCTION__);
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
		EmcpalExtIrpInformationFieldSet(PIrp, sizeof(fbe_package_register_app_notify_t));
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_SUCCESS;

	}

    /*we were supposed to get here the application id of the application who wishes to unregister
	so we just go over the queue and release this application from the queue*/
    
	fbe_spinlock_lock(&registered_apps_table_lock);
	
	if (registered_applications_table[fbe_package_register_app_notify->application_id].registered != APP_REGISTER_STATUS_REGISTERED) {
		fbe_spinlock_unlock(&registered_apps_table_lock);
		KvTrace("%s: illegal application id sent %d , can't find it in table. Register Status:%d\n", 
			__FUNCTION__, fbe_package_register_app_notify->application_id,
			registered_applications_table[fbe_package_register_app_notify->application_id].registered);
		/*and complete the packet*/
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
		EmcpalExtIrpInformationFieldSet(PIrp, sizeof(fbe_package_register_app_notify_t));
		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_SUCCESS;
	}

    /*save Irp for later so we can complete outside of the lock*/
	registered_irp = registered_applications_table[fbe_package_register_app_notify->application_id].PIrp;
	/*and reset the entry*/
    registered_applications_table[fbe_package_register_app_notify->application_id].PIrp = NULL;
	registered_applications_table[fbe_package_register_app_notify->application_id].registered = APP_REGISTER_STATUS_UN_REGISTER_IN_PROGRESS;
	/*we don't need it anymore*/
    while (registered_applications_table[fbe_package_register_app_notify->application_id].waiting) {
       fbe_spinlock_unlock(&registered_apps_table_lock);
       csx_p_thr_yield();
       fbe_spinlock_lock(&registered_apps_table_lock);
    }
	registered_applications_table[fbe_package_register_app_notify->application_id].registered = APP_REGISTER_STATUS_NOT_REGISTERED;
	registered_applications--;
	fbe_semaphore_destroy (&registered_applications_table[fbe_package_register_app_notify->application_id].packet_arrived_semaphore);
    fbe_spinlock_unlock(&registered_apps_table_lock);

	/*check if there was even an IRP there*/
    KvTrace("%s:Pkg:%d, appid:%d\n", __FUNCTION__, package_id, fbe_package_register_app_notify->application_id);
	KvTrace("removing app:%s\n", registered_applications_table[fbe_package_register_app_notify->application_id].name);

	if (registered_irp == NULL) {
		/*and complete the ioctl that asked as to do the undergister*/
		EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
		EmcpalExtIrpInformationFieldSet(PIrp, sizeof(fbe_package_register_app_notify_t));

		EmcpalIrpCompleteRequest(PIrp);
		return EMCPAL_STATUS_SUCCESS;
	}

	/* Remove the cancel routine */
	EmcpalIrpCancelRoutineSet(registered_irp, NULL);

	/*we complete the ioctl that was sent to us to get events with. the thread that waits for it should check the status and
	once it sees the status is cancelled, it would know to stop the thread and go out*/
	EmcpalExtIrpStatusFieldSet(registered_irp, EMCPAL_STATUS_CANCELLED);
	EmcpalExtIrpInformationFieldSet(registered_irp, 0);
	EmcpalIrpCompleteRequest(registered_irp);

	/*and complete the ioctl that asked as to do the undergister*/
	EmcpalExtIrpStatusFieldSet(PIrp, EMCPAL_STATUS_SUCCESS);
	EmcpalExtIrpInformationFieldSet(PIrp, sizeof(fbe_package_register_app_notify_t));

	EmcpalIrpCompleteRequest(PIrp);

	/*at this point, the application_id counter for this application is wasted, but we have gazzilinons of them available.
	it is an overkill to manage these numbers*/

	return EMCPAL_STATUS_SUCCESS;
}

/*********************************************************************
 *            get_next_available_registration_table_entry ()
 *********************************************************************
 *
 *  Description: get the next free row in the table and mark it as being used
 *
 *  Input: pointer to get result in
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    10/17/07: sharel	created
 *    
 *********************************************************************/
static fbe_status_t get_next_available_registration_table_entry (application_id_t * application_id)
{	
	fbe_u32_t			idx = 0;

	fbe_spinlock_lock(&registered_apps_table_lock);
	for (idx = 0; idx < MAX_REG_APP; idx ++) {
		if (registered_applications_table[idx].registered == APP_REGISTER_STATUS_NOT_REGISTERED) {
	                registered_applications_table[idx].registered = APP_REGISTER_STATUS_IN_USE;
			fbe_spinlock_unlock(&registered_apps_table_lock);
			*application_id = idx;
			return FBE_STATUS_OK;
		}
	}

	fbe_spinlock_unlock(&registered_apps_table_lock);

	/*all slots are full*/
	KvTrace("%s: all application table slots are full, can't register new application\n", __FUNCTION__);
	return FBE_STATUS_GENERIC_FAILURE;

}

/*********************************************************************
 *            initialize_registration_table ()
 *********************************************************************
 *
 *  Description: duuuhhhhhh
 *
 *  Input: none
 *
 *  Return Value:  none
 *
 *  History:
 *    10/17/07: sharel	created
 *    
 *********************************************************************/
static void initialize_registration_table()
{
	fbe_u32_t			idx = 0;

	for (idx = 0; idx < MAX_REG_APP; idx ++) {
		registered_applications_table[idx].registered = APP_REGISTER_STATUS_NOT_REGISTERED;
		registered_applications_table[idx].PIrp = NULL;
		
	}
}

/*********************************************************************
 *            remove_application_from_table ()
 *********************************************************************
 *
 *  Description: if an application goes down w/o propper unregistration,
 *               Windows will call this function which  we registered for in the
 *				 fbe_driver_notification_get_next_event function.
 *
 *  Input: pointers to the device and the Irp to complete
 *
 *  Return Value:  none
 *
 *  History:
 *    10/19/07: sharel	created
 *    
 *********************************************************************/
static void remove_application_from_table(PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp)
{
	fbe_u32_t			app_id = 0;
	fbe_bool_t			removed = FBE_FALSE;
	fbe_package_id_t								package_id;

	EmcpalIrpCancelLockRelease (EmcpalExtIrpCancelIrqlGet(Irp));

	fbe_get_package_id(&package_id);

	KvTrace("%s:Pkg %d, Entry\n", __FUNCTION__, package_id);

    /*look for the correct one to remove*/
	for (app_id = 0; app_id < MAX_REG_APP; app_id ++) {
		if (Irp == registered_applications_table[app_id].PIrp) {
			KvTrace("%s:Pkg %d removing entry %d from registration table irp 0x%X\n", __FUNCTION__, package_id, app_id, (unsigned int)registered_applications_table[app_id].PIrp);
			KvTrace("removing app %s\n", registered_applications_table[app_id].name);
            fbe_spinlock_lock(&registered_apps_table_lock);

			/*these needs to be done here before we let down the lock for the first time.
			This will prevent the other thread from sneaking in while we do csx_p_thr_yield() and try and use the IRP since as soon as it will
			release the lock, we'll touhc the completion function and it will assert on it*/

			registered_applications_table[app_id].registered = APP_REGISTER_STATUS_UN_REGISTER_IN_PROGRESS;
			registered_applications_table[app_id].PIrp = NULL;
            (void) EmcpalIrpCancelRoutineSet (Irp, NULL);

            while (registered_applications_table[app_id].waiting) {
				fbe_spinlock_unlock(&registered_apps_table_lock);
				KvTrace("%s:Pkg %d removing entry %d, waiting for completion...\n", __FUNCTION__,package_id, app_id);
				csx_p_thr_yield();
				fbe_spinlock_lock(&registered_apps_table_lock);
			}
            registered_applications_table[app_id].registered = APP_REGISTER_STATUS_NOT_REGISTERED;
            registered_applications--;
            fbe_semaphore_release(&registered_applications_table[app_id].packet_arrived_semaphore, 0, 1, FALSE);
            fbe_semaphore_destroy(&registered_applications_table[app_id].packet_arrived_semaphore);
            fbe_spinlock_unlock(&registered_apps_table_lock);
			removed = FBE_TRUE;
			break;
		}
	}
    
	if (FBE_IS_FALSE(removed)) {
		KvTrace("%s: failed to removed from registration table irp = %p\n", __FUNCTION__, Irp);
	}
	
	EmcpalExtIrpStatusFieldSet(Irp, EMCPAL_STATUS_CANCELLED);
	EmcpalExtIrpInformationFieldSet(Irp, 0);
	EmcpalIrpCompleteRequest(Irp);
	return ;

}

/*********************************************************************
 *            get_notification_mem(void)
 *********************************************************************
 *
 *  Description: Returns address of the allocated memory
 *
 *  Input: None
 *
 *  Return Value:  Pointer to the memory location
 *
 *  History:
 *    04/29/13: Created. Preethi Poddatur
 *    
 *********************************************************************/
static notification_data_t * get_notification_mem(void)
{
   notification_mem_packet_q_element_t *           mem_packet_q_element;
   notification_data_t * notif_mem;
   
   fbe_spinlock_lock(&notification_mem_spinlock);

   /* if pre-allocated memory is NOT occupied, get memory from here */
   if (!fbe_queue_is_empty(&pre_allocated_queue_head)) {
        mem_packet_q_element = (notification_mem_packet_q_element_t *)fbe_queue_pop(&pre_allocated_queue_head);
        notif_mem = &(mem_packet_q_element->mem);
        mem_packet_q_element->dynamically_allocated = FBE_FALSE;
    }
   /* if pre-allocated memory is occupied, allocate memory*/
   else {
      mem_packet_q_element = (notification_mem_packet_q_element_t *) fbe_allocate_nonpaged_pool_with_tag(sizeof(notification_mem_packet_q_element_t),'memd' );
      if (mem_packet_q_element == NULL) {
         KvTrace("%s: can't allocate contig memory \n", __FUNCTION__); 
         return NULL;
      }
      mem_packet_q_element->dynamically_allocated = FBE_TRUE;
      notif_mem = &(mem_packet_q_element->mem);
   }

   /* push 'notification_mem_packet_q_element_t' for memory that is being used, into the outstanding queue. */
   fbe_queue_push(&outstanding_queue_head, &mem_packet_q_element->q_element);/*move to outstanding queue*/

   fbe_spinlock_unlock(&notification_mem_spinlock);

   return notif_mem;
}

/*********************************************************************
 *            release_notification_mem(notification_data_t *)
 *********************************************************************
 *
 *  Description: Releases memory allocated for notifications
 *
 *  Input: Pointer to the memory location
 *
 *  Return Value:  None
 *
 *  History:
 *    04/29/13: Created. Preethi Poddatur
 *    
 *********************************************************************/
static void release_notification_mem(notification_data_t * notif_mem)
{
   notification_mem_packet_q_element_t * mem_packet_q_element = (notification_mem_packet_q_element_t *)((fbe_u8_t *)((fbe_u8_t *)notif_mem - (sizeof(notification_mem_packet_q_element_t)-sizeof(notification_data_t))));

   fbe_spinlock_lock(&notification_mem_spinlock);

   /* if outstanding queue is NOT empty: this should always be true */
   if (!fbe_queue_is_empty(&outstanding_queue_head)) {

      /*remove from outstanding queue*/
      fbe_queue_remove(&(mem_packet_q_element->q_element));

      /* if memory was pre-allocated, release/push it into the pre-allocated queue */
      if(!mem_packet_q_element->dynamically_allocated){
         mem_packet_q_element->dynamically_allocated = FBE_FALSE;
         fbe_queue_push_front(&pre_allocated_queue_head, &mem_packet_q_element->q_element);
      }
      /* if memory was dynamically allocated, free it*/
      else {
         fbe_release_nonpaged_pool_with_tag(mem_packet_q_element, 'memd');
      }
   }
   else {
      KvTrace("%s: Outstanding queue is empty. Notification to release does not exist! \n", __FUNCTION__); 
   }

   fbe_spinlock_unlock(&notification_mem_spinlock);

}



















 
