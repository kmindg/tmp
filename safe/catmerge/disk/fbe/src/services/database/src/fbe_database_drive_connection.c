/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_drive_connection.c
***************************************************************************
*
* @brief
*  This file contains functions needed to connect to physical package.
*  
* @version
*  04/08/2011 - Created. 
*
***************************************************************************/
#include "fbe_database_common.h"
#include "fbe_database_drive_connection.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe_database_config_tables.h"
#include "fbe_job_service.h"
#include "fbe/fbe_notification_lib.h"
#include "fbe_private_space_layout.h"
#include "fbe_database_homewrecker_fru_descriptor.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe_database_private.h"
#include "fbe_database_homewrecker_db_interface.h"

#define INVALID_SLOT_NUMBER 0x3c
#define DATABASE_SYSTEM_DRIVE_BUS 0
#define DATABASE_SYSTEM_DRIVE_ENCLOSURE 0

extern fbe_database_service_t fbe_database_service;

static fbe_time_t last_force_garbage_collection_time = 0;

typedef struct database_system_pvd_destroy_context_s
{
    fbe_u64_t   job_number;
    database_pvd_operation_t*   pvd_operation;
}database_system_pvd_destroy_context_t;

typedef struct database_system_pvd_creation_context_s
{
    fbe_u64_t   job_number;
    database_pvd_operation_t*   pvd_operation;
}database_system_pvd_creation_context_t;

static database_pvd_discover_object_t   *fbe_database_pp_notification_resource = NULL;
static fbe_thread_t                     fbe_database_pvd_operation_thread_handle;

static fbe_notification_element_t       fbe_database_drive_connect_notification_element;

static database_system_pvd_creation_context_t *fbe_database_system_pvd_created_notification_context = NULL;
static fbe_notification_element_t       *fbe_database_system_pvd_created_notification_element = NULL;

static database_system_pvd_destroy_context_t  *fbe_database_system_pvd_destroyed_notification_context = NULL;
static fbe_notification_element_t       *fbe_database_system_pvd_destroyed_notification_element = NULL;
static void *fbe_database_drive_conn_mem = NULL;

static fbe_status_t 
fbe_database_sys_pvd_created_notification_callback(fbe_object_id_t object_id, 
                                                                                                                                                                                         fbe_notification_info_t notification_info,      
                                                                                                                                                                                         fbe_notification_context_t context);

static fbe_status_t fbe_database_physical_package_notification_callback(fbe_object_id_t object_id, 
                                                                        fbe_notification_info_t notification_info,
                                                                        fbe_notification_context_t context);

static void fbe_database_pvd_operation_thread_func(void * context);
static fbe_status_t fbe_database_connect_pvd_to_pdo_info(fbe_object_id_t pvd, fbe_object_id_t pdo, fbe_database_physical_drive_info_t *pdo_info);
static fbe_status_t fbe_database_connect_pvds(database_pvd_operation_t *connect_context);
static fbe_status_t fbe_database_process_physical_drives(database_pvd_operation_t *connect_context, fbe_bool_t fast_connect);

static fbe_bool_t fbe_database_pvd_operation_thread_has_more_work(database_pvd_operation_t *connect_context);
static fbe_status_t fbe_database_drive_discover_get_resource(database_pvd_operation_t *operation_context, 
                                                             database_pvd_discover_object_t **free_entry);
static fbe_status_t fbe_database_drive_discover_release_resource(database_pvd_operation_t *operation_context, 
                                                                 fbe_queue_head_t *queue_head);
static fbe_status_t fbe_database_destroy_pvd_via_job(
                                                                fbe_job_service_destroy_provision_drive_t* pvd_destroy_request,
                                                                fbe_object_id_t pvd_object_id);
static fbe_status_t fbe_database_update_pvd_sn_via_job(
                                                                fbe_job_service_update_provision_drive_t* pvd_update_request,
                                                                fbe_object_id_t pvd_object_id, fbe_u8_t* serial_num);
static fbe_status_t fbe_database_reinitialize_system_pvd_via_job(
                                                                fbe_job_service_update_provision_drive_t* pvd_update_request,
                                                                fbe_database_physical_drive_info_t* pdo_info);
static fbe_status_t fbe_database_add_item_into_create_pvd_request_PVD_list(
                                                                fbe_job_service_create_provision_drive_t* pvd_create_request,
                                                                fbe_database_physical_drive_info_t* pdo_info);
static fbe_status_t fbe_database_send_create_pvd_request(
                                                                fbe_job_service_create_provision_drive_t* pvd_create_request);
static fbe_status_t fbe_database_send_connect_pvd_request(
                                                                fbe_job_service_connect_drive_t* pvd_connect_request);

static fbe_status_t fbe_database_find_original_slot_for_original_system_drive(
                                                                database_pvd_operation_t*   pvd_operation_context,
                                                                fbe_database_physical_drive_info_t* pdo_info,
                                                                fbe_u32_t* slot_number);
static fbe_status_t fbe_database_add_item_into_connect_pvd_request_PVD_list(
                                                                fbe_job_service_connect_drive_t *pvd_connect_request,
                                                                fbe_object_id_t  pvd_id);

static fbe_status_t fbe_database_is_pvd_connected_to_drive(fbe_object_id_t pvd_object_id, fbe_bool_t *is_connected);

static fbe_status_t fbe_database_is_pvd_in_single_loop_failure_state(fbe_object_id_t object_id, fbe_bool_t* in_slf);

static fbe_status_t fbe_database_get_raidgroup_drive_tier(fbe_object_id_t rg_object_id, 
                                                          fbe_drive_performance_tier_type_np_t *drive_tier_p);
static fbe_status_t fbe_database_get_pdo_drive_tier(fbe_object_id_t pdo_object_id, 
                                                    fbe_drive_performance_tier_type_np_t *drive_tier_p);
static fbe_bool_t fbe_database_skip_connect_drive_tier(fbe_object_id_t pdo_object_id);
static fbe_bool_t fbe_database_verify_and_handle_unsupported_drive(fbe_object_id_t pdo_object_id, fbe_physical_drive_mgmt_get_drive_information_t *drive_info_p);


/*!***************************************************************
 * @fn fbe_database_drive_connection_init
 *****************************************************************
 * @brief
 *   initialize resources for handling drive connection
 *
 * @param  database_service - database service
 *
 * @return none
 *
 * @version
 *    04/27/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_drive_connection_init(fbe_database_service_t *database_service)
{
    fbe_u32_t    i;
    fbe_status_t status;

    database_pvd_discover_object_t *resource = NULL;
    fbe_u32_t   database_system_drives_num = 0;
    fbe_u32_t   mem_size = 0;
    fbe_u32_t   max_phy_drivers = 0;

    database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();

    max_phy_drivers = fbe_database_service.logical_objects_limits.platform_fru_count < DATABASE_MAX_SUPPORTED_PHYSICAL_DRIVES ? DATABASE_MAX_SUPPORTED_PHYSICAL_DRIVES : fbe_database_service.logical_objects_limits.platform_fru_count;
    mem_size = 2* max_phy_drivers*sizeof(database_pvd_discover_object_t) +                        /*for fbe_database_pp_notification_resource and connecting_resource*/
                 database_system_drives_num*sizeof(database_system_pvd_destroy_context_t) +          /*for fbe_database_system_pvd_destroyed_notification_context*/
                 database_system_drives_num*sizeof(fbe_notification_element_t) +                     /*for fbe_database_system_pvd_destroyed_notification_element*/
                 database_system_drives_num*sizeof(database_system_pvd_creation_context_t) +         /*for fbe_database_system_pvd_created_notification_context*/
                 database_system_drives_num*sizeof(fbe_notification_element_t) ;                    /*for fbe_database_system_pvd_created_notification_element*/
                        
    fbe_database_drive_conn_mem = (fbe_u8_t*)fbe_memory_native_allocate(mem_size);
    if (fbe_database_drive_conn_mem == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "database_init can't get memory resource\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_database_pp_notification_resource = (database_pvd_discover_object_t*)fbe_database_drive_conn_mem;
    
    fbe_database_system_pvd_destroyed_notification_context = (database_system_pvd_destroy_context_t*)   \
                                                            ((fbe_u8_t*)fbe_database_pp_notification_resource + max_phy_drivers*sizeof(database_pvd_discover_object_t));
                                                            
    fbe_database_system_pvd_destroyed_notification_element = (fbe_notification_element_t*)  \
                                                            ((fbe_u8_t*)fbe_database_system_pvd_destroyed_notification_context + database_system_drives_num*sizeof(database_system_pvd_destroy_context_t));
                                                            
    fbe_database_system_pvd_created_notification_context = (database_system_pvd_creation_context_t*)    \
                                                            ((fbe_u8_t*)fbe_database_system_pvd_destroyed_notification_element + database_system_drives_num*sizeof(fbe_notification_element_t));
                                                            
    fbe_database_system_pvd_created_notification_element = (fbe_notification_element_t*)    \
                                                            ((fbe_u8_t*)fbe_database_system_pvd_created_notification_context + database_system_drives_num*sizeof(database_system_pvd_creation_context_t));
    
    /* initialize queues */
    fbe_spinlock_init(&database_service->pvd_operation.discover_queue_lock);
    database_service->pvd_operation.object_table_ptr = &database_service->object_table;
    fbe_queue_init(&database_service->pvd_operation.connect_queue_head);
    fbe_queue_init(&database_service->pvd_operation.discover_queue_head);
    fbe_semaphore_init(&database_service->pvd_operation.pvd_operation_thread_semaphore, 0, FBE_MAX_PHYSICAL_OBJECTS);
    database_service->pvd_operation.pvd_operation_thread_flag = PVD_OPERATION_THREAD_INVALID;

    /*here just zero fru descriptor, which would be actually initialized when homewrecker logic runs*/
    fbe_zero_memory(&database_service->pvd_operation.system_fru_descriptor, sizeof(fbe_homewrecker_fru_descriptor_t));
    fbe_spinlock_init(&database_service->pvd_operation.system_fru_descriptor_lock);
    
    /* resource for discover notification */
    fbe_queue_init(&database_service->pvd_operation.free_entry_queue_head);
    for(i=0, resource = fbe_database_pp_notification_resource; i < max_phy_drivers * 2; i++)
    {
        fbe_queue_push(&database_service->pvd_operation.free_entry_queue_head, &resource->queue_element);
        resource ++;
    }

    database_service->pvd_operation.pvd_operation_thread_flag = PVD_OPERATION_THREAD_WAIT_RUN;
    fbe_thread_init(&fbe_database_pvd_operation_thread_handle, "fbe_db_pvd_op", fbe_database_pvd_operation_thread_func, (void*)&database_service->pvd_operation);

    // register notification from physical package
    fbe_database_drive_connect_notification_element.notification_function = fbe_database_physical_package_notification_callback;
    fbe_database_drive_connect_notification_element.notification_context = &database_service->pvd_operation;
    fbe_database_drive_connect_notification_element.targe_package = FBE_PACKAGE_ID_PHYSICAL;
    fbe_database_drive_connect_notification_element.notification_type = FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY;
    fbe_database_drive_connect_notification_element.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;

    status = database_common_register_notification ( &fbe_database_drive_connect_notification_element,
                                                     FBE_PACKAGE_ID_PHYSICAL);

    if(FBE_STATUS_OK != status)
    {
        fbe_memory_native_release(fbe_database_drive_conn_mem);
        fbe_database_drive_conn_mem = NULL;
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: failed to register connect notification.\n", __FUNCTION__);
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_database_drive_connection_start_drive_process
 *****************************************************************
 * @brief
 *   start drive processing thread
 *
 * @param  database_service - database service
 *
 * @return none
 *
 * @version
 *    08/24/2012:   created Zhipeng Hu
 *
 ****************************************************************/
fbe_status_t fbe_database_drive_connection_start_drive_process(fbe_database_service_t *database_service)
{    
    if(PVD_OPERATION_THREAD_WAIT_RUN == database_service->pvd_operation.pvd_operation_thread_flag)
    {
        database_service->pvd_operation.pvd_operation_thread_flag = PVD_OPERATION_THREAD_RUN;
        fbe_semaphore_release(&database_service->pvd_operation.pvd_operation_thread_semaphore, 0, 1, FALSE);
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: pvd operation thread will actually run from now on.\n", __FUNCTION__);
    }
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_drive_connection_destroy
 *****************************************************************
 * @brief
 *   release resources allocated for handling drive connection
 *
 * @param  database_service - database service
 *
 * @return none
 *
 * @version
 *    04/27/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_drive_connection_destroy(fbe_database_service_t *database_service)
{
    fbe_status_t status;

    status = database_common_unregister_notification ( &fbe_database_drive_connect_notification_element,
                                                       FBE_PACKAGE_ID_PHYSICAL);


    // stop the pvd connect thread.
    database_service->pvd_operation.pvd_operation_thread_flag = PVD_OPERATION_THREAD_STOP;
    fbe_semaphore_release(&database_service->pvd_operation.pvd_operation_thread_semaphore, 0, 1, FALSE);

    fbe_thread_wait(&fbe_database_pvd_operation_thread_handle);
    fbe_thread_destroy(&fbe_database_pvd_operation_thread_handle);

    /* After check the memory from discover queue, we don't care what's in the queue at this point */
    fbe_queue_destroy(&database_service->pvd_operation.connect_queue_head);
    fbe_queue_destroy(&database_service->pvd_operation.discover_queue_head);
    fbe_queue_destroy(&database_service->pvd_operation.free_entry_queue_head);
    fbe_spinlock_destroy(&database_service->pvd_operation.discover_queue_lock);
    database_service->pvd_operation.object_table_ptr = NULL;
    fbe_semaphore_destroy(&database_service->pvd_operation.pvd_operation_thread_semaphore);
    fbe_spinlock_destroy(&database_service->pvd_operation.system_fru_descriptor_lock);

    if (fbe_database_drive_conn_mem!=NULL)
    {
        fbe_memory_native_release(fbe_database_drive_conn_mem);
        fbe_database_drive_conn_mem = NULL;
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_physical_package_notification_callback
 *****************************************************************
 * @brief
 *   callback function from physical package.
 *
 * @param  object_id - the id of the object that generates the notification
 * @param  notification_info - info of the notification
 * @param  context - context passed in at the timeof registering the notification.
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/27/2011:   created
 *
 ****************************************************************/
static fbe_status_t fbe_database_physical_package_notification_callback(fbe_object_id_t object_id, 
                                                                   fbe_notification_info_t notification_info,
                                                                   fbe_notification_context_t context)
{
    database_pvd_operation_t *  pvd_operation_context = (database_pvd_operation_t *)context;
    fbe_lifecycle_state_t       state;
    fbe_status_t                status = FBE_STATUS_OK;

    // only care about PDO lifecycle state change to ready 
    if ((notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE)&&
        ((notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE)||
         (notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY)))
    {
        status = fbe_notification_convert_notification_type_to_state(notification_info.notification_type, &state);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: can't map notification type: 0x%llx drop notification\n", 
                           __FUNCTION__, notification_info.notification_type);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_database_add_drive_to_be_processed(pvd_operation_context, 
                                               object_id, 
                                               state,
                                               DATABASE_PVD_DISCOVER_FLAG_NORMAL_PROCESS);
    }
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_database_pvd_operation_thread_has_more_work
 *****************************************************************
 * @brief
 *   checks if there's more work for this thread to do
 *
 * @param  pvd_operation_context - context for database service
 *
 * @return none
 *
 * @version
 *    04/20/2011:   created
 *
 ****************************************************************/
static fbe_bool_t fbe_database_pvd_operation_thread_has_more_work(database_pvd_operation_t *pvd_operation_context)
{
    fbe_bool_t                      more_work = FALSE;
    fbe_queue_element_t             *queue_element, *queue_element_next;
    database_pvd_discover_object_t  *discover_request;

    fbe_spinlock_lock(&pvd_operation_context->discover_queue_lock);
    queue_element = fbe_queue_front(&pvd_operation_context->discover_queue_head);
    while (queue_element!=NULL)
    {
        discover_request = (database_pvd_discover_object_t *)queue_element;
        queue_element_next = fbe_queue_next(&pvd_operation_context->discover_queue_head, queue_element);
        // ready to process
        if (discover_request->lifecycle_state == FBE_LIFECYCLE_STATE_READY)
        {
            more_work = TRUE;
            break;
        }
        queue_element = queue_element_next;
    }
    fbe_spinlock_unlock(&pvd_operation_context->discover_queue_lock);

    if (more_work == TRUE)
    {
        return more_work;
    }

    fbe_spinlock_lock(&pvd_operation_context->object_table_ptr->table_lock);
    if (!(fbe_queue_is_empty(&pvd_operation_context->connect_queue_head)))
    {
        more_work = TRUE;
    }
    fbe_spinlock_unlock(&pvd_operation_context->object_table_ptr->table_lock);

    return more_work;
}

/*!***************************************************************
 * @fn fbe_database_pvd_operation_thread_func
 *****************************************************************
 * @brief
 *   thread performs connecting PVD to PDO and discovery of PDO.
 *
 * @param  context - context for database service
 *
 * @return none
 *
 * @version
 *    04/20/2011:   created
 *
 ****************************************************************/
void fbe_database_pvd_operation_thread_func(void * context)
{
    fbe_status_t  connect_status = FBE_STATUS_OK;
    fbe_status_t  discover_status = FBE_STATUS_OK;

    database_pvd_operation_t *pvd_operation_context = (database_pvd_operation_t *) context;

    database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);
    while (1)
    {
        fbe_semaphore_wait_ms(&pvd_operation_context->pvd_operation_thread_semaphore, 2000);    
        // has work to do
        if (pvd_operation_context->pvd_operation_thread_flag == PVD_OPERATION_THREAD_RUN)
        {
            do 
            {
                /* fast connect first */
                discover_status = fbe_database_process_physical_drives(pvd_operation_context, FBE_TRUE);
                /* process connect request */
                connect_status = fbe_database_connect_pvds(pvd_operation_context);
                /* then discovery request */
                discover_status = fbe_database_process_physical_drives(pvd_operation_context, FBE_FALSE);
                if ((discover_status != FBE_STATUS_OK) || (connect_status != FBE_STATUS_OK))
                {
                    /* let's take a break */
                    break;
                }
            }
            while (fbe_database_pvd_operation_thread_has_more_work(pvd_operation_context));
        }
        else if(pvd_operation_context->pvd_operation_thread_flag == PVD_OPERATION_THREAD_WAIT_RUN)
        {
            continue;
        }
        else
        {
            break;  // we need to stop
        }
    }

    pvd_operation_context->pvd_operation_thread_flag = PVD_OPERATION_THREAD_DONE;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s exit\n", __FUNCTION__);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}


/*!***************************************************************
 * @fn fbe_database_drive_discover_get_resource
 *****************************************************************
 * @brief
 *   get resource to queue pdo discover request
 *
 * @param  connect_context - connect context for database service
 * @param  pdo - object id
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/27/2011:   created
 *
 ****************************************************************/
static fbe_status_t fbe_database_drive_discover_get_resource(database_pvd_operation_t *operation_context, 
                                                             database_pvd_discover_object_t **free_entry)
{
    *free_entry = NULL;
    
    if ((fbe_database_pp_notification_resource == NULL) ||
        (fbe_queue_is_empty(&operation_context->free_entry_queue_head)))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    *free_entry = (database_pvd_discover_object_t *)fbe_queue_pop(&operation_context->free_entry_queue_head);
    if(NULL != *free_entry)
    {
        (*free_entry)->drive_process_flag = DATABASE_PVD_DISCOVER_FLAG_NORMAL_PROCESS;
    }
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_drive_discover_release_resource
 *****************************************************************
 * @brief
 *   release notification resource back to pool
 *
 * @param  connect_context - connect context for database service
 * @param  pdo - object id
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/27/2011:   created
 *
 ****************************************************************/
static fbe_status_t fbe_database_drive_discover_release_resource(database_pvd_operation_t *operation_context, 
                                                                 fbe_queue_head_t *queue_head)
{

    fbe_queue_merge(&operation_context->free_entry_queue_head, queue_head);

    return FBE_STATUS_OK;
}

/*!*************************************************************************************
 * @fn fbe_database_add_drive_to_be_processed
 ***************************************************************************************
 * @brief
 *   adds pdo to queue waiting to be picked up
 *
 * @param  connect_context - connect context for database service
 * @param  pdo - object id
 * @param  lifecycle_state - state of the object
 * @param  pvd_discover_flag - how to process this discovered drive
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/26/2011:   created
 *    07/10/2012:   added pvd discover flag. Zhipeng Hu
 *
 **************************************************************************************/
fbe_status_t 
fbe_database_add_drive_to_be_processed(database_pvd_operation_t *operation_context, 
                                       fbe_object_id_t pdo,
                                       fbe_lifecycle_state_t lifecycle_state,
                                       database_pvd_discover_flag_t  pvd_discover_flag)
{
    fbe_status_t  status;
    database_pvd_discover_object_t *notification_resource = NULL;
    database_pvd_discover_object_t *new_notification_resource = NULL;

    /* search for duplicate from discover queue */
    fbe_spinlock_lock(&operation_context->discover_queue_lock);
    notification_resource = (database_pvd_discover_object_t *)fbe_queue_front(&operation_context->discover_queue_head);
    while (notification_resource!=NULL)
    {
        if (notification_resource->object_id > pdo)
        {
            break;  // we are inserting here.
        }
        if (notification_resource->object_id == pdo) 
        {
            notification_resource->lifecycle_state = lifecycle_state;
            notification_resource->drive_process_flag |= pvd_discover_flag;
            fbe_spinlock_unlock(&operation_context->discover_queue_lock);
            if (lifecycle_state == FBE_LIFECYCLE_STATE_READY)
            {
                // signal more work
                fbe_semaphore_release(&operation_context->pvd_operation_thread_semaphore, 0, 1, FALSE);
            }
            return FBE_STATUS_OK;
        }
        notification_resource = (database_pvd_discover_object_t *)fbe_queue_next(&operation_context->discover_queue_head, 
                                                                            (fbe_queue_element_t *)notification_resource);
    }

    fbe_database_drive_discover_get_resource(operation_context, &new_notification_resource);
    if (new_notification_resource != NULL)
    {
        new_notification_resource->object_id = pdo;
        new_notification_resource->lifecycle_state = lifecycle_state;
        new_notification_resource->drive_process_flag |= pvd_discover_flag;
        new_notification_resource->process_timestamp = 0;
        if (notification_resource == NULL)
        {
            // add to the tail
            fbe_queue_push(&operation_context->discover_queue_head, &new_notification_resource->queue_element);
        }
        else
        {
            // add prior to the node which pdo obj id is bigger than us.
            fbe_queue_insert(&new_notification_resource->queue_element, &notification_resource->queue_element);
        }
        status = FBE_STATUS_OK;
        fbe_spinlock_unlock(&operation_context->discover_queue_lock);
        // signal more work
        fbe_semaphore_release(&operation_context->pvd_operation_thread_semaphore, 0, 1, FALSE);
    }
    else
    {
        fbe_spinlock_unlock(&operation_context->discover_queue_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "database_add_drive_to_be_processed out of resource, pdo %d will not be processed\n", pdo);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_database_process_physical_drives
 *****************************************************************
 * @brief
 *   function process PDOs, failed processing will drop that PDO
 *
 * @param  discover_context - connect context for database service
 * @param  fast_connect     - connect PVD to PDO only
 *
 * @return fbe_status_t
 *
 * @version
 *    04/20/2011:   created
 *    01/04/2012:   added processing for system drives
 *    07/26/2012:   added processing PVD under each drive movement type by Gao Jian
 *    09/01/2012:   change the infrastructure of processing drives, where only active side
 *                          does determination and processing and all needed actions are done
 *                          through jobs. Zhipeng Hu |THIS IS CHANGED|
 ****************************************************************/
static fbe_status_t fbe_database_process_physical_drives(database_pvd_operation_t *discover_context, fbe_bool_t fast_connect)
{
    fbe_queue_head_t                                local_pvd_queue_head;
    fbe_queue_head_t                                local_not_connect_queue_head;
    fbe_queue_head_t                                local_completed_queue_head;
    fbe_queue_element_t                             *queue_element;
    fbe_queue_element_t                             *queue_element_next;
    fbe_status_t                                    status;
    fbe_status_t                                    ret_status = FBE_STATUS_OK;
    fbe_database_physical_drive_info_t              pdo_info;
    fbe_physical_drive_mgmt_get_drive_information_t pdo_drive_info;
    fbe_job_service_create_provision_drive_t        *pvd_create_request = NULL;
    fbe_job_service_connect_drive_t                 *pvd_connect_request = NULL;
    fbe_job_service_update_provision_drive_t        *pvd_update_request = NULL;
    fbe_job_service_destroy_provision_drive_t       *pvd_destroy_request = NULL;
    database_pvd_discover_object_t                  *discover_request;
    database_pvd_discover_object_t                  *new_discover_request;
    database_object_entry_t                         *object_table_entry = NULL;
    database_drive_movement_type                    drive_movement_type = INVALID_DRIVE_MOVEMENT;
    event_log_param_selector_t                      event_log_context;
    homewrecker_disk_in_wrong_slot_event_t          disk_in_wrong_slot;
    fbe_u8_t                                        zeroed_serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_u32_t                                       original_slot_number = INVALID_SLOT_NUMBER;
    fbe_object_id_t                                 pvd_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                                      need_connect = FBE_FALSE;
    fbe_bool_t                                      is_in_connect_queue = FBE_FALSE;
    fbe_u32_t                                       num_drive_entries = 0;
    fbe_bool_t                                      need_force_garbage_collection = FBE_FALSE;
    fbe_u32_t                                       pending_drive_connections = 0;
    fbe_bool_t                                      drive_should_not_be_processed = FBE_FALSE;

    fbe_zero_memory(zeroed_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);

    /* initialize event_log_context */
    event_log_context.disk_in_wrong_slot = NULL;
    event_log_context.disordered_slot = NULL;
    event_log_context.integrity_disk_type = NULL;
    event_log_context.normal_boot_with_warning = NULL;
 
    if (discover_context == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "database_process_physical_drives: Invalid param, discover_context == NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    pvd_create_request = (fbe_job_service_create_provision_drive_t *)fbe_transport_allocate_buffer();
    if (pvd_create_request==NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "database_process_physical_drives: failed to get memory for create pvd request\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    pvd_create_request->request_size = 0;

    pvd_update_request = (fbe_job_service_update_provision_drive_t *)fbe_transport_allocate_buffer();
    if (pvd_update_request==NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "database_process_physical_drives: failed to get memory for update pvd request\n");
        fbe_transport_release_buffer(pvd_create_request);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pvd_destroy_request = (fbe_job_service_destroy_provision_drive_t *)fbe_transport_allocate_buffer();
    if (pvd_destroy_request==NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "database_process_physical_drives: failed to get memory for destroy pvd request\n");
        fbe_transport_release_buffer(pvd_create_request);
        fbe_transport_release_buffer(pvd_update_request);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pvd_connect_request = (fbe_job_service_connect_drive_t*)fbe_transport_allocate_buffer();
    if(pvd_connect_request == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "database_process_physical_drives: failed to get memory for connect pvd request\n");
        fbe_transport_release_buffer(pvd_create_request);
        fbe_transport_release_buffer(pvd_update_request);
        fbe_transport_release_buffer(pvd_destroy_request);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    pvd_connect_request->request_size = 0;
    
    fbe_queue_init(&local_pvd_queue_head);
    fbe_queue_init(&local_completed_queue_head);
    fbe_queue_init(&local_not_connect_queue_head);

    // copy the pvds to local queue
    fbe_spinlock_lock(&discover_context->discover_queue_lock);
    while (!(fbe_queue_is_empty(&discover_context->discover_queue_head)))
    {
        /* after we process the existing requests, there may be more queued up */
        queue_element = fbe_queue_front(&discover_context->discover_queue_head);
        while (queue_element!=NULL)
        {
            discover_request = (database_pvd_discover_object_t *)queue_element;
            queue_element_next = fbe_queue_next(&discover_context->discover_queue_head, queue_element);
            // ready to process
            if (discover_request->lifecycle_state == FBE_LIFECYCLE_STATE_READY)
            {
                fbe_queue_remove(queue_element);
                fbe_queue_push(&local_pvd_queue_head, queue_element);
            }
            queue_element = queue_element_next;
        }
        if (fbe_queue_is_empty(&local_pvd_queue_head))
        {
            // no more work
            break;
        }
        
        fbe_spinlock_unlock(&discover_context->discover_queue_lock);
        // let's process them
        while (!(fbe_queue_is_empty(&local_pvd_queue_head)))
        {
            need_connect = FBE_FALSE;
            is_in_connect_queue = FBE_FALSE;
            queue_element = fbe_queue_pop(&local_pvd_queue_head);
            discover_request = (database_pvd_discover_object_t *)queue_element;
            /* remember this resource, to be freed later */
            fbe_queue_push(&local_completed_queue_head, queue_element);

            /* get PDO info */
            status = fbe_database_get_pdo_info(discover_request->object_id, &pdo_info);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "database_process_physical_drives: failed to get pdo info for 0x%x. Skip!\n",
                                discover_request->object_id);
                continue;
            }

            database_trace(FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: Got PDO info successfully: 0x%x\n", 
                            discover_request->object_id);

            if (pdo_info.maintenance_mode_bitmap != 0) 
            { 
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                              "database_process_physical_drives: %d_%d_%d pdo:0x%x is in maintenance mode. Skip!\n", 
                               pdo_info.bus, pdo_info.enclosure, pdo_info.slot, discover_request->object_id); 

                /* Skip connecting PVD to PDO.  Notify PDO that drive is Logically Offline. 
                   Note, after issue is resolved and PVD is subsequently connected, PVD will 
                   clear the logical offline state by setting it online */
                fbe_database_notify_pdo_logically_offline_due_to_maintenance(discover_request->object_id, &pdo_info);
                continue; 
            }

            status = fbe_database_get_pdo_drive_info(discover_request->object_id, &pdo_drive_info);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "database_process_physical_drives: failed to get physical drive info for 0x%x. Skip!\n",
                                discover_request->object_id);                
                continue;
            }
  
            if (fbe_database_verify_and_handle_unsupported_drive(discover_request->object_id, &pdo_drive_info))
            {
                continue;  /*not supported*/
            }

            if (fbe_database_mini_mode())
            {
                fbe_database_slot_type slot_type = FBE_DATABASE_INVALID_SLOT_TYPE;
                /* get slot type */
                status = fbe_database_determine_slot_type(&pdo_info, &slot_type);
                if (slot_type == FBE_DATABASE_USER_SLOT)
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "database_process_physical_drives: %d_%d_%d pdo:0x%x is in user slot. Skip!\n", 
                                   pdo_info.bus, pdo_info.enclosure, pdo_info.slot, discover_request->object_id); 
                    continue;
                }
            }
            
            /* check to see if it's already configured or not */
            pvd_obj_id = FBE_OBJECT_ID_INVALID;
            fbe_spinlock_lock(&discover_context->object_table_ptr->table_lock);            
            status = fbe_database_config_table_get_last_object_entry_by_pvd_SN_or_max_entries(
                                                                        discover_context->object_table_ptr->table_content.object_entry_ptr, 
                                                                        discover_context->object_table_ptr->table_size,
                                                                        pdo_info.serial_num,
                                                                        &object_table_entry,
                                                                        &num_drive_entries /* only valid if object entry not found */);
            if (status != FBE_STATUS_OK)
            {
                fbe_spinlock_unlock(&discover_context->object_table_ptr->table_lock);
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: failed to get pvd by SN, PDO id: 0x%x. Skip this drive!\n", 
                                __FUNCTION__, 
                                discover_request->object_id);
                continue;
            }

            
            if(NULL != object_table_entry)
            {
                pvd_obj_id = object_table_entry->header.object_id;
            }
            else
            {
                if ((num_drive_entries+pending_drive_connections) >= fbe_database_service.logical_objects_limits.platform_fru_count)
                {                    
                    need_force_garbage_collection = FBE_TRUE;

                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "database_process_physical_drives: %d_%d_%d pdo:0x%x exceeded db slot limit of %d.\n", 
                                   pdo_info.bus, pdo_info.enclosure, pdo_info.slot, discover_request->object_id, fbe_database_service.logical_objects_limits.platform_fru_count); 
                    drive_should_not_be_processed = FBE_TRUE;
                } 
                else if((pdo_info.block_geometry == FBE_BLOCK_EDGE_GEOMETRY_4160_4160) &&
                        (!fbe_database_is_4k_drive_committed()))
                {   
                    /* This is a 4K drive but the code is not committed */
                    database_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "database_process_physical_drives: 4K uncommited %d_%d_%d pdo:0x%x \n", 
                                   pdo_info.bus, pdo_info.enclosure, pdo_info.slot, discover_request->object_id); 
                    drive_should_not_be_processed = FBE_TRUE;
                }
                else
                {    
                    drive_should_not_be_processed = FBE_FALSE;
                }
                if(drive_should_not_be_processed)
                {
                    // No resource to connect until destroy_request completes.  Put it back on queue so it can be retried.                    
                    fbe_queue_remove(queue_element);
                    fbe_queue_push(&local_not_connect_queue_head, queue_element);                                      


                    /* retry the connection. we need to return not ok status so that connect thread will take a break */
                    ret_status = FBE_STATUS_NO_OBJECT;

                    fbe_spinlock_unlock(&discover_context->object_table_ptr->table_lock);
                    continue;
                }
            }

            fbe_spinlock_unlock(&discover_context->object_table_ptr->table_lock);
            /*if found an PVD, check whether it is connected or is going to be connected*/
            if(FBE_OBJECT_ID_INVALID != pvd_obj_id)
            {
                    fbe_bool_t  is_connected_to_drive = FBE_FALSE;
                    
                    /*Only process drive that is not in the connect queue or is not already connected
                     *to a drive. This will save a lot of CMI communications between two SPs and
                     *also save the effort to read fru signature from drive*/

                    /*1) Is the drive already in the connect queue?*/
                    is_in_connect_queue = fbe_database_is_pvd_to_be_connected(discover_context, pvd_obj_id);

                    /* we will defer this to the connect queue process if we are not on fast track */
                    if ((FBE_TRUE == is_in_connect_queue) && ((FBE_FALSE == fast_connect)))
                    {
                        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        "db_process_physical_drive: drive %d_%d_%d is already going to connect PVD 0x%x. Skip it\n", 
                                        pdo_info.bus, pdo_info.enclosure, pdo_info.slot, pvd_obj_id);
                        continue;
                    }

                    /*2) Is pvd already connected to a drive?*/
                    status = fbe_database_is_pvd_connected_to_drive(pvd_obj_id, &is_connected_to_drive);
                    if (FBE_STATUS_OK != status)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "db_process_physical_drive: failed to determine PVD 0x%x connect state. Skip it\n", 
                                        pvd_obj_id);
                        continue;
                    }

                    if(FBE_TRUE == is_connected_to_drive)
                    {
                        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        "db_process_physical_drive: drive %d_%d_%d already connects PVD 0x%x. Skip it\n", 
                                        pdo_info.bus, pdo_info.enclosure, pdo_info.slot, pvd_obj_id);
                        continue;
                    }
            }

            /* if it's known to our database, we always connect it right away.  
             * This applys to both active and passive side.
             */
            if (object_table_entry != NULL)
            {
                fbe_database_slot_type slot_type = FBE_DATABASE_INVALID_SLOT_TYPE;
                fbe_u32_t index;

                /* get slot type */
                status = fbe_database_determine_slot_type(&pdo_info, &slot_type);
                if (FBE_STATUS_OK != status)
                {
                    database_trace (FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "db_process_physical_drive: get slot type failed, fast connect skip drive %d_%d_%d.\n",
                                    pdo_info.bus, pdo_info.enclosure, pdo_info.slot);
                    continue;
                }

                if (slot_type == FBE_DATABASE_DB_SLOT)
                {
                    /* check whether it is one of the system drives */
                    fbe_spinlock_lock(&discover_context->system_fru_descriptor_lock);
                    for (index = 0; index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; index++)
                    {
                        if (!fbe_compare_string(pdo_info.serial_num, 
                                                FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                                discover_context->system_fru_descriptor.system_drive_serial_number[index],
                                                FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                                FBE_TRUE))
                        {
                            if (slot_type == FBE_DATABASE_DB_SLOT && pdo_info.slot == index)
                            {
                                database_trace (FBE_TRACE_LEVEL_WARNING,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "db_process_physical_drive: connect existing system drive %d_%d_%d. sn %s\n",
                                                pdo_info.bus, pdo_info.enclosure, pdo_info.slot, pdo_info.serial_num);
                                need_connect = FBE_TRUE;                              
                            }
                            break;
                        }
                    }
                    fbe_spinlock_unlock(&discover_context->system_fru_descriptor_lock);
                }
                else
                {
                    /* for user slot, we need to make sure it's not system drive */
                    if (!fbe_private_space_layout_object_id_is_system_pvd(pvd_obj_id))
                    {
                        need_connect = FBE_TRUE;
                    }
                }

                if (need_connect == FBE_TRUE)
                {
                    status = fbe_database_connect_pvd_to_pdo_info(pvd_obj_id, pdo_info.drive_object_id, &pdo_info);
                    if (FBE_STATUS_OK != status)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                       "db_process_physical_drive: failed to connect PVD 0x%x to PDO 0x%x. Skip it\n", 
                                       pvd_obj_id, pdo_info.drive_object_id);
                    }
                    continue;
                }
            }

            /* we don't want to deal with creating new objects if fast_connect is set */
            if (fast_connect == FBE_TRUE)
            {
                /* we only need to save this for later if it's not in the connect queue.
                 * if it's in connect queue, it will find out that path state is already enabled
                 * and simply skip it.  Thus, we don't have to remove it from the queue after 
                 * connect the edge here.
                 */
                if ((need_connect == FBE_FALSE) && (is_in_connect_queue == FBE_FALSE))
                {
                    /* save this for later */
                    fbe_queue_remove(queue_element);
                    fbe_queue_push(&local_not_connect_queue_head, queue_element);
                }
                continue;
            }
            
            /* don't create drive on passive side, why? */
            if(!database_common_cmi_is_active())
            {
                continue;
            }
            /* Below pieces of code for PVD validation */
            /* drive movement type is the base of that PVD initialization */
            status = fbe_database_determine_drive_movement_type(discover_context, &pdo_info, &drive_movement_type);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: failed to determine drive movement type for PDO: 0x%x (%d_%d_%d). Skip this drive!\n",
                                discover_request->object_id, pdo_info.bus, pdo_info.enclosure, pdo_info.slot);
                continue;
            }
            else{
                database_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: Drive is in %d_%d_%d\n",
                                pdo_info.bus, pdo_info.enclosure, pdo_info.slot);
                database_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: Drive movement type: 0x%x\n", 
                                drive_movement_type);
            }


            /* Do object topology modification based on drive movement type
              *  before initialize that PVD
              */
            if (INSERT_A_NEW_DRIVE_TO_A_USER_SLOT == drive_movement_type ||
                   INSERT_OTHER_ARRAY_USER_DRIVE_TO_A_USER_SLOT == drive_movement_type)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is a new drive or other array user drive, it is in user slot.\n", 
                                discover_request->object_id);
                                
                status = fbe_database_add_item_into_create_pvd_request_PVD_list(pvd_create_request, &pdo_info);
                if (FBE_STATUS_OK != status)
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: failed to add the PDO 0x%x into pvd create request. Skip this drive!\n", 
                                    __FUNCTION__, 
                                    discover_request->object_id);
                    continue;
                }                                
                pending_drive_connections++;

                /* send the job if the create request PVD list is full
                  * else accumulate more PVD create item into list to send
                  */
                if (pvd_create_request->request_size >= 
                      FBE_JOB_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS)
                {
                    status = fbe_database_send_create_pvd_request(pvd_create_request);
                    if (FBE_STATUS_OK != status)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: failed to send the pvd create request for PDO 0x%x. Skip this drive!\n", 
                                        __FUNCTION__, 
                                        discover_request->object_id);                        
                    }
                    
                    /*reset create pvd request*/
                    pvd_create_request->request_size = 0;
                }
                
                continue;
            }
            else if (INSERT_A_NEW_DRIVE_TO_A_DB_SLOT == drive_movement_type ||
                            INSERT_OTHER_ARRAY_USER_DRIVE_TO_A_DB_SLOT == drive_movement_type)
            {                
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is a new drive or other array user drive, it is in system slot.\n", 
                                discover_request->object_id);

                /*skip drive connect if database in mini mode*/
                if (fbe_database_mini_mode())
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "db_process_physical_drive: database in mini mode, skip pdo 0x%x.\n",
                               discover_request->object_id);
                    continue;
                }    

                if (fbe_database_skip_connect_drive_tier(pdo_info.drive_object_id)) 
                {
                    fbe_database_shoot_drive(pdo_info.drive_object_id);
                    fbe_database_send_kill_drive_request_to_peer(pdo_info.drive_object_id);
                    continue;
                } 

                /* Return the element to discover_queue_head and set the timestamp.
                 * If the reinit system PVD job failed, a new reinit job will be started.
                 * */
                fbe_queue_remove(queue_element);
                fbe_queue_push(&local_not_connect_queue_head, queue_element);
                /* Only start the reinit system pvd job after 120 seconds*/
                if (fbe_get_elapsed_seconds(discover_request->process_timestamp) <= SECONDS_TO_RESTART_REINIT_SYSTEM_PVD_JOB)
                    continue;
                discover_request->process_timestamp = fbe_get_time();

                status = fbe_database_reinitialize_system_pvd_via_job(pvd_update_request, 
                                                                    &pdo_info);
                if (FBE_STATUS_OK != status)
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: failed to reinitialize system pvd for PDO 0x%x. Skip this drive!\n", 
                        __FUNCTION__, 
                        discover_request->object_id);
                    continue;
                }

                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "db_process_physical_drive: PVD reinit job 0x%x is sent for PVD 0x%x\n", 
                                (unsigned int)pvd_update_request->job_number, pvd_update_request->object_id);
                
                /* success */
                continue;
            }
            else if (INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_A_USER_SLOT == drive_movement_type)
            {                
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is current array unbind user drive, it is in user slot.\n", 
                                discover_request->object_id);

                // if we found an entry
                if (NULL != object_table_entry)
                {
                    status = fbe_database_connect_pvd_to_pdo_info(pvd_obj_id, pdo_info.drive_object_id, &pdo_info);
                    if (FBE_STATUS_OK != status)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "db_process_physical_drive: failed to connect PVD 0x%x to PDO 0x%x. Skip it\n", 
                                        pvd_obj_id, pdo_info.drive_object_id);
                        continue;
                    }
                    
                }
                
                /* success */
                continue;
            }
            else if (INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_A_DB_SLOT == drive_movement_type)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is current array unbind drive, it is in system slot.\n", 
                                discover_request->object_id);

                /*skip pdo connect if database in mini mode*/
                if (fbe_database_mini_mode())
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "db_process_physical_drive: database in mini mode, skip pdo 0x%x.\n",
                               discover_request->object_id);
                    continue;
                }

                if (fbe_database_skip_connect_drive_tier(pdo_info.drive_object_id)) 
                {
                    fbe_database_shoot_drive(pdo_info.drive_object_id);
                    fbe_database_send_kill_drive_request_to_peer(pdo_info.drive_object_id);
                    continue;
                } 
                
                /* Return the element to discover_queue_head and set the timestamp.
                 * If the reinit system PVD job failed, a new reinit job will be started.
                 * */
                fbe_queue_remove(queue_element);
                fbe_queue_push(&local_not_connect_queue_head, queue_element);
                /* Only start the reinit system pvd job after 120 seconds*/
                if (fbe_get_elapsed_seconds(discover_request->process_timestamp) <= SECONDS_TO_RESTART_REINIT_SYSTEM_PVD_JOB)
                    continue;
                discover_request->process_timestamp = fbe_get_time();
                
                /* Merge the destroy original user PVD logic into reinitialize system PVD job */
                status = fbe_database_reinitialize_system_pvd_via_job(pvd_update_request, &pdo_info);
                if (FBE_STATUS_OK != status)
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: failed to reinitialize system pvd via job for PDO object 0x%x\n",
                                    __FUNCTION__,
                                    pdo_info.drive_object_id);
                    continue;
                }

                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: successfully send reinit PVD job for PVD 0x%x\n",
                                __FUNCTION__, pvd_update_request->object_id);
                /* success */
                continue;
            }
            else if (INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_A_USER_SLOT == drive_movement_type)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is current array bind user drive, it is in user slot.\n", 
                                discover_request->object_id);
                
                   // if we found an entry
                   if (NULL != object_table_entry)
                   {
                        status = fbe_database_connect_pvd_to_pdo_info(pvd_obj_id, pdo_info.drive_object_id, &pdo_info);
                        if (FBE_STATUS_OK != status)
                        {
                            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                           "db_process_physical_drive: failed to connect PVD 0x%x to PDO 0x%x. Skip it\n", 
                                            pvd_obj_id, pdo_info.drive_object_id);
                            continue;
                        }
                       
                   }

                /* success */
                continue;
            }
            else if (INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_A_DB_SLOT == drive_movement_type)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is current array bind user drive, it is in system slot.\n", 
                                discover_request->object_id);
                
                if (discover_request->drive_process_flag == DATABASE_PVD_DISCOVER_FLAG_NORMAL_PROCESS)
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s, A user drive  PDO: 0x%x  bound into at least one RG gets inserted into DB slot: bus %d, enc %d, slot %d, does not make this drive online!\n", 
                                     __FUNCTION__,
                                     pdo_info.drive_object_id,
                                     pdo_info.bus,
                                     pdo_info.enclosure,
                                     pdo_info.slot);
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s, Please insert this drive back into a user slot, any slot except first 4 slots.\n", 
                                    __FUNCTION__);
                    
                    if (object_table_entry != NULL)
                        disk_in_wrong_slot.disk_pvd_id = pvd_obj_id;
                    else
                        disk_in_wrong_slot.disk_pvd_id = FBE_OBJECT_ID_INVALID;
                    disk_in_wrong_slot.bus = pdo_info.bus;
                    disk_in_wrong_slot.enclosure = pdo_info.enclosure;
                    disk_in_wrong_slot.slot = pdo_info.slot;
                    disk_in_wrong_slot.its_correct_slot = INVALID_SLOT_NUMBER;                  
                    event_log_context.disk_in_wrong_slot = &disk_in_wrong_slot;
                    
                    status = database_homewrecker_send_event_log(SEP_ERROR_CODE_BIND_USER_DRIVE_IN_SYSTEM_SLOT,
                                                                                                                         &event_log_context);
                    if (FBE_STATUS_OK != status )
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, Send event log failed, BIND_USER_DRIVE_IN_SYSTEM_SLOT", 
                                        __FUNCTION__);
                    }
                    
                continue;
                }
                else if (discover_request->drive_process_flag == DATABASE_PVD_DISCOVER_FLAG_HOMEWRECKER_FORECE_ONLINE)
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: See force online flag on drive, force a drive in %d_%d_%d online when Homewrecker think it should not be\n",
                                 __FUNCTION__,
                                 pdo_info.bus,
                                 pdo_info.enclosure,
                                 pdo_info.slot);

                    /*skip pdo connect if database in mini mode*/
                    if (fbe_database_mini_mode())
                    {
                        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                   "db_process_physical_drive: database in mini mode, skip force online flag.\n");
                        continue;
                    }
                    
                    /* Return the element to discover_queue_head and set the timestamp.
                     * If the reinit system PVD job failed, a new reinit job will be started.
                     * */
                    fbe_queue_remove(queue_element);
                    fbe_queue_push(&local_not_connect_queue_head, queue_element);
                    /* Only start the reinit system pvd job after 120 seconds*/
                    if (fbe_get_elapsed_seconds(discover_request->process_timestamp) <= SECONDS_TO_RESTART_REINIT_SYSTEM_PVD_JOB)
                        continue;
                    discover_request->process_timestamp = fbe_get_time();

                    /* Merge the destroy original user PVD logic into reinitialize system PVD job */
                    status = fbe_database_reinitialize_system_pvd_via_job(pvd_update_request, &pdo_info);
                    if (FBE_STATUS_OK != status)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: failed to add reinitialize system pvd via job for PDO object 0x%x\n",
                                        __FUNCTION__,
                                        pdo_info.drive_object_id);
                    }
                    else
                    {
                        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        "db_process_physical_drive: successfully send PVD reinit job 0x%x for PVD 0x%x\n",
                                        (unsigned int)pvd_update_request->job_number, pvd_update_request->object_id);
                    }
                }
                continue;
            }
            else if (INSERT_OTHER_ARRAY_DB_DRIVE_TO_A_USER_SLOT == drive_movement_type)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is other array system drive, it is in user slot.\n", 
                                discover_request->object_id);
                
                status = fbe_database_add_item_into_create_pvd_request_PVD_list(pvd_create_request, &pdo_info);
                if (FBE_STATUS_OK != status)
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: failed to add the PDO 0x%x into pvd create request. Skip this drive!\n", 
                                    __FUNCTION__, 
                                    discover_request->object_id);
                    continue;
                }                
                pending_drive_connections++;

                /* send the job if the create request PVD list is full
                  * else accumulate more PVD create item into list to send
                  */
                if (pvd_create_request->request_size >= 
                      FBE_JOB_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS)
                {
                    status = fbe_database_send_create_pvd_request(pvd_create_request);
                    if (FBE_STATUS_OK != status)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: failed to send the pvd create request after adding PDO 0x%x into create request!\n", 
                                        __FUNCTION__, 
                                        discover_request->object_id);                        
                    }
                     /*reset create pvd request*/
                    pvd_create_request->request_size = 0;
                }
                /* success */
                continue;
            }
            else if (INSERT_OTHER_ARRAY_DB_DRIVE_TO_A_DB_SLOT == drive_movement_type)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is other array system drive, it is in system slot.\n", 
                                discover_request->object_id);

                /*skip pdo connect if database in mini mode*/
                if (fbe_database_mini_mode())
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "db_process_physical_drive: database in mini mode, skip pdo 0x%x.\n",
                               discover_request->object_id);
                    continue;
                }    

                if (fbe_database_skip_connect_drive_tier(pdo_info.drive_object_id)) 
                {
                    fbe_database_shoot_drive(pdo_info.drive_object_id);
                    fbe_database_send_kill_drive_request_to_peer(pdo_info.drive_object_id);
                    continue;
                } 

                /* Return the element to discover_queue_head and set the timestamp.
                 * If the reinit system PVD job failed, a new reinit job will be started.
                 * */
                fbe_queue_remove(queue_element);
                fbe_queue_push(&local_not_connect_queue_head, queue_element);
                /* Only start the reinit system pvd job after 120 seconds*/
                if (fbe_get_elapsed_seconds(discover_request->process_timestamp) <= SECONDS_TO_RESTART_REINIT_SYSTEM_PVD_JOB)
                    continue;
                discover_request->process_timestamp = fbe_get_time();
                status = fbe_database_reinitialize_system_pvd_via_job(pvd_update_request, 
                                                                                                                                &pdo_info);
                if (FBE_STATUS_OK != status)
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: failed to reinitialize system pvd for PDO 0x%x. Skip this drive!\n", 
                                    __FUNCTION__, 
                                    discover_request->object_id);
                }
                else
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "db_process_physical_drive: successfully send PVD reinit job 0x%x for PVD 0x%x\n", 
                                    (unsigned int)pvd_update_request->job_number, pvd_update_request->object_id);
                }
                /* success */            
                continue;
            }
            else if (INSERT_CURRENT_ARRAY_DB_DRIVE_TO_ITS_ORIGINAL_SLOT == drive_movement_type)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is current array system drive, it is in its original slot.\n", 
                                discover_request->object_id);
                
                   // if we found an entry
                   if (NULL != object_table_entry)
                   {
                        status = fbe_database_connect_pvd_to_pdo_info(pvd_obj_id, pdo_info.drive_object_id, &pdo_info);
                        if (FBE_STATUS_OK != status)
                        {
                            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                            "db_process_physical_drive: failed to connect PVD 0x%x to PDO 0x%x. Skip it\n", 
                                            pvd_obj_id, pdo_info.drive_object_id);
                            continue;
                        }
                       
                   }

                /* success */                
                continue;
            }
            else if (INSERT_CURRENT_ARRAY_DB_DRIVE_TO_A_USER_SLOT == drive_movement_type)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is current array system drive, it is in a user slot.\n", 
                                discover_request->object_id);
                
                if (discover_request->drive_process_flag == DATABASE_PVD_DISCOVER_FLAG_NORMAL_PROCESS)
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s, A system drive  PDO: 0x%x  gets inserted into user slot: bus %d, enc %d, slot %d, does not make this drive online!!!\n", 
                                 __FUNCTION__,
                                 pdo_info.drive_object_id,
                                 pdo_info.bus,
                                 pdo_info.enclosure,
                                 pdo_info.slot);
                    
                    status = fbe_database_find_original_slot_for_original_system_drive(discover_context, &pdo_info,
                                                                                                                                                              &original_slot_number);
                    if (FBE_STATUS_OK != status || original_slot_number == INVALID_SLOT_NUMBER)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: The drive is not original system drive, unknown error!!\n",
                                        __FUNCTION__);
                        continue;
                    }
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s, Please move this drive to bus: 0 enc: 0 slot: %d.\n", 
                                    __FUNCTION__,
                                    original_slot_number);

                    
                    if (object_table_entry != NULL)
                        disk_in_wrong_slot.disk_pvd_id = pvd_obj_id;
                    else
                        disk_in_wrong_slot.disk_pvd_id = FBE_OBJECT_ID_INVALID;
                    disk_in_wrong_slot.bus = pdo_info.bus;
                    disk_in_wrong_slot.enclosure = pdo_info.enclosure;
                    disk_in_wrong_slot.slot = pdo_info.slot;
                    disk_in_wrong_slot.its_correct_bus = 0;
                    disk_in_wrong_slot.its_correct_encl = 0;
                    disk_in_wrong_slot.its_correct_slot = original_slot_number;
                    event_log_context.disk_in_wrong_slot = &disk_in_wrong_slot;
                    
                    status = database_homewrecker_send_event_log(SEP_ERROR_CODE_SYSTEM_DRIVE_IN_WRONG_SLOT,
                                                                                                                        &event_log_context);
                    if (FBE_STATUS_OK != status )
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, Send event log failed, SYSTEM_DRIVE_IN_WRONG_SLOT", 
                                        __FUNCTION__);
                    }
                    
                    continue;
                }
                else if (discover_request->drive_process_flag == DATABASE_PVD_DISCOVER_FLAG_HOMEWRECKER_FORECE_ONLINE)
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: See force online flag on drive, then force a drive in bus: %d, enc: %d, slot: %d online when Homewrecker think it should not be.\n",
                                    __FUNCTION__,
                                    pdo_info.bus,
                                    pdo_info.enclosure,
                                    pdo_info.slot);
                    
                    status = fbe_database_find_original_slot_for_original_system_drive(discover_context, &pdo_info,
                                                                                                                                                              &original_slot_number);
                    if (FBE_STATUS_OK != status || original_slot_number == INVALID_SLOT_NUMBER)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                        "%s: The drive is not original system drive, unknown error!!\n",
                                                        __FUNCTION__);
                        continue;
                    }
                    
                    /* zeroed the drive's original system PVD serial number */
                    status = fbe_database_update_pvd_sn_via_job(pvd_update_request, 
                                                               original_slot_number + FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, 
                                                               zeroed_serial_num);
                    if (FBE_STATUS_OK != status)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: Fail to update system pvd: 0x%x serial number\n",
                                        __FUNCTION__,
                                        pvd_obj_id);
                        continue;
                    }

                    status = fbe_database_add_item_into_create_pvd_request_PVD_list(pvd_create_request, &pdo_info);
                    if (FBE_STATUS_OK != status)
                    {
                        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: failed to add the PDO 0x%x into pvd create request. Skip this drive!\n", 
                                        __FUNCTION__, 
                                        discover_request->object_id);
                        continue;
                    }                    
                    pending_drive_connections++;

                    /* send the job if the create request PVD list is full
                      * else accumulate more PVD create item into list to send
                      */
                    if (pvd_create_request->request_size >= 
                         FBE_JOB_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS)
                    {
                        status = fbe_database_send_create_pvd_request(pvd_create_request);
                        if (FBE_STATUS_OK != status)
                        {
                            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: failed to send the pvd create request after adding PDO 0x%x into request. Skip this drive!\n", 
                                            __FUNCTION__, 
                                            discover_request->object_id);                            
                        }
                    
                        /*reset create pvd request*/
                        pvd_create_request->request_size = 0;
                    }                    
                }
                continue;
            }
            else if (INSERT_CURRENT_ARRAY_DB_DRIVE_TO_ANOTHER_DB_SLOT == drive_movement_type)
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: the PDO 0x%x is current array system drive, it is in another system slot.\n", 
                                discover_request->object_id);
                
                database_trace(FBE_TRACE_LEVEL_INFO, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, A system drive  PDO 0x%x  moved to another system slot: bus %d, enc %d, slot %d, does not make this drive online!\n", 
                               __FUNCTION__,
                               pdo_info.drive_object_id,
                               pdo_info.bus,
                               pdo_info.enclosure,
                               pdo_info.slot);

                database_trace(FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, This situation could NOT be fixed by setting drive froce online flag. Move drive back to its original slot is the only way!!\n",
                               __FUNCTION__);
                
                status = fbe_database_find_original_slot_for_original_system_drive(discover_context, &pdo_info, &original_slot_number);
                if (FBE_STATUS_OK != status || original_slot_number == INVALID_SLOT_NUMBER)
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: The drive is not original system drive, unknown error!!\n",
                                   __FUNCTION__);
                    continue;
                }
                
                database_trace(FBE_TRACE_LEVEL_INFO, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, Please move this drive to bus: 0 enc: 0 slot: %d.\n", 
                               __FUNCTION__,
                               original_slot_number);
                    
                if (object_table_entry != NULL)
                {
                    disk_in_wrong_slot.disk_pvd_id = pvd_obj_id;
                }
                else
                {
                    disk_in_wrong_slot.disk_pvd_id = FBE_OBJECT_ID_INVALID;
                }

                disk_in_wrong_slot.bus = pdo_info.bus;
                disk_in_wrong_slot.enclosure = pdo_info.enclosure;
                disk_in_wrong_slot.slot = pdo_info.slot;
                disk_in_wrong_slot.its_correct_bus = 0;
                disk_in_wrong_slot.its_correct_encl = 0;
                disk_in_wrong_slot.its_correct_slot = original_slot_number;
                event_log_context.disk_in_wrong_slot = &disk_in_wrong_slot;

                status = database_homewrecker_send_event_log(SEP_ERROR_CODE_SYSTEM_DRIVE_IN_WRONG_SLOT,
                                                             &event_log_context);
                if (FBE_STATUS_OK != status )
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s, Send event log failed, SYSTEM_DRIVE_IN_WRONG_SLOT", 
                                   __FUNCTION__);
                }        
                continue;
            }
            else
            {
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s:Invalid drive movement type or this drive is not necessary to be processed on passive side. TYPE:0x%x. Skip!\n", 
                               __FUNCTION__, 
                               drive_movement_type);
                continue;
            }
        }  /* finish processing local queue */

        /* we need to lock the queue again before checking if there's more request */
        fbe_spinlock_lock(&discover_context->discover_queue_lock);

    }
    
    /* release the notification resource*/
    fbe_database_drive_discover_release_resource(discover_context, &local_completed_queue_head);

    if (!(fbe_queue_is_empty(&local_not_connect_queue_head)))
    {
        new_discover_request = (database_pvd_discover_object_t *)fbe_queue_pop(&local_not_connect_queue_head);
        discover_request = (database_pvd_discover_object_t *)fbe_queue_front(&discover_context->discover_queue_head);
        while (new_discover_request!=NULL)
        {
            while (discover_request!=NULL)
            {
                if (discover_request->object_id >= new_discover_request->object_id)
                {
                    break;  // we are inserting here.
                }

                discover_request = (database_pvd_discover_object_t *)fbe_queue_next(&discover_context->discover_queue_head, 
                                                                                    (fbe_queue_element_t *)discover_request);
            }
            if (discover_request==NULL)
            {
                database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                "database discover save pdo: 0x%x\n", 
                                new_discover_request->object_id);
                fbe_queue_push(&discover_context->discover_queue_head, &new_discover_request->queue_element);
                /* we don't have to search from the beginning */
                discover_request = new_discover_request;
            }
            else
            {
                /* we don't have to requeue it if it's in the queue already */
                if (discover_request->object_id > new_discover_request->object_id)
                {
                    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                "database discover save pdo: 0x%x before 0x%x\n", 
                                new_discover_request->object_id, discover_request->object_id);
                    // make sure the queue is properly sorted
                    fbe_queue_insert(&new_discover_request->queue_element, &discover_request->queue_element);
                }
                else
                {
                    // release the resource
                    fbe_queue_push(&discover_context->free_entry_queue_head, &new_discover_request->queue_element);
                }
                /* we will search again from the last found node */
            }
            new_discover_request = (database_pvd_discover_object_t *)fbe_queue_pop(&local_not_connect_queue_head);

        }
        ret_status = FBE_STATUS_NO_OBJECT;  /* non-ok status will tell the caller to take a break */
    }

    fbe_spinlock_unlock(&discover_context->discover_queue_lock);

    /* at the very end, if there's more items in pvd request need to be processed */
    if (pvd_create_request->request_size > 0 && database_common_cmi_is_active())
    {
            status = fbe_database_send_create_pvd_request(pvd_create_request);
            if (status != FBE_STATUS_OK) 
            {
                database_trace(FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "%s: send create pvd request failed.  Error 0x%x\n", 
                               __FUNCTION__, 
                               status);
            }
    }

    /* at the very end, if there's more items in pvd request need to be processed */
    if (pvd_connect_request->request_size > 0 && database_common_cmi_is_active())
    {
            status = fbe_database_send_connect_pvd_request(pvd_connect_request);
            if (status != FBE_STATUS_OK) 
            {
                database_trace(FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "%s: send connect pvd request failed.  Error 0x%x\n", 
                               __FUNCTION__, 
                               status);
            }
    }


    /* Force garbage collection to allow more PVDs to be created */
    if (need_force_garbage_collection)
    {
        fbe_time_t current_time = fbe_get_time();

        if (current_time-last_force_garbage_collection_time >= fbe_database_service.forced_garbage_collection_debounce_timeout)
        {    
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: forcing pvd garbage collection. current:0x%llx\n", __FUNCTION__, current_time);

            last_force_garbage_collection_time = current_time;

            fbe_database_destroy_pvd_via_job(pvd_destroy_request, FBE_DATABASE_TRANSACTION_DESTROY_UNCONSUMED_PVDS_ID);            
        }
        else
        {
             database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO, 
                            "db_process_physical_drive: garbage debounce. last:0x%llx current:0x%llx next:0x%llx\n", 
                            last_force_garbage_collection_time, current_time, 
                            last_force_garbage_collection_time + fbe_database_service.forced_garbage_collection_debounce_timeout);
        }
    }


    fbe_transport_release_buffer(pvd_create_request);
    fbe_transport_release_buffer(pvd_update_request);
    fbe_transport_release_buffer(pvd_destroy_request);
    fbe_transport_release_buffer(pvd_connect_request);
    return ret_status;
}

fbe_bool_t fbe_database_is_pvd_to_be_connected(database_pvd_operation_t *connect_context, fbe_object_id_t pvd_object_id)
{
    fbe_bool_t  in_queue = FBE_FALSE;
    database_pvd_discover_object_t *notification_resource = NULL;

    /* search for duplicate from the queue */
    fbe_spinlock_lock(&connect_context->discover_queue_lock);
    notification_resource = (database_pvd_discover_object_t *)fbe_queue_front(&connect_context->connect_queue_head);
    while (notification_resource!=NULL)
    {
        if (notification_resource->pvd_obj_id == pvd_object_id)
        {
            in_queue = FBE_TRUE;
            break;  // we are inserting here.
        }
        notification_resource = (database_pvd_discover_object_t *)fbe_queue_next(&connect_context->connect_queue_head, 
                                                                                (fbe_queue_element_t *)notification_resource);
    }
    fbe_spinlock_unlock(&connect_context->discover_queue_lock);

    return (in_queue);
}

/*!***************************************************************
 * @fn fbe_database_add_pvd_to_be_connected
 *****************************************************************
 * @brief
 *   adds pvd to queue waiting to be picked up, only if
 * it's not in the queue yet.
 *
 * @param  connect_context - connect context for database service
 * @param  pvd_object_id
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/20/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_add_pvd_to_be_connected(database_pvd_operation_t *connect_context, fbe_object_id_t pvd_object_id)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;
    database_pvd_discover_object_t *notification_resource = NULL;
    database_pvd_discover_object_t *new_notification_resource = NULL;

    /* search for duplicate from the queue */
    fbe_spinlock_lock(&connect_context->discover_queue_lock);
    notification_resource = (database_pvd_discover_object_t *)fbe_queue_front(&connect_context->connect_queue_head);
    while (notification_resource!=NULL)
    {
        if (notification_resource->pvd_obj_id >= pvd_object_id)
        {
            break;  // we are inserting here.
        }
        notification_resource = (database_pvd_discover_object_t *)fbe_queue_next(&connect_context->connect_queue_head, 
                                                                                (fbe_queue_element_t *)notification_resource);
    }

    if ((notification_resource==NULL) ||
        (notification_resource->pvd_obj_id > pvd_object_id))
    {
        fbe_database_drive_discover_get_resource(connect_context, &new_notification_resource);
        if (new_notification_resource != NULL)
        {
            new_notification_resource->pvd_obj_id = pvd_object_id;
            if (notification_resource == NULL)
            {
                // add to the tail
                fbe_queue_push(&connect_context->connect_queue_head, &new_notification_resource->queue_element);
            }
            else
            {
                // add prior to the node which pdo obj id is bigger than us.
                fbe_queue_insert(&new_notification_resource->queue_element, &notification_resource->queue_element);
            }
            status = FBE_STATUS_OK;
            // signal more work
            /* TODO: Change the following to debug level */
            database_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                "%s: Add pvd 0x%x to connect_q.\n", 
                __FUNCTION__, 
                pvd_object_id);
            // signal more work
            fbe_semaphore_release(&connect_context->pvd_operation_thread_semaphore, 0, 1, FALSE);
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "database_add_drive_to_be_connected out of resource, pvd %d will not be processed\n", pvd_object_id);
            status = FBE_STATUS_GENERIC_FAILURE;
        }

    }
    fbe_spinlock_unlock(&connect_context->discover_queue_lock);

    return status;
}

/*!***************************************************************
 * @fn fbe_database_connect_pvds
 *****************************************************************
 * @brief
 *   function performs edge connect from PVD to PDO.  If the connect
 * failed, the same entry will be put back to the queue for retry later.
 *
 * @param  connect_context - connect context for database service
 *
 * @return fbe_status_t -  not OK indicates something being rescheduled
 *
 * @version
 *    04/20/2011:   created
 *    01/05/2012:   added system drive processing
 *    04/05/2012:   midofied by zhangy26. Add MD raw mirror rebuild thread wake up
 *    07/24/2012:   stamp fru signature to user disk by Jian Gao
 ****************************************************************/
static fbe_status_t fbe_database_connect_pvds(database_pvd_operation_t *connect_context)
{
    fbe_queue_head_t                                local_pvd_queue_head;
    fbe_queue_head_t                                local_not_completed_pvd_queue_head;
    fbe_queue_head_t                                local_completed_pvd_queue_head;
    fbe_queue_element_t                             *queue_element;
    database_object_entry_t                         *object_entry;
    fbe_status_t                                    status;
    fbe_status_t                                    ret_status = FBE_STATUS_OK;
    fbe_class_id_t                                  fbe_class_id;
    fbe_object_id_t                                 pvd_object_id;
    fbe_u8_t                                        serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_database_physical_drive_info_t              pdo_info;
    fbe_base_config_downstream_edge_list_t          down_edges;
    fbe_path_state_t                                path_state = FBE_PATH_STATE_ENABLED;
    fbe_fru_signature_t                         fru_signature;
    fbe_database_fru_signature_IO_operation_status_t    fru_signature_operation_status;
    database_pvd_discover_object_t                  *connect_request;
    database_pvd_discover_object_t                  *new_connect_request;

    fbe_queue_init(&local_pvd_queue_head);
    fbe_queue_init(&local_not_completed_pvd_queue_head);
    fbe_queue_init(&local_completed_pvd_queue_head);

    // copy the pvds to local queue
    fbe_spinlock_lock(&connect_context->discover_queue_lock);
    fbe_queue_merge(&local_pvd_queue_head, &connect_context->connect_queue_head);
    fbe_spinlock_unlock(&connect_context->discover_queue_lock);

    // let's process them.  We still need to lock to protect the entry itself.
    while (!(fbe_queue_is_empty(&local_pvd_queue_head)))
    {
        /* TODO: this function can get stuck here during system shutdown.  Needs a way to quit after retries or somethign like that. */
        queue_element = fbe_queue_pop(&local_pvd_queue_head);
        connect_request = (database_pvd_discover_object_t *)queue_element;
        // save information

        fbe_spinlock_lock(&connect_context->object_table_ptr->table_lock);
        status = fbe_database_config_table_get_object_entry_by_id(connect_context->object_table_ptr,
                                                                  connect_request->pvd_obj_id,
                                                                  &object_entry);
        /* AR 690469 One SP is trying to connect PVD to PDO, meantime, the other SP is destroying PVD. 
          * Caused the SP lost the PVD entry which is under connecting. So the SP who was doing PVD 
          * connection thought it was in critical bad situation and panic the array.*/
        if (object_entry->header.state == DATABASE_CONFIG_ENTRY_INVALID)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                                                    FBE_TRACE_MESSAGE_ID_INFO, 
                                                    "%s: The PVD under connecting is erased, PVD: 0x%x.", 
                                                    __FUNCTION__, connect_request->pvd_obj_id);
            fbe_spinlock_unlock(&connect_context->object_table_ptr->table_lock);
            fbe_queue_push(&local_completed_pvd_queue_head, queue_element);
            continue;
        }
        pvd_object_id = connect_request->pvd_obj_id;
        fbe_copy_memory(serial_num, object_entry->set_config_union.pvd_config.serial_num, sizeof(serial_num));
        fbe_class_id = database_common_map_class_id_database_to_fbe(object_entry->db_class_id);
        fbe_spinlock_unlock(&connect_context->object_table_ptr->table_lock);

        switch(fbe_class_id) {
        case FBE_CLASS_ID_PROVISION_DRIVE:
            // check for connection to PDO
            status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_LIST,
                                                 &down_edges,
                                                 sizeof(down_edges),
                                                 pvd_object_id,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
            if (FBE_STATUS_OK == status)
            {
                if (down_edges.number_of_downstream_edges == 0)
                {
                    status = fbe_database_connect_to_pdo_by_serial_number(pvd_object_id,
                                                                          serial_num,
                                                                          &pdo_info);
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                                    FBE_TRACE_MESSAGE_ID_INFO, 
                                                    "%s: PVD 0x%x connect to pdo 0x%x. status: 0x%x\n", 
                                                    __FUNCTION__, pvd_object_id, pdo_info.drive_object_id, status);
                    
                    /* check connect pvd to pdo successfully */
                    if (FBE_STATUS_OK == status)
                    {
                        /* write fru signature to disk, after that drive get connected to PVD
               * No mater the pvd is a system one or user one, but the connection 
               * must be established successfully.
               */
                        if(database_common_cmi_is_active())
                        {
                            /* read fru signature */
                            status = fbe_database_read_fru_signature_from_disk(pdo_info.bus, 
                                                                               pdo_info.enclosure, 
                                                                               pdo_info.slot, 
                                                                               &fru_signature, 
                                                                               &fru_signature_operation_status);
                            if (status != FBE_STATUS_OK ||
                                fru_signature_operation_status == FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE)
                            {
                                fbe_trace_level_t trace_level = (fru_signature_operation_status == FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE)?
                                                                 FBE_TRACE_LEVEL_WARNING : FBE_TRACE_LEVEL_ERROR;  /* not a sw bug if drive not accessible, so trace a warn */
                                database_trace (trace_level,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "%s: read fru signature from %d_%d_%d failed. stat:%d sig_stat:%d\n",
                                                __FUNCTION__, pdo_info.bus, pdo_info.enclosure, pdo_info.slot, status, fru_signature_operation_status);
                                continue;
                            }

                            /* write fru signature */
                            if (fru_signature_operation_status == FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_UNINITIALIZED ||
                                (fru_signature.system_wwn_seed != connect_context->system_fru_descriptor.wwn_seed) ||
                                (fru_signature.bus != pdo_info.bus) ||
                                (fru_signature.enclosure != pdo_info.enclosure) ||
                                (fru_signature.slot != pdo_info.slot))
                            {
                                fbe_copy_memory(fru_signature.magic_string, FBE_FRU_SIGNATURE_MAGIC_STRING, FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE);
                                fru_signature.system_wwn_seed = connect_context->system_fru_descriptor.wwn_seed;
                                fru_signature.version = FBE_FRU_SIGNATURE_VERSION;
                                fru_signature.bus = pdo_info.bus;
                                fru_signature.enclosure = pdo_info.enclosure;
                                fru_signature.slot = pdo_info.slot;
                            
                                database_trace(FBE_TRACE_LEVEL_INFO, 
                                                            FBE_TRACE_MESSAGE_ID_INFO, 
                                                            "%s: Write fru signature to disks in bus %d, enc %d, slot %d \n", 
                                                            __FUNCTION__, pdo_info.bus, pdo_info.enclosure, pdo_info.slot);
                                
                                status = fbe_database_write_fru_signature_to_disk(pdo_info.bus, pdo_info.enclosure, pdo_info.slot, 
                                                                                  &fru_signature, &fru_signature_operation_status);
                                if (status == FBE_STATUS_OK &&
                                    fru_signature_operation_status == FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_ACCESS_OK)
                                {
                                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                                                FBE_TRACE_MESSAGE_ID_INFO, 
                                                                "%s: Write fru signature to user disks successfully, disk bus %d, enc %d, slot %d.\n", 
                                                                __FUNCTION__, pdo_info.bus, pdo_info.enclosure, pdo_info.slot);
                                }
                                else
                                {
                                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                                                FBE_TRACE_MESSAGE_ID_INFO, 
                                                                "%s: Write fru signature to user disks failed, disk bus %d, enc %d, slot %d.\n", 
                                                                __FUNCTION__, pdo_info.bus, pdo_info.enclosure, pdo_info.slot);                          
                                }
                            }
                        }
                        
                    }
                    else
                    {
                        serial_num[sizeof(serial_num) - 1] = '\0';
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: Fail to connect disk %s.\n", __FUNCTION__, serial_num);
                    }
                    
                }
                else
                {
                    status = fbe_base_transport_get_path_state(&(down_edges.downstream_edge_list[0]), &path_state);
                }
            }
            
            /*In current implementation, a PVD is requested to connect to downstream PDO by active side through job.
          However, at that time the passive side PDO may still in specialize state. So the fbe_database_connect_to_pdo_by_serial_number
          function would return FBE_STATUS_NO_OBJECT. In this case, we have to put the entry back into connect queue
          to try later*/
            if ((status != FBE_STATUS_OK && status != FBE_STATUS_NO_OBJECT)||
                (status == FBE_STATUS_NO_OBJECT && !database_common_cmi_is_active()) ||
                ((status == FBE_STATUS_OK)&&(down_edges.number_of_downstream_edges != 0)&&(path_state != FBE_PATH_STATE_ENABLED)))
            {
                /* we need to return not ok status so that connect thread will take a break */
                ret_status = FBE_STATUS_NO_OBJECT;
                database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                "database connect pvds: 0x%x, status: %d\n", pvd_object_id, status);
                // we are not done with connection, put it back to check later
                fbe_queue_push(&local_not_completed_pvd_queue_head, queue_element);
                
            }
            else
            {
                fbe_queue_push(&local_completed_pvd_queue_head, queue_element);
            }

            break;
        default:
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "database connect pvds: invalid class id 0x%x\n", fbe_class_id);
            fbe_spinlock_lock(&connect_context->object_table_ptr->table_lock);
            status = fbe_database_config_table_get_object_entry_by_id(connect_context->object_table_ptr,
                                                                                                                                    connect_request->pvd_obj_id,
                                                                                                                                    &object_entry);
            /* AR 690469 One SP is trying to connect PVD to PDO, meantime, the other SP is destroying PVD. 
            * Caused the SP lost the PVD entry which is under connecting. So the SP who was doing PVD 
            * connection thought it was in critical bad situation and panic the array.*/
            if (object_entry->header.state == DATABASE_CONFIG_ENTRY_INVALID)
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, 
                                                FBE_TRACE_MESSAGE_ID_INFO, 
                                                "%s: The PVD under connecting is erased, PVD: 0x%x.", 
                                                __FUNCTION__, connect_request->pvd_obj_id);
                fbe_spinlock_unlock(&connect_context->object_table_ptr->table_lock);
                fbe_queue_push(&local_completed_pvd_queue_head, queue_element);
                break;
            }

            fbe_spinlock_unlock(&connect_context->object_table_ptr->table_lock);
            fbe_queue_push(&local_completed_pvd_queue_head, queue_element);
            break;
            
        }

    }

        
    fbe_spinlock_lock(&connect_context->discover_queue_lock);
  
    /* release the notification resource*/
    fbe_database_drive_discover_release_resource(connect_context, &local_completed_pvd_queue_head);

    if (!(fbe_queue_is_empty(&local_not_completed_pvd_queue_head)))
    {
        new_connect_request = (database_pvd_discover_object_t *)fbe_queue_pop(&local_not_completed_pvd_queue_head);
        connect_request = (database_pvd_discover_object_t *)fbe_queue_front(&connect_context->connect_queue_head);
        while (new_connect_request!=NULL)
        {
            while (connect_request!=NULL)
            {
                if (connect_request->pvd_obj_id >= new_connect_request->pvd_obj_id)
                {
                    break;  // we are inserting here.
                }

                connect_request = (database_pvd_discover_object_t *)fbe_queue_next(&connect_context->connect_queue_head, 
                                                                                    (fbe_queue_element_t *)connect_request);
            }
            if (connect_request==NULL)
            {
                database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                "database connect save pvd: 0x%x\n", new_connect_request->pvd_obj_id);
                fbe_queue_push(&connect_context->connect_queue_head, &new_connect_request->queue_element);
                /* we don't have to search from the beginning */
                connect_request = new_connect_request;
            }
            else
            {
                /* we don't have to requeue it if it's in the queue already */
                if (connect_request->pvd_obj_id > new_connect_request->pvd_obj_id)
                {
                    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                "database connect save pvd: 0x%x before 0x%x\n", 
                                new_connect_request->pvd_obj_id, connect_request->pvd_obj_id);
                    // make sure the queue is properly sorted
                    fbe_queue_insert(&new_connect_request->queue_element, &connect_request->queue_element);
                }
                else
                {
                    // release the resource
                    fbe_queue_push(&connect_context->free_entry_queue_head, &new_connect_request->queue_element);
                }
                /* we will search again from the last found node */
            }
            new_connect_request = (database_pvd_discover_object_t *)fbe_queue_pop(&local_not_completed_pvd_queue_head);

        }
        ret_status = FBE_STATUS_NO_OBJECT;  /* non-ok status will tell the caller to take a break */
    }

    fbe_spinlock_unlock(&connect_context->discover_queue_lock);

    return ret_status;
}

/*!***************************************************************
 * @fn fbe_database_get_pdo_info
 *****************************************************************
 * @brief
 *   query PDO for its information
 *
 * @param  pdo - object id for PDO
 * @param  pdo_info - stores PDO info.
 *
 * @return fbe_status_t
 *
 * @version
 *    04/20/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_get_pdo_info(fbe_object_id_t pdo, fbe_database_physical_drive_info_t *pdo_info)
{
    fbe_physical_drive_get_location_t          drive_info;
    fbe_physical_drive_attributes_t            drive_attr;
    fbe_status_t                               status;

    status = fbe_database_send_packet_to_object(FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOCATION,
                                                &drive_info, 
                                                sizeof(drive_info),
                                                pdo,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);

    if( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database connect: failed to get info for pdo %u, status 0x%x\n", pdo, status);

        pdo_info->drive_object_id = FBE_OBJECT_ID_INVALID;
        return status;
    }
    status = fbe_database_send_packet_to_object(FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES,
                                                &drive_attr, 
                                                sizeof(drive_attr),
                                                pdo,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database connect: failed to get attr for pdo %u, status 0x%x\n", pdo, status);

        pdo_info->drive_object_id = FBE_OBJECT_ID_INVALID;
        return status;
    }
    pdo_info->drive_object_id  = pdo;
    pdo_info->bus              = drive_info.port_number;
    pdo_info->enclosure        = drive_info.enclosure_number;
    pdo_info->slot             = drive_info.slot_number;

    if ( drive_attr.physical_block_size == 520 )
    {
        pdo_info->block_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
    }
    else if ( drive_attr.physical_block_size == 4160 )
    {
        pdo_info->block_geometry = FBE_BLOCK_EDGE_GEOMETRY_4160_4160;
    }
    else
    {
        /* Unsupported physical block size.
         */
        database_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s get pdo attr obj: 0x%x unsupported physical block size: %d\n",
                          __FUNCTION__, pdo, drive_attr.physical_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(pdo_info->serial_num, drive_attr.initial_identify.identify_info, sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE );

    pdo_info->serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] ='\0';

    pdo_info->exported_capacity = drive_attr.imported_capacity;

    pdo_info->maintenance_mode_bitmap =  drive_attr.maintenance_mode_bitmap;

    return status;
}

/*!***************************************************************
 * @fn fbe_database_notify_pdo_logically_offline_due_to_maintenance
 *****************************************************************
 * @brief
 *   Notifies PDO that it's logically offline
 *
 * @param  pdo - object id for PDO
 * @param  pdo_info - PDO info.
 * 
 * @return fbe_status_t
 *
 * @version
 *    12/11/2013:   Wayne Garrett - created
 *
 ****************************************************************/
fbe_status_t fbe_database_notify_pdo_logically_offline_due_to_maintenance(fbe_object_id_t pdo, const fbe_database_physical_drive_info_t *pdo_info)
{
    fbe_block_transport_logical_state_t     state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED;
    fbe_status_t                            status;

    if (pdo_info->maintenance_mode_bitmap & FBE_PDO_MAINTENANCE_FLAG_EQ_NOT_SUPPORTED)
    {
        state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_NON_EQ;
    }
    else if (pdo_info->maintenance_mode_bitmap & FBE_PDO_MAINTENANCE_FLAG_INVALID_IDENTITY)
    {
        state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_INVALID_IDENTITY;
    }
    /*else if - add other logically offline cases here as we come across them.*/
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "database connect: unhandled maintenance mode. pdo:0x%x mode:0x%x\n", pdo, pdo_info->maintenance_mode_bitmap);
        
        state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_OTHER;
    }

    status = fbe_database_notify_pdo_logically_offline(pdo, state);

    return status;
}

/*!***************************************************************
 * @fn fbe_database_notify_pdo_logically_offline
 *****************************************************************
 * @brief
 *   Notifies PDO that it's logically offline
 *
 * @param  pdo - object id for PDO
 * @param  pdo_info - PDO info.
 * 
 * @return fbe_status_t
 *
 * @version
 *    12/11/2013:   Wayne Garrett - created
 *
 ****************************************************************/
fbe_status_t fbe_database_notify_pdo_logically_offline(fbe_object_id_t pdo, fbe_block_transport_logical_state_t state)
{
    fbe_status_t status;

    status = fbe_database_send_packet_to_object(FBE_BLOCK_TRANSPORT_CONTROL_CODE_LOGICAL_DRIVE_STATE_CHANGED,
                                                &state, 
                                                sizeof(state),
                                                pdo,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);

    if( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database connect: failed sending logical offline state. pdo:0x%x state:%d status:%d\n", pdo, state,status);
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_database_connect_pvd_to_pdo_info
 *****************************************************************
 * @brief
 *   connects PDO with PVD
 *
 * @param  pvd - object id for PVD
 * @param  pdo - object id for PDO
 * @param  drive_info - PDO info.  This will be updated
 *
 * @return fbe_status_t
 *
 * @version
 *    04/20/2011:   created
 *
 ****************************************************************/
static fbe_status_t fbe_database_connect_pvd_to_pdo_info(fbe_object_id_t pvd, fbe_object_id_t pdo, fbe_database_physical_drive_info_t *drive_info)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_block_transport_control_create_edge_t       create_edge;
    fbe_database_slot_type slot_type = FBE_DATABASE_INVALID_SLOT_TYPE;

    create_edge.capacity                   = drive_info->exported_capacity;
    create_edge.offset                     = 0;
    create_edge.client_index               = 0;
    create_edge.server_id                  = pdo;

    status = fbe_database_determine_slot_type(drive_info, &slot_type);
    if (slot_type == FBE_DATABASE_DB_SLOT) {
        fbe_c4_mirror_update_pvd(drive_info->slot, drive_info->serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    }

    /* Send create edge command to specific object to create an edge with
     * specific information.
     */
    status = fbe_database_send_packet_to_object(FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE,
                                                &create_edge, 
                                                sizeof(create_edge),
                                                pvd,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                     "database connect: connect pvd 0x%x to pdo 0x%x, status 0x%x\n", 
                     pvd, pdo, status);

    return status;

}

/*!***************************************************************
 * @fn fbe_database_connect_to_pdo_by_location
 *****************************************************************
 * @brief
 *   locates PDO based on physical location, and connects PDO with PVD
 * 
 *
 * @param  pvd - object id for PVD
 * @param  bus - PDO bus location
 * @param  enc - PDO enclosure location
 * @param  slt - PDO slot location
 * @param  drive_info - PDO info.  This will be updated
 *
 * @return fbe_status_t
 *
 * @version
 *    04/20/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_connect_to_pdo_by_location(fbe_object_id_t pvd_object_id, 
                                                     fbe_u32_t bus,
                                                     fbe_u32_t enclosure,
                                                     fbe_u32_t slot,
                                                     fbe_database_physical_drive_info_t *drive_info)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_topology_control_get_physical_drive_by_location_t    topology_location;
    fbe_object_id_t pdo_object_id;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_physical_drive_mgmt_get_drive_information_t pdo_drive_info;

    // get pdo id based on location
    topology_location.port_number      = bus;
    topology_location.enclosure_number = enclosure;
    topology_location.slot_number      = slot;

    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
                                                 &topology_location, 
                                                 sizeof(topology_location),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: failed to pdo by location %u_%u_%u, status 0x%x\n", 
                     __FUNCTION__, bus, enclosure, slot, status);

        drive_info->drive_object_id = FBE_OBJECT_ID_INVALID;
        return status;
    }

    pdo_object_id = topology_location.physical_drive_object_id;

    //PDO not in READY state should not be connected; will retry
    status = fbe_database_generic_get_object_state(pdo_object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: fail get pdo 0x%x lifecycle state. status:0x%x\n", 
                     __FUNCTION__, pdo_object_id, status);
        return status;
    }

    if (FBE_LIFECYCLE_STATE_READY != lifecycle_state)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify the drive type */
    status = fbe_database_get_pdo_drive_info(pdo_object_id, &pdo_drive_info);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to get physical drive info for 0x%x. status:0x%x\n",
                        __FUNCTION__, pdo_object_id, status);
        return status;
    }
  
    if (fbe_database_verify_and_handle_unsupported_drive(pdo_object_id, &pdo_drive_info))
    {
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    
    // fill in the info
    status = fbe_database_get_pdo_info(pdo_object_id, drive_info);
    if( status != FBE_STATUS_OK )
    {
        // we have enough logging in get_pdo_info
        return status;
    }

    status = fbe_database_connect_pvd_to_pdo_info(pvd_object_id, pdo_object_id, drive_info);

    return status;
}

/*!***************************************************************
 * @fn fbe_database_connect_to_pdo_by_serial_number
 *****************************************************************
 * @brief
 *   locates PDO based on serial number, and connects PDO with PVD
 *
 * @param  pvd - object id for PVD
 * @param  SN  - PDO serial number
 * @param  drive_info - PDO info.  This will be updated
 *
 * @return fbe_status_t
 *
 * @version
 *    04/20/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_connect_to_pdo_by_serial_number(fbe_object_id_t pvd_object_id, 
                                                          fbe_u8_t *SN,
                                                          fbe_database_physical_drive_info_t *drive_info)
{
    fbe_status_t                                   status = FBE_STATUS_OK;
    fbe_topology_control_get_physical_drive_by_sn_t topology_search_sn;
    fbe_object_id_t pdo_object_id;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_physical_drive_mgmt_get_drive_information_t pdo_drive_info;

    topology_search_sn.sn_size = sizeof(topology_search_sn.product_serial_num) - 1;
    fbe_copy_memory(topology_search_sn.product_serial_num, SN, topology_search_sn.sn_size);
    topology_search_sn.product_serial_num[topology_search_sn.sn_size] = '\0';

    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_SERIAL_NUMBER,
                                                 &topology_search_sn, 
                                                 sizeof(topology_search_sn),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: failed to get pdo by serial number %s, status 0x%x\n", 
                     __FUNCTION__, topology_search_sn.product_serial_num, status);

        drive_info->drive_object_id = FBE_OBJECT_ID_INVALID;
        return status;
    }

    pdo_object_id = topology_search_sn.object_id;

    //PDO not in READY state should not be connected; will retry
    status = fbe_database_generic_get_object_state(pdo_object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: fail get pdo 0x%x lifecycle state. status:0x%x\n", 
                     __FUNCTION__, pdo_object_id, status);
        return status;
    }

    if (FBE_LIFECYCLE_STATE_READY != lifecycle_state)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: lifecycle of pdo 0x%x is : %u\n",
                     __FUNCTION__, pdo_object_id, lifecycle_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // verify the drive type
    status = fbe_database_get_pdo_drive_info(pdo_object_id, &pdo_drive_info);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to get physical drive info for 0x%x. status:0x%x\n",
                        __FUNCTION__, pdo_object_id, status);
        return status;
    }
  
    if (fbe_database_verify_and_handle_unsupported_drive(pdo_object_id, &pdo_drive_info))
    {
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    // fill in the info
    status = fbe_database_get_pdo_info(pdo_object_id, drive_info);
    if( status != FBE_STATUS_OK )
    {
        // we have enough logging in get_pdo_info
        return status;
    }

	/* New add to fix AR606032 by Song,Yingying BEGIN
     * This is to fix AR606032, when swapping a bound user drive and a DB drive offline, after power on system, 
     * Passive SP will receive serial num of system PVD from Active SP 
     * and connect to the original db drive based on serial num, which is in user slot. This shouldn't be connected.
     * So we add below: if system PVD is trying to connected to wrong slot, prevent the connection between PVD and LDO
     */
    if((pvd_object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST)&&
       (pvd_object_id < (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + fbe_private_space_layout_get_number_of_system_drives())))
    {
        if((drive_info->bus != DATABASE_SYSTEM_DRIVE_BUS) ||
           (drive_info->enclosure != DATABASE_SYSTEM_DRIVE_ENCLOSURE) ||
           (drive_info->slot != (pvd_object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST)))
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "database connect: system drive is wrongly inserted to wrong slot: %d_%d_%d, PVD: 0x%x\n",
                           drive_info->bus, drive_info->enclosure, drive_info->slot, pvd_object_id);
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "database connect: prevented system PVD 0x%x connect to wrong slot: %d_%d_%d successfully\n",
                           pvd_object_id, drive_info->bus, drive_info->enclosure, drive_info->slot);
            
            return FBE_STATUS_GENERIC_FAILURE; 
        }
    }/* New add to fix AR606032 by Song,Yingying END*/

    
    status = fbe_database_connect_pvd_to_pdo_info(pvd_object_id, pdo_object_id, drive_info);

    return status;
}

/*!***************************************************************
 * @fn fbe_database_drive_discover
 *****************************************************************
 * @brief
 *   find all the ready PDO, 
 *      if no matching PVD, create PVD object
 *      otherwise, connect the PVD with PDO
 *
 * @param  database_service - database service
 *
 * @return fbe_status_t
 *
 * @version
 *    04/20/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_drive_discover(database_pvd_operation_t *pvd_operation)
{
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_total_objects_of_class_t   get_total_objects;
    fbe_topology_control_enumerate_class_t              enumerate_class;
    fbe_base_object_mgmt_get_lifecycle_state_t          base_object_mgmt_get_lifecycle_state;   
    fbe_object_id_t                     *obj_array = NULL;
    fbe_u32_t                           i;
    fbe_sg_element_t                    *sg_list;
    fbe_sg_element_t                    *sg_element;
    fbe_u32_t                           alloc_data_size;
    fbe_u32_t                           alloc_sg_size;
    
    fbe_u32_t   database_system_drives_num = 0;
    database_system_drives_num = fbe_private_space_layout_get_number_of_system_drives();

    // get total # of PDOs
    get_total_objects.class_id = FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST;
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS,
                                                 &get_total_objects,
                                                 sizeof(fbe_topology_control_get_total_objects_of_class_t),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database drive discover: found %d object\n", get_total_objects.total_objects);
    if (get_total_objects.total_objects == 0) {
        return status;
    }
    // allocate buffer and sg_list
    alloc_data_size = get_total_objects.total_objects*sizeof (fbe_object_id_t);
    alloc_sg_size   = FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT*sizeof (fbe_sg_element_t);
    sg_list = (fbe_sg_element_t *)fbe_memory_ex_allocate(alloc_data_size + alloc_sg_size);
    if (sg_list == NULL) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database drive discover: failed to allocate memory for %d objects\n", get_total_objects.total_objects);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    sg_element = sg_list;
    fbe_sg_element_init(sg_element, alloc_data_size, (fbe_u8_t *)sg_list + alloc_sg_size);
    fbe_sg_element_increment(&sg_element);
    fbe_sg_element_terminate(sg_element);
    obj_array = (fbe_object_id_t *)(sg_list->address);

    enumerate_class.number_of_objects = get_total_objects.total_objects;
    enumerate_class.class_id = FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST;
    enumerate_class.number_of_objects_copied = 0;

    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS,
                                                 &enumerate_class,
                                                 sizeof(fbe_topology_control_enumerate_class_t),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 sg_list,  
                                                 FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT,  
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        fbe_memory_ex_release(sg_list);
        return status;
    }

    // go through all the ready PDOs
    for (i = 0; i < enumerate_class.number_of_objects_copied; ++i) {
        status = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                &base_object_mgmt_get_lifecycle_state, 
                                                sizeof(base_object_mgmt_get_lifecycle_state),
                                                obj_array[i],
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "database drive discover: failed to get lifecycle state for %d, status 0x%x\n", obj_array[i], status);
            continue;
        }
        if (base_object_mgmt_get_lifecycle_state.lifecycle_state != FBE_LIFECYCLE_STATE_READY)
        {
            // skip non-ready PDOs
            continue;
        }

        status = fbe_database_add_drive_to_be_processed(pvd_operation, obj_array[i], FBE_LIFECYCLE_STATE_READY, DATABASE_PVD_DISCOVER_FLAG_NORMAL_PROCESS);        

    }
        
    fbe_memory_ex_release(sg_list);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_determine_drive_movement_type
 *****************************************************************
 * @brief
 * determine what type of drive movement occurs now.
 *
 * The drive movement type is determined by drive original type and its location.
 *
 * The drive original type determined based on FRU Signature only. 
 *       No valid FRU Signature on that drive, then it is a new drive
 *       FRU Signature wwn seed shows the array to which drive belongs.
 *       FRU Signature bus_enc_slot combination shows the drive was a user drive or a DB drive
 *       Leverage fbe_database_is_pvd_consumed_by_raid_group to check a drive is bound or not
 * Slot type determined based on PDO information and FRU Signature 
 *       bus_enc_slot in PDO shows the drive's current location
 *       If the drive type is current array db drive and the PDO information shows it is in DB slot, 
 *       then double-check that is it in its original DB slot by compare the bus_enc_slot in PDO with
 *       bus_enc_slot in FRU Signature.
 *       if the drive type is current array bind/unbind user drive and the PDO information shows it is
 *       in user slot, NOT do further check.
 *
 * @param in            *discover_context
 * @param in            pdo_info
 * @param out         *drive_movement_type
 *
 * @return fbe_status_t
 *
 * @version
 *    02/17/2012:   created
 *    07/24/2012:   modified to determine more drive movement type via 
 *                                 adding check a drive is consumed by RG or not, and adding
 *                                 FRU Signature support
 *
 ****************************************************************/

fbe_status_t fbe_database_determine_drive_movement_type
(database_pvd_operation_t* discover_context,
  fbe_database_physical_drive_info_t* pdo_info,
 database_drive_movement_type* drive_movement_type)
{
    fbe_status_t                                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_drive_type                          drive_original_type = FBE_DATABASE_INVALID_DRIVE_TYPE;
    fbe_database_slot_type                           slot_type = FBE_DATABASE_INVALID_SLOT_TYPE;
    fbe_fru_signature_t                              fru_signature;
    fbe_bool_t                                       is_consumed_user_drive = FBE_FALSE;
    fbe_bool_t                                       is_drive_in_its_original_slot = FBE_FALSE;
    database_object_entry_t*                         pvd_object_entry = NULL;
    fbe_database_fru_signature_IO_operation_status_t operation_status;

    if (drive_movement_type == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: invalid param, drive_movement_type == NULL.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Initialize drive movement with invalid value */
    *drive_movement_type = INVALID_DRIVE_MOVEMENT;
    
    /* Param check */
    if (discover_context == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: invalid param, discover_context == NULL.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (pdo_info == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: invalid param, pdo_info == NULL.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* read disk fru signature */
    status = fbe_database_read_fru_signature_from_disk(pdo_info->bus, 
                                                       pdo_info->enclosure, 
                                                       pdo_info->slot, 
                                                       &fru_signature, 
                                                       &operation_status);
    if (FBE_STATUS_OK != status && 
        FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE != operation_status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: read fru signature failed. stat:%d sig_stat%d\n",
                        __FUNCTION__, status, operation_status);
        return status;
    }

    if (FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE == operation_status)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: drive could not be access, drive: bus %d, enc %d, slot %d.\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* the uninitialized FRU Signature is one of expected cases here,
      * so the operation of status may not be OK, we do not need to do
      * that check.
      */
      
    /* determine drive type */
    status = fbe_database_determine_drive_type(discover_context, pdo_info, &drive_original_type);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: determine drive type failed, drive: bus %d, enc %d, slot %d.\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
        return status;
    }

    /* determine slot type */
    status = fbe_database_determine_slot_type(pdo_info, &slot_type);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: determine slot type failed, drive: bus %d, enc %d, slot %d.\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
        return status;
    }

    if (drive_original_type == FBE_DATABASE_NEW_DRIVE ||
         drive_original_type == FBE_DATABASE_OTHER_ARRAY_DB_DRIVE ||
         drive_original_type == FBE_DATABASE_OTHER_ARRAY_USER_DRIVE ||
         drive_original_type == FBE_DATABASE_INVALID_DRIVE_TYPE)
    {
        /* do nothing, do not check the drive is in original slot or not
          * check is useless when the drive type is one of new, invalid or other array drive, 
          * becasue when a drive is a new one then is_drive_in_its_original_slot must be FALSE
          */
        ;
    }
    else if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_DB_DRIVE ||
                  drive_original_type == FBE_DATABASE_CURRENT_ARRAY_USER_DRIVE)
    {
        /* check the drive is in its original slot or not */
        if (fru_signature.bus == pdo_info->bus &&
             fru_signature.enclosure == pdo_info->enclosure &&
            fru_signature.slot == pdo_info->slot)
        {
            is_drive_in_its_original_slot = FBE_TRUE;
        }
        else
        {
            is_drive_in_its_original_slot = FBE_FALSE;
        }
    }
    

    /* check the drive is consumed or not when drive is 
      *  FBE_DATABASE_CURRENT_ARRAY_USER_DRIVE */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_USER_DRIVE)
    {
          /* get the PVD object id by PDO SN */
        fbe_spinlock_lock(&discover_context->object_table_ptr->table_lock);
        status = fbe_database_config_table_get_last_object_entry_by_pvd_SN(discover_context->object_table_ptr->table_content.object_entry_ptr, 
                                                                           discover_context->object_table_ptr->table_size,
                                                                           pdo_info->serial_num,
                                                                           &pvd_object_entry);
        fbe_spinlock_unlock(&discover_context->object_table_ptr->table_lock);

        if (FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: failed to get PVD when the drive fru signature shows it is a user drive, drive: bus %d, enc %d, slot %d.\n",
                            __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
            return status;
        }

        if (pvd_object_entry == NULL)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: do NOT find PVD when the drive fru signature shows it is a user drive, drive: bus %d, enc %d, slot %d.\n",
                            __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
            return status;
        }
        
        status = fbe_database_is_pvd_consumed_by_raid_group(pvd_object_entry->header.object_id, 
                                                            &is_consumed_user_drive);
        if (FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: failed to check this drive is consumed or not, drive: bus %d, enc %d, slot %d.\n",
                            __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
            return status;
        }

        if (is_consumed_user_drive)
            drive_original_type = FBE_DATABASE_CURRENT_ARRAY_BIND_USER_DRIVE;
        else
            drive_original_type = FBE_DATABASE_CURRENT_ARRAY_UNBIND_USER_DRIVE;
    }

    /* combine the drive_original_type, slot_type and flags
      * to get drive_movement_type
      */
      
    /* 1. INSERT_A_NEW_DRIVE_TO_A_USER_SLOT */
    if (drive_original_type == FBE_DATABASE_NEW_DRIVE &&
         slot_type == FBE_DATABASE_USER_SLOT)
    {
        *drive_movement_type = INSERT_A_NEW_DRIVE_TO_A_USER_SLOT;
        return FBE_STATUS_OK;
    }

    /* 2. INSERT_A_NEW_DRIVE_TO_A_DB_SLOT */
    if (drive_original_type == FBE_DATABASE_NEW_DRIVE &&
         slot_type == FBE_DATABASE_DB_SLOT)
    {
        *drive_movement_type = INSERT_A_NEW_DRIVE_TO_A_DB_SLOT;
        return FBE_STATUS_OK;
    }

    /* 3. INSERT_OTHER_ARRAY_USER_DRIVE_TO_A_USER_SLOT */
    if (drive_original_type == FBE_DATABASE_OTHER_ARRAY_USER_DRIVE &&
         slot_type == FBE_DATABASE_USER_SLOT)
    {
        *drive_movement_type = INSERT_OTHER_ARRAY_USER_DRIVE_TO_A_USER_SLOT;
        return FBE_STATUS_OK;
    }

    /* 4. INSERT_OTHER_ARRAY_USER_DRIVE_TO_A_DB_SLOT */
    if (drive_original_type == FBE_DATABASE_OTHER_ARRAY_USER_DRIVE &&
         slot_type == FBE_DATABASE_DB_SLOT)
    {
        *drive_movement_type = INSERT_OTHER_ARRAY_USER_DRIVE_TO_A_DB_SLOT;
        return FBE_STATUS_OK;
    }

    /* 5. INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_ITS_ORIGINAL_SLOT */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_UNBIND_USER_DRIVE &&
         slot_type == FBE_DATABASE_USER_SLOT &&
         is_drive_in_its_original_slot)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_ITS_ORIGINAL_SLOT;
    }

    /* 6. INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_ANOTHER_USER_SLOT */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_UNBIND_USER_DRIVE &&
         slot_type == FBE_DATABASE_USER_SLOT &&
         !is_drive_in_its_original_slot)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_ANOTHER_USER_SLOT;
    }

    /* 7. INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_A_USER_SLOT */
    if (*drive_movement_type == INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_ITS_ORIGINAL_SLOT ||
         *drive_movement_type == INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_ANOTHER_USER_SLOT)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_A_USER_SLOT;
        return FBE_STATUS_OK;
    }

    /* 8. INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_A_DB_SLOT */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_UNBIND_USER_DRIVE &&
         slot_type == FBE_DATABASE_DB_SLOT)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_UNBIND_USER_DRIVE_TO_A_DB_SLOT;
        return FBE_STATUS_OK;
    }

    /* 9. INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_ITS_ORIGINAL_SLOT */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_BIND_USER_DRIVE &&
         slot_type == FBE_DATABASE_USER_SLOT &&
         is_drive_in_its_original_slot)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_ITS_ORIGINAL_SLOT;
    }

    /* 10. INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_ANOTHER_USER_SLOT */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_BIND_USER_DRIVE &&
         slot_type == FBE_DATABASE_USER_SLOT &&
         !is_drive_in_its_original_slot)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_ANOTHER_USER_SLOT;
    }

    /* 11. INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_A_USER_SLOT */
    if (*drive_movement_type == INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_ITS_ORIGINAL_SLOT ||
         *drive_movement_type == INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_ANOTHER_USER_SLOT)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_A_USER_SLOT;
        return FBE_STATUS_OK;
    }

    /* 12. INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_A_DB_SLOT */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_BIND_USER_DRIVE &&
         slot_type == FBE_DATABASE_DB_SLOT)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_BIND_USER_DRIVE_TO_A_DB_SLOT;
        return FBE_STATUS_OK;
    }

    /* 13. INSERT_OTHER_ARRAY_DB_DRIVE_TO_A_USER_SLOT */
    if (drive_original_type == FBE_DATABASE_OTHER_ARRAY_DB_DRIVE &&
         slot_type == FBE_DATABASE_USER_SLOT)
    {
        *drive_movement_type = INSERT_OTHER_ARRAY_DB_DRIVE_TO_A_USER_SLOT;
        return FBE_STATUS_OK;
    }

    /* 14. INSERT_OTHER_ARRAY_DB_DRIVE_TO_A_DB_SLOT */
    if (drive_original_type == FBE_DATABASE_OTHER_ARRAY_DB_DRIVE &&
         slot_type == FBE_DATABASE_DB_SLOT)
    {
        *drive_movement_type = INSERT_OTHER_ARRAY_DB_DRIVE_TO_A_DB_SLOT;
        return FBE_STATUS_OK;
    }

    /* 15. INSERT_CURRENT_ARRAY_DB_DRIVE_TO_ITS_ORIGINAL_SLOT */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_DB_DRIVE &&
         slot_type == FBE_DATABASE_DB_SLOT &&
         is_drive_in_its_original_slot)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_DB_DRIVE_TO_ITS_ORIGINAL_SLOT;
        return FBE_STATUS_OK;
    }

    /* 16. INSERT_CURRENT_ARRAY_DB_DRIVE_TO_A_USER_SLOT */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_DB_DRIVE &&
         slot_type == FBE_DATABASE_USER_SLOT)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_DB_DRIVE_TO_A_USER_SLOT;
        return FBE_STATUS_OK;
    }

    /* 17. INSERT_CURRENT_ARRAY_DB_DRIVE_TO_ANOTHER_DB_SLOT */
    if (drive_original_type == FBE_DATABASE_CURRENT_ARRAY_DB_DRIVE &&
         slot_type == FBE_DATABASE_DB_SLOT &&
         !is_drive_in_its_original_slot)
    {
        *drive_movement_type = INSERT_CURRENT_ARRAY_DB_DRIVE_TO_ANOTHER_DB_SLOT;
        return FBE_STATUS_OK;
    }
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_determine_drive_type_by_fru_signature
 *****************************************************************
 * @brief
 * determine the type of drive, the information leveraged to get the type is fru signature on 
 * that drive.
 *
 * The drive original type determined based on FRU Signature only. 
 *       No valid FRU Signature on that drive, then it is a new drive
 *       FRU Signature wwn seed shows the array to which drive belongs.
 *       FRU Signature bus_enc_slot combination shows the drive was a user drive or a DB drive
 *       Leverage fbe_database_is_pvd_consumed_by_raid_group to check a drive is bound or not
 *
 * @param in            fru_sig
 * @param out         *drive_type
 *
 * @return fbe_status_t
 *
 * @version
 *    07/25/2012:   created
 *
 ****************************************************************/

fbe_status_t fbe_database_determine_drive_type
    (database_pvd_operation_t* discover_context,
     fbe_database_physical_drive_info_t* pdo_info, 
     fbe_database_drive_type* drive_type)
{
    fbe_status_t                                      status = FBE_STATUS_GENERIC_FAILURE;   
    fbe_fru_signature_t                               fru_signature;
    fbe_database_fru_signature_IO_operation_status_t  operation_status;
    fbe_database_fru_signature_validation_status_t    signature_status = FBE_DATABASE_UNINITIALIZED_FRU_SIGNATURE_VALIDATION_STATUS;
    fbe_bool_t                                        is_foreign_drive = FBE_FALSE;
    fbe_homewrecker_fru_descriptor_t                  out_fru_descriptor;
    fbe_u8_t                                          index = 0;
    database_object_entry_t*                          object_table_entry = NULL;

    /* validate param pointer */
    if (discover_context == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: invalid param, discover_context == NULL.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (pdo_info == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: invalid param, pdo_info == NULL.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (drive_type == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: invalid param, drive_type == NULL.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    
    /* initialize the return value as INVALID_DRIVE_TYPE */
    *drive_type = FBE_DATABASE_INVALID_DRIVE_TYPE;

    /* read disk fru signature */
    status = fbe_database_read_fru_signature_from_disk(pdo_info->bus, 
                                                       pdo_info->enclosure, 
                                                       pdo_info->slot, 
                                                       &fru_signature, 
                                                       &operation_status);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: read fru signature failed.\n",
                        __FUNCTION__);
        return status;
    }

    if (FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE == operation_status)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: drive could not be access, drive: %d_%d_%d.\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* RETURN #1, NEW DRIVE */
    /* if no valid magic string in fru signature, the drive is new drive */ 
    status = fbe_database_validate_fru_signature(&fru_signature, &signature_status);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: fru signature validation failed, drive: %d_%d_%d.\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (signature_status == FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_MAGIC_STRING_ERROR ||
         signature_status == FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_MAGIC_STRING_AND_VERSION_ERROR)
    {
        *drive_type = FBE_DATABASE_NEW_DRIVE;
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: the drive in %d_%d_%d type is NEW_DRIVE\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
        return FBE_STATUS_OK;
    }

#if 0
    /* check is it a foreign drive or not by wwn seed*/
    status = fbe_database_get_board_resume_prom_wwnseed(&chassis_wwn_seed);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to get chassis wwn seed\n",
                        __FUNCTION__);
        return status;
    }
#endif

    if (fru_signature.system_wwn_seed == discover_context->system_fru_descriptor.wwn_seed)
    {
        is_foreign_drive = FBE_FALSE;
    }
    else 
    {
        is_foreign_drive = FBE_TRUE;
    }

    /* the logic branch when drive is a foreign one */
    if (is_foreign_drive)
    {
        /* RETURN #2, FOREIGN DB DRIVE */
        if ((fru_signature.bus == 0) && (fru_signature.enclosure == 0) && (fru_signature.slot <4))
               //@TODO: call PSL function ???
        {
            *drive_type = FBE_DATABASE_OTHER_ARRAY_DB_DRIVE;
            database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: the drive in %d_%d_%d type is OTHER_ARRAY_DB_DRIVE\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
            return FBE_STATUS_OK;
        }
        else
        /* RETURN #3, FOREIGN USER DRIVE */
        {
            *drive_type = FBE_DATABASE_OTHER_ARRAY_USER_DRIVE;
            database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: the drive in %d_%d_%d type is OTHER_ARRAY_USER_DRIVE\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
            return FBE_STATUS_OK;
        }
    } /* Process of foreign drive done!! */

    /* Start to process local drive types */
    fbe_spinlock_lock(&discover_context->system_fru_descriptor_lock);
    fbe_copy_memory(&out_fru_descriptor, &discover_context->system_fru_descriptor, sizeof(out_fru_descriptor));
    fbe_spinlock_unlock(&discover_context->system_fru_descriptor_lock);

    /* compare PDO SN with system drive SNs in FRU Descriptor */
    /* RETURN #4, FBE_DATABASE_CURRENT_ARRAY_DB_DRIVE */
    for (index = 0; index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; index++)
    {
        if (!fbe_compare_string(pdo_info->serial_num, 
                                FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                out_fru_descriptor.system_drive_serial_number[index],
                                FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                FBE_TRUE))
        {
            *drive_type = FBE_DATABASE_CURRENT_ARRAY_DB_DRIVE;
            database_trace (FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: the drive in %d_%d_%d is CURRENT_ARRAY_DB_DRIVE\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
            return FBE_STATUS_OK;
        }
    }/* CURRENT ARRAY DB DRIVE type check done */

   /* check to see if it's already configured or not */
   fbe_spinlock_lock(&discover_context->object_table_ptr->table_lock);
   status = fbe_database_config_table_get_last_object_entry_by_pvd_SN
                                                                (discover_context->object_table_ptr->table_content.object_entry_ptr, 
                                                                 discover_context->object_table_ptr->table_size,
                                                                 pdo_info->serial_num,
                                                                 &object_table_entry);
    fbe_spinlock_unlock(&discover_context->object_table_ptr->table_lock);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to get object by SN\n",
                        __FUNCTION__);
        return status;        
    }

    /* RETURN #5, FBE_DATABASE_CURRENT_ARRAY_USER_DRIVE */
    if (object_table_entry != NULL && 
       !fbe_private_space_layout_object_id_is_system_pvd(object_table_entry->header.object_id))
    {
        *drive_type = FBE_DATABASE_CURRENT_ARRAY_USER_DRIVE;
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: the drive in %d_%d_%d type is CURRENT_ARRAY_USER_DRIVE\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
        return status;
    }
    /* RETURN #6, */
    else
    {
        *drive_type = FBE_DATABASE_NEW_DRIVE;
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: the drive in %d_%d_%d type is NEW_DRIVE\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
        return status;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_database_determine_slot_type
 *****************************************************************
 * @brief
 * determine what type of slot which the PDO locates in.
 * 
 * Slot type determined based on PDO information
 *       bus_enc_slot in PDO shows the drive's current location
 *
 * @param in            pdo_info
 * @param out         *drive_type
 *
 * @return fbe_status_t
 *
 * @version
 *    07/25/2012:   created
 *    
 ****************************************************************/
fbe_status_t  fbe_database_determine_slot_type
    (fbe_database_physical_drive_info_t* pdo_info, fbe_database_slot_type* slot_type)
{
    if (slot_type == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: invalid param, slot_type == NULL.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *slot_type = FBE_DATABASE_INVALID_SLOT_TYPE;
    
    if (pdo_info == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: invalid param, pdo_info == NULL.\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* check the bus_enc_slot combination to return the slot type */
    if (0 == pdo_info->bus && 0 == pdo_info->enclosure && FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER > pdo_info->slot)
        *slot_type = FBE_DATABASE_DB_SLOT;
    else
        *slot_type = FBE_DATABASE_USER_SLOT;
    
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_database_sys_pvd_created_notification_callback(fbe_object_id_t object_id, 
                                                                                                                                                                                         fbe_notification_info_t notification_info,      
                                                                                                                                                                                         fbe_notification_context_t context)
{

    database_system_pvd_creation_context_t*  pvd_creation_context = (database_system_pvd_creation_context_t*)context;
    fbe_u32_t   pvd_index;
    fbe_object_id_t operation_object_id;
//    fbe_base_object_mgmt_get_lifecycle_state_t lifecycle_state;
//    fbe_topology_control_get_drive_by_location_t    pdo_info;
//    fbe_status_t status;

    operation_object_id = notification_info.notification_data.job_service_error_info.object_id;
    
    if(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED != notification_info.notification_type)
        return FBE_STATUS_OK;

    if(FBE_JOB_TYPE_CREATE_PROVISION_DRIVE != notification_info.notification_data.job_service_error_info.job_type)
        return FBE_STATUS_OK;

    if(operation_object_id< FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0 || 
        operation_object_id > FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_3)
        return FBE_STATUS_OK;

    pvd_index = operation_object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0;

    /*judge whether this notification is triggered by the destroy pvd job we sent*/
    if(notification_info.notification_data.job_service_error_info.job_number != pvd_creation_context->job_number)
    {
        return FBE_STATUS_OK;
    }

    if(FBE_JOB_SERVICE_ERROR_NO_ERROR != notification_info.notification_data.job_service_error_info.error_code)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "sys_pvd_created_notify_callback: fail to create pvd. objid: %d\n", object_id);
        return FBE_STATUS_OK;
    }

    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "sys_pvd_created_notify_callback_gordon: fail to destroy pvd. objid: %d\n", object_id);
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_validate_pvd
 *****************************************************************
 * @brief
 *  This function validates a PVD can connect to the PDO or not on booting path. The PDO is designated 
 *  by serial number in PVD configuration.
 *  This function is called for avoiding a PVD connect to its original PDO wrongly due to
 *  someone moved that drive around or the PDO is not READY yet.
 *  This function is called after load topology from disk and before connect the PVD to its PDO.
 *
 * @param  fbe_object_entry_t PVD information
 *
 * @return return TRUE if this PVD can connect to the PDO record in its configuration
 *
 * @version
 *    7/24/2012: Jian Gao - created
 *
 ****************************************************************/       
fbe_bool_t fbe_database_validate_pvd(database_object_entry_t* pvd_entry)
{
    fbe_object_id_t pvd_object_id;
    fbe_object_id_t pdo_object_id;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_topology_control_get_physical_drive_by_sn_t topology_search_sn;
    fbe_database_physical_drive_info_t pdo_info;
    fbe_u32_t   system_drive_number = fbe_private_space_layout_get_number_of_system_drives();
    fbe_lifecycle_state_t lifecycle_state;
    fbe_bool_t is_system_pvd = FBE_FALSE;
    fbe_bool_t is_pdo_in_system_slot = FBE_FALSE;
    
    if (pvd_entry == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: invalid param of pvd entry \n", 
                           __FUNCTION__);
            return FBE_FALSE;
    }
    pvd_object_id = pvd_entry->header.object_id;
    
    topology_search_sn.sn_size = sizeof(topology_search_sn.product_serial_num) - 1;
    fbe_copy_memory(topology_search_sn.product_serial_num, 
                                           pvd_entry->set_config_union.pvd_config.serial_num, 
                                           topology_search_sn.sn_size);
    topology_search_sn.product_serial_num[topology_search_sn.sn_size] = '\0';

    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_SERIAL_NUMBER,
                                                 &topology_search_sn, 
                                                 sizeof(topology_search_sn),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_NO_OBJECT)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database_validate_pvd: failed to get pdo by serial number %s, status 0x%x\n", 
                     topology_search_sn.product_serial_num, status);
  
        return FBE_FALSE;
    }
    
    if (status == FBE_STATUS_NO_OBJECT)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database_validate_pvd: does not get pdo by serial number %s, status 0x%x\n", 
                     topology_search_sn.product_serial_num, status);
  
        return FBE_FALSE;      
    }

    pdo_object_id = topology_search_sn.object_id;

    /*PDO not in READY state should not be connected*/
    status = fbe_database_generic_get_object_state(pdo_object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                     "database_validate_pvd: fail get pdo 0x%x lifecycle state.It may be in SPEC state. status:0x%x\n", 
                     pdo_object_id, status);
        return FBE_FALSE;
    }

    if(FBE_LIFECYCLE_STATE_READY != lifecycle_state)
    {
        return FBE_FALSE;
    }
    
    status = fbe_database_get_pdo_info(pdo_object_id, &pdo_info);
    if( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database_validate_pvd: failed to get pdo info 0x%x, status 0x%x\n", 
                     pdo_object_id, status);
  
        return FBE_FALSE;
    }

    is_system_pvd = fbe_private_space_layout_object_id_is_system_pvd(pvd_object_id);
    if (0 == pdo_info.bus && 0 == pdo_info.enclosure && system_drive_number > pdo_info.slot)
        is_pdo_in_system_slot = FBE_TRUE;
    else
        is_pdo_in_system_slot = FBE_FALSE;

    /* if the pvd type mis-match pdo type return false, 
          else if both two of types are user/system at the same time
          return true */          
    return !(is_system_pvd ^ is_pdo_in_system_slot);
}


/* this function capsules constructing and sending destroy pvd job */
static fbe_status_t fbe_database_destroy_pvd_via_job(
                                                                fbe_job_service_destroy_provision_drive_t* pvd_destroy_request,
                                                                fbe_object_id_t pvd_object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (NULL == pvd_destroy_request)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                        "%s: Invalid param, pvd_destroy_request == NULL \n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    pvd_destroy_request->object_id = pvd_object_id;
    database_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "database: Destroy user PVD: 0x%x\n", 
                                    pvd_destroy_request->object_id);

    status = fbe_database_send_packet_to_service(FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE,
                                                 pvd_destroy_request,
                                                 sizeof(fbe_job_service_destroy_provision_drive_t),
                                                 FBE_SERVICE_ID_JOB_SERVICE,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                         "%s: Destroy user PVD: 0x%x failed.  Error status: 0x%x\n", 
                                         __FUNCTION__, 
                                         pvd_destroy_request->object_id,
                                         status);
        return status;
    }

    return status;
}

static fbe_status_t fbe_database_update_pvd_sn_via_job(
                                                                fbe_job_service_update_provision_drive_t* pvd_update_request,
                                                                fbe_object_id_t pvd_object_id, fbe_u8_t* serial_num)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
    if (NULL == pvd_update_request)
    {
         database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, pvd_update_request == NULL.\n", 
                                        __FUNCTION__);
         return FBE_STATUS_GENERIC_FAILURE;
    }

    if (NULL == serial_num)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, serial_num == NULL.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Now, populate reinitialize pvd request */
    pvd_update_request->object_id = pvd_object_id;
    pvd_update_request->update_type = FBE_UPDATE_PVD_SERIAL_NUMBER;
    fbe_copy_memory(pvd_update_request->serial_num, serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);

    database_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "Update PVD serial number: 0x%x\n", 
                                    pvd_update_request->object_id);
        /*let's send it */
    status = fbe_database_send_packet_to_service(FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE,
                                                                                                    pvd_update_request,
                                                                                                    sizeof(fbe_job_service_update_provision_drive_t),
                                                                                                    FBE_SERVICE_ID_JOB_SERVICE,
                                                                                                    NULL,  /* no sg list*/
                                                                                                    0,  /* no sg list*/
                                                                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                                                                    FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                        "%s: Reinitialize system pvd failed, pvd_object %d. Error status 0x%x\n", 
                                        __FUNCTION__, 
                                        pvd_update_request->object_id,
                                        status);
    }

    return status;
}


/* this function capsules constructing and sending reinitialize pvd job */
static fbe_status_t fbe_database_reinitialize_system_pvd_via_job(
                                                                fbe_job_service_update_provision_drive_t* pvd_update_request,
                                                                fbe_database_physical_drive_info_t* pdo_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t sys_pvd_object_id;

    if (NULL == pvd_update_request)
    {
         database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, pvd_update_request == NULL.\n", 
                                        __FUNCTION__);
         return FBE_STATUS_GENERIC_FAILURE;
    }

    if (NULL == pdo_info)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, pdo_info == NULL.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sys_pvd_object_id = pdo_info->slot + FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST;
    
    /* the object is not system pvd */
    if (!fbe_private_space_layout_object_id_is_system_pvd(sys_pvd_object_id))
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, the PDO object is not system slot.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Now, populate reinitialize pvd request */
    pvd_update_request->object_id = sys_pvd_object_id;
    /* popular the PVD field values.*/
    status = fbe_database_common_get_provision_drive_exported_size(pdo_info->exported_capacity, 
                                                                   pdo_info->block_geometry,
                                                                   &(pvd_update_request->configured_capacity));

    fbe_copy_memory(pvd_update_request->serial_num, pdo_info->serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
    
    if ( pdo_info->block_geometry == FBE_BLOCK_EDGE_GEOMETRY_520_520 )
    {
        pvd_update_request->configured_physical_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520;
    }
    else if ( pdo_info->block_geometry == FBE_BLOCK_EDGE_GEOMETRY_4160_4160 )
    {
        pvd_update_request->configured_physical_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160;
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s, pdo: 0x%x invalid block edge geometry: %d\n", 
                       __FUNCTION__, pvd_update_request->object_id, pdo_info->block_geometry);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    database_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "Reinitialize system PVD: 0x%x\n", 
                                    pvd_update_request->object_id);

    /*let's send it */
    status = fbe_database_send_packet_to_service(FBE_JOB_CONTROL_CODE_REINITIALIZE_PROVISION_DRIVE,
                                                                                                    pvd_update_request,
                                                                                                    sizeof(fbe_job_service_update_provision_drive_t),
                                                                                                    FBE_SERVICE_ID_JOB_SERVICE,
                                                                                                    NULL,  /* no sg list*/
                                                                                                    0,  /* no sg list*/
                                                                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                                                                    FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                        "%s: Reinitialize system pvd failed, pvd_object %d. Error status 0x%x\n", 
                                        __FUNCTION__, 
                                        pvd_update_request->object_id,
                                        status);
    }

    return status;
}

/* this function capsules adding an item into create pvd request PVD_list */
static fbe_status_t fbe_database_add_item_into_create_pvd_request_PVD_list(
                                                                fbe_job_service_create_provision_drive_t* pvd_create_request,
                                                                fbe_database_physical_drive_info_t* pdo_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t found_duplicate = FBE_FALSE;
    fbe_u8_t i = 0;
    fbe_u8_t pos_in_PVD_list = 0;

    if (NULL == pvd_create_request)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, pvd_create_request == NULL.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (NULL == pdo_info)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, pdo_info == NULL.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // check for duplicate
    for (i = 0; i < pvd_create_request->request_size; i++)
    {
        if(fbe_compare_string(pvd_create_request->PVD_list[i].serial_num, 
                                      sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                      pdo_info->serial_num, 
                                      sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                      FBE_FALSE) == 0)
        {
            found_duplicate = FBE_TRUE;
        }
    }
    
    if (found_duplicate)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, no need to add this pdo into create pvd request, it is duplicated.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_OK;  // skip
    }

    /* initialize the index as tail of PVD_list in pvd create request */
    /* then add an item(pvd configuration) into pvd create request */
    pos_in_PVD_list = pvd_create_request->request_size;
    /* popular the PVD field values.*/
    status = fbe_database_common_get_provision_drive_exported_size(pdo_info->exported_capacity, 
                                                                   pdo_info->block_geometry,
                                                                   &(pvd_create_request->PVD_list[pos_in_PVD_list].configured_capacity));

    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                        "database_process_physical_drives: failed to get pvd exported capacity for %d. Skip!\n",
                        pdo_info->drive_object_id);
        return status;        
    }   

    /*! @note Default pvd configuration type is `automatic' spare. */
    pvd_create_request->PVD_list[pos_in_PVD_list].config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;
    fbe_copy_memory(pvd_create_request->PVD_list[pos_in_PVD_list].serial_num, pdo_info->serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
    if ( pdo_info->block_geometry == FBE_BLOCK_EDGE_GEOMETRY_520_520 )
    {
        pvd_create_request->PVD_list[pos_in_PVD_list].configured_physical_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520;
    }
    else if ( pdo_info->block_geometry == FBE_BLOCK_EDGE_GEOMETRY_4160_4160 )
    {
        pvd_create_request->PVD_list[pos_in_PVD_list].configured_physical_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160;
    }
    else
    {
        pvd_create_request->PVD_list[pos_in_PVD_list].configured_physical_block_size = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID;
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s: invalid block size for attached to pdo %d \n", 
                                        __FUNCTION__, 
                                        pdo_info->drive_object_id);
    }

    pvd_create_request->PVD_list[pos_in_PVD_list].object_id = FBE_OBJECT_ID_INVALID;

    /* By default sniffing should be enabled. */
    pvd_create_request->PVD_list[pos_in_PVD_list].sniff_verify_state = FBE_TRUE;

    /* increase the request size of pvd create request */
    pos_in_PVD_list++;
    pvd_create_request->request_size = pos_in_PVD_list;

    return status;
}

static fbe_status_t fbe_database_send_create_pvd_request(
                                                                fbe_job_service_create_provision_drive_t* pvd_create_request)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (NULL == pvd_create_request)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, pvd_create_request == NULL.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }        
    
    status = fbe_database_send_packet_to_service(FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE,
                                                                                                    pvd_create_request,
                                                                                                    sizeof(fbe_job_service_create_provision_drive_t),
                                                                                                    FBE_SERVICE_ID_JOB_SERVICE,
                                                                                                    NULL,  /* no sg list*/
                                                                                                    0,  /* no sg list*/
                                                                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                                                                    FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                        "%s: send pvd create request failed.  Error status: 0x%x\n", 
                                        __FUNCTION__, 
                                        status);
        return status;
    }

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_database_find_original_slot_for_original_system_drive(
                                                                database_pvd_operation_t*   pvd_operation_context,
                                                                fbe_database_physical_drive_info_t* pdo_info,
                                                                fbe_u32_t* slot_number)
{
    int index = 0;
    fbe_homewrecker_fru_descriptor_t    system_fru_descriptor;

    if (NULL == pdo_info)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, pdo_info == NULL.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }    

    fbe_spinlock_lock(&pvd_operation_context->system_fru_descriptor_lock);
    fbe_copy_memory(&system_fru_descriptor, &pvd_operation_context->system_fru_descriptor, sizeof(system_fru_descriptor));
    fbe_spinlock_unlock(&pvd_operation_context->system_fru_descriptor_lock);
    
    for (index = 0; index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; index++)
    {
        if (!fbe_compare_string(pdo_info->serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                                          system_fru_descriptor.system_drive_serial_number[index],
                                                          FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE,
                                                          FBE_TRUE))
        {
            *slot_number = index;
            database_trace (FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: the drive in bus %d, enc %d, slot %d is CURRENT_ARRAY_DB_DRIVE\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);
            return FBE_STATUS_OK;
        }
    }
    /* the drive is not original system drive */
    *slot_number = INVALID_SLOT_NUMBER;

    database_trace (FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: the drive in bus %d, enc %d, slot %d is not system drive\n",
                        __FUNCTION__, pdo_info->bus, pdo_info->enclosure, pdo_info->slot);

    return FBE_STATUS_OK;
    
}

/*!***************************************************************
 * @fn fbe_database_add_item_into_connect_pvd_request_PVD_list
 *****************************************************************
 * @brief
 *  This function adds a PVD into the connect pvd job request.
 *
 * @param[in]  pvd_connect_request - PVD connect request which is sent to job service
 * @param[in]  pvd_id - object id of the PVD
 *
 * @return FBE_STATUS_OK if there is no problem
 *
 * @version
 *    9/01/2012: Zhipeng Hu - created
 *
 ****************************************************************/
static fbe_status_t fbe_database_add_item_into_connect_pvd_request_PVD_list(
                                                                fbe_job_service_connect_drive_t *pvd_connect_request,
                                                                fbe_object_id_t  pvd_id)
{
    fbe_bool_t found_duplicate = FBE_FALSE;
    fbe_u32_t i = 0;

    if (NULL == pvd_connect_request || FBE_OBJECT_ID_INVALID == pvd_id)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s: Invalid param.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*check whether there is efficient resource*/
    if(pvd_connect_request->request_size >= FBE_JOB_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: there is not enough resources.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* check for duplicate */
    for (i = 0; i < pvd_connect_request->request_size; i++)
    {
        if(pvd_connect_request->PVD_list[i] == pvd_id)
        {
            found_duplicate = FBE_TRUE;
            break;
        }
    }
    
    if (found_duplicate)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, no need to add this pvd into connect pvd request, it is duplicated.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_OK;
    }

    pvd_connect_request->PVD_list[pvd_connect_request->request_size] = pvd_id;
    pvd_connect_request->request_size++;

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_is_pvd_connected_to_drive
 *****************************************************************
 * @brief
 *  This function determines whether a PVD has already been connected to a drive
 *
 * @param[in]  pvd_object_id - object id of the PVD
 * @param[out]  is_connected - the output bool result
 *
 * @return FBE_STATUS_OK if there is no problem
 *
 * @version
 *    9/01/2012: Zhipeng Hu - created
 *
 ****************************************************************/
static fbe_status_t fbe_database_is_pvd_connected_to_drive(fbe_object_id_t pvd_object_id, fbe_bool_t *is_connected)
{
    fbe_status_t status;
    fbe_base_config_downstream_edge_list_t          down_edges;

    if(NULL == is_connected)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *is_connected = FBE_FALSE;

    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_LIST,
                                         &down_edges,
                                         sizeof(down_edges),
                                         pvd_object_id,
                                         NULL,  /* no sg list*/
                                         0,  /* no sg list*/
                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                         FBE_PACKAGE_ID_SEP_0);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: failed to get downstream edges of PVD 0x%x.\n", 
                                        __FUNCTION__, pvd_object_id);
        return status;
    }

    if(0 != down_edges.number_of_downstream_edges)
    {
        *is_connected = FBE_TRUE;
    }

    return FBE_STATUS_OK;
    
}

/*!***************************************************************
 * @fn fbe_database_send_connect_pvd_request
 *****************************************************************
 * @brief
 *  This function sends a connect PVDs request to the job service
 *
 * @param[in]  pvd_connect_request - the request data
 *
 * @return FBE_STATUS_OK if there is no problem
 *
 * @version
 *    9/01/2012: Zhipeng Hu - created
 *
 ****************************************************************/
static fbe_status_t fbe_database_send_connect_pvd_request(
                                                                fbe_job_service_connect_drive_t* pvd_connect_request)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (NULL == pvd_connect_request)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Invalid param, pvd_connect_request == NULL.\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }        
    
    status = fbe_database_send_packet_to_service(FBE_JOB_CONTROL_CODE_CONNECT_DRIVE,
                                                                                                    pvd_connect_request,
                                                                                                    sizeof(*pvd_connect_request),
                                                                                                    FBE_SERVICE_ID_JOB_SERVICE,
                                                                                                    NULL,  /* no sg list*/
                                                                                                    0,  /* no sg list*/
                                                                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                                                                    FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                        "%s: send pvd connect request failed.  Error status: 0x%x\n", 
                                        __FUNCTION__, 
                                        status);
    }

    return status;
}

static fbe_status_t fbe_database_is_pvd_in_single_loop_failure_state(fbe_object_id_t object_id, fbe_bool_t* in_slf)
{
    fbe_status_t    status;
    fbe_provision_drive_info_t  pvd_info;

    if(NULL == in_slf || FBE_OBJECT_ID_INVALID == object_id)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                        "%s: not valid parameters\n", 
                                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *in_slf = FBE_FALSE;

    status = fbe_database_send_packet_to_object(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                         &pvd_info,
                                         sizeof(pvd_info),
                                         object_id,
                                         NULL,  /* no sg list*/
                                         0,  /* no sg list*/
                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                         FBE_PACKAGE_ID_SEP_0);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: failed to get information of PVD 0x%x.\n", 
                                        __FUNCTION__, object_id);
        return status;
    }

    if(FBE_PROVISION_DRIVE_SLF_THIS_SP == pvd_info.slf_state)
    {
        *in_slf = FBE_TRUE;
    }

    return status;   

}

/*!****************************************************************************
 * fbe_database_determine_drive_performance_tier_type()
 ******************************************************************************
 * @brief
 *  Given a 32bit drive type, determine the drive performance tier type
 *  to place in the raidgroup's nonpaged. Return the 32 bit value as 16bits 
 *
 * @param drive_info_p - pointer to the physical drive information
 *
 * @return drive performance tier type
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
fbe_drive_performance_tier_type_np_t 
fbe_database_determine_drive_performance_tier_type(fbe_physical_drive_mgmt_get_drive_information_t *drive_info_p) 
{

    /* Anything up to 14K is still considered 10K */
    fbe_u32_t RPM_THRESHOLD_15K = 14000;
    switch (drive_info_p->get_drive_info.drive_type) 
    {
        case (FBE_DRIVE_TYPE_SAS_NL):
            return (fbe_drive_performance_tier_type_np_t)FBE_DRIVE_PERFORMANCE_TIER_TYPE_7K;
        case (FBE_DRIVE_TYPE_SAS):
            if (drive_info_p->get_drive_info.drive_RPM < RPM_THRESHOLD_15K) {
                return (fbe_drive_performance_tier_type_np_t)FBE_DRIVE_PERFORMANCE_TIER_TYPE_10K;
            }
            return (fbe_drive_performance_tier_type_np_t)FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K;
        case (FBE_DRIVE_TYPE_SAS_FLASH_HE):
        case (FBE_DRIVE_TYPE_SATA_FLASH_HE):
        case (FBE_DRIVE_TYPE_SAS_FLASH_ME):
        case (FBE_DRIVE_TYPE_SAS_FLASH_LE):
        case (FBE_DRIVE_TYPE_SAS_FLASH_RI):
            return (fbe_drive_performance_tier_type_np_t)FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K;
        default:
            return (fbe_drive_performance_tier_type_np_t)FBE_DRIVE_PERFORMANCE_TIER_TYPE_INVALID;
    }
}

/*!****************************************************************************
 * fbe_database_get_pdo_drive_tier()
 ******************************************************************************
 * @brief
 *  Determine the drive performance tier type of the specified pdo
 *
 * @param pdo_object_id - object of the pdo to query
 * @param drive_tier_p - pointer to the physical drive performance tier
 *
 * @return status
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
static fbe_status_t fbe_database_get_pdo_drive_tier(fbe_object_id_t pdo_object_id, 
                                                    fbe_drive_performance_tier_type_np_t *drive_tier_p)
{
    fbe_status_t    status;
    fbe_physical_drive_mgmt_get_drive_information_t drive_info;

    *drive_tier_p = 0;
    status = fbe_database_send_packet_to_object(FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
                                                &drive_info,
                                                sizeof(fbe_physical_drive_mgmt_get_drive_information_t),
                                                pdo_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: failed to get drive tier information of PDO 0x%x.\n", 
                                        __FUNCTION__, pdo_object_id);
        return status;
    }

    *drive_tier_p = fbe_database_determine_drive_performance_tier_type(&drive_info);

    return status;   

}


/*!****************************************************************************
 * fbe_database_get_raidgroup_drive_tier()
 ******************************************************************************
 * @brief
 *  Determine the drive performance tier type of the specified raid group
 *
 * @param rg_object_id - object of the raid group to query
 * @param drive_tier_p - pointer to the physical drive performance tier
 *
 * @return status
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
static fbe_status_t fbe_database_get_raidgroup_drive_tier(fbe_object_id_t rg_object_id, 
                                                          fbe_drive_performance_tier_type_np_t *drive_tier_p)
{
    fbe_status_t    status;
    fbe_get_lowest_drive_tier_t                lowest_drive_tier;

    *drive_tier_p = 0;
    status = fbe_database_send_packet_to_object(FBE_RAID_GROUP_CONTROL_CODE_GET_LOWEST_DRIVE_TIER,
                                                &lowest_drive_tier,
                                                sizeof(fbe_get_lowest_drive_tier_t),
                                                rg_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get drive tier information of Raid Group 0x%x.\n", 
                       __FUNCTION__, rg_object_id);
        return status;
    }

    *drive_tier_p = lowest_drive_tier.drive_tier;

    return status;   

}


/*!****************************************************************************
 * fbe_database_connect_drive_tier()
 ******************************************************************************
 * @brief
 *  Determine whether or not to connect the drive based on the vault's drive tier
 *  and the new pdo's drive tier
 *
 * @param pdo_object_id - object of the pdo to query
 *
 * @return boolean
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
static fbe_bool_t fbe_database_skip_connect_drive_tier(fbe_object_id_t pdo_object_id) 
{
    fbe_drive_performance_tier_type_np_t rg_drive_tier = 0;
    fbe_drive_performance_tier_type_np_t pdo_drive_tier = 0;
    fbe_status_t status = FBE_STATUS_OK;

    if (!fbe_database_is_vault_drive_tier_enabled()) 
    {
        return FBE_FALSE;
    }

    fbe_database_get_raidgroup_drive_tier(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG, &rg_drive_tier);
    status = fbe_database_get_pdo_drive_tier(pdo_object_id, &pdo_drive_tier);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Error retrieving pdo drive tier: pdo 0x%x rg drive tier 0x%x. status 0x%x\n", 
                    __FUNCTION__, pdo_object_id, rg_drive_tier, status);
        return FBE_FALSE;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: pdo 0x%x drive tier 0x%x rg drive tier 0x%x.\n", 
                    __FUNCTION__, pdo_object_id, pdo_drive_tier, rg_drive_tier);

    if (pdo_drive_tier < rg_drive_tier) {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!****************************************************************************
 * fbe_database_verify_and_handle_unsupported_drive()
 ******************************************************************************
 * @brief
 *  Determine if the additional drive type is supported.  If not, it will take the
 *  drive offline.
 *
 * @param pdo_object_id_object_id - object of the pdo to query
 *
 * @return boolean  - TRUE if drive is unsupported
 *
 * @author 
 *   8/16/2015 - Created. Wayne Garrett
 ******************************************************************************/
static fbe_bool_t fbe_database_verify_and_handle_unsupported_drive(fbe_object_id_t pdo_object_id, fbe_physical_drive_mgmt_get_drive_information_t *drive_info_p)
{
    if (drive_info_p->get_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE)
    {
        if (!fbe_database_is_drive_type_le_supported(&fbe_database_service))
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "database_process_physical_drives: LE SSD not supported for 0x%x Skip!\n",
                            pdo_object_id);
            fbe_database_notify_pdo_logically_offline(pdo_object_id, FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_LE);
            return FBE_TRUE;
        }
    }

    if (drive_info_p->get_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)
    {
        if (!fbe_database_is_drive_type_ri_supported(&fbe_database_service))
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "database_process_physical_drives: RI SSD not supported for 0x%x Skip!\n",
                            pdo_object_id);
            fbe_database_notify_pdo_logically_offline(pdo_object_id, FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_RI);
            return FBE_TRUE;
        }
    }                      

    if (!fbe_database_is_flash_drive(drive_info_p->get_drive_info.drive_type) && 
         drive_info_p->get_drive_info.block_size == FBE_BE_BYTES_PER_BLOCK)
    {
        if (!fbe_database_is_drive_type_520_hdd_supported(&fbe_database_service))
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "database_process_physical_drives: 520 HDD not supported for 0x%x Skip!\n",
                            pdo_object_id);
            fbe_database_notify_pdo_logically_offline(pdo_object_id, FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_HDD_520);
            return FBE_TRUE;
        }
    }

    if(drive_info_p->get_drive_info.speed_capability < FBE_DRIVE_SPEED_12GB)
    {
        if (drive_info_p->get_drive_info.speed_capability == FBE_DRIVE_SPEED_6GB && 
            fbe_database_is_drive_type_6g_link_supported(&fbe_database_service))
        {
            // allow exception for 6G if it's enabled.   Let it come online.
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "database_process_physical_drives: Link Rate enum:%d not supported for 0x%x Skip!\n",
                            drive_info_p->get_drive_info.speed_capability, pdo_object_id);
            fbe_database_notify_pdo_logically_offline(pdo_object_id, FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LESS_12G_LINK);
            return FBE_TRUE;
        }
    }

    return FBE_FALSE;
}
