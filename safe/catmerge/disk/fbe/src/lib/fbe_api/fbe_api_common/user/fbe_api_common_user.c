/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_common_user.c
 ***************************************************************************
 *
 * @brief
 *   User/Kernel split implementation for common API interface.
 *      
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_common_user_interface
 *
 * @version
 *      10/21/08    sharel - Created
 *    
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_payload_control_operation.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_sep.h"
#include "fbe_packet_serialize_lib.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_neit_package.h"
#include "fbe_api_packet_queue.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_kms.h"

/**************************************
                Local variables
**************************************/

/*!********************************************************************* 
 * @def IOCTL_COMPLETION_IDLE_TIMER
 *  
 * @brief 
 *   This represents ioctl completion thread delay in msec
 *
 * @ingroup fbe_api_common_sim_interface
 **********************************************************************/
#define IOCTL_COMPLETION_IDLE_TIMER 500 /* ioctl completion thread delay in msec.*/

/*!********************************************************************* 
 * @def NOTIFICATION_IDLE_TIMER
 *  
 * @brief 
 *   This represents notification thread delay in msec
 *
 * @ingroup fbe_api_common_sim_interface
 **********************************************************************/
#define NOTIFICATION_IDLE_TIMER 500 /* default notification thread delay in msec.*/
#define QUICK_NOTIFICATION_IDLE_TIMER 1 /*switch to this value as notifications start to come in*/

/*!********************************************************************* 
 * @enum thread_flag_t 
 *  
 * @brief 
 *   This contains the enum data info for Thread flag.
 *
 * @ingroup fbe_api_common_user_interface
 * @ingroup thread_flag
 **********************************************************************/
typedef enum thread_flag_e{
    THREAD_RUN,  /*!< Run Thread */
    THREAD_STOP, /*!< Stop Thread */
    THREAD_DONE  /*!< Done Thread */
}thread_flag_t;

/*!********************************************************************* 
 * @struct fbe_api_user_package_connection_info_t 
 *  
 * @brief 
 *   This contains the data info for the package connection info.
 *
 * @ingroup fbe_api_common_user_interface
 * @ingroup fbe_api_user_package_connection_info
 **********************************************************************/
typedef struct fbe_api_user_package_connection_info_s{
    HANDLE          package_handle;               /*!< Package Handle */
    fbe_u8_t *      driver_link_name;             /*!< Pointer to the Driver Link Name */
    fbe_u32_t       get_event_ioctl;              /*!< Get Event IOCTL */
    fbe_u32_t       register_for_event_ioctl;     /*!< Register Event IOCTL */
    fbe_u32_t       unregister_from_event_ioctl;  /*!< Unregister Event IOCTL */
    fbe_u32_t       control_ioctl;                /*!< Control IOCTL */
    fbe_u32_t       io_ioctl;                     /*!< IO IOCTL */
}fbe_api_user_package_connection_info_t;

/*this is where we set up the package names to start with, and as we go we would also add the handles*/
/* NOTE: This table should be synced with fbe_package_id_t definition. 
         Please add a new entry for each new item added to fbe_package_id_t.
 */
static fbe_api_user_package_connection_info_t   packages_handles[FBE_PACKAGE_ID_LAST + 1] = 
{
    /* FBE_PACKAGE_ID_INVALID */
    {NULL, NULL, NULL, NULL, NULL, NULL},

    /* FBE_PACKAGE_ID_PHYSICAL */
    {NULL,
     "\\\\.\\PhysicalPackageLink",
     FBE_PHYSICAL_PACKAGE_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_PHYSICAL_PACKAGE_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_PHYSICAL_PACKAGE_UNREGISTER_APP_NOTIFICATION_IOCTL,
                        FBE_PHYSICAL_PACKAGE_MGMT_PACKET_IOCTL, 
     FBE_PHYSICAL_PACKAGE_IO_PACKET_IOCTL},

    /* FBE_PACKAGE_ID_NEIT */
    {NULL,
     "\\\\.\\NeitPackageLink",
     FBE_NEIT_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_NEIT_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_NEIT_UNREGISTER_APP_NOTIFICATION_IOCTL,
     FBE_NEIT_MGMT_PACKET_IOCTL,
     NULL},

    /* FBE_PACKAGE_ID_SEP_0 */
    {NULL,
     "\\\\.\\sepLink",
     FBE_SEP_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_SEP_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_SEP_UNREGISTER_APP_NOTIFICATION_IOCTL,
     FBE_SEP_MGMT_PACKET_IOCTL,
     NULL},

     /* FBE_PACKAGE_ID_ESP */
    {NULL,
     "\\\\.\\espLink",
     FBE_ESP_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_ESP_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_ESP_UNREGISTER_APP_NOTIFICATION_IOCTL,
     FBE_ESP_MGMT_PACKET_IOCTL,
     NULL},

     /* FBE_PACKAGE_ID_KMS */
    {NULL,
     "\\\\.\\kmsLink",
     FBE_KMS_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_KMS_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_KMS_UNREGISTER_APP_NOTIFICATION_IOCTL,
     FBE_KMS_MGMT_PACKET_IOCTL,
     NULL},

   /* FBE_PACKAGE_ID_LAST */
    {NULL,NULL, NULL, NULL, NULL, NULL},

};
    
static fbe_atomic_t                         fbe_api_common_user_init_reference_count = 0;
static fbe_thread_t                         ioctl_completion_thread;
static thread_flag_t                        ioctl_completion_thread_flag;
static fbe_semaphore_t                      ioctl_completion_thread_semaphore;
static fbe_spinlock_t                       ioctl_completion_queue_lock;
static fbe_queue_head_t                     ioctl_completion_queue_head;
static fbe_spinlock_t                       event_notifications_queue_lock;
static fbe_queue_head_t                     event_notifications_queue_head;
static fbe_thread_t                         event_notification_thread;
static volatile thread_flag_t               event_notification_thread_flag;
static fbe_semaphore_t                      event_notification_thread_semaphore;
static application_id_t                     event_registration_id[FBE_PACKAGE_ID_LAST + 1] = {0xDEADBEEF};

/*we handle one event at a time per package, so we need only one of these two*/
static notification_data_t                  notification_data[FBE_PACKAGE_ID_LAST + 1];

static OVERLAPPED                           event_overlapped_info[FBE_PACKAGE_ID_LAST + 1];
static void *								cmm_chunk_addr[FBE_API_PRE_ALLOCATED_PACKETS_CHUNKS];
static fbe_bool_t							notification_init = FBE_TRUE;

static fbe_bool_t							fbe_api_trace_inited = FBE_TRUE;

/*!********************************************************************* 
 * @struct ioctl_completion_context_t 
 *  
 * @brief 
 *   This contains the data info for ioctl completion context.
 *
 * @ingroup fbe_api_common_user_interface
 * @ingroup ioctl_completion_context
 **********************************************************************/
typedef struct ioctl_completion_context_s {
    fbe_queue_element_t                     queue_element;          /*!< Queue Element */
    fbe_serialized_control_transaction_t *  serialized_transaction; /*!< Serialized Control Transaction */
    fbe_packet_t *                          packet;                 /*!< Packet */
    OVERLAPPED *                            overlapped_info;        /*!< Overlapped Info */
}ioctl_completion_context_t;


/*******************************************
                Local functions
********************************************/
static void ioctl_completion_dispatch_queue(void);
static void ioctl_complete_thread_func(void * context);
static void io_complete_thread_func(void * context);
static fbe_status_t send_packet_to_driver (fbe_packet_t * packet);
static fbe_status_t register_notification_callback(fbe_packet_t *packet);
static fbe_status_t unregister_notification_callback(fbe_packet_t *packet);
static fbe_status_t register_event_notification (fbe_package_id_t package_id);
static fbe_status_t unregister_event_notification (fbe_package_id_t package_id);
static void event_notification_handler_dispatch_queue (fbe_package_id_t package_id);
static fbe_status_t get_next_event (fbe_package_id_t package_id);
static void event_notification_thread_func(void * context);
static fbe_status_t send_io_to_driver (fbe_packet_t * io_packet);
static fbe_status_t copy_io_packet_data(fbe_packet_t *new_packet, fbe_packet_t *io_packet);
static void sync_returned_and_user_packets (fbe_packet_t *user_packet, fbe_u8_t * serialized_packet);
static void get_sg_area_size(fbe_packet_t *packet, fbe_u32_t *sg_area_size);
static void copy_sg_data(fbe_sg_element_t * sg_element, fbe_u32_t sg_count, fbe_u8_t * sg_area);
static void copy_back_sg_data (fbe_packet_t *packet, fbe_u8_t * sg_area);
static fbe_status_t fbe_api_common_send_control_packet_to_kernel_driver (fbe_packet_t *packet);
static fbe_status_t fbe_api_common_send_io_packet_to_kernel_driver (fbe_packet_t *packet);
static fbe_status_t fbe_api_user_init_package_connection(void);
static fbe_status_t fbe_api_user_get_control_operation(fbe_packet_t * packet, fbe_payload_control_operation_t **control_operation);
static fbe_status_t fbe_api_common_destroy_contiguous_packet_queue_user(void);
static fbe_status_t fbe_api_common_init_contiguous_packet_queue_user(void);
/*!***************************************************************
 * @fn fbe_api_common_init_user()
 *****************************************************************
 * @brief
 *   simulation version of init
 * @param init_notifications - FBE_TRUE is user wishes to be able to get notifications from kernel. Choose FBE_FALSE if you don't have to.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_common_init_user (fbe_bool_t init_notifications)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    EMCPAL_STATUS                       nt_status;
    fbe_api_libs_function_entries_t     lib_entries;
    fbe_package_id_t                    package_id = FBE_PACKAGE_ID_INVALID;

    //
    //  Now actually destroy the fbe_ktrace infrastructure. This is needed to 
    //  shut down ktrace threads that are started to handle the actual logging.
    //  And we only want to do this if the tracing was not init'ed already before we
    //  get here.  FBE clis, like fbecli.exe, initialize the tracing sooner than this
    //  function.
    //
    if (fbe_api_common_user_init_reference_count == 0)
    {
        if ( fbe_ktrace_is_initialized() == 0 )
        {
            fbe_trace_init();
            fbe_api_trace_inited = TRUE;
        }
    }

    fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: USER MODE starting initializaton!\n", __FUNCTION__); 

    if (fbe_api_common_user_init_reference_count > 0 ) {
        fbe_atomic_increment(&fbe_api_common_user_init_reference_count);
        return FBE_STATUS_OK; /* We already did init */
    }

    fbe_api_common_user_init_reference_count++;

    /*initialize the queue head and the lock that would be used for ioctl completion managament*/
    fbe_queue_init(&ioctl_completion_queue_head);
    fbe_spinlock_init(&ioctl_completion_queue_lock);
    fbe_semaphore_init(&ioctl_completion_thread_semaphore, 0, 1);
    ioctl_completion_thread_flag = THREAD_RUN;
    notification_init = init_notifications;/*if the user chose FBE_FALSE we'll skip the notification registration which is costly */

    /*completion functinos queue for events notification*/
    fbe_queue_init (&event_notifications_queue_head);
    fbe_spinlock_init(&event_notifications_queue_lock);

    status  = fbe_api_common_init_contiguous_packet_queue_user();
    if (status != FBE_STATUS_OK) {
        return status;
    }
        
    /*start the thread that will check ioctl completion*/
    nt_status = fbe_thread_init(&ioctl_completion_thread, "fbe_ioctl_comp", ioctl_complete_thread_func, NULL);

    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't start ioctl completion thread %X\n",  __FUNCTION__, nt_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*initialize the connection to all the packages we might have*/
    status  = fbe_api_user_init_package_connection();
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed to initialize packages connections\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }


	/*set up the fbe api to forward data to us, but we fill only the packages we will talk to*/
    for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
        if (packages_handles[package_id].package_handle != NULL) {
            lib_entries.lib_control_entry = fbe_api_common_send_control_packet_to_kernel_driver;
            lib_entries.lib_io_entry = fbe_api_common_send_io_packet_to_kernel_driver;
            
            status = fbe_api_common_set_function_entries(&lib_entries, package_id);
            if (status != FBE_STATUS_OK) {
                return status;
            }
        }
    }

	if (FBE_IS_TRUE(notification_init)) {
		/*we register ourselves to get notifications packets from the packages*/
		for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
			if ((packages_handles[package_id].package_handle != NULL) && (packages_handles[package_id].register_for_event_ioctl != NULL)) {
				status = register_event_notification (package_id);
				if (status != FBE_STATUS_OK) {
					fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed to register with package %d for notifications\n", __FUNCTION__, package_id); 
					CloseHandle(packages_handles[package_id].package_handle);
					packages_handles[package_id].package_handle = NULL;
				}
			}
		}
	

    
		/*start the thread that will get notifications ioctls and handle them*/
        fbe_semaphore_init(&event_notification_thread_semaphore, 0, 1);
		event_notification_thread_flag = THREAD_RUN;
		nt_status = fbe_thread_init(&event_notification_thread, "fbe_evt_notif", event_notification_thread_func, NULL);
	
		if (nt_status != EMCPAL_STATUS_SUCCESS) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't start notification queue thread, error code: %X\n",__FUNCTION__,  nt_status);
			return FBE_STATUS_GENERIC_FAILURE;
		}


		status  = fbe_api_notification_interface_init();/*this would initialize the notification interface in the upper level of the fbe api*/
		if (status != FBE_STATUS_OK) {
			fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init notification interface\n", __FUNCTION__); 
		}
	
		status = fbe_api_common_init_job_notification();
	
	}

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s: (USER MODE)init done\n", __FUNCTION__); 

    return status; 
}

/*!***************************************************************
 * @fn fbe_api_common_destroy_user()
 *****************************************************************
 * @brief
 *   simulation version of destroy
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_common_destroy_user (void)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           count = 0;
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_LAST;

    if(fbe_api_common_user_init_reference_count > 1) {
        fbe_atomic_decrement(&fbe_api_common_user_init_reference_count);
        return FBE_STATUS_OK;
    }

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:entry\n", __FUNCTION__); 

    if (FBE_IS_TRUE(notification_init)) {
        fbe_api_common_destroy_job_notification();
        fbe_api_notification_interface_destroy();
      
        /*unregister ourselves from the packages
        This request would, in turn, stop the event_notification_thread_func function
        because once we unregister, the package should cancel the ioctl we are waiting
        for in this thread. In any case we check for this thread termination later down here*/
        for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
            if (packages_handles[package_id].package_handle != NULL && (packages_handles[package_id].register_for_event_ioctl != NULL)) {
                status  = unregister_event_notification (package_id);
                if (status != FBE_STATUS_OK) {
                    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: unregister_event_notification failed for package %d, continue with rest of destroy\n", __FUNCTION__, package_id); 
                }
            }
        }
    
        /*we wait for a while and just go out. The cancellation packet from each package should arrive to the thread when we unregister
        TODO: we can be fancy and add a check for each cancellation, to see if everyone cannceled and then just exit the event_notification_thread*/
        event_notification_thread_flag = THREAD_STOP;
        fbe_semaphore_wait_ms(&event_notification_thread_semaphore,
                              (NOTIFICATION_IDLE_TIMER*30));

        if (event_notification_thread_flag == THREAD_DONE) {
            fbe_thread_wait(&event_notification_thread);
        } else {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: event notification thread (%p) did not exit!\n",
                          __FUNCTION__, event_notification_thread.thread);
        }
        fbe_semaphore_destroy(&event_notification_thread_semaphore);
        fbe_thread_destroy(&event_notification_thread);
    }

    /*wait for threads to end*/
    ioctl_completion_thread_flag = THREAD_STOP;
    fbe_semaphore_wait_ms(&ioctl_completion_thread_semaphore,
                          (IOCTL_COMPLETION_IDLE_TIMER*30));

    if (THREAD_DONE == ioctl_completion_thread_flag) {
        fbe_thread_wait(&ioctl_completion_thread);
    } else {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s: ioctl completion thread (%p) did not exit!\n",
                      __FUNCTION__, ioctl_completion_thread.thread);
    }
    fbe_semaphore_destroy(&ioctl_completion_thread_semaphore);
    fbe_thread_destroy(&ioctl_completion_thread);

    fbe_spinlock_destroy(&ioctl_completion_queue_lock);
    fbe_queue_destroy (&ioctl_completion_queue_head);

    fbe_queue_destroy (&event_notifications_queue_head);
    fbe_spinlock_destroy(&event_notifications_queue_lock);
    
    for (count = 0; count < FBE_PACKAGE_ID_LAST; count ++) {
        if (packages_handles[count].package_handle != NULL){
            /*disconnect from the drivers*/
            CloseHandle(packages_handles[count].package_handle);
        }
    }

    fbe_api_common_destroy_contiguous_packet_queue_user();

    fbe_api_common_user_init_reference_count --;

    fbe_api_common_destroy_cluster_lock_user();

    fbe_trace_destroy();

//    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s: destroy done\n", __FUNCTION__); 

    //
    //  Now actually destroy the fbe_ktrace infrastructure. This is needed to shut 
    //  down ktrace threads that are started to handle the actual logging.  
    //  Again, only do this if this if the API is who init'ed the tracing.
    //
    if (fbe_api_common_user_init_reference_count == 0)
    {
        if ( fbe_api_trace_inited )
        {
            fbe_trace_destroy();
            fbe_api_trace_inited = FALSE;
        }
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_to_kernel_driver(
 *       fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   simulation version of sending packet
 *
 * @param packet - pointer to packet info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/10/08: sharel    created
 *
 ****************************************************************/
static fbe_status_t fbe_api_common_send_control_packet_to_kernel_driver (fbe_packet_t *packet)
{  

    fbe_payload_control_operation_opcode_t      control_code = 0;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_package_id_t                            package_id = FBE_PACKAGE_ID_INVALID;
    fbe_cpu_id_t                                cpu_id = FBE_CPU_ID_INVALID;


    if (fbe_api_common_user_init_reference_count == 0) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s is called but API is not initialized\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;/*we already did init*/
    }

    /*we also check if we have a handle to this driver*/
    fbe_transport_get_package_id(packet, &package_id);
    if (packages_handles[package_id].package_handle == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s no handle aquired for this package, can't send\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We do some tricks here to look for some special opcodes we don't want the user to send down
    for example, The event registration is done with us and not the real notification service */
    fbe_api_user_get_control_operation(packet, &control_operation);
    fbe_payload_control_get_opcode (control_operation, &control_code);
    
    /* Set the CPU ID if it is invalid */
    fbe_transport_get_cpu_id(packet, &cpu_id);
    if (cpu_id == FBE_CPU_ID_INVALID) 
    {
        cpu_id = fbe_get_cpu_id();
        fbe_transport_set_cpu_id(packet, cpu_id);
    }
       
    /*the code below is for taking care of some special cases if needed.
    when not needed, we just send the packet*/
    switch (control_code) {
    case FBE_NOTIFICATION_CONTROL_CODE_REGISTER:
        return register_notification_callback(packet);
        break;
    case FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER:
        return unregister_notification_callback(packet);
        break;
    default:
        return send_packet_to_driver (packet);
    }
}

/*!***************************************************************
 * @fn fbe_api_common_send_io_packet_to_kernel_driver(
 *       fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   simulation version of sending io packet
 *
 * @param packet - pointer to packet info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
static fbe_status_t fbe_api_common_send_io_packet_to_kernel_driver (fbe_packet_t *packet)
{  
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    if (fbe_api_common_user_init_reference_count == 0) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s is called but API is not initialized\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;/*we already did init*/
    }

    /*we also check if we have a handle to this driver*/
    fbe_transport_get_package_id(packet, &package_id);
    if (packages_handles[package_id].package_handle == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s no handle aquired for this package, can't send\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* H A C K - to prevent from using it, remove this when IO path enabled*/
    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s IO path not enabled in user mode !!!\n", __FUNCTION__);
    return FBE_STATUS_GENERIC_FAILURE;


    /*we send the packet to the driver via marshaling*/
    return send_io_to_driver (packet);

}


/*!***************************************************************
 * @fn send_packet_to_driver(
 *       fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   actual sending of the packet to the driver
 *
 * @param packet - pointer to packet info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/02/08: sharel    created
 *
 ****************************************************************/
static fbe_status_t 
send_packet_to_driver (fbe_packet_t * packet)
{
    
    DWORD                               BytesReturned = 0;
    fbe_u32_t                           serialized_transaction_size;
    OVERLAPPED *                        overlapped_info = (OVERLAPPED *)malloc (sizeof(OVERLAPPED));
    DWORD                               ioctl_return_code = 0;
    ioctl_completion_context_t *        ioctl_completion_context = NULL;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_serialized_control_transaction_t * serialized_transaction = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_package_id_t                    package_id = FBE_PACKAGE_ID_INVALID;
    fbe_payload_ex_t *							original_payload = NULL;

	fbe_packet_attr_t 					packet_attr;
	DWORD								wait_result;

	fbe_transport_get_packet_attr(packet, &packet_attr);

    /*we use the library to serizlie the packet*/
    status  = fbe_serialize_lib_serialize_packet(packet, &serialized_transaction, &serialized_transaction_size);
    if (status != FBE_STATUS_OK) {
        payload = fbe_transport_get_payload_ex(packet);
        fbe_payload_ex_increment_control_operation_level (payload);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet (packet);
        free (overlapped_info);
        free (serialized_transaction);
        return status;
    }

   /*set up the overlapped info for later*/
    overlapped_info->Offset = 0;
    overlapped_info->OffsetHigh = 0;
    overlapped_info->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    fbe_transport_get_package_id(packet, &package_id);

    /* Send the serialized packet to the package. */
    if (DeviceIoControl(packages_handles[package_id].package_handle, 
                        packages_handles[package_id].control_ioctl, 
                        serialized_transaction,      /* input pointer */ 
                        serialized_transaction_size, /* input size */
                        serialized_transaction,      /* output pointer */ 
                        serialized_transaction_size, /* output size */
                        &BytesReturned, 
                        overlapped_info)){

        /*if we got here by chance it means we completed right away*/
        CloseHandle (overlapped_info->hEvent);

        payload = fbe_transport_get_payload_ex(packet);
        fbe_payload_ex_increment_control_operation_level (payload);

		fbe_serialize_lib_deserialize_transaction(serialized_transaction, packet);

        fbe_transport_complete_packet (packet);
        free(serialized_transaction);
        free (overlapped_info);
        return FBE_STATUS_OK;

    } else {

        ioctl_return_code = GetLastError();

        if (ioctl_return_code == ERROR_IO_PENDING) {

			if (packet_attr & FBE_PACKET_FLAG_SYNC) {
				/*for most cases, the packet will be a synch type so we just wait here for completion and not complicate things*/
				wait_result = WaitForSingleObject (overlapped_info->hEvent, INFINITE);
    
				if (wait_result == WAIT_OBJECT_0) {
					/*good, we got it back*/
					CloseHandle (overlapped_info->hEvent);

					payload = fbe_transport_get_payload_ex(packet);
					fbe_payload_ex_increment_control_operation_level (payload);

					fbe_serialize_lib_deserialize_transaction(serialized_transaction, packet);

                    fbe_transport_complete_packet (packet);/*complete to user*/
            
					free(serialized_transaction);
					free (overlapped_info);
					return FBE_STATUS_OK;


				}else{

					/*this is a real error*/
					fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
								   "%s:Got an error: 0x%X from the package when trying to send packet\n",
									__FUNCTION__, ioctl_return_code); 
		
					CloseHandle (overlapped_info->hEvent);

					payload = fbe_transport_get_payload_ex(packet);
					fbe_payload_ex_increment_control_operation_level (payload);

                    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
					fbe_transport_complete_packet (packet);

					free(serialized_transaction);
					free (overlapped_info);
					return FBE_STATUS_GENERIC_FAILURE;

				}

			}else{
				/* this is the unusual case of an asynch IOCTL case, we need to pend this operation,
				 we put it on the queue and the thread will check on it once a while*/
				ioctl_completion_context = (ioctl_completion_context_t *) malloc (sizeof(ioctl_completion_context_t));
				ioctl_completion_context->packet = packet;
				ioctl_completion_context->overlapped_info = overlapped_info;
				ioctl_completion_context->serialized_transaction = serialized_transaction;
	
			if(packet->packet_attr & FBE_PACKET_FLAG_ASYNC){
				fbe_spinlock_lock (&ioctl_completion_queue_lock);
				fbe_queue_push(&ioctl_completion_queue_head, &ioctl_completion_context->queue_element);
				fbe_spinlock_unlock (&ioctl_completion_queue_lock);
			} else { /* It is syncronous call. We will wait for the completion here */

		        WaitForSingleObject (ioctl_completion_context->overlapped_info->hEvent, INFINITE);
                
				/*we can complete it*/
                CloseHandle (ioctl_completion_context->overlapped_info->hEvent);

                fbe_serialize_lib_deserialize_transaction(ioctl_completion_context->serialized_transaction, ioctl_completion_context->packet);

                /*and complete it for the original sender of the packet*/
                original_payload = fbe_transport_get_payload_ex (ioctl_completion_context->packet);
                fbe_payload_ex_increment_control_operation_level (original_payload);

                fbe_transport_complete_packet (ioctl_completion_context->packet);/*complete to user*/
    
                /*get rid of all the resources we consumed*/
                free (ioctl_completion_context->serialized_transaction);
                free (ioctl_completion_context->overlapped_info);
                free(ioctl_completion_context);
			}
	
				return FBE_STATUS_PENDING;
			}

        } else {
            /*this is a real error*/
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                           "%s:Got an error: 0x%X from the package when trying to send packet\n",
                            __FUNCTION__, ioctl_return_code); 

            CloseHandle (overlapped_info->hEvent);
			fbe_transport_complete_packet (packet);/*complete to user*/

            free(serialized_transaction);
            free (overlapped_info);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    }

}

/*!***************************************************************
 * @fn ioctl_complete_thread_func(
 *       void * context)
 *****************************************************************
 * @brief
 *   the thread that will dispatch completion from the queue
 *
 * @param context - pointer to the context data
 *
 * @return 
 *
 * @version
 *  10/05/07: sharel    created
 *
 ****************************************************************/
static void ioctl_complete_thread_func(void * context)
{
    FBE_UNREFERENCED_PARAMETER(context);
    
    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s entry\n", __FUNCTION__);

    while(1)    
    {
        if(ioctl_completion_thread_flag == THREAD_RUN) {
			fbe_thread_delay (IOCTL_COMPLETION_IDLE_TIMER);
                ioctl_completion_dispatch_queue();
            }else {
            break;
        }
    }

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s done\n", __FUNCTION__);

    ioctl_completion_thread_flag = THREAD_DONE;
    fbe_semaphore_release(&ioctl_completion_thread_semaphore, 0, 1, FALSE);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/*!***************************************************************
 * @fn ioctl_completion_dispatch_queue()
 *****************************************************************
 * @brief
 *   look for events that need to be handled
 *
 * @return 
 *
 * @version
 *  10/05/07: sharel    created
 *
 ****************************************************************/
static void ioctl_completion_dispatch_queue()
{
    fbe_queue_element_t * 					current_queue_element = NULL;
    fbe_queue_element_t * 					next_queue_element = NULL;

    ioctl_completion_context_t *			ioctl_completion_context = NULL;
    DWORD									wait_result;
    fbe_payload_ex_t *							original_payload = NULL;
	fbe_u32_t queue_length;
	fbe_u32_t i;

	fbe_spinlock_lock(&ioctl_completion_queue_lock);
	queue_length = fbe_queue_length(&ioctl_completion_queue_head);
	current_queue_element = fbe_queue_front(&ioctl_completion_queue_head);
    fbe_spinlock_unlock(&ioctl_completion_queue_lock);

    /*we will go over the queued ioctls and see if any one of them was completed (IoCompleteRequest called in package)*/
    for (i = 0; i < queue_length; i++){
		ioctl_completion_context = (ioctl_completion_context_t *)current_queue_element;            

        /*let's see if this event is signaled*/
            wait_result = WaitForSingleObject (ioctl_completion_context->overlapped_info->hEvent, 0);
            if (wait_result == WAIT_OBJECT_0) {
			fbe_spinlock_lock(&ioctl_completion_queue_lock);
			next_queue_element = fbe_queue_next(&ioctl_completion_queue_head, current_queue_element);
			fbe_queue_remove(current_queue_element);
			current_queue_element = next_queue_element;
			fbe_spinlock_unlock(&ioctl_completion_queue_lock);
			
            /*we can complete it*/
            CloseHandle (ioctl_completion_context->overlapped_info->hEvent);
                fbe_serialize_lib_deserialize_transaction(ioctl_completion_context->serialized_transaction, ioctl_completion_context->packet);

                /*and complete it for the original sender of the packet*/
                original_payload = fbe_transport_get_payload_ex (ioctl_completion_context->packet);
                fbe_payload_ex_increment_control_operation_level (original_payload);

                fbe_transport_complete_packet (ioctl_completion_context->packet);/*complete to user*/
    
                /*get rid of all the resources we consumed*/
                free (ioctl_completion_context->serialized_transaction);
                free (ioctl_completion_context->overlapped_info);
                free(ioctl_completion_context);
			continue;
        }

                fbe_spinlock_lock(&ioctl_completion_queue_lock);
		current_queue_element = fbe_queue_next(&ioctl_completion_queue_head, current_queue_element);
		fbe_spinlock_unlock(&ioctl_completion_queue_lock);
    }
}

/*!***************************************************************
 * @fn register_notification_callback(fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   register functions that needs to be called once we get 
 *   events notifications from the package driver
 *
 * @param packet - pointer to the packet data
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/15/07: sharel    created
 *
 ****************************************************************/
static fbe_status_t register_notification_callback(fbe_packet_t *packet)
{
    fbe_notification_element_t *		notification_element;
    fbe_u32_t							buffer_size = 0;
    fbe_payload_control_operation_t *	control_operation = NULL;

    
    /* We do some tricks here to look for some special opcodes we don't want the user to send down
    for example, The event registration is done with us and not the real notification service */
    fbe_api_user_get_control_operation(packet, &control_operation);
    fbe_payload_control_get_buffer_length (control_operation, &buffer_size);
  
    if (buffer_size != sizeof(fbe_notification_element_t)) {
        fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_increment_stack_level (packet);
        fbe_transport_complete_packet (packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer (control_operation, &notification_element);

    if (notification_element == NULL) {
        fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_increment_stack_level (packet);
        fbe_transport_complete_packet (packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	if (FBE_IS_FALSE(notification_init)) {
		fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_increment_stack_level (packet);
        fbe_transport_complete_packet (packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    /*put the notification element on the queue so we can call it once needed*/
    /*we also update the package part which we are intereseted in*/
    fbe_transport_get_package_id(packet, &notification_element->targe_package);
    
    fbe_spinlock_lock (&event_notifications_queue_lock);
    fbe_queue_push(&event_notifications_queue_head, &notification_element->queue_element);
    fbe_spinlock_unlock (&event_notifications_queue_lock);

    /*complete the packet*/
    fbe_transport_set_status (packet, FBE_STATUS_OK ,0);
    fbe_transport_increment_stack_level (packet);
    fbe_transport_complete_packet (packet);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_status_t register_event_notification(fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *    register ourselves with the package driver.
 *               once we are registered, we will get notifications by ioctls
 *
 * @param package_id - package ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/22/07: sharel    created
 *
 ****************************************************************/
static fbe_status_t register_event_notification(fbe_package_id_t package_id)
{
    DWORD                                           BytesReturned = 0;
    fbe_u32_t                                       buffer_size;
    DWORD                                           ioctl_return_code = 0;
    fbe_package_register_app_notify_t               register_app;
    fbe_status_t									status;
    OVERLAPPED 										overlapped_info;
    csx_size_t										copied_chars = 0;
	csx_status_e									csx_status;

	/*let's make sure we send down the name of whoever is registering*/
	fbe_zero_memory(&register_app, sizeof(fbe_package_register_app_notify_t));
	csx_status = csx_p_native_executable_path_get((void *)&register_app.registered_app_name, APP_NAME_LENGTH, &copied_chars);
	if (csx_status == CSX_STATUS_BUFFER_OVERFLOW) {
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*to be on the safe side, force last char to NULL*/
	register_app.registered_app_name[APP_NAME_LENGTH - 1] = 0;

    /*we must use the overlapped structure because we opened the device with this flag,
    however, since it's a synchoronous call, we can declare it on the stack here */
    overlapped_info.Offset = 0;
    overlapped_info.OffsetHigh = 0;
    overlapped_info.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    buffer_size = sizeof(fbe_package_register_app_notify_t);

    /*send to the driver, if the return value is true we are all set, if not we need to block for a while*/
    if (!DeviceIoControl(packages_handles[package_id].package_handle, 
                         packages_handles[package_id].register_for_event_ioctl, 
                         &register_app, /* input pointer */ 
                         buffer_size, /* input size */
                         &register_app, /* output pointer */ 
                         buffer_size, /* output size */
                         &BytesReturned, 
                         &overlapped_info)){

        ioctl_return_code = GetLastError();

        /*if by any chance it's pending we just wait for it to complete(technically this should not happen, but the driver might change)*/
        if (ioctl_return_code == ERROR_IO_PENDING) {
            GetOverlappedResult (packages_handles[package_id].package_handle, &overlapped_info, &BytesReturned, TRUE);
            ioctl_return_code = GetLastError();
        }

        if (ioctl_return_code != ERROR_SUCCESS) {
            CloseHandle(overlapped_info.hEvent);
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s:Got an error: 0x%X from package id:%d when trying to send IRP\n",
                           __FUNCTION__, ioctl_return_code, package_id); 

            return FBE_STATUS_GENERIC_FAILURE;
        }

    }


    CloseHandle(overlapped_info.hEvent);

    /*when we get back, we save the registration id we got for future use*/
    event_registration_id[package_id] = register_app.application_id;

    fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                  "%s: Registration Complete from package id:%d with appID:%d\n",
                  __FUNCTION__, package_id, register_app.application_id);

    /*after we registered, we send a packet to the package that will pend on
    a queue and be ready to return to us once there is an event*/

    status = get_next_event(package_id);

    if (status != FBE_STATUS_OK) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn event_notification_thread_func(void * context)
 *****************************************************************
 * @brief
 *    the thread that will dispatch the event propagation 
 *
 * @param context - pointer to the context data
 *
 * @return
 *
 * @version
 *  10/15/07: sharel    created
 *
 ****************************************************************/
static void event_notification_thread_func(void * context)
{
    DWORD 					BytesReturned = 0;
    BOOL                    event_present = FALSE;
    fbe_package_id_t        package_id = FBE_PACKAGE_ID_INVALID;
	fbe_u32_t				delay_time = NOTIFICATION_IDLE_TIMER;
	fbe_bool_t				at_least_one_event = FBE_FALSE;
	fbe_u32_t				active_folks;
	fbe_u32_t				pass_count = 0;
	fbe_u32_t				last_error = 0;

    FBE_UNREFERENCED_PARAMETER(context);
    
    fbe_api_trace(FBE_TRACE_LEVEL_INFO,"%s entry\n", __FUNCTION__);

    while(1)    
    {
        /*wait for a while*/
        fbe_thread_delay (delay_time);

		at_least_one_event = FBE_FALSE;
		active_folks = 0;

        for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {

            if (packages_handles[package_id].package_handle == NULL ||event_overlapped_info[package_id].hEvent == NULL) {
                continue;
            }

            event_present = FALSE;
            /*look if any of the packages returned some data(we will also get the cancelled ioctl when we unregister)*/
            event_present = GetOverlappedResult (packages_handles[package_id].package_handle, &event_overlapped_info[package_id], &BytesReturned, FALSE);
			last_error = GetLastError();
            if (last_error == ERROR_IO_INCOMPLETE && !event_present) {
				active_folks++;
                continue;/*no event on this package yet*/
            }

            if (last_error == ERROR_OPERATION_ABORTED && !event_present) {
                CloseHandle (event_overlapped_info[package_id].hEvent);
                event_overlapped_info[package_id].hEvent = NULL; 
                continue;
            }
			at_least_one_event = FBE_TRUE;

            if (event_present && BytesReturned == 0) {
				CloseHandle (event_overlapped_info[package_id].hEvent);/*we open it again later*/
				event_overlapped_info[package_id].hEvent = NULL; 
                fbe_api_trace(FBE_TRACE_LEVEL_INFO,"%s:got request from package to cancel the ioctl\n", __FUNCTION__); 
                /*no need to be here again. The code in the driver is not supposed to cancel us*/
				continue;

            }

			active_folks++;

            /*if we got here we have an event set for this package and we need to process it*/
            CloseHandle (event_overlapped_info[package_id].hEvent);/*we open it again later*/
            event_overlapped_info[package_id].hEvent = NULL; /* SAFEBUG - multiple closes */
        
			if(event_notification_thread_flag == THREAD_RUN) {
				/*check if there are any notifictions waiting on the queue*/
				event_notification_handler_dispatch_queue(package_id);
				/*afer an event, usually there are more, so we want to process them quickly and not wait NOTIFICATION_IDLE_TIMER
				which by default is a long time*/
				delay_time = QUICK_NOTIFICATION_IDLE_TIMER;
			} else {
				break;
			}
        }

		pass_count++;

		if (!at_least_one_event && delay_time == QUICK_NOTIFICATION_IDLE_TIMER && pass_count > 2000) {
			pass_count = 0;
			delay_time = NOTIFICATION_IDLE_TIMER;/*if no event was detected, just restore to default in case we cahnged it*/
		}

		if (event_notification_thread_flag == THREAD_STOP) {
			break;
		}
    }

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s done\n", __FUNCTION__);

    event_notification_thread_flag = THREAD_DONE;
    fbe_semaphore_release(&event_notification_thread_semaphore, 0, 1, FALSE);

    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/*!***************************************************************
 * @fn event_notification_handler_dispatch_queue(fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *    send notifications to all the modules that registered
 *				 with us using the FBE_NOTIFICATION_CONTROL_CODE_REGISTER ioctl
 *
 * @param package_id - package ID
 *
 * @return
 *
 * @version
 *  10/22/07: sharel    created
 *
 ****************************************************************/
static void event_notification_handler_dispatch_queue (fbe_package_id_t package_id)
{
    fbe_queue_element_t * 			queue_element = NULL;
    fbe_notification_element_t *	notification_element;
   
    /*we will go over all the completion functions and send them the information we got*/

    fbe_spinlock_lock(&event_notifications_queue_lock);
    queue_element = fbe_queue_front(&event_notifications_queue_head);

    fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s : Received Notification from PakgID %d, AppID %d \n",
                           __FUNCTION__, package_id, notification_data[package_id].application_id);
    
    while(queue_element != NULL){
        
        notification_element = (fbe_notification_element_t *)queue_element;

        /*we send the notifications only to entities that wanted updates from this pacakge.
        Since we keep a single queue, it might have registrations for more than one package*/
        if (notification_element->targe_package == notification_data[package_id].notification_info.source_package) {

			fbe_spinlock_unlock(&event_notifications_queue_lock);

            fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s: Invoking Notify callback. PakgID %d, AppID %d. Notify Type %llu \n",
                          __FUNCTION__, package_id, notification_data[package_id].application_id,
                          (unsigned long long)notification_data[package_id].notification_info.notification_type);

            /*use the last data we got when the notification service completed the call
            this data is static and not in a queuesince this operation is synchronized*/
            notification_element->notification_function(notification_data[package_id].object_id,
                                                        notification_data[package_id].notification_info,
														notification_element->notification_context);

			fbe_spinlock_lock(&event_notifications_queue_lock);
        }

        queue_element = fbe_queue_next(&event_notifications_queue_head, queue_element);       
    }

    fbe_spinlock_unlock(&event_notifications_queue_lock);

    /*now that we sent the notifications about this event to all the registered functions
    we can send another packet to the notification service so it can return it
    with more events if there are any*/
	notification_data[package_id].notification_info.source_package = FBE_PACKAGE_ID_INVALID;/*reset to next time if somehow we get garbage*/
    get_next_event (package_id);

}

/*!***************************************************************
 * @fn get_next_event(fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *    send an ioctl to the package. This ioctl would wait on a queue
 *    and once an event happen in the package, this ioctl
 *			     would be completed and contain the event information.
 *               the completion of this ioctl would trigger the GetOverlappedResult in the
 *			   	 event_notification_thread_func function
 *
 * @param package_id - package ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/23/07: sharel    created
 *
 ****************************************************************/
static fbe_status_t get_next_event (fbe_package_id_t package_id)
{
    DWORD 											BytesReturned = 0;
    fbe_u32_t 										buffer_size;
    DWORD											ioctl_return_code = 0;
    
    /*we must use the overlapped structure because we opened the device with this flag*/
    event_overlapped_info[package_id].Offset = 0;
    event_overlapped_info[package_id].OffsetHigh = 0;
    event_overlapped_info[package_id].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    buffer_size = sizeof(notification_data_t);

    /*clear the data, sine we used this memory before*/
    notification_data[package_id].application_id = event_registration_id[package_id];
    notification_data[package_id].context = NULL;/*use this in the future if needed*/
    
    /*send to the driver, this should be an asynchronous IOCTL*/
    if (DeviceIoControl(packages_handles[package_id].package_handle, 
                         packages_handles[package_id].get_event_ioctl, 
                         &notification_data[package_id], /* input pointer */ 
                         buffer_size, /* input size */
                         &notification_data[package_id], /* output pointer */ 
                         buffer_size, /* output size */
                         &BytesReturned, 
                         &event_overlapped_info[package_id])){

        /*if we got here it means we did not pend at all, this should not happen normally
         unless we had a very quick event right after we sent the ioctl*/
        event_notification_handler_dispatch_queue(package_id);
        return FBE_STATUS_OK;

    } else {

        ioctl_return_code = GetLastError();

        /*normally, we would pend, so we just go out, the thread will wait for it to be finished*/
        if (ioctl_return_code == ERROR_IO_PENDING) {
            return FBE_STATUS_OK;
        } else {
            CloseHandle(event_overlapped_info[package_id].hEvent);
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s:Got an error: 0x%X from package 0x%X when trying to send packet\n",
                           __FUNCTION__, ioctl_return_code, package_id); 

            return FBE_STATUS_GENERIC_FAILURE;
        }

    }
    
}

/*!***************************************************************
 * @fn unregister_notification_callback(fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *    take a module off our queue of module we need to notify 
 *    once we get an event from package
 *
 * @param packet - pointer to packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/23/07: sharel    created
 *
 ****************************************************************/
static fbe_status_t unregister_notification_callback(fbe_packet_t *packet)
{
    fbe_notification_element_t *		notification_element = NULL;
    fbe_u32_t							buffer_size = 0; 
    fbe_notification_element_t *		registered_notification_element = NULL;
    fbe_queue_element_t *				queue_element = NULL;
    fbe_queue_element_t *				next_element = NULL;
    fbe_bool_t							element_found = FALSE;
    fbe_payload_control_operation_t *	control_operation = NULL;

    
    /* We do some tricks here to look for some special opcodes we don't want the user to send down
    for example, The event registration is done with us and not the real notification service */
    fbe_api_user_get_control_operation(packet, &control_operation);
    fbe_payload_control_get_buffer_length (control_operation, &buffer_size);

    if (buffer_size != sizeof(fbe_notification_element_t)) {
        fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_increment_stack_level (packet);
        fbe_transport_complete_packet (packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer (control_operation, &notification_element);

    if (notification_element == NULL) {
        fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_increment_stack_level (packet);
        fbe_transport_complete_packet (packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	if (FBE_IS_FALSE(notification_init)) {
		fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_increment_stack_level (packet);
        fbe_transport_complete_packet (packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_spinlock_lock (&event_notifications_queue_lock);
    queue_element = fbe_queue_front(&event_notifications_queue_head);

    while((queue_element != NULL) && (queue_element != &event_notifications_queue_head)){
        registered_notification_element = (fbe_notification_element_t *)queue_element;
        /*look for the one we need to take out*/
        if (registered_notification_element->notification_function == notification_element->notification_function) {
            fbe_queue_remove (queue_element);
            element_found = TRUE;
            break;
        }

        next_element = queue_element->next;
        queue_element = next_element;

    }
    fbe_spinlock_unlock (&event_notifications_queue_lock);

    if (!element_found) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:got an unregister request from a module that is not even registered\n", __FUNCTION__); 
        fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_increment_stack_level (packet);
        fbe_transport_complete_packet (packet);
        return FBE_STATUS_GENERIC_FAILURE;

    }
    
    /*complete the packet*/
    fbe_transport_set_status (packet, FBE_STATUS_OK ,0);
    fbe_transport_increment_stack_level (packet);
    fbe_transport_complete_packet (packet);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn unregister_event_notification(fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *    tell the package we don't want to get notifications anymore
 *
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/23/07: sharel    created
 *
 ****************************************************************/
static fbe_status_t unregister_event_notification (fbe_package_id_t package_id)
{
    DWORD                                           BytesReturned = 0;
    fbe_u32_t                                       buffer_size;
    DWORD                                           ioctl_return_code = 0;
    fbe_package_register_app_notify_t               register_app;
    OVERLAPPED                                      overlapped_info;
	DWORD											wait_result;
    
    /*we must use the overlapped structure because we opened the device with this flag,
    however, since it's a synchoronous call, we can declare it on the stack here */
    overlapped_info.Offset = 0;
    overlapped_info.OffsetHigh = 0;
    overlapped_info.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    buffer_size = sizeof(fbe_package_register_app_notify_t);
    /*should be set from before but jsut to be on the safe  side*/
    register_app.application_id = event_registration_id[package_id];

    /*send to the driver, if the return value is true we are all set, if not we need to block for a while*/
    if (!DeviceIoControl(packages_handles[package_id].package_handle, 
                         packages_handles[package_id].unregister_from_event_ioctl, 
                         &register_app, /* input pointer */ 
                         buffer_size, /* input size */
                         &register_app, /* output pointer */ 
                         buffer_size, /* output size */
                         &BytesReturned, 
                         &overlapped_info)){

		/*for most cases, the packet will be a synch type so we just wait here for completion and not complicate things*/
		wait_result = WaitForSingleObject (overlapped_info.hEvent, INFINITE);
    
		if (wait_result != WAIT_OBJECT_0) {
           ioctl_return_code = GetLastError();
           fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s:Got an error: 0x%X from package %s when trying to send packet\n",
                           __FUNCTION__, ioctl_return_code, packages_handles[package_id].driver_link_name); 
        }

    }


    CloseHandle(overlapped_info.hEvent);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn send_io_to_driver(fbe_packet_t * io_packet)
 *****************************************************************
 * @brief
 *    the actual sending of the io to the driver
 *
 * @param io_packet - pointer to the IO packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  02/04/08: sharel    created
 *
 ****************************************************************/
static fbe_status_t send_io_to_driver (fbe_packet_t * io_packet)
{
    /*fbe_api does not support IO at this point.  This is just a place holder. */
    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s:fbe_api does not support IO!!!\n",
                   __FUNCTION__); 
    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 *            sync_returned_and_user_packets ()
 *********************************************************************
 *
 *  Description:
 *    copy back data from kernel to user mode
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    2/14/08: sharel	created
 *    
 *********************************************************************/
static void sync_returned_and_user_packets (fbe_packet_t *user_packet, fbe_u8_t * serialized_packet)
{
    /* fbe_io_block_t *			user_io_block = fbe_transport_get_io_block (user_packet); */
    fbe_packet_t *				returned_packet = (fbe_packet_t *)serialized_packet;

    /*get the status of the io pakcet that was allocated in kernel packet */
    fbe_transport_copy_packet_status (returned_packet, user_packet);
    
    /*TODO  - copy here allwe need*/
}

/*********************************************************************
 *            get_sg_area_size ()
 *********************************************************************
 *
 *  Description:
 *    get the size of memory we would consume when copying the sg lists and data.
 *
 *  Return Value:  none
 *
 *  History:
 *    11/10/08: sharel	created
 *    
 *********************************************************************/
static void get_sg_area_size(fbe_packet_t *packet, fbe_u32_t *sg_area_size)
{
    fbe_sg_element_t *	sg_element = NULL;
    fbe_u32_t			sg_count = 0;
    fbe_u32_t			total_sg_entries_byte_size = 0;
    fbe_u32_t			total_sg_data_byte_size = 0;
    fbe_payload_ex_t *     payload;

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_count);

    if (sg_element == NULL) {
        *sg_area_size = 0;
        return;/*nothing to copy*/
    }

    /*we don't wnat to rely on the count so we just walk the list until we get the terminating one*/
    while (sg_element->count != 0) {
        total_sg_data_byte_size += sg_element->count;
        total_sg_entries_byte_size += sizeof (fbe_sg_element_t);
        sg_element ++;
    }

    /*the extra one we add at the end is the terminating sg element we want to put*/
    *sg_area_size = total_sg_entries_byte_size + total_sg_data_byte_size + sizeof (fbe_sg_element_t);

    return;
}


/*!***************************************************************
 * @fn copy_sg_data(
 *       fbe_sg_element_t * sg_element, 
 *       fbe_u32_t sg_count, 
 *       fbe_u8_t * sg_area)
 *****************************************************************
 * @brief
 *    copy the sg list and the data into the contiguous buffer
 *
 * @param sg_element - pointer to the sg element
 * @param sg_count - sg count
 * @param sg_area - pointer to the SG area
 *
 * @return 
 *
 * @version
 *  10/05/07: sharel    created
 *
 ****************************************************************/
static void copy_sg_data(fbe_sg_element_t * sg_element, fbe_u32_t sg_count, fbe_u8_t * sg_area)
{
    fbe_sg_element_t *	temp_sg_element = NULL;
    fbe_sg_element_t *	sg_element_head = (fbe_sg_element_t *)sg_area;

    temp_sg_element = sg_element;

    if (sg_element == NULL) {
        return;/*nothing to copy*/
    }

    /*first thing is the sg list entries*/
    while (temp_sg_element->count != 0) {
        memcpy (sg_area ,temp_sg_element, sizeof (fbe_sg_element_t));
        temp_sg_element ++;
        sg_area += sizeof (fbe_sg_element_t);
    }

    /*this is the terminating entry*/
    memset(sg_area, 0, sizeof(fbe_sg_element_t));
    sg_area += sizeof (fbe_sg_element_t);

    /*and now the data*/
    while (sg_element->count != 0) {
        memcpy (sg_area ,sg_element->address, sg_element->count);
        sg_area += sg_element->count;
        sg_element ++;
    }

    sg_element = sg_element_head;/*roll it back so the data in the packet is accurate*/
    sg_area = (fbe_u8_t *)sg_element_head;

    return;

}

/*!***************************************************************
 * @fn copy_back_sg_data(
 *       fbe_packet_t *packet, 
 *       fbe_u8_t * sg_area)
 *****************************************************************
 * @brief
 *   Copy back SG data
 *
 * @param packet - pointer to packet info
 * @param sg_area - pointer to the SG area
 *
 * @return 
 *
 * @version
 *  10/05/07: sharel    created
 *
 ****************************************************************/
static void copy_back_sg_data (fbe_packet_t *packet, fbe_u8_t * sg_area)
{
    fbe_sg_element_t *  sg_element = NULL;
    fbe_u32_t           sg_count = 0;
    fbe_sg_element_t *  temp_sg_element = NULL;

    fbe_payload_ex_t *     payload;

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_count);

    temp_sg_element = sg_element;

    if (sg_element == NULL) {
        return;/*nothing to copy*/
    }

    /*we need to roll ourselvs to the beggining of the actual data*/
    while (temp_sg_element->count != 0) {
        temp_sg_element ++;
        sg_area += sizeof (fbe_sg_element_t);
    }

    /*and skip the 0 one*/
    sg_area += sizeof (fbe_sg_element_t);

    /*now let's copy the data*/
    while (sg_element->count != 0) {
        memcpy (sg_element->address, sg_area, sg_element->count);
        sg_area += sg_element->count;
        sg_element ++;

    }

}

/*!***************************************************************
 * @fn fbe_api_user_init_package_connection()
 *****************************************************************
 * @brief
 *   Init package connection
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/05/07: sharel    created
 *
 ****************************************************************/
static fbe_status_t fbe_api_user_init_package_connection(void)
{
    fbe_u32_t           package_idx = 0;
    fbe_bool_t          package_found  = FBE_FALSE;

    for (package_idx = 0; package_idx < FBE_PACKAGE_ID_LAST; package_idx++) {

        if (packages_handles[package_idx].driver_link_name != NULL) {
            SetLastError((DWORD)0);

            /* We need a package handle to send an IOCTL , pay attention we want to support asynch IO using the FILE_FLAG_OVERLAPPED flag*/
            packages_handles[package_idx].package_handle = CreateFile(packages_handles[package_idx].driver_link_name, 
                                                                       GENERIC_READ | GENERIC_WRITE,
                                                                       0,
                                                                       NULL,
                                                                       OPEN_EXISTING, 
                                                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                                                       NULL);
        
            if (GetLastError() != (DWORD)0) {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:Unable to connect to %s driver\n", __FUNCTION__, packages_handles[package_idx].driver_link_name); 
                packages_handles[package_idx].package_handle = NULL;
            }else{
                package_found  = FBE_TRUE;
                fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:Connected to package:%d, handle:0x%p\n", __FUNCTION__, package_idx, (void *)packages_handles[package_idx].package_handle); 

            }
        }

    }

    if (!package_found) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:Could not find any package to connect to\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_api_user_get_control_operation(
 *       fbe_packet_t * packet, 
 *       fbe_payload_control_operation_t **control_operation)
 *****************************************************************
 * @brief
 *   Get the control operation
 *
 * @param packet - pointer to the packet
 * @param control_operation - pointer pointer to the control operation
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/05/07: sharel    created
 *
 ****************************************************************/
static fbe_status_t fbe_api_user_get_control_operation(fbe_packet_t * packet, fbe_payload_control_operation_t **control_operation)
{
    fbe_payload_ex_t * 	payload_ex = NULL;
    fbe_payload_operation_header_t * payload_operation_header = NULL;

    payload_ex = (fbe_payload_ex_t *)fbe_transport_get_payload_ex(packet);
    *control_operation = NULL;

    if(payload_ex->current_operation == NULL){	
        if(payload_ex->next_operation != NULL){
            payload_operation_header = (fbe_payload_operation_header_t *)payload_ex->next_operation;

            /* Check if current operation is control operation */
            if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
                return FBE_STATUS_GENERIC_FAILURE;
            }

            *control_operation = (fbe_payload_control_operation_t *)payload_operation_header;
        } else {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    } else {
        payload_operation_header = (fbe_payload_operation_header_t *)payload_ex->current_operation;

        /* Check if current operation is control operation */
        if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
            return FBE_STATUS_GENERIC_FAILURE;
        }

        *control_operation = (fbe_payload_control_operation_t *)payload_operation_header;
    }

    return FBE_STATUS_OK;

}


static fbe_status_t fbe_api_common_init_contiguous_packet_queue_user(void)
{
    fbe_u32_t 						count = 0;
    fbe_api_packet_q_element_t *	packet_q_element = NULL;
    fbe_status_t					status;
    fbe_u8_t * 						ptr = NULL;
    fbe_u32_t						chunk = 0;
    fbe_queue_head_t *				p_queue_head = fbe_api_common_get_contiguous_packet_q_head();
    fbe_spinlock_t *				spinlock = fbe_api_common_get_contiguous_packet_q_lock();
    fbe_queue_head_t *				o_queue_head = fbe_api_common_get_outstanding_contiguous_packet_q_head();

    fbe_queue_init(p_queue_head);
    fbe_spinlock_init(spinlock);
    fbe_queue_init(o_queue_head);

    /*we are going to allocate a 1MB chunk from CMM so let's make sure it will fit*/
    FBE_ASSERT_AT_COMPILE_TIME((sizeof(fbe_api_packet_q_element_t) * FBE_API_PRE_ALLOCATED_PACKETS_PER_CHUNK) <= 1048576);
    
    for (;chunk < FBE_API_PRE_ALLOCATED_PACKETS_CHUNKS; chunk++) {
    
        /* Allocate the memory */
        ptr = malloc(1048576);
        if (ptr == NULL) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR,"%s ecan't get memory\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        cmm_chunk_addr[chunk] = (void *)ptr;/*keep it so we can free it*/
    
          for (count = 0; count < FBE_API_PRE_ALLOCATED_PACKETS_PER_CHUNK; count ++) {
            packet_q_element = (fbe_api_packet_q_element_t *)ptr;
            fbe_zero_memory(packet_q_element, sizeof(fbe_api_packet_q_element_t));
            fbe_queue_element_init(&packet_q_element->q_element);
            status = fbe_transport_initialize_packet(&packet_q_element->packet);
            if (status != FBE_STATUS_OK) {
                fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't inti packet\n", __FUNCTION__); 
                return FBE_STATUS_GENERIC_FAILURE;/*potenital leak but this is very bad anyway*/
            }
    
            fbe_queue_push(p_queue_head, &packet_q_element->q_element);
    
            ptr += sizeof(fbe_api_packet_q_element_t);
        }
    
    }
    
    return FBE_STATUS_OK;
    
}

static fbe_status_t fbe_api_common_destroy_contiguous_packet_queue_user(void)
{
    
    fbe_u32_t  					chunk = 0;
    fbe_u32_t 					count = 0;
    fbe_api_packet_q_element_t *packet_q_element = NULL;
    fbe_queue_head_t *			p_queue_head = fbe_api_common_get_contiguous_packet_q_head();
    fbe_spinlock_t *			spinlock = fbe_api_common_get_contiguous_packet_q_lock();
    fbe_queue_head_t *			o_queue_head = fbe_api_common_get_outstanding_contiguous_packet_q_head();

    fbe_spinlock_lock(spinlock);

    /*do we have any outstanding ones ?*/
    while (!fbe_queue_is_empty(o_queue_head) && count < 10) {
        /*let's wait for completion of a while*/
        fbe_spinlock_unlock(spinlock);
        fbe_api_sleep(500);
        count++;
        fbe_spinlock_lock(spinlock);
    }

    if (!fbe_queue_is_empty(o_queue_head)) {
        /*there will be a memory leak eventually but we can't stop here forever*/
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:outstanding packets did not complete in 5 sec\n", __FUNCTION__); 
    }

    /*remove pre-allocated queued packets*/
    while(!fbe_queue_is_empty(p_queue_head)) {
        packet_q_element = (fbe_api_packet_q_element_t *)fbe_queue_pop(p_queue_head);
        packet_q_element->packet.magic_number = FBE_MAGIC_NUMBER_BASE_PACKET;
        fbe_transport_destroy_packet(&packet_q_element->packet);
    }
    
    fbe_spinlock_unlock(spinlock);
    fbe_spinlock_destroy(spinlock);

    /*free CMM memory*/
    for (;chunk < FBE_API_PRE_ALLOCATED_PACKETS_CHUNKS; chunk++) {
        free(cmm_chunk_addr[chunk]);
    }
    
    return FBE_STATUS_OK;

}

#ifdef C4_INTEGRATED
fbe_status_t fbe_api_common_init_from_fbe_cli (csx_module_context_t csx_mcontext)
{
    csx_status_e     status;
    csx_ic_id_t      safe_ic_id;
    call_from_fbe_cli_user = 1;
    /* open PP device so we can send rdevice ioctls to it from fbecli */
    if(0) fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s, fbe_api_common_init_from_fbe_cli...\n", __FUNCTION__);

    status = csx_p_get_ic_id_by_ic_name("safe", &safe_ic_id);
    if (CSX_FAILURE(status))
    {
        csx_p_display_std_note ("%s, failed to open SAFE is...\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = csx_p_rdevice_open(csx_mcontext, safe_ic_id, "\\Device\\PhysicalPackage", &ppDeviceRef);
    if (!CSX_SUCCESS(status))
    {
        csx_p_display_std_note ("%s, failed to open PPFDPort0 device...\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        if(0) csx_p_display_std_note ("%s, successfully opened PPFDPort0 device...\n", __FUNCTION__);
    }

   
    return FBE_STATUS_OK;
}

#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity - C4BUG - IOCTL model mixing */

