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


#ifdef ALAMOSA_WINDOWS_ENV
typedef csx_uap_nwdevice_reference_t        fbe_api_user_package_handle_t;
typedef csx_uap_nwdevice_async_op_id_t      fbe_api_user_async_op_id_t;
typedef csx_uap_nwdevice_async_op_context_t fbe_api_user_async_op_context_t;
#define FBE_API_USER_DEVICE_OPEN        csx_uap_nwdevice_open
#define FBE_API_USER_DEVICE_CLOSE       csx_uap_nwdevice_close
#define FBE_API_USER_DEVICE_IOCTL       csx_uap_nwdevice_ioctl
#define FBE_API_USER_DEVICE_IOCTL_ASYNC csx_uap_nwdevice_ioctl_async
#else
typedef csx_p_rdevice_reference_t        fbe_api_user_package_handle_t;
typedef csx_p_rdevice_async_op_id_t      fbe_api_user_async_op_id_t;
typedef csx_p_rdevice_async_op_context_t fbe_api_user_async_op_context_t;
#define FBE_API_USER_DEVICE_OPEN        csx_p_rdevice_open
#define FBE_API_USER_DEVICE_CLOSE       csx_p_rdevice_close
#define FBE_API_USER_DEVICE_IOCTL       csx_p_rdevice_ioctl_nt_like
#define FBE_API_USER_DEVICE_IOCTL_ASYNC csx_p_rdevice_ioctl_nt_like_async
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */

/**************************************
                Local variables
**************************************/

/*!********************************************************************* 
 * @def IOCTL_COMPLETION_IDLE_TIMER
 *  
 * @brief 
 *   This represents ioctl completion wait time in msec
 *
 * @ingroup fbe_api_common_sim_interface
 **********************************************************************/
#define IOCTL_COMPLETION_IDLE_TIMER 500 /* ioctl completion delay in msec.*/

/*!********************************************************************* 
 * @def NOTIFICATION_IDLE_TIMER
 *  
 * @brief 
 *   This represents notification wait time in msec
 *
 * @ingroup fbe_api_common_sim_interface
 **********************************************************************/
#define NOTIFICATION_IDLE_TIMER 500 /* default notification delay in msec.*/

/*!********************************************************************* 
 * @struct event_context_t 
 *  
 * @brief 
 *   This contains the data info for event notification context.
 *
 * @ingroup fbe_api_common_user_interface
 * @ingroup event_context
 **********************************************************************/
typedef struct event_context_s {
    fbe_queue_element_t        queue_element;          /*!< Queue Element */
    fbe_packet_t *             packet;                 /*!< Packet */
    fbe_api_user_async_op_id_t async_op_id;            /*!< Asynch IO Info */
} event_context_t;

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
    fbe_api_user_package_handle_t package_handle; /*!< Package Handle */
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
#ifdef ALAMOSA_WINDOWS_ENV
     "\\\\.\\PhysicalPackageLink",
#else
     "\\Device\\PhysicalPackage",
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - may be able to collapse */
     FBE_PHYSICAL_PACKAGE_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_PHYSICAL_PACKAGE_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_PHYSICAL_PACKAGE_UNREGISTER_APP_NOTIFICATION_IOCTL,
     FBE_PHYSICAL_PACKAGE_MGMT_PACKET_IOCTL, 
     FBE_PHYSICAL_PACKAGE_IO_PACKET_IOCTL},

    /* FBE_PACKAGE_ID_NEIT */
    {NULL,
#ifdef ALAMOSA_WINDOWS_ENV
     "\\\\.\\NeitPackageLink",
#else
     "\\Device\\NeitPackage",
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - may be able to collapse */
     FBE_NEIT_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_NEIT_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_NEIT_UNREGISTER_APP_NOTIFICATION_IOCTL,
     FBE_NEIT_MGMT_PACKET_IOCTL,
     NULL},

    /* FBE_PACKAGE_ID_SEP_0 */
    {NULL,
#ifdef ALAMOSA_WINDOWS_ENV
     "\\\\.\\sepLink",
#else
     "\\Device\\sep",
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - may be able to collapse */
     FBE_SEP_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_SEP_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_SEP_UNREGISTER_APP_NOTIFICATION_IOCTL,
     FBE_SEP_MGMT_PACKET_IOCTL,
     NULL},

     /* FBE_PACKAGE_ID_ESP */
    {NULL,
#ifdef ALAMOSA_WINDOWS_ENV
     "\\\\.\\espLink",
#else
     "\\Device\\esp",
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - may be able to collapse */
     FBE_ESP_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_ESP_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_ESP_UNREGISTER_APP_NOTIFICATION_IOCTL,
     FBE_ESP_MGMT_PACKET_IOCTL,
     NULL},

     /* FBE_PACKAGE_ID_KMS */
    {NULL,
#ifdef ALAMOSA_WINDOWS_ENV
     "\\\\.\\kmsLink",
#else
     "\\Device\\kms",
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - may be able to collapse */
     /*FBE_KMS_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_KMS_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_KMS_UNREGISTER_APP_NOTIFICATION_IOCTL,*/
	 NULL, NULL, NULL,
     FBE_KMS_MGMT_PACKET_IOCTL,
     NULL},

   /* FBE_PACKAGE_ID_LAST */
    {NULL,NULL, NULL, NULL, NULL, NULL},

};
    
static fbe_atomic_t        fbe_api_common_user_init_reference_count = 0;
static fbe_bool_t          fbe_api_common_user_init_stopping = FBE_FALSE;
static fbe_semaphore_t     ioctl_completion_done_semaphore;
static fbe_spinlock_t      ioctl_completion_queue_lock;
static fbe_queue_head_t    ioctl_completion_queue_head;
static fbe_semaphore_t     event_done_semaphore;
static fbe_spinlock_t      event_queue_lock;
static fbe_queue_head_t    event_queue_head;
static fbe_spinlock_t      event_notifications_queue_lock;
static fbe_queue_head_t    event_notifications_queue_head;
/*we handle one event at a time per package, so we need only one of these */
static application_id_t    event_registration_id[FBE_PACKAGE_ID_LAST + 1] = {0xDEADBEEF};
static event_context_t     event_context[FBE_PACKAGE_ID_LAST + 1];
static notification_data_t notification_data[FBE_PACKAGE_ID_LAST + 1];
static void *              cmm_chunk_addr[FBE_API_PRE_ALLOCATED_PACKETS_CHUNKS];
static fbe_bool_t          notification_init = FBE_TRUE;
static fbe_bool_t          notification_inited = FBE_FALSE;

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
    fbe_api_user_async_op_id_t              async_op_id;            /*!< Asynch IO Info */
} ioctl_completion_context_t;

/*******************************************
                Local functions
********************************************/
static void send_packet_to_drive_asynch_callback(fbe_api_user_async_op_context_t, fbe_api_user_async_op_id_t, csx_status_e, csx_size_t);

static void event_notification_callback(fbe_api_user_async_op_context_t, fbe_api_user_async_op_id_t, csx_status_e, csx_size_t);

static fbe_status_t send_packet_to_driver (fbe_packet_t * packet);
static fbe_status_t register_notification_callback(fbe_packet_t *packet);
static fbe_status_t unregister_notification_callback(fbe_packet_t *packet);
static fbe_status_t register_event_notification (fbe_package_id_t package_id);
static fbe_status_t unregister_event_notification (fbe_package_id_t package_id);
static fbe_status_t get_next_event (fbe_package_id_t package_id);
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
    fbe_atomic_increment(&fbe_api_common_user_init_reference_count);

    fbe_api_common_user_init_stopping = FBE_FALSE;

    status = fbe_api_common_init_contiguous_packet_queue_user();
    if (status != FBE_STATUS_OK) {
        fbe_api_common_destroy_contiguous_packet_queue_user();
        if ( fbe_api_trace_inited )
        {
            fbe_trace_destroy();
            fbe_api_trace_inited = FALSE;
        }
        fbe_atomic_decrement(&fbe_api_common_user_init_reference_count);
        return status;
    }
        
    /*initialize the connection to all the packages we might have*/
    status = fbe_api_user_init_package_connection();
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed to initialize packages connections\n", __FUNCTION__); 

        fbe_api_common_destroy_contiguous_packet_queue_user();
        if ( fbe_api_trace_inited )
        {
            fbe_trace_destroy();
            fbe_api_trace_inited = FALSE;
        }
        fbe_atomic_decrement(&fbe_api_common_user_init_reference_count);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the fbe api to forward data to us, but we fill only the packages we will talk to*/
    for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
        if (packages_handles[package_id].package_handle != NULL) {
            lib_entries.lib_control_entry = fbe_api_common_send_control_packet_to_kernel_driver;
            lib_entries.lib_io_entry = fbe_api_common_send_io_packet_to_kernel_driver;
            
            status = fbe_api_common_set_function_entries(&lib_entries, package_id);
            if (status != FBE_STATUS_OK) {
                fbe_u32_t count = 0;
                for (count = 0; count < FBE_PACKAGE_ID_LAST; count ++) {
                    if (packages_handles[count].package_handle != NULL){
                        /*disconnect from the drivers*/
                        (void) FBE_API_USER_DEVICE_CLOSE(packages_handles[count].package_handle);
                        packages_handles[count].package_handle = NULL;
                    }
                }

                fbe_api_common_destroy_contiguous_packet_queue_user();
                if ( fbe_api_trace_inited )
                {
                    fbe_trace_destroy();
                    fbe_api_trace_inited = FALSE;
                }
                fbe_atomic_decrement(&fbe_api_common_user_init_reference_count);

                return status;
            }
        }
    }

    /*initialize the queue head and the lock that would be used for ioctl completion managament*/
    fbe_queue_init(&ioctl_completion_queue_head);
    fbe_spinlock_init(&ioctl_completion_queue_lock);
    fbe_semaphore_init(&ioctl_completion_done_semaphore, 0, 1);
    notification_init = init_notifications;/*if the user chose FBE_FALSE we'll skip the notification registration which is costly */

    /*completion functinos queue for events notification*/
    fbe_queue_init (&event_notifications_queue_head);
    fbe_spinlock_init(&event_notifications_queue_lock);

    if (FBE_IS_TRUE(notification_init)) {
        fbe_queue_init(&event_queue_head);
        fbe_spinlock_init(&event_queue_lock);
        fbe_semaphore_init(&event_done_semaphore, 0, 1);

        /*we register ourselves to get notifications packets from the packages*/
        for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
            if ((packages_handles[package_id].package_handle != NULL) && (packages_handles[package_id].register_for_event_ioctl != NULL)) {

                status = register_event_notification (package_id);
                if (status != FBE_STATUS_OK) {
                    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed to register with package %d for notifications\n", __FUNCTION__, package_id); 
                    (void) FBE_API_USER_DEVICE_CLOSE(packages_handles[package_id].package_handle);
                    packages_handles[package_id].package_handle = NULL;
                }
            }
        }
        status  = fbe_api_notification_interface_init();/*this would initialize the notification interface in the upper level of the fbe api*/
        if (status != FBE_STATUS_OK) { 
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init notification interface\n", __FUNCTION__);
        }
        status = fbe_api_common_init_job_notification();
        notification_inited = FBE_TRUE;

        if (status != FBE_STATUS_OK) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s failed to init user interface\n", __FUNCTION__);
            fbe_api_common_destroy_user();
            notification_inited = FBE_FALSE;
        }

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
    fbe_api_common_user_init_stopping = FBE_TRUE;

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:entry\n", __FUNCTION__); 

    if (FBE_IS_TRUE(notification_inited)) {
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

        /* we wait for a while and just go out. The cancellation packet from
         * each package should arrive to the thread when we unregister
         */
        fbe_spinlock_lock(&event_queue_lock);
        if (!fbe_queue_is_empty(&event_queue_head)) {
            fbe_spinlock_unlock(&event_queue_lock);

            fbe_semaphore_wait_ms(&event_done_semaphore,
                                  (NOTIFICATION_IDLE_TIMER*50));

            fbe_spinlock_lock(&event_queue_lock);
            if (!fbe_queue_is_empty(&event_queue_head)) {
               fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                              "%s: event notifications did not complete!\n",
                              __FUNCTION__);
            }
        }
        fbe_spinlock_unlock(&event_queue_lock);

        fbe_spinlock_destroy(&event_queue_lock);
        fbe_semaphore_destroy(&event_done_semaphore);
        fbe_queue_destroy(&event_queue_head);
        notification_inited = FBE_FALSE;
    }

    /*wait for IO to end*/
    fbe_spinlock_lock(&ioctl_completion_queue_lock);
    if (!fbe_queue_is_empty(&ioctl_completion_queue_head)) {
        fbe_spinlock_unlock(&ioctl_completion_queue_lock);

            fbe_semaphore_wait_ms(&ioctl_completion_done_semaphore,
                                  (IOCTL_COMPLETION_IDLE_TIMER*50));

            fbe_spinlock_lock(&ioctl_completion_queue_lock);
            if (!fbe_queue_is_empty(&ioctl_completion_queue_head)) {
                fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                              "%s: ioctls did not complete!\n",
                              __FUNCTION__);
            }
    }
    fbe_spinlock_unlock(&ioctl_completion_queue_lock);

    fbe_spinlock_destroy(&ioctl_completion_queue_lock);
    fbe_semaphore_destroy(&ioctl_completion_done_semaphore);
    fbe_queue_destroy(&ioctl_completion_queue_head);

    fbe_queue_destroy(&event_notifications_queue_head);
    fbe_spinlock_destroy(&event_notifications_queue_lock);
    
    for (count = 0; count < FBE_PACKAGE_ID_LAST; count ++) {
        if (packages_handles[count].package_handle != NULL){
            /*disconnect from the drivers*/
            (void) FBE_API_USER_DEVICE_CLOSE(packages_handles[count].package_handle);
            packages_handles[count].package_handle = NULL;
        }
    }

    fbe_api_common_destroy_contiguous_packet_queue_user();

    fbe_atomic_decrement(&fbe_api_common_user_init_reference_count);

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s: destroy done\n", __FUNCTION__); 

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
 *  01/15/13: wahlm     rewrote to use csx_p_rdevice/csx_uap_nwdevice routines
 *
 ****************************************************************/
static fbe_status_t 
send_packet_to_driver (fbe_packet_t * packet)
{
    csx_size_t                             bytes_returned;
    fbe_u32_t                              serialized_transaction_size;
    fbe_status_t                           status = FBE_STATUS_GENERIC_FAILURE;
    csx_status_e                           csx_status;
    fbe_serialized_control_transaction_t * serialized_transaction = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_package_id_t                       package_id = FBE_PACKAGE_ID_INVALID;
    fbe_packet_attr_t 			   packet_attr;

     if (fbe_api_common_user_init_stopping) {
        status = FBE_STATUS_GENERIC_FAILURE;
    } else {
        /*we use the library to serizlie the packet*/
        status  = fbe_serialize_lib_serialize_packet(packet,
                                                     &serialized_transaction,
                                                     &serialized_transaction_size);
    }
    if (status != FBE_STATUS_OK) {
        payload = fbe_transport_get_payload_ex(packet);
        fbe_payload_ex_increment_control_operation_level(payload);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_transport_get_packet_attr(packet, &packet_attr);
    fbe_transport_get_package_id(packet, &package_id);

    /* Send the serialized packet to the package. */
    if (packet_attr & FBE_PACKET_FLAG_ASYNC) {
        void *                       mem_p;
        ioctl_completion_context_t * completion_context = NULL;

        mem_p = malloc(sizeof(ioctl_completion_context_t));
        if (NULL == mem_p) {
            payload = fbe_transport_get_payload_ex(packet);
            fbe_payload_ex_increment_control_operation_level(payload);

            status = FBE_STATUS_INSUFFICIENT_RESOURCES;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            free(serialized_transaction);
            return status;
        }
        completion_context = (ioctl_completion_context_t *)mem_p;
        completion_context->packet = packet;
	completion_context->serialized_transaction = serialized_transaction;
        fbe_queue_element_init(&completion_context->queue_element);

        fbe_spinlock_lock(&ioctl_completion_queue_lock);
        fbe_queue_push(&ioctl_completion_queue_head,
                       &completion_context->queue_element);
        fbe_spinlock_unlock(&ioctl_completion_queue_lock);

        csx_status = FBE_API_USER_DEVICE_IOCTL_ASYNC(
                         packages_handles[package_id].package_handle,
                         packages_handles[package_id].control_ioctl, 
                         serialized_transaction,      /* in buf   */ 
                         serialized_transaction_size, /* in size  */
                         serialized_transaction,      /* out buf  */ 
                         serialized_transaction_size, /* out size */
                         &bytes_returned, /* actual size returned */
                         send_packet_to_drive_asynch_callback,
                         (fbe_api_user_async_op_context_t)completion_context,
                         &completion_context->async_op_id);

        if (CSX_STATUS_PENDING == csx_status) {
	    return FBE_STATUS_PENDING;
        }
        fbe_spinlock_lock(&ioctl_completion_queue_lock);
        if (fbe_queue_is_element_on_queue(&completion_context->queue_element)) {
            fbe_queue_remove(&completion_context->queue_element);
        } else {
            fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                          "%s: Async I/O call returned 0x%X but completion element is not on queue\n",
			__FUNCTION__, (int)csx_status); 
        }
        fbe_spinlock_unlock(&ioctl_completion_queue_lock);
        free(mem_p);
    } else {
        csx_status = FBE_API_USER_DEVICE_IOCTL(
                         packages_handles[package_id].package_handle,
                         packages_handles[package_id].control_ioctl,
                         serialized_transaction,      /* in buf   */ 
                         serialized_transaction_size, /* in size  */
                         serialized_transaction,      /* out buf  */ 
                         serialized_transaction_size, /* out size */
                         &bytes_returned  /* actual size returned */);
    }

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_increment_control_operation_level(payload);

    if (!CSX_SUCCESS(csx_status)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s:Got an error: 0x%X from the package when trying to send packet\n",
			__FUNCTION__, (int)csx_status); 
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet, status, 0);
    } else {
        status = FBE_STATUS_OK;
        fbe_serialize_lib_deserialize_transaction(serialized_transaction,
                                                  packet);
    }

    fbe_transport_complete_packet(packet);
    free(serialized_transaction);
    return status;
}

/*!***************************************************************
 * @fn send_packet_to_drive_asynch_callback(
 *****************************************************************
 * @brief
 *   callback for completion of asynchronous I/O requests
 *
 * @return 
 *
 * @version
 *  01/15/13: wahlm    created
 ****************************************************************/
static void send_packet_to_drive_asynch_callback(
    fbe_api_user_async_op_context_t rdevice_async_op_context,
    fbe_api_user_async_op_id_t      rdevice_async_op_id,
    csx_status_e                    status,
    csx_size_t                      io_out_size_actual)
{
    ioctl_completion_context_t *context;
    fbe_payload_ex_t *original_payload = NULL;

    CSX_UNREFERENCED_PARAMETER(rdevice_async_op_id);
    CSX_UNREFERENCED_PARAMETER(io_out_size_actual);

    context = (ioctl_completion_context_t *)rdevice_async_op_context;

    fbe_serialize_lib_deserialize_transaction(context->serialized_transaction,
                                              context->packet);
    if (!CSX_SUCCESS(status)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s:Got an error: 0x%x trying to send packet\n",
                      __FUNCTION__, (int)status); 
        fbe_transport_set_status(context->packet, FBE_STATUS_GENERIC_FAILURE, 0);
    }

    /*and complete it for the original sender of the packet*/
    original_payload = fbe_transport_get_payload_ex(context->packet);
    fbe_payload_ex_increment_control_operation_level(original_payload);

    fbe_transport_complete_packet(context->packet);/*complete to user*/
    
    /* remove from ioctl completion list and signal if stopping */
    fbe_spinlock_lock(&ioctl_completion_queue_lock);
    fbe_queue_remove(&context->queue_element);
    if (fbe_api_common_user_init_stopping &&
        fbe_queue_is_empty(&ioctl_completion_queue_head)) {

        fbe_semaphore_release(&ioctl_completion_done_semaphore, 0, 1, FALSE);
    }
    fbe_spinlock_unlock(&ioctl_completion_queue_lock);

    /*get rid of all the resources we consumed*/
    free((void *)context->serialized_transaction);
    free((void *)context);

    return ;
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
    csx_size_t                        bytes_returned;
    fbe_u32_t                         buffer_size;
    fbe_package_register_app_notify_t register_app;
    fbe_status_t                      status;
    csx_status_e                      csx_status;
    csx_size_t                        copied_chars = 0;

    /*let's make sure we send down the name of whoever is registering*/
    fbe_zero_memory(&register_app, sizeof(fbe_package_register_app_notify_t));

    csx_status = csx_p_native_executable_path_get((void *)&register_app.registered_app_name, APP_NAME_LENGTH, &copied_chars);

    if (csx_status == CSX_STATUS_BUFFER_OVERFLOW) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*to be on the safe side, force last char to NULL*/
    register_app.registered_app_name[APP_NAME_LENGTH - 1] = 0;

    buffer_size = sizeof(fbe_package_register_app_notify_t);

    /*send to the driver */
    csx_status = FBE_API_USER_DEVICE_IOCTL(
                     packages_handles[package_id].package_handle, 
                     packages_handles[package_id].register_for_event_ioctl, 
                     &register_app,  /* input buf   */ 
                     buffer_size,    /* input size  */
                     &register_app,  /* output buf  */ 
                     buffer_size,    /* output size */
                     &bytes_returned /* actual size returned */);

    if (!CSX_SUCCESS(csx_status)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s:Got an error: 0x%X from package id:%d when trying to send IRP\n",
                      __FUNCTION__, (int)csx_status, package_id); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

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
 * @fn event_notification_callback
 *****************************************************************
 * @brief
 *    Callback for asynch IOCTL to package requesting event notification.
 *
 * @param 
 *
 * @return
 *
 * @version
 *  01/15/13: wahlm    created
 *
 ****************************************************************/
static void event_notification_callback(
    fbe_api_user_async_op_context_t rdevice_async_op_context,
    fbe_api_user_async_op_id_t      rdevice_async_op_id,
    csx_status_e                    status,
    csx_size_t                      io_out_size_actual)
{
    notification_data_t * notification_p;
    fbe_package_id_t      package_id;
    fbe_queue_element_t * queue_element;

    package_id = (fbe_package_id_t)rdevice_async_op_context;
    notification_p = &notification_data[package_id];
    
    fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                  "%s : Received Notification from PkgID %d, AppID %d \n",
                  __FUNCTION__, package_id,
                  notification_p->application_id);

    if (io_out_size_actual == 0) {
        fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                      "%s:got request from package to cancel the ioctl\n",
                       __FUNCTION__); 
    }
    /*
     * Send notifications to all the modules that registered with us using the
     * FBE_NOTIFICATION_CONTROL_CODE_REGISTER ioctl.
     */ 
    fbe_spinlock_lock(&event_notifications_queue_lock);
    queue_element = fbe_queue_front(&event_notifications_queue_head);
    
    while (queue_element != NULL) {
        fbe_notification_element_t *element_p;
        
        element_p = (fbe_notification_element_t *)queue_element;
        /*
         * We send the notifications only to entities that wanted updates from
         * this pacakge. Since we keep a single queue, the queue can have
         * registrations for more than one package.
         */
        if (element_p->targe_package == notification_p->notification_info.source_package) {

            fbe_spinlock_unlock(&event_notifications_queue_lock);

            fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s: Invoking Notify callback. PkgID %d, AppID %d. Notify Type %llu \n",
                          __FUNCTION__, package_id,
                          notification_p->application_id,
                          (unsigned long long)notification_p->notification_info.notification_type);
            /*
             * Use the last data we got when the notification service completed
             * the call. This data is static and not in a queue since this
             * operation is synchronized.
             */
            element_p->notification_function(notification_p->object_id,
                                             notification_p->notification_info,
                                             element_p->notification_context);

            fbe_spinlock_lock(&event_notifications_queue_lock);
        }

        queue_element = fbe_queue_next(&event_notifications_queue_head,
                                       queue_element);       
    }

    fbe_spinlock_unlock(&event_notifications_queue_lock);
    /*
     * Now that we sent the notifications about this event to all the
     * registered functions we can send another packet to the notification
     * service so it can return it with more events if there are any.
     */
    notification_p->notification_info.source_package = FBE_PACKAGE_ID_INVALID;/*reset to next time if somehow we get garbage*/

    /* remove from event list and signal if stopping */
    fbe_spinlock_lock(&event_queue_lock);
    fbe_queue_remove(&event_context[package_id].queue_element);
    if (fbe_api_common_user_init_stopping &&
        fbe_queue_is_empty(&event_queue_head)) {

        fbe_semaphore_release(&event_done_semaphore, 0, 1, FALSE);
    }
    fbe_spinlock_unlock(&event_queue_lock);

    get_next_event(package_id);
}

/*!***************************************************************
 * @fn get_next_event(fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *    Send an ioctl to the package. This ioctl would wait on a queue,
 *    and once an event happen in the package, this ioctl will be
 *    completed and contain the event information. The completion of
 *    this ioctl will invoke the event_notification_callback routine.
 *
 * @param package_id - package ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/23/07: sharel    created
 *  01/15/13: wahlm     rewrote to use csx_p_rdevice/csx_uap_nwdevice routine.
 *
 ****************************************************************/
static fbe_status_t get_next_event(fbe_package_id_t package_id)
{
    csx_size_t                 bytes_returned;
    csx_status_e               status;
    
    if (fbe_api_common_user_init_stopping) {
        return FBE_STATUS_OK;
    }

    fbe_queue_element_init(&event_context[package_id].queue_element);
    fbe_spinlock_lock(&event_queue_lock);
    fbe_queue_push(&event_queue_head, &event_context[package_id].queue_element);
    fbe_spinlock_unlock(&event_queue_lock);

    /*clear the data, sine we used this memory before*/
    notification_data[package_id].application_id = event_registration_id[package_id];
    notification_data[package_id].context = NULL;/*use this in the future if needed*/
    
    /*send to the driver, this should be an asynchronous IOCTL*/
    status = FBE_API_USER_DEVICE_IOCTL_ASYNC(
                         packages_handles[package_id].package_handle,
                         packages_handles[package_id].get_event_ioctl, 
                         &notification_data[package_id], /* input buf   */ 
                         sizeof(notification_data_t),    /* input size  */
                         &notification_data[package_id], /* output buf  */ 
                         sizeof(notification_data_t),    /* output size */
                         &bytes_returned,                /* actual size */
                         event_notification_callback,
                         (fbe_api_user_async_op_context_t)package_id,
                         &event_context[package_id].async_op_id);

    /* normally, we will pend, so we just go out */
    if (CSX_STATUS_PENDING == status) {
        return FBE_STATUS_OK;
    }
    fbe_spinlock_lock(&event_queue_lock);
    if (fbe_queue_is_element_on_queue(&event_context[package_id].queue_element)) {
        fbe_queue_remove(&event_context[package_id].queue_element);
    }
    fbe_spinlock_unlock(&event_queue_lock);

    if (!CSX_SUCCESS(status)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s:Got an error: 0x%X from package 0x%X when trying to send packet\n",
                      __FUNCTION__, (int)status, package_id); 

        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* If we got here it means we did not pend at all, this should not happen.*/
    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s:Get event IOCTL to package 0x%X did not pend!\n",
                      __FUNCTION__, package_id); 

    return FBE_STATUS_GENERIC_FAILURE;
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
    csx_size_t                        bytes_returned;
    csx_status_e                      status;
    fbe_u32_t                         buffer_size;
    fbe_package_register_app_notify_t register_app;
    
    buffer_size = sizeof(fbe_package_register_app_notify_t);

    /* should be set from before but just to be on the safe side*/
    register_app.application_id = event_registration_id[package_id];

    /* Send to the driver. */
    status = FBE_API_USER_DEVICE_IOCTL(
                    packages_handles[package_id].package_handle,
                    packages_handles[package_id].unregister_from_event_ioctl,
                    &register_app,  /* input buf   */
                    buffer_size,    /* input size  */
                    &register_app,  /* output buf  */
                    buffer_size,    /* output size */
                    &bytes_returned /* actual size returned */);

    if (!CSX_SUCCESS(status)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s:Got an error: 0x%X from package %s when trying to send packet\n",
                      __FUNCTION__, (int)status,
                      packages_handles[package_id].driver_link_name);
    }
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
    fbe_u32_t     package_idx = 0;
    fbe_bool_t    package_found  = FBE_FALSE;
    csx_status_e  status;

    for (package_idx = 0; package_idx < FBE_PACKAGE_ID_LAST; package_idx++) {

        if (packages_handles[package_idx].driver_link_name != NULL) {

            /* We need a package handle to send an IOCTL.
             * NOTE: we support asynch IO so cannot use EmcutilRemoteDevice
             * routines (which only support synch IO).
             */
            status = FBE_API_USER_DEVICE_OPEN(
                            CSX_MY_MODULE_CONTEXT(),
#ifndef ALAMOSA_WINDOWS_ENV
                            csx_p_rdevice_default_peer_ic_id_get(),
#endif /* ALAMOSA_WINDOWS_ENV - ARCH - system */
                            packages_handles[package_idx].driver_link_name, 
                            &packages_handles[package_idx].package_handle);

            if (!CSX_SUCCESS(status)) {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                              "%s:Unable to connect to %s driver\n",
                              __FUNCTION__,
                              packages_handles[package_idx].driver_link_name); 
                packages_handles[package_idx].package_handle = NULL;
            } else {
                package_found  = FBE_TRUE;
                fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                              "%s:Connected to package:%d, handle:0x%p\n",
                              __FUNCTION__, package_idx,
                              (void *)packages_handles[package_idx].package_handle); 
            }
        }

    }

    if (!package_found) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s:Could not find any package to connect to\n",
                      __FUNCTION__); 
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
    FBE_ASSERT_AT_COMPILE_TIME((sizeof(fbe_packet_t) * FBE_API_PRE_ALLOCATED_PACKETS_PER_CHUNK) <= 1048576);
    
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

    /* destroy outstanding queued packets. This is ugly but we can't do much since we are being shutdown */
    while(!fbe_queue_is_empty(o_queue_head)) {
        packet_q_element = (fbe_api_packet_q_element_t *)fbe_queue_pop(o_queue_head);
        packet_q_element->packet.magic_number = FBE_MAGIC_NUMBER_BASE_PACKET;
        fbe_transport_destroy_packet(&packet_q_element->packet);
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
        if (cmm_chunk_addr[chunk]) {
            free(cmm_chunk_addr[chunk]);
        }
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

