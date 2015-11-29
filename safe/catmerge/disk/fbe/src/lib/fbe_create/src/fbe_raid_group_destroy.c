/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_destroy.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for Raid Group destroy used by job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   01/06/2010:  Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_parity.h"
#include "fbe_database.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe_private_space_layout.h"

/* Number of MAX objects a Raid Group will ever have.
 *
 *  This is for each Raid Group destroy
 *
 * 16 Virtual Drives 
 *  8 Mirror under striper Raid Groups
 *  1 Striper Raid Group
 */
#define FBE_RAID_GROUP_MAX_OBJECTS 25 

/*! @def FBE_JOB_SERVICE_PACKET_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a
 *         semaphore for a packet.  If it does not complete
 *         in 20 seconds, then something is wrong.
 */
#define FBE_JOB_SERVICE_RAID_GROUP_DESTROY_PACKET_TIMEOUT  (-20 * (10000000L))

/*!*******************************************************************
 * @struct fbe_raid_group_destroy_lifecycle_state_ns_context_t
 *********************************************************************
 * @brief This is the notification context for lifecycle_state
 *
 *********************************************************************/
typedef struct fbe_raid_group_destroy_lifecycle_state_ns_context_s
{
#if 0 /* SAFEBUG - not destroyed properly, but not used either */
    fbe_semaphore_t sem;
#endif
    fbe_u32_t total_number_of_objects_to_destroy;
    fbe_u32_t fixed_total_number_of_objects_to_destroy;
    fbe_object_id_t destroy_object_list[FBE_RAID_GROUP_MAX_OBJECTS];
    fbe_object_id_t destroy_object_list_marked[FBE_RAID_GROUP_MAX_OBJECTS];
    /* add to here, if more fields to match */
}
fbe_raid_group_destroy_lifecycle_state_ns_context_t;

/*!*******************************************************************
 * @struct fbe_raid_group_destroy_notification_info_t
 *********************************************************************
 * @brief This is the notification information for a destroy
 *
 *********************************************************************/
typedef struct fbe_raid_group_destroy_notification_info_s
{
    fbe_raid_group_destroy_lifecycle_state_ns_context_t notification_context;
    fbe_notification_element_t  notification_element; 
}
fbe_raid_group_destroy_notification_info_t;

static fbe_raid_group_destroy_notification_info_t raid_group_destroy_notification_info;

fbe_status_t fbe_raid_group_destroy_all_raid_groups(job_service_raid_group_destroy_t *raid_group_destroy_request_p);

/* fill notification parameters */
void fbe_raid_group_destroy_set_notification_parameters(
        job_service_raid_group_destroy_t *in_raid_group_destroy_request_p,  
        fbe_status_t status,
        fbe_job_service_error_type_t error_code,
        fbe_u64_t job_number,
        fbe_notification_info_t *notification_info);

void fbe_raid_group_destroy_get_total_number_of_objects_to_destroy(
        fbe_raid_group_destroy_lifecycle_state_ns_context_t *raid_group_destroy_context,
        job_service_raid_group_destroy_t *raid_group_destroy_request_p);

/* Raid Group destroy library request validation function.
 */
static fbe_status_t fbe_raid_group_destroy_validation_function (fbe_packet_t *packet_p);

/* Raid Group destroy library update configuration in memory function.
 */
static fbe_status_t fbe_raid_group_destroy_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* Raid Group destroy library persist configuration in db function.
 */
static fbe_status_t fbe_raid_group_destroy_persist_configuration_db_function (fbe_packet_t *packet_p);

/* Raid Group destroy library rollback function.
 */
static fbe_status_t fbe_raid_group_destroy_rollback_function (fbe_packet_t *packet_p);

/* Raid Group destroy library commit function.
 */
static fbe_status_t fbe_raid_group_destroy_commit_function (fbe_packet_t *packet_p);

/* Job service Raid Group destroy registration.
*/
fbe_job_service_operation_t fbe_raid_group_destroy_job_service_operation = 
{
    FBE_JOB_TYPE_RAID_GROUP_DESTROY,
    {
        /* validation function */
        fbe_raid_group_destroy_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_raid_group_destroy_update_configuration_in_memory_function,

        /* persist function */
        fbe_raid_group_destroy_persist_configuration_db_function,

        /* response/rollback function */
        fbe_raid_group_destroy_rollback_function,

        /* commit function */
        fbe_raid_group_destroy_commit_function,
    }
};

static void fbe_raid_group_destroy_init_destroy_info(job_service_raid_group_destroy_t *raid_group_destroy_request_p);

static void fbe_raid_group_destroy_set_packet_status(fbe_payload_control_status_t payload_status,
                                                     fbe_packet_t *packet, fbe_status_t packet_status);

static fbe_status_t fbe_raid_group_destroy_determine_what_downstream_objects_to_remove(
        fbe_object_id_t upstream_object_id,
        fbe_object_id_t object_to_remove,
        fbe_bool_t *object_destroy);

static fbe_status_t fbe_raid_group_destroy_get_list_of_downstream_edges(
        fbe_object_id_t object_id, fbe_u32_t *number_of_edges,
        fbe_base_edge_t edge_list[]);

static fbe_status_t fbe_raid_group_destroy_get_list_of_downstream_objects(fbe_object_id_t object_id,
                                                                          fbe_object_id_t downstream_object_list[],
                                                                          fbe_u32_t *number_of_downstream_objects);

static fbe_status_t fbe_raid_group_set_raid_group_destroy(fbe_database_transaction_id_t transaction_id,
                                                   fbe_object_id_t raid_group_object_id);

static fbe_status_t fbe_raid_group_destroy_remove_virtual_drives(job_service_raid_group_destroy_t *raid_group_destroy_request_p);

static fbe_status_t fbe_raid_group_destroy_fill_list_objects_raid10(job_service_raid_group_destroy_t *raid_group_destroy_request_p,
                                                                    fbe_u32_t number_of_downstream_objects,
                                                                    fbe_object_id_t downstream_object_list[]);

static fbe_status_t fbe_raid_group_destroy_fill_list_objects_raid(job_service_raid_group_destroy_t *raid_group_destroy_request_p,
                                                                  fbe_u32_t number_of_downstream_objects,
                                                                  fbe_object_id_t downstream_object_list[]);

static fbe_status_t fbe_raid_group_destroy_pvd_update_config_type(job_service_raid_group_destroy_t *raid_group_destroy_request_p);


/*!**************************************************************
 * fbe_raid_group_init_notification_context()
 ****************************************************************
 * @brief
 * This function  inits the notification context
 *
 * @param context IN - pointer to notification context
 *
 * @return none
 *
 * @author
 *  10/08/2010 - Created. Jesus Medina
 ****************************************************************/

static void fbe_raid_group_init_notification_context(
        fbe_raid_group_destroy_lifecycle_state_ns_context_t *notification_context)
{
    fbe_u32_t i = 0;

#if 0 /* SAFEBUG - not destroyed properly, but not used either */
    fbe_semaphore_init(&notification_context->sem, 0, 1);
#endif

    notification_context->total_number_of_objects_to_destroy = 0;
    notification_context->fixed_total_number_of_objects_to_destroy = 0;

    /* init array that defines if a VD should be removed based on number of upstream edges */
    for (i = 0 ; i < FBE_RAID_GROUP_MAX_OBJECTS; i++)
    {
        notification_context->destroy_object_list[i] = FBE_OBJECT_ID_INVALID;
        notification_context->destroy_object_list_marked[i] = FBE_FALSE;
    }
    return;
}
/************************************************
 * end fbe_raid_group_init_notification_context()
 ***********************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_init_notification_info()
 ****************************************************************
 * @brief
 * This function initializes the raid group destroy notification
 * information global structure.
 *
 * @param None
 *
 * @return none
 *
 * @author
 *  10/08/2010 - Created. Jesus Medina
 ****************************************************************/

static void fbe_raid_group_destroy_init_notification_info(void)
{
    raid_group_destroy_notification_info.notification_element.notification_function = NULL;
    raid_group_destroy_notification_info.notification_element.registration_id = 0;
    raid_group_destroy_notification_info.notification_element.notification_context = NULL;
    raid_group_destroy_notification_info.notification_element.targe_package = FBE_PACKAGE_ID_SEP_0;
    fbe_raid_group_init_notification_context(&raid_group_destroy_notification_info.notification_context);
    return;
}
/****************************************************
 * end fbe_raid_group_destroy_init_notification_info()
 ****************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_notification_info_destroy()
 ****************************************************************
 * @brief
 * This function destroys the raid group destroy notification
 * information global structure.
 *
 * @param None
 *
 * @return none
 *
 * @author
 *  01/27/2011  Ron Proulx  - Created
 ****************************************************************/

void fbe_raid_group_destroy_notification_info_destroy(void)
{
    raid_group_destroy_notification_info.notification_element.notification_function = NULL;
    raid_group_destroy_notification_info.notification_element.registration_id = 0;
    raid_group_destroy_notification_info.notification_element.notification_context = NULL;
    raid_group_destroy_notification_info.notification_element.targe_package = FBE_PACKAGE_ID_SEP_0;
#if 0 /* SAFEBUG - unused and not destroyed properly */
    fbe_semaphore_destroy(&raid_group_destroy_notification_info.notification_context.sem);
#endif
    return;
}
/********************************************************
 * end fbe_raid_group_destroy_notification_info_destroy()
 *******************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_get_notification_context()
 ****************************************************************
 * @brief
 * This function returns the pointer to the global destroy
 * notification context
 *
 * @param None
 *
 * @return destroy_notification_context_p
 *
 * @author
 *  01/27/2011  Ron Proulx  - Created
 ****************************************************************/

static fbe_raid_group_destroy_lifecycle_state_ns_context_t *fbe_raid_group_destroy_get_notification_context(void)
{
    return(&raid_group_destroy_notification_info.notification_context);
}
/*******************************************************
 * end fbe_raid_group_destroy_get_notification_context()
 *******************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param destroy_request_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_destroy_validation_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    job_service_raid_group_destroy_t *raid_group_destroy_request_p = NULL;
    fbe_u32_t                            number_of_upstream_edges, i = 0;
    fbe_object_id_t                      downstream_object_list[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /*!< set of downstream objects */
    fbe_u32_t                            number_of_downstream_objects;
    fbe_bool_t                           double_degraded;
    fbe_bool_t                           is_broken = FBE_FALSE;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);

    number_of_downstream_objects = 0;
    for (i = 0; i < FBE_RAID_MAX_DISK_ARRAY_WIDTH; i++)
    {
        downstream_object_list[i] = FBE_OBJECT_ID_INVALID;
    }

    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p, FBE_STATUS_OK);
       return status;
    }

    /* get the parameters passed to the validation function */
    raid_group_destroy_request_p = (job_service_raid_group_destroy_t *)job_queue_element_p->command_data;

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* init virtual drive, edge array values */
    fbe_raid_group_destroy_init_destroy_info(raid_group_destroy_request_p);
 
    /* Get the object id from DB service */
    status = fbe_create_lib_database_service_lookup_raid_object_id(
            raid_group_destroy_request_p->user_input.raid_group_id, 
            &raid_group_destroy_request_p->raid_group_object_id);

    if(status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, Invalid raid group id %d\n", __FUNCTION__, 
                raid_group_destroy_request_p->user_input.raid_group_id);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_UNKNOWN_ID;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p,
                                                 FBE_STATUS_OK); 
        return status;
    }

    status = fbe_create_lib_validate_system_triple_mirror_healthy(&double_degraded);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s check system triple mirror healthy failed.\n", 
                __FUNCTION__);
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                packet_p,
                                                FBE_STATUS_GENERIC_FAILURE);
        return status;
    }
    if (double_degraded == FBE_TRUE)
    {
        /* If we don't allow to destroy a broken rg when double degraded, just return error */
        if (raid_group_destroy_request_p->user_input.allow_destroy_broken_rg == FBE_FALSE) {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s More than one DB drive degraded.\n", 
                    __FUNCTION__);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED;
            fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p,
                                                 FBE_STATUS_OK); 
         return status;
        } else { /* We allow to destory a broken rg */
            status = fbe_create_lib_get_raid_health(
                                           raid_group_destroy_request_p->raid_group_object_id, 
                                           &is_broken);
            if (status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s failed to check if raid group is broken\n", 
                        __FUNCTION__);
                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                packet_p,
                                                FBE_STATUS_GENERIC_FAILURE);
                return status;
            }
            /* If it is not broken, we don't allow to destroy it when system 3-way mirror is double degraded*/
            if (is_broken == FBE_FALSE) {
                job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s More than one DB drive degraded.\n", 
                    __FUNCTION__);
                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED;
                fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                     packet_p,
                                                     FBE_STATUS_OK); 
                return status;
            }
            /* If the raid is broken,
                    * we allow to destroy this broken raid group even system 3-way mirror is double degraded */
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s: system RG double degraded. but allow destroying broken RG\n", 
                                    __FUNCTION__);
        }

    }

    /* verify there are no objects above the RG object */
    number_of_upstream_edges = 0;
    status = fbe_create_lib_get_upstream_edge_count(raid_group_destroy_request_p->raid_group_object_id, 
                                                    &number_of_upstream_edges);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, could not determine number of upstream edges for RG object %d\n", 
                __FUNCTION__,
                raid_group_destroy_request_p->raid_group_object_id);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p, 
                                                 FBE_STATUS_OK); 

        return FBE_STATUS_GENERIC_FAILURE;
    } 

    if (number_of_upstream_edges > 0)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "RG Destroy Validation: RG id %d has %d upstream objects, cannot destroy\n", 
                raid_group_destroy_request_p->raid_group_object_id, number_of_upstream_edges);

        job_queue_element_p->status = status; 
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                 packet_p,
                                                 FBE_STATUS_OK); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
     
    /* obtain the Raid type, this is important for Raid 10 type as this type of raid group
     * consists of striper and mirror under striper 
     */
    status =  fbe_create_lib_get_raid_group_type(raid_group_destroy_request_p->raid_group_object_id,
                                                 &raid_group_destroy_request_p->raid_type); 

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, RG object %d, has invalid RG type %d\n", __FUNCTION__, 
                raid_group_destroy_request_p->raid_group_object_id, raid_group_destroy_request_p->raid_type);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p, 
                                                 FBE_STATUS_OK); 
        return status;
    }

    /* obtain the list of downstream objects, for Raid 10 type the list of downstream 
     * objects represents a list of mirror under striper raid type objects, for
     * all other raid group types, the list of downstream objects represents
     * a list of virtual drive objects
     */
    status = fbe_raid_group_destroy_get_list_of_downstream_objects(raid_group_destroy_request_p->raid_group_object_id,
                                                                   downstream_object_list,
                                                                   &number_of_downstream_objects);


    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                 packet_p, 
                                                 FBE_STATUS_OK); 
        return status;
    }

    /* for Raid 10, the objects below the striper raid group are mirror under striper raid groups */
    if (raid_group_destroy_request_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_raid_group_destroy_fill_list_objects_raid10(raid_group_destroy_request_p,
                                                                number_of_downstream_objects,
                                                                downstream_object_list);

        if(status != FBE_STATUS_OK)
        {
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                     packet_p, 
                                                     FBE_STATUS_OK); 
            return status;
        }
    }
    else
    {
        /*Because system Raid group doesn't have VD below it, so skip this step */
        if (!fbe_private_space_layout_object_id_is_system_raid_group(raid_group_destroy_request_p->raid_group_object_id))
        {
            status = fbe_raid_group_destroy_fill_list_objects_raid(raid_group_destroy_request_p,
                                                               number_of_downstream_objects,
                                                               downstream_object_list);
        

            if(status != FBE_STATUS_OK)
            {
                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

                fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                        packet_p, 
                                                        FBE_STATUS_OK); 
                return status;
            }
        }
    }

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_OK,
                                             packet_p, FBE_STATUS_OK);

    return status;
}
/**************************************************
 * end fbe_raid_group_destroy_validation_function()
 *************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_destroy_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    job_service_raid_group_destroy_t *raid_group_destroy_request_p = NULL;
    fbe_raid_group_destroy_lifecycle_state_ns_context_t *raid_group_destroy_context_p = NULL;
    LARGE_INTEGER                        timeout, *p_timeout; 

    timeout.QuadPart = FBE_JOB_SERVICE_RAID_GROUP_DESTROY_PACKET_TIMEOUT; 
    p_timeout = &timeout; 

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);
 
    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p, FBE_STATUS_OK);
       return status;
    }

    /* get the parameters passed to the validation function */
    raid_group_destroy_request_p = (job_service_raid_group_destroy_t *)job_queue_element_p->command_data;

    /* fbe_database_open_transaction. Should the type of transaction be given here? */
    status = fbe_create_lib_start_database_transaction(&raid_group_destroy_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, call to create transaction id failed, status %d",
                __FUNCTION__, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                 packet_p, FBE_STATUS_OK);
        return status;
    }
   
    /* init destroy raids group information  */ 
    fbe_raid_group_destroy_init_notification_info();

    /* get notification context */
    raid_group_destroy_context_p = fbe_raid_group_destroy_get_notification_context();

    /* determine number of objects to destroy so notification function callback
     * does not signal our sempahore until all objects have been sent a destroy
     * notification 
     */
    fbe_raid_group_destroy_get_total_number_of_objects_to_destroy(raid_group_destroy_context_p, 
                                                                  raid_group_destroy_request_p);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "Raid Group destroy: total_number_of_objects_to_destroy %d \n",
            raid_group_destroy_context_p->total_number_of_objects_to_destroy);

    /* first destroy all Raid groups,  single destroy Raid Group for all raid group types
     * except Raid 10 type
    */
    status = fbe_raid_group_destroy_all_raid_groups(raid_group_destroy_request_p);
    if (status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p, FBE_STATUS_OK); 
        return status;
    }

    /*update pvd config type*/
    status = fbe_raid_group_destroy_pvd_update_config_type(raid_group_destroy_request_p);

    /* call the function to destroy the virtual drives, only the ones that are determined to be destroyed */
    status = fbe_raid_group_destroy_remove_virtual_drives(raid_group_destroy_request_p);
    if(status != FBE_STATUS_OK)  
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, call to remove VD objects from RG object failed, status %d\n",
                __FUNCTION__, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                 packet_p, FBE_STATUS_OK); 
        return status;
    }

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_OK,
                                             packet_p, FBE_STATUS_OK);

    return status;
}
/**********************************************************
 * end fbe_raid_group_destroy_update_configuration_in_memory_function()
 **********************************************************/


/*!**************************************************************
 * fbe_raid_group_destroy_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_destroy_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    job_service_raid_group_destroy_t *raid_group_destroy_request_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);

    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p, FBE_STATUS_OK);
       return status;
    }

    /* get the parameters passed to the validation function */
    raid_group_destroy_request_p = (job_service_raid_group_destroy_t *)job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(raid_group_destroy_request_p->transaction_id);
    if(status != FBE_STATUS_OK) 
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                 packet_p, FBE_STATUS_OK);
        return status;
    }

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_OK,
                                             packet_p, FBE_STATUS_OK);

    return status;

}
/**********************************************************
 * end fbe_raid_group_destroy_persist_configuration_db_function()
 **********************************************************/


/*!**************************************************************
 * fbe_raid_group_destroy_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_destroy_rollback_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    job_service_raid_group_destroy_t     *raid_group_destroy_request_p = NULL;
    fbe_notification_info_t              notification_info;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);

    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p, FBE_STATUS_OK);
       return status;
    }

    /* get the parameters passed to the validation function */
    raid_group_destroy_request_p = (job_service_raid_group_destroy_t *)job_queue_element_p->command_data;

    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY))
    {
        /* in the memory function we register for object destroy notification,
         * on error we must unregister 
         */

        if (raid_group_destroy_request_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s: aborting transaction id %llu\n",
                    __FUNCTION__,
		    (unsigned long long)raid_group_destroy_request_p->transaction_id);

            status = fbe_create_lib_abort_database_transaction(raid_group_destroy_request_p->transaction_id);
            if(status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: Could not abort transaction id %llu with configuration service\n", 
                        __FUNCTION__,
		        (unsigned long long)raid_group_destroy_request_p->transaction_id);

                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

                fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                         packet_p, FBE_STATUS_OK);
                return status;
            }
        }
    }

    /* fill notification parameters */
    fbe_raid_group_destroy_set_notification_parameters(raid_group_destroy_request_p,  
                                                       job_queue_element_p->status,
                                                       job_queue_element_p->error_code,
                                                       job_queue_element_p->job_number,
                                                       &notification_info);

    /* Send a notification for this rollback*/
    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: job status notification failed, status: 0x%X\n",
                __FUNCTION__, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                 packet_p,
                                                 FBE_STATUS_OK);
        return status;
   }

    /* Don't set the status and error_code here. Just return what is in the job_queue_element. */

    fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_OK,
                                             packet_p,
                                             FBE_STATUS_OK);
    return status;
}
/**********************************************************
 * end fbe_raid_group_destroy_rollback_function()
 **********************************************************/


/*!**************************************************************
 * fbe_raid_group_destroy_commit_function()
 ****************************************************************
 * @brief
 * This function is used to committ the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 *  11/15/2012 - Modified by Vera Wang
 ****************************************************************/
static fbe_status_t fbe_raid_group_destroy_commit_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    job_service_raid_group_destroy_t *raid_group_destroy_request_p = NULL;
    fbe_notification_info_t              notification_info;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);
 
    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p, FBE_STATUS_OK);
       return status;
    }

    /* get the parameters passed to the validation function */
    raid_group_destroy_request_p = (job_service_raid_group_destroy_t *)job_queue_element_p->command_data;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s entry, job number 0x%llx, status 0x%x, error_code %d\n",
        __FUNCTION__,
        (unsigned long long)job_queue_element_p->job_number, 
        job_queue_element_p->status,
        job_queue_element_p->error_code);

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    job_queue_element_p->object_id = raid_group_destroy_request_p->raid_group_object_id;
    job_queue_element_p->need_to_wait = raid_group_destroy_request_p->user_input.wait_destroy;
    job_queue_element_p->timeout_msec = raid_group_destroy_request_p->user_input.destroy_timeout_msec;

    /* fill notification parameters */
    fbe_raid_group_destroy_set_notification_parameters(raid_group_destroy_request_p,  
                                                       FBE_STATUS_OK,
                                                       FBE_JOB_SERVICE_ERROR_NO_ERROR,
                                                       job_queue_element_p->job_number,
                                                       &notification_info);

    /*! if upper_layer(user) choose to wait for RG/LUN create/destroy lifecycle ready/destroy before job finishes.
        So that upper_layer(user) can update RG/LUN only depending on job notification without worrying about lifecycle state.
    */
    if(raid_group_destroy_request_p->user_input.wait_destroy) {
        status = fbe_job_service_wait_for_expected_lifecycle_state(raid_group_destroy_request_p->raid_group_object_id, 
                                                                  FBE_LIFECYCLE_STATE_NOT_EXIST, 
                                                                  raid_group_destroy_request_p->user_input.destroy_timeout_msec);
        if (status != FBE_STATUS_OK)
        {

            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s timeout for waiting rg object id %d destroy. status %d\n", 
                              __FUNCTION__,  raid_group_destroy_request_p->raid_group_object_id, status);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_TIMEOUT;
            fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                     packet_p, FBE_STATUS_OK);
            return status;
        }
    }

    /* Send a notification for this commit*/
    status = fbe_notification_send(raid_group_destroy_request_p->raid_group_object_id, notification_info);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: job status notification failed, status: 0x%X\n",
                          __FUNCTION__, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_FAILURE, 
                                                 packet_p, FBE_STATUS_OK);
        return status;
    }

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_raid_group_destroy_set_packet_status(FBE_PAYLOAD_CONTROL_STATUS_OK,
                                             packet_p, FBE_STATUS_OK);
    return status;
}
/**********************************************************
 * end fbe_raid_group_destroy_commit_function()
 **********************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_set_notification_parameters()
 ****************************************************************
 * @brief
 * This funtion sets the valus for a notification info structure
 *
 * @param raid_group_destroy_request_p - job service request
 * @param status - status to send to receiver of notification.
 * @param error_code - if valid error, error found while destroying raid group
 * @param notification_info - extended data for receiver of notification.
 *
 * @return status - none
 *
 * @author
 *  10/11/2010 - Created. Jesus Medina
 ****************************************************************/

void fbe_raid_group_destroy_set_notification_parameters(
        job_service_raid_group_destroy_t *raid_group_destroy_request_p,  
        fbe_status_t status,
        fbe_job_service_error_type_t error_code,
        fbe_u64_t job_number,
        fbe_notification_info_t *notification_info)
{
    notification_info->notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info->notification_data.job_service_error_info.object_id = 
        raid_group_destroy_request_p->raid_group_object_id;

    notification_info->notification_data.job_service_error_info.status = status;
    notification_info->notification_data.job_service_error_info.error_code = error_code;
    notification_info->notification_data.job_service_error_info.job_number = job_number;
    notification_info->notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_RAID_GROUP_DESTROY;
    notification_info->class_id = FBE_CLASS_ID_INVALID;
    notification_info->object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
}
/**************************************************
 * end fbe_raid_group_destroy_set_notification_parameters()
 **************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_get_list_of_downstream_edges()
 ****************************************************************
 * @brief
 * This function sends a packet to the base config to obtain a
 * list of downstream edges that are attached to the raid group
 * object
 *
 * @param edge_list - data structure that will be filled with
 * array of edges
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/23/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_destroy_get_list_of_downstream_edges(
        fbe_object_id_t object_id, fbe_u32_t *number_of_edges,
        fbe_base_edge_t edge_list[])
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_base_config_downstream_edge_list_t downstream_edge_list;
    fbe_u32_t                              i = 0;

    *number_of_edges = 0;
    downstream_edge_list.number_of_downstream_edges = 0;
    status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_LIST,
                                                 &downstream_edge_list,
                                                 sizeof(fbe_base_config_downstream_edge_list_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, get downstream edge list call failed, status %d\n", __FUNCTION__,
                status);
    }
    else
    {    
        *number_of_edges =  downstream_edge_list.number_of_downstream_edges;
        for (i = 0; i < downstream_edge_list.number_of_downstream_edges; i++)
        {
            edge_list[i] = downstream_edge_list.downstream_edge_list[i];
        }
    }
    return status;
}
/***********************************************************
 * end fbe_raid_group_destroy_get_list_of_downstream_edges()
 **********************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_get_list_of_downstream_objects()
 ****************************************************************
 * @brief
 * This function sends a packet to the base config to obtain a
 * list of downstream objects that are attached to the raid group
 * object
 *
 * @param edge_list - data structure that will be filled with
 * array of objects
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/16/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_destroy_get_list_of_downstream_objects(fbe_object_id_t object_id,
                                                                          fbe_object_id_t downstream_object_list[],
                                                                          fbe_u32_t *number_of_downstream_objects)

{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_downstream_object_list_t        config_downstream_object_list;
    fbe_u16_t                                       index = 0;

    /* Initialize downstream object list before sending control command to get info. */
    *number_of_downstream_objects = 0; 
    config_downstream_object_list.number_of_downstream_objects = 0;
    for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        config_downstream_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }
 
    status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                 &config_downstream_object_list,
                                                 sizeof(fbe_base_config_downstream_object_list_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, get downstream object list call failed, status %d\n", __FUNCTION__,
                status);
        return status;
    }
    
    for(index = 0; index < config_downstream_object_list.number_of_downstream_objects; index++)
    {
        downstream_object_list[index] = config_downstream_object_list.downstream_object_list[index];
    }
    *number_of_downstream_objects = config_downstream_object_list.number_of_downstream_objects; 

    return status;
}
/***********************************************************
 * end fbe_raid_group_destroy_get_list_of_downstream_objects()
 **********************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_determine_what_downstream_objects_to_remove()
 ****************************************************************
 * @brief
 * This function sends a packet to the base config to determine
 * which of the virtual drive objects can be destroyed
 *
 * @param upstream_object_id - parent object id
 * @param object_to_remove - object to remove
 * @param object_destroy - defines if object to remove can be
 * removed
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/18/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_destroy_determine_what_downstream_objects_to_remove(
        fbe_object_id_t upstream_object_id,
        fbe_object_id_t object_to_remove,
        fbe_bool_t *object_destroy)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_base_config_can_object_be_removed_t config_object_to_remove;

    config_object_to_remove.object_id = FBE_OBJECT_ID_INVALID;
    config_object_to_remove.object_to_remove = FBE_FALSE;

    if (object_to_remove != FBE_OBJECT_ID_INVALID)
    {
        config_object_to_remove.object_id = upstream_object_id;
        status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_CAN_OBJECT_BE_REMOVED,
                                                     &config_object_to_remove,
                                                     sizeof(fbe_base_config_can_object_be_removed_t),
                                                     FBE_PACKAGE_ID_SEP_0,
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     object_to_remove,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

            if (status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "rg_destroy_determ_dwnobj_rm , can't determine obj cfg for obj id %d, status %d\n",
                        object_to_remove,
                        status);

                return status;
            }
            else
            {
                *object_destroy = config_object_to_remove.object_to_remove;  
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "rg_destroy_determ_dwnobj_rm, obj %d removed value %d(1=yes, 0=no)\n", 
                        object_to_remove,
                        *object_destroy);
            }
    }
    return status;
}
/**************************************************************************
 * end fbe_raid_group_destroy_determine_what_downstream_objects_to_remove()
 *************************************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_init_destroy_info()
 ****************************************************************
 * @brief
 * This function inits the arrays used for edges, virtual drives
 * inside the data structure passed

 * @param raid_group_destroy_request_p - data structure containing
 * the array of virtual drives to query if they can be removed
 *
 * @return none 
 *
 * @author
 *  02/23/2010 - Created. Jesus Medina
 ****************************************************************/

static void fbe_raid_group_destroy_init_destroy_info(
        job_service_raid_group_destroy_t *raid_group_destroy_request_p)
{
    fbe_u32_t                               i = 0;
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

     raid_group_destroy_request_p->status = FBE_STATUS_OK;
     raid_group_destroy_request_p->number_of_virtual_drive_objects = 0;
     raid_group_destroy_request_p->number_of_mirror_under_striper_objects = 0;
     for ( i = 0; i < FBE_RAID_MAX_DISK_ARRAY_WIDTH; ++i )
     {
         raid_group_destroy_request_p->virtual_drive_object_list[i] = FBE_OBJECT_ID_INVALID;
         raid_group_destroy_request_p->virtual_drive_object_destroy_list[i] = FBE_FALSE;
     }
     for ( i = 0; i < FBE_RAID_MAX_DISK_ARRAY_WIDTH / 2; ++i )
     {
         raid_group_destroy_request_p->raid_object_list[i] = FBE_OBJECT_ID_INVALID;	 
     }
     
     raid_group_destroy_request_p->transaction_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
     raid_group_destroy_request_p->raid_type = FBE_RAID_GROUP_TYPE_UNKNOWN;
     raid_group_destroy_request_p->status = FBE_STATUS_OK;
}
/************************************************
 * end fbe_raid_group_destroy_init_destroy_info()
 ************************************************/

/*!**************************************************************
 * fbe_raid_group_set_raid_group_destroy()
 ****************************************************************
 * @brief
 * This function destroys a raid group
 *
 * @param transaction_id IN- id used for the configuration service
 * to persist objects
 * @param object_id IN -  id of raid group to destroy
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/23/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_set_raid_group_destroy(fbe_database_transaction_id_t transaction_id,
                                                          fbe_object_id_t raid_group_object_id)
{
    fbe_status_t                                   status = FBE_STATUS_OK;
    fbe_database_control_destroy_object_t          destroy_object;

    destroy_object.transaction_id = transaction_id;
    destroy_object.object_id      = raid_group_object_id;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry, raid group id %d \n", 
            __FUNCTION__, raid_group_object_id);

     status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_DESTROY_RAID,
                                                 &destroy_object,
                                                 sizeof(fbe_database_control_destroy_object_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 destroy_object.object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

     if (status == FBE_STATUS_OK)
     {
         job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                 "%s entry\n", 
                 __FUNCTION__);
     }

     return status;
}
/*********************************************
 * end fbe_raid_group_set_raid_group_destroy()
 *********************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_remove_virtual_drives()
 ****************************************************************
 * @brief
 * This function destroys a raid group's virtual drives
 *
 * @param raid_group_destroy_request_p IN - pointer to data structure
 * which contains transction_id and ids of raid group virtual drives
 * to destroy
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/23/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_destroy_remove_virtual_drives(job_service_raid_group_destroy_t *raid_group_destroy_request_p)
{
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_database_control_destroy_object_t        destroy_virtual_drive;
    fbe_u32_t                                         i = 0;

    for ( i = 0; i < raid_group_destroy_request_p->number_of_virtual_drive_objects; ++i )
    {
        if (raid_group_destroy_request_p->virtual_drive_object_list[i] != FBE_OBJECT_ID_INVALID)
        {
            if (raid_group_destroy_request_p->virtual_drive_object_destroy_list[i] == FBE_TRUE)
            {
                destroy_virtual_drive.transaction_id = raid_group_destroy_request_p->transaction_id;
                destroy_virtual_drive.object_id      = raid_group_destroy_request_p->virtual_drive_object_list[i];

                status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_DESTROY_VD,
                                                             &destroy_virtual_drive,
                                                             sizeof(fbe_database_control_destroy_object_t),
                                                             FBE_PACKAGE_ID_SEP_0,                                                             
                                                             FBE_SERVICE_ID_DATABASE,
                                                             FBE_CLASS_ID_INVALID,
                                                             FBE_OBJECT_ID_INVALID,
                                                             FBE_PACKET_FLAG_NO_ATTRIB);

                if (status == FBE_STATUS_OK)
                {
                    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry, removed virtual drive %d\n", 
                            __FUNCTION__, destroy_virtual_drive.object_id);
                }
            }
            else
            {
                job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry, did not removed virtual drive %d because it has more upstream objects connected to it\n", 
                        __FUNCTION__,
                        raid_group_destroy_request_p->virtual_drive_object_list[i]);
            }
        }
    }

    return status;
}
/****************************************************
 * end fbe_raid_group_destroy_remove_virtual_drives()
 ***************************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_set_packet_status()
 ****************************************************************
 * @brief
 * This function sets the packet status and payload status
 *
 * @param control_operation - control operation for payload status 
 * @param fbe_payload_control_status_t - payload status
 * @param packet - packet whose status will be set
 * @param packet_status - setting of packet's status
 *
 * @return status
 *
 * @author
 *  01/22/2010 - Created. Jesus Medina
 ****************************************************************/
static void fbe_raid_group_destroy_set_packet_status(fbe_payload_control_status_t payload_status,
                                                     fbe_packet_t *packet_p, fbe_status_t packet_status)
{
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, packet_status, 0);
    fbe_transport_complete_packet(packet_p);
}
/***********************************************
 * end fbe_raid_group_destroy_set_packet_status()
 ***********************************************/

/*!**************************************************************
 * fbe_raid_group_destroy_fill_list_objects_raid10()
 ****************************************************************
 * @brief
 * This function determines what mirror under striper objects and
 * VDs connected to the mirrors are part of the raid to be destroyed
 *
 * @param raid_group_destroy_request_p IN - pointer to data structure
 * which contains transaction_id and ids of raid group virtual drives
 * to destroy
 * @param number_of_downstream_objects IN -number of mirror objects 
 * @param downstream_object_list IN - array of mirror objects
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/18/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_destroy_fill_list_objects_raid10(job_service_raid_group_destroy_t *raid_group_destroy_request_p,
                                                                  fbe_u32_t number_of_downstream_objects,
                                                                  fbe_object_id_t downstream_object_list[])
{
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_u32_t                                         i = 0, j = 0;
    fbe_u32_t                                         vd_index = 0;
    fbe_bool_t                                        virtual_drive_object_destroy_list[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s: Raid type -> %d\n",
            __FUNCTION__, raid_group_destroy_request_p->raid_type);

    /* init array that defines if a VD should be removed based on number of upstream edges */
    for (i=0; i < FBE_RAID_MAX_DISK_ARRAY_WIDTH; i++)
    {
        virtual_drive_object_destroy_list[i] = FBE_FALSE;
    }

    /* for a raid 10 type the number of downstream objects
     * means the number of mirror under striper  
     */
    raid_group_destroy_request_p->number_of_mirror_under_striper_objects = number_of_downstream_objects;
    for (i = 0; i < number_of_downstream_objects; ++i)
    {
        raid_group_destroy_request_p->raid_object_list[i] = downstream_object_list[i];
    }

    /* for each mirror under striper obtain the set of VD objects--should be 2 per raid */
    for (i = 0; i < raid_group_destroy_request_p->number_of_mirror_under_striper_objects; i++)
    {
        status = fbe_raid_group_destroy_get_list_of_downstream_objects(raid_group_destroy_request_p->raid_object_list[i],
                                                                       downstream_object_list,
                                                                       &number_of_downstream_objects);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s entry, can not obtain downstream objects list for object for %d, status %d\n",
                    __FUNCTION__,
                    raid_group_destroy_request_p->raid_object_list[i],
                    status);
            return status;
        }

        /* each mirror under striper should have 2 downstream objects, 
         * for each mirror under striper loop thru the objects to get the VD object id
         */
        for (j = 0; j < number_of_downstream_objects; j++)
        {
            raid_group_destroy_request_p->virtual_drive_object_list[vd_index] = downstream_object_list[j];

            /* determine if downstream object can be removed when the destroy command is executed,
            */
            status = fbe_raid_group_destroy_determine_what_downstream_objects_to_remove(
                    raid_group_destroy_request_p->raid_object_list[i],
                    raid_group_destroy_request_p->virtual_drive_object_list[vd_index],
                    &raid_group_destroy_request_p->virtual_drive_object_destroy_list[vd_index]);

            if (status != FBE_STATUS_OK) 
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s entry, can not determine what objects to remove, status %d\n",
                        __FUNCTION__,
                        status);
                return status;
            }
            vd_index += 1;
        }
    }

    /* number of downstream objects should be the total number of VDs the R10 consumes */
    raid_group_destroy_request_p->number_of_virtual_drive_objects = vd_index;

    return status;
}
/*******************************************************
 * end fbe_raid_group_destroy_fill_list_objects_raid10()
 ******************************************************/


/*!**************************************************************
 * fbe_raid_group_destroy_fill_list_objects_raid()
 ****************************************************************
 * @brief
 * This function determines what Vd objects connected to the mirrors
 * are part of the raid to be destroyed
 *
 * @param raid_group_destroy_request_p IN - pointer to data structure
 * which contains transaction_id and ids of raid group virtual drives
 * to destroy
 * @param number_of_downstream_objects IN -number of VD objects 
 * @param downstream_object_list IN - array of VD objects
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/18/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_destroy_fill_list_objects_raid(job_service_raid_group_destroy_t *raid_group_destroy_request_p,
                                                                  fbe_u32_t number_of_downstream_objects,
                                                                  fbe_object_id_t downstream_object_list[])
{
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_u32_t                                         i = 0;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s: Raid type -> %d\n",
            __FUNCTION__, raid_group_destroy_request_p->raid_type);

    raid_group_destroy_request_p->number_of_virtual_drive_objects = number_of_downstream_objects;
    for (i = 0; i < number_of_downstream_objects; ++i)
    {
        raid_group_destroy_request_p->virtual_drive_object_list[i] = downstream_object_list[i];

        /* determine which downstream objects can be removed when the destroy command is executed,
         */
        status = fbe_raid_group_destroy_determine_what_downstream_objects_to_remove(
                raid_group_destroy_request_p->raid_group_object_id,
                raid_group_destroy_request_p->virtual_drive_object_list[i],
                &raid_group_destroy_request_p->virtual_drive_object_destroy_list[i]);

        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s entry, can not determine what objects to remove, status %d\n",
                    __FUNCTION__,
                    status);
            return status;
        }
    }

    return status;
}
/*****************************************************
 * end fbe_raid_group_destroy_fill_list_objects_raid()
 ****************************************************/

/*!*********************************************************************
 * fbe_raid_group_destroy_all_raid_groups()
 ***********************************************************************
 * @brief
 * This function destroys single or multiple Raid groups-Raid 10 case
 *
 * @param raid_group_destroy_request_p IN - pointer to destroy data
 *
 * @return none
 *
 * @author
 *  10/12/2010 - Created. Jesus Medina
 ****************************************************************/
fbe_status_t fbe_raid_group_destroy_all_raid_groups(job_service_raid_group_destroy_t *raid_group_destroy_request_p)
{
    fbe_u32_t i = 0;
    fbe_status_t status = FBE_STATUS_OK;

    /* call the function to destroy the raid group */
    status = fbe_raid_group_set_raid_group_destroy(raid_group_destroy_request_p->transaction_id,
                                                   raid_group_destroy_request_p->raid_group_object_id);

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, call to destroy RG object %d failed, status %d\n",
                __FUNCTION__, raid_group_destroy_request_p->raid_group_object_id, status);
        return status;
    }

    /* if raid type is Raid 10, destroy all mirror under striper raid groups next */
    if (raid_group_destroy_request_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {       
        for (i = 0; i < raid_group_destroy_request_p->number_of_mirror_under_striper_objects; i++)
        {
            /* call the function to destroy the middle raid groups */
            status = fbe_raid_group_set_raid_group_destroy(raid_group_destroy_request_p->transaction_id,
                                                           raid_group_destroy_request_p->raid_object_list[i]);

            if (status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s entry, call to destroy RG object %d failed, status %d\n",
                        __FUNCTION__, raid_group_destroy_request_p->raid_object_list[i], status);
                return status;
            }
        }
    }

    return status;
}


/*!*********************************************************************
 * fbe_raid_group_destroy_get_total_number_of_objects_to_destroy()
 ***********************************************************************
 * @brief
 * This function collects the ids of all objects to be destroyed
 *
 * @param context IN - pointer to notification context
 *
 * @return none
 *
 * @author
 *  10/08/2010 - Created. Jesus Medina
 ****************************************************************/
void fbe_raid_group_destroy_get_total_number_of_objects_to_destroy(
        fbe_raid_group_destroy_lifecycle_state_ns_context_t *raid_group_destroy_context,
        job_service_raid_group_destroy_t *raid_group_destroy_request_p)
{
    fbe_u32_t i = 0, destroy_index = 0;

    raid_group_destroy_context->destroy_object_list[destroy_index] = 
        raid_group_destroy_request_p->raid_group_object_id;

    raid_group_destroy_context->total_number_of_objects_to_destroy += 1;

    if (raid_group_destroy_request_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {       
        for (i = 0; i < raid_group_destroy_request_p->number_of_mirror_under_striper_objects; i++)
        {
            destroy_index += 1;
            raid_group_destroy_context->destroy_object_list[destroy_index] =
                raid_group_destroy_request_p->raid_object_list[i];
            raid_group_destroy_context->total_number_of_objects_to_destroy += 1;
        }
    }

    for ( i = 0; i < raid_group_destroy_request_p->number_of_virtual_drive_objects; ++i )
    {
        if (raid_group_destroy_request_p->virtual_drive_object_list[i] != FBE_OBJECT_ID_INVALID)
        {
            if (raid_group_destroy_request_p->virtual_drive_object_destroy_list[i] == FBE_TRUE)
            {
                destroy_index += 1;
                raid_group_destroy_context->destroy_object_list[destroy_index]  = 
                    raid_group_destroy_request_p->virtual_drive_object_list[i];
                raid_group_destroy_context->total_number_of_objects_to_destroy += 1;
            }
        }
    }

    raid_group_destroy_context->fixed_total_number_of_objects_to_destroy = 
        raid_group_destroy_context->total_number_of_objects_to_destroy;
}
/*********************************************************************
 * end fbe_raid_group_destroy_get_total_number_of_objects_to_destroy()
 ********************************************************************/

/*!*********************************************************************
 * fbe_raid_group_destroy_pvd_update_config_type()
 ***********************************************************************
 * @brief
 * This function updates the config type of a pvd on rg destroy
 *
 * @param context IN - raid_group_destroy_request_p IN - pointer to destroy data
 *
 * @return none
 *
 * @author
 *  07/2011 - Created. Sonali K
 ****************************************************************/
fbe_status_t fbe_raid_group_destroy_pvd_update_config_type(job_service_raid_group_destroy_t *raid_group_destroy_request_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
#if 0 /* This is a day one bug: double update simply does not work */
    fbe_database_control_update_pvd_t               update_provision_drive = {0};  
#endif  
    fbe_base_config_upstream_object_list_t          upstream_object_list = {0};
    fbe_base_config_downstream_object_list_t        in_raid_group_downstream_object_list_p = {0};
    fbe_u16_t                                       i = 0, j = 0;

    for ( i = 0; i < raid_group_destroy_request_p->number_of_virtual_drive_objects; ++i )
    {
        if (raid_group_destroy_request_p->virtual_drive_object_list[i] != FBE_OBJECT_ID_INVALID)
        {
            if (raid_group_destroy_request_p->virtual_drive_object_destroy_list[i] == FBE_TRUE)
            {               

                /*Find the number of  upstream objects connected to VD so we know if it belongs to more than 1 RG*/
                /*Since the RG object is already destroyed when we enter this function we need to update PVD only
                  if number of upstream objects are zero */
                status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                             &upstream_object_list,
                                                             sizeof(fbe_base_config_upstream_object_list_t),
                                                             FBE_PACKAGE_ID_SEP_0,
                                                             FBE_SERVICE_ID_TOPOLOGY,
                                                             FBE_CLASS_ID_INVALID,
                                                             raid_group_destroy_request_p->virtual_drive_object_list[i],
                                                             FBE_PACKET_FLAG_NO_ATTRIB);  
                
                if (status != FBE_STATUS_OK)
                {
                    job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s entry, get upstream object list call failed, status %d\n", __FUNCTION__,
                                      status);
                    return status;
                }
                        
                if(upstream_object_list.number_of_upstream_objects == 0)
                {
                    /*find the downstream object of the VD to get the PVD object id*/                    
                     /*get PVD obj id*/
                    status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                                 &in_raid_group_downstream_object_list_p,
                                                                 sizeof(fbe_base_config_downstream_object_list_t),
                                                                 FBE_PACKAGE_ID_SEP_0,
                                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                                 FBE_CLASS_ID_INVALID,
                                                                 raid_group_destroy_request_p->virtual_drive_object_list[i],
                                                                 FBE_PACKET_FLAG_NO_ATTRIB); 
                }    
                else
                {   
                    continue;
                }
				
                if (status != FBE_STATUS_OK)
                {
                    job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s entry, get downstream object list call failed, status %d\n", __FUNCTION__,
                                      status);
                    return status;
                }

				for(j = 0; j < in_raid_group_downstream_object_list_p.number_of_downstream_objects; j++)
				{
					/*Both source drive and destination drive should be marked UNCONSUMED and INVALID_POOL_ID*/
					status = fbe_create_lib_database_service_update_pvd_config_type(
											raid_group_destroy_request_p->transaction_id, 
											in_raid_group_downstream_object_list_p.downstream_object_list[j],
											FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED);

					if (status != FBE_STATUS_OK) {
						job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s entry, could not update the PVD config type, PVD object:%d, status %d\n",
								__FUNCTION__, in_raid_group_downstream_object_list_p.downstream_object_list[j], status);
					}


#if 0 /* This is a day one bug: double update simply does not work */

	                /* Update the transaction details to update the configuration. */
	                update_provision_drive.transaction_id = raid_group_destroy_request_p->transaction_id;                            
	                update_provision_drive.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED,
	                update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE;
	                update_provision_drive.object_id = in_raid_group_downstream_object_list_p.downstream_object_list[j];
	                                                         
	                /*update pvd config type*/
	                 status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
	                                                             &update_provision_drive,
	                                                             sizeof(fbe_database_control_update_pvd_t),
	                                                             FBE_PACKAGE_ID_SEP_0,
	                                                             FBE_SERVICE_ID_DATABASE,
	                                                             FBE_CLASS_ID_INVALID,
	                                                             FBE_OBJECT_ID_INVALID,
	                                                             FBE_PACKET_FLAG_NO_ATTRIB);   
	                 if(status == FBE_STATUS_OK)
	                 {
	                     job_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,
	                                       FBE_TRACE_MESSAGE_ID_INFO,
	                                       "%s: pvd object's config type updated, pvd_obj_id:0x%x\n",
	                                       __FUNCTION__,update_provision_drive.object_id);
	        
	                 }      

					 /* Mark the drive as not being in a pool. */
	                update_provision_drive.transaction_id = raid_group_destroy_request_p->transaction_id;                            
	                update_provision_drive.pool_id = FBE_POOL_ID_INVALID,
	                update_provision_drive.update_type = FBE_UPDATE_PVD_POOL_ID;
	                update_provision_drive.object_id = in_raid_group_downstream_object_list_p.downstream_object_list[j];
	                                                         
	                /*update pvd config type*/
	                status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
	                                                             &update_provision_drive,
	                                                             sizeof(fbe_database_control_update_pvd_t),
	                                                             FBE_PACKAGE_ID_SEP_0,
	                                                             FBE_SERVICE_ID_DATABASE,
	                                                             FBE_CLASS_ID_INVALID,
	                                                             FBE_OBJECT_ID_INVALID,
	                                                             FBE_PACKET_FLAG_NO_ATTRIB);   
	                if(status == FBE_STATUS_OK)
	                {
	                     job_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,
	                                       FBE_TRACE_MESSAGE_ID_INFO,
	                                       "%s: pvd object's config type updated, pvd_obj_id:0x%x\n",
	                                       __FUNCTION__,update_provision_drive.object_id);	        
	                }
#endif
				} /* for(j = 0; j < in_raid_group_downstream_object_list_p.number_of_downstream_objects; j++) */
            } /* (raid_group_destroy_request_p->virtual_drive_object_destroy_list[i] == FBE_TRUE) */
        }
    }
	
    return status;
}




