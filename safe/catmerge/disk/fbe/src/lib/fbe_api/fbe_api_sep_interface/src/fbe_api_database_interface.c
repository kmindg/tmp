/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************/
/** @file fbe_api_database_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_database interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_database_interface
 *
 * @version
 *   03/25/2011:  Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe_database.h"
#include "fbe_fru_signature.h"
#include "fbe_imaging_structures.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/**************************************
                Defines
**************************************/
#define FBE_JOB_SERVICE_WAIT_VALIDATION_TIMEOUT	30000 /*ms*/

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

/****************************************************************/
/* send_control_packet_to_database_service()
 ****************************************************************
 * @brief
 *  This function starts a transaction
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param function - pointer to the function
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/14/11 - Created. 
 *
 ****************************************************************/
static fbe_status_t send_control_packet_to_database_service(fbe_payload_control_operation_opcode_t control_code,
                                                          fbe_payload_control_buffer_t           buffer,
                                                          fbe_payload_control_buffer_length_t    buffer_length)
{
    fbe_api_control_operation_status_info_t  status_info;

    fbe_status_t status = fbe_api_common_send_control_packet_to_service (control_code, buffer, buffer_length,
                                                                         FBE_SERVICE_ID_DATABASE,
                                                                         FBE_PACKET_FLAG_INTERNAL, &status_info,
                                                                         FBE_PACKAGE_ID_SEP_0);
    
    if (status == FBE_STATUS_NO_OBJECT && status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                       "%s:No object found!!\n",
                       __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if ( ((status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) &&
        (status != FBE_STATUS_NOT_INITIALIZED) && (status != FBE_STATUS_COMPONENT_NOT_FOUND))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/****************************************************************/
/* send_control_packet_to_database_service_with_sg_list()
 ****************************************************************
 * @brief
 *  This function starts a transaction
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param sg_list - pointer to scatter gather list.
 * @param sg_list_count - number of scatter gather list elements.
 * @param function - pointer to the function
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/21/10 - Created. Sanjay Bhave
 *
 ****************************************************************/
static fbe_status_t send_control_packet_to_database_service_with_sg_list(fbe_payload_control_operation_opcode_t control_code, 
                                                                         fbe_payload_control_buffer_t           buffer, 
                                                                         fbe_payload_control_buffer_length_t    buffer_length, 
                                                                         fbe_sg_element_t                       *sg_list, 
                                                                         fbe_u32_t                              sg_list_count, 
                                                                         const char                             *function)
{
    fbe_api_control_operation_status_info_t  status_info;

    fbe_status_t status = fbe_api_common_send_control_packet_to_service_with_sg_list (control_code, buffer, buffer_length, 
                                                                                      FBE_SERVICE_ID_DATABASE, 
                                                                                      FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED, 
                                                                                      sg_list, sg_list_count, 
                                                                                      &status_info, 
                                                                                      FBE_PACKAGE_ID_SEP_0);
    
    if ( ((status != FBE_STATUS_OK) || (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) &&
        (status != FBE_STATUS_NOT_INITIALIZED) && (status != FBE_STATUS_COMPONENT_NOT_FOUND))
    {
fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", function,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*****************************************************************/
/** fbe_api_database_lookup_raid_group_by_number
 ****************************************************************
 * @brief
 *  This function looks up a raid group's object id by using it's number
 *
 * @param raid_group_id(in) - the number of the raid group
 * @param object_id(out)    - the number looked up object id
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/14/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_number_t          raid_group_id,
                                             fbe_object_id_t            *object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_lookup_raid_t lookup_raid;
    
    if (object_id == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lookup_raid.raid_id  = raid_group_id;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_NUMBER,
                                                    &lookup_raid, sizeof(lookup_raid));

    /* return the discovered object id */
    *object_id = lookup_raid.object_id;
    
    return status;
}

/*****************************************************************/
/** fbe_api_database_lookup_raid_group_by_object_id
 ****************************************************************
 * @brief
 *  This function looks up a raid group's number by using it's object id
 *
 * @param object_id(in)      - the object id
 * @param raid_group_id(out) - the number of the raid group
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/14/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_lookup_raid_group_by_object_id(fbe_object_id_t            object_id,
                                                fbe_raid_group_number_t        *raid_group_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_lookup_raid_t lookup_raid;
    
    if (raid_group_id == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lookup_raid.object_id  = object_id;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_OBJECT_ID,
                                                    &lookup_raid, sizeof(lookup_raid));

    /* return the discovered object id */
    *raid_group_id = lookup_raid.raid_id;
    
    return status;
}

/*****************************************************************
 * fbe_api_database_get_state
 *****************************************************************
 * @brief
 *  This function enables persistence data.
 *
 * @param state - the current state of the database service
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/14/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_state(fbe_database_state_t *state)
{
    fbe_status_t                          status;
    fbe_database_control_get_state_t control_state;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_STATE,
                                                    &control_state, sizeof(control_state));

    *state = control_state.state;
    
    return status;
}

/*****************************************************************
 * fbe_api_database_get_maintenance_reason
 *****************************************************************
 * @brief
 *  This function get the reason when database enter service mode
 *  NOTE: When display the reason logs to user, we use degraded mode instead.
 * @param ctrl_reason - the database get service reason pointer
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/11/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_service_mode_reason(fbe_database_control_get_service_mode_reason_t *ctrl_reason)
{
    fbe_status_t                          status;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_SERVICE_MODE_REASON,
                                                      ctrl_reason, sizeof(*ctrl_reason));

    return status;
}


/*****************************************************************
 * fbe_api_database_get_tables
 *****************************************************************
 * @brief
 *  This function looks up the database tables given the object id
 *
 * @param object_id(in) - the object id
 * @param tables(out) - the database tables content
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/14/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_tables(fbe_object_id_t	object_id,
                                                      fbe_database_get_tables_t * tables)
{
    fbe_status_t                          status;
    fbe_database_control_get_tables_t     control_tables;
    
    if (tables == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_tables.object_id  = object_id;
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_TABLES,
                                                    &control_tables, sizeof(control_tables));
    *tables = control_tables.tables;
    
    return status;
}

/*****************************************************************/
/** fbe_api_database_lookup_lun_by_number
 ****************************************************************
 * @brief
 *  This function looks up a lun number object id by using it's number
 *
 * @param lun_number(in) - lun_number
 * @param object_id(out)    - object id
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_lookup_lun_by_number(fbe_lun_number_t          lun_number,
                                             fbe_object_id_t            *object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_lookup_lun_t lookup_lun;
    
    if (object_id == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lookup_lun.lun_number  = lun_number;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_NUMBER,
                                                    &lookup_lun, sizeof(lookup_lun));

    /* return the discovered object id */
    *object_id = lookup_lun.object_id;
    
    return status;
}

/*****************************************************************/
/** fbe_api_database_lookup_lun_by_object_id
 ****************************************************************
 * @brief
 *  This function looks up a lun's number by using it's object id
 *
 * @param object_id(in)      - the object id
 * @param lun number(out) - lun number
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_lookup_lun_by_object_id(fbe_object_id_t            object_id,
                                                fbe_lun_number_t        *lun_number)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_lookup_lun_t lookup_lun;
    
    if (lun_number == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lookup_lun.object_id  = object_id;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_OBJECT_ID,
                                                    &lookup_lun, sizeof(lookup_lun));

    /* return the discovered object id */
    *lun_number = lookup_lun.lun_number;
    
    return status;
}

/*****************************************************************/
/** fbe_api_database_get_lun_info
 ****************************************************************
 * @brief
 *  This function looks up the lun info of a lun given the object id
 *
 * @param lun_info(in/out)
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_lun_info(fbe_database_lun_info_t *lun_info)

{
    fbe_status_t                           status = FBE_STATUS_OK;
    
    if (lun_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_LUN_INFO,
                                                    lun_info, sizeof(*lun_info) );
  

    return status;
}

/*****************************************************************/
/** fbe_api_database_update_virtual_drive
 ****************************************************************
 * @brief
 *  This function updates a virtual drive with the given params.
 *
 * @param virtual_drive - virtual drive
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_update_virtual_drive(fbe_database_control_update_vd_t *virtual_drive)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (virtual_drive == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_UPDATE_VD,
                                                    virtual_drive, sizeof(*virtual_drive));                                                  
    return status;
}
/**************************************
 * end fbe_api_database_update_virtual_drive()
 **************************************/



/****************************************************************/
/** fbe_api_database_get_system_object_id
 ****************************************************************
 * @brief
 *  This function gets the object id for a system object
 *
 * @param trace_level - the trace level to use
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_get_system_object_id(fbe_object_id_t *object_id)
{
    fbe_status_t                                 status = FBE_STATUS_OK;
/*    fbe_database_control_lookup_system_object_t  lookup_system_object;
    
    lookup_system_object.system_object = system_object;
    
    status = send_control_packet_to_config_service (FBE_DATABASE_CONTROL_CODE_LOOKUP_SYSTEM_OBJECT_ID,
                                                    &lookup_system_object, sizeof(lookup_system_object),
                                                    __FUNCTION__ );
*/
    /* TODO: Temp hack.*/
    *object_id = 15;

    return status;
}

/****************************************************************/
/** fbe_api_database_get_system_objects
 ****************************************************************
 * @brief
 *  This function will enumerate all the system objects.
 *
 * @param class_id - objects of this class will be enumerated.
 * @param system_object_list_p - user allocated pointer to valid
 *                               block of memory
 * @param total_objects - Max objects that can be enumerated
 * @param actual_num_objects_p - Pointer to actual number of 
 *                               objects in list
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/21/10 - Created. Sanjay Bhave
 *
 ***************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_get_system_objects (fbe_class_id_t class_id,
                                      fbe_object_id_t *system_object_list_p,
                                      fbe_u32_t total_objects,
                                      fbe_u32_t *actual_num_objects_p)
{
    fbe_status_t status;
    fbe_database_control_enumerate_system_objects_t enumerate_system_objects;
    fbe_sg_element_t sg[FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT];

    /* do a validation check before passing the buffer ahead */
    if (system_object_list_p == NULL)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
               "%s:system_object_list_p: can not be NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*! @todo Currently enumerating by class id isn't supported.
     */
    if (class_id != FBE_CLASS_ID_INVALID)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:class_id: 0x%x, currently not supported\n", __FUNCTION__,
                       class_id);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    sg[0].address = (fbe_u8_t *)system_object_list_p;
    sg[0].count = total_objects * sizeof(fbe_object_id_t);
    sg[1].address = NULL;
    sg[1].count = 0;

    enumerate_system_objects.class_id = class_id;
    enumerate_system_objects.number_of_objects = total_objects;
    enumerate_system_objects.number_of_objects_copied = 0;

    status = send_control_packet_to_database_service_with_sg_list (FBE_DATABASE_CONTROL_CODE_ENUMERATE_SYSTEM_OBJECTS, 
                                                                   &enumerate_system_objects, sizeof(enumerate_system_objects), 
                                                                   sg, FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT, 
                                                                   __FUNCTION__ );

    if (status == FBE_STATUS_OK)
    {
        *actual_num_objects_p = enumerate_system_objects.number_of_objects_copied;
    }

    return status;
}


/****************************************************************/
/** fbe_api_database_is_system_object
 ****************************************************************
 * @brief
 *  This function will check whether object id provided is system 
 *  object or not.
 *
 * @param object_id_to_check - objects id to check.
 * @param b_found - FBE_TRUE if object id is system object
 *                  else it is FBE_FALSE
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if successful
 *
 * @version
 *  1/31/11 - Created. Swati Fursule
 *
 ***************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_is_system_object(fbe_object_id_t object_id_to_check, fbe_bool_t *b_found)
{
    fbe_status_t              status;
    fbe_u32_t                  total_system_objects = 0;
    fbe_object_id_t            *system_object_list_p = NULL;
    fbe_u32_t                  max_system_objects = 512 / sizeof(fbe_object_id_t);
    fbe_u32_t obj_index = 0;

    /* Now enumerate all the system objects.
    */
    system_object_list_p = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * max_system_objects);
    if (system_object_list_p == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
            "%s: cannot allocate memory for enumerating system id object\n", 
            __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_set_memory(system_object_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * max_system_objects);


    status = fbe_api_database_get_system_objects(FBE_CLASS_ID_INVALID, 
        system_object_list_p, 
        max_system_objects, 
        &total_system_objects);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_free_memory(system_object_list_p);
        return(status);
    }

    /* simply iterate through the system object list to see if we find 
    * 'object_id_to_check' object in that list
    */
    for (obj_index = 0; obj_index < total_system_objects; obj_index++)
    {
        if(object_id_to_check == system_object_list_p[obj_index])
        {
            *b_found = FBE_TRUE;
            break;
        }
    }
    fbe_api_free_memory(system_object_list_p);
    return FBE_STATUS_OK;
}

/*****************************************************************/
/** fbe_api_database_get_stats
 ****************************************************************
 * @brief
 *  This function enables persistence data.
 *
 * @param num_pvds - number of provision drives
 * @param num_vds - number of virtual drives
 * @param num_rgs - number of RGs
 * @param num_luns - number of LUNs
 * @param num_edges - number of edges
 * @param num_physical_edges - number of physical edges
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  02/01/10 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_stats( fbe_database_control_get_stats_t *get_stats)
{
    fbe_status_t                          status;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_STATS, 
                                                      get_stats, sizeof(fbe_database_control_get_stats_t));

    return status;
}
/*!***************************************************************
   @fn fbe_api_database_start_transaction()
 ****************************************************************
 * @brief
 *  This function starts a transaction in the database service
 *
 * @param transaction_info - pointer to fbe_database_transaction_info   
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_start_transaction(fbe_database_transaction_info_t *transaction_info)
{
    fbe_status_t                                status;
   

    if (transaction_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Invalid Transaction ID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_TRANSACTION_START,
                                                      transaction_info, sizeof(fbe_database_transaction_info_t));
    return status;
}

/*!***************************************************************
   @fn fbe_api_database_start_transaction()
 ****************************************************************
 * @brief
 *  This function starts a transaction in the database service
 *
 * @param database_transaction_t    
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_abort_transaction(fbe_database_transaction_id_t transaction_id)
{
    fbe_status_t status;
 
    if (transaction_id == 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_TRANSACTION_ABORT,
                                                      &transaction_id, sizeof(transaction_id));
    return status;
}


/*!***************************************************************
   @fn fbe_api_database_commit_transaction()
 ****************************************************************
 * @brief
 *  This function commits a transaction in the database service
 *
 * @param database_transaction_t    
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_commit_transaction(fbe_database_transaction_id_t transaction_id)
{
    fbe_status_t status;
    if (transaction_id == 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

     status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_TRANSACTION_COMMIT,
                                                       &transaction_id, sizeof(transaction_id));
    return status;
}

/*****************************************************************/
/** fbe_api_database_create_lun
 ****************************************************************
 * @brief
 *  This function create lun with the given params.
 *
 * @param lun - pointer to lun structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_create_lun(fbe_database_control_lun_t lun)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_CREATE_LUN,
                                                       &lun, sizeof(fbe_database_control_lun_t));
    return status;
}

/*****************************************************************/
/** fbe_api_database_destroy_lun
 ****************************************************************
 * @brief
 *  This function destroy lun with the given params.
 *
 * @param lun - pointer to lun structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_destroy_lun(fbe_database_control_destroy_object_t lun)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_DESTROY_LUN,
                                                       &lun, sizeof(fbe_database_control_destroy_object_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_destroy_raid
 ****************************************************************
 * @brief
 *  This function destroy raid with the given params.
 *
 * @param raid - pointer to raid structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_destroy_raid(fbe_database_control_destroy_object_t raid)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_DESTROY_RAID,
                                                       &raid, sizeof(fbe_database_control_destroy_object_t));
    return status;
}

/*****************************************************************/
/** fbe_api_database_destroy_vd
 ****************************************************************
 * @brief
 *  This function destroy vd with the given params.
 *
 * @param vd - pointer to vd
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_destroy_vd(fbe_database_control_destroy_object_t vd)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_DESTROY_VD,
                                                       &vd, sizeof(fbe_database_control_destroy_object_t));
    return status;
}

/*****************************************************************/
/** fbe_api_database_update_raid_group
 ****************************************************************
 * @brief
 *  This function updates raid group object 
 *
 * @param raid_group_id(in) - the number of the raid group
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_update_raid_group(fbe_database_control_update_raid_t  raid_grp)                                            
{
    fbe_status_t   status = FBE_STATUS_OK;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_UPDATE_RAID,
                                                      &raid_grp, sizeof(fbe_database_control_update_raid_t));
    return status;
}

/*****************************************************************/
/** fbe_api_database_set_power_save
 ****************************************************************
 * @brief
 *  This function sets the power save
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_set_power_save(fbe_database_power_save_t power_save_info)                                            
{
    fbe_status_t status = FBE_STATUS_OK;	

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_SET_POWER_SAVE,
                                                      &power_save_info, sizeof(fbe_database_power_save_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_get_power_save
 ****************************************************************
 * @brief
 *  This function gets the power save info
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_power_save(fbe_database_power_save_t *power_save_info)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (power_save_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_GET_POWER_SAVE,
                                                      power_save_info, sizeof(fbe_database_power_save_t));

    return status;
}  

/*****************************************************************/
/** fbe_api_database_set_encryption
 ****************************************************************
 * @brief
 *  This function sets the encryption
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_set_encryption(fbe_database_encryption_t encryption_info)                                            
{
    fbe_status_t status = FBE_STATUS_OK;	

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_SET_ENCRYPTION,
                                                      &encryption_info, sizeof(fbe_database_encryption_t));

    return status;
}


/*****************************************************************/
/** fbe_api_database_get_encryption
 ****************************************************************
 * @brief
 *  This function gets the encryption info
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_encryption(fbe_database_encryption_t *encryption_info)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (encryption_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_GET_ENCRYPTION,
                                                      encryption_info, sizeof(fbe_database_encryption_t));

    return status;
}  



/*****************************************************************/
/** fbe_api_database_get_transaction_info
 ****************************************************************
 * @brief
 *  This function get the in-progress transaction
 *
 * @param database_transaction_t - ptr_info
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/19/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_transaction_info(database_transaction_t *ptr_info)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (ptr_info == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_TRANSACTION_INFO,
                                                    ptr_info, sizeof(database_transaction_t));

    
    return status;
}


/*****************************************************************/
/** fbe_api_database_system_db_send_cmd
 ****************************************************************
 * @brief
 *  This function send packet to database service to execute
 *  system db entries operation command.
 *
 * @param 
 *  fbe_database_control_system_db_op_t    system db op command
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/28/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_system_db_send_cmd(fbe_database_control_system_db_op_t *cmd_buffer)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    if (cmd_buffer == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SYSTEM_DB_OP_CMD,
                                                    cmd_buffer, sizeof(fbe_database_control_system_db_op_t));

    
    return status;	
}

/*****************************************************************/
/** fbe_api_database_system_send_clone_cmd
 ****************************************************************
 * @brief
 *  This function send packet to database service to trigger clone object.
 *
 * @param 
 *  fbe_database_control_clone_object_t    object clone command
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/12/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_system_send_clone_cmd(fbe_database_control_clone_object_t *cmd_buffer)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    if (cmd_buffer == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_CLONE_OBJECT,
                                                    cmd_buffer, sizeof(fbe_database_control_clone_object_t));

    
    return status;
}

/*****************************************************************/
/** fbe_api_database_set_time_threshold
 ****************************************************************
 * @brief
 *  This function sets the time threshold in database global entry
 *
 * @param time_threshold_info - the time threshold in days.
 *        If pvd unused time reaches the threshold, it will
 *        trigger the pvd garbage collection
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_set_time_threshold(fbe_database_time_threshold_t time_threshold_info)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_SET_PVD_DESTROY_TIMETHRESHOLD,
                                                      &time_threshold_info, sizeof(fbe_database_time_threshold_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_get_time_threshold
 ****************************************************************
 * @brief
 *  This function gets the time threshold info in database global entry
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_time_threshold(fbe_database_time_threshold_t *time_threshold_info)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (time_threshold_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_GET_PVD_DESTROY_TIMETHRESHOLD,
                                                      time_threshold_info, sizeof(fbe_database_time_threshold_t));

    return status;
}  

fbe_status_t FBE_API_CALL fbe_api_database_get_fru_descriptor_info(fbe_homewrecker_fru_descriptor_t* out_descriptor, 
                                                                   fbe_homewrecker_get_fru_disk_mask_t disk_mask,
                                                                   fbe_bool_t* warning)
{
    fbe_status_t status;
    fbe_database_control_get_fru_descriptor_t fru_descriptor;

    fru_descriptor.disk_mask = disk_mask;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_FRU_DESCRIPTOR,
                                                      &fru_descriptor, sizeof(fru_descriptor));

    *out_descriptor = fru_descriptor.descriptor;
    
    if (fru_descriptor.access_status == FBE_GET_HW_STATUS_OK)
        *warning = FBE_FALSE;
    else
        *warning = FBE_TRUE;
    
    return status;
}

fbe_status_t FBE_API_CALL fbe_api_database_get_disk_signature_info(fbe_fru_signature_t* signature)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_signature_t out_signature;

    
    if (signature == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    out_signature.bus = signature->bus;
    out_signature.enclosure = signature->enclosure;
    out_signature.slot = signature->slot;
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_DISK_SIGNATURE,
                                                      &out_signature, sizeof(out_signature));
                                                      
    fbe_copy_memory(signature, out_signature.signature.magic_string, FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE);
    signature->system_wwn_seed = out_signature.signature.system_wwn_seed;
    signature->version = out_signature.signature.version;
    signature->bus = out_signature.signature.bus;
    signature->enclosure = out_signature.signature.enclosure;
    signature->slot = out_signature.signature.slot;

    
    return status;
}


fbe_status_t FBE_API_CALL fbe_api_database_set_disk_signature_info(fbe_fru_signature_t* signature)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_signature_t in_signature;
    
    if (signature == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_signature.bus = signature->bus;
    in_signature.enclosure = signature->enclosure;
    in_signature.slot = signature->slot;
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_DISK_SIGNATURE,
                                                      &in_signature, sizeof(in_signature));

    return status;
}

fbe_status_t FBE_API_CALL fbe_api_database_clear_disk_signature_info(fbe_fru_signature_t* signature)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_signature_t in_signature;
    
    if (signature == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_signature.bus = signature->bus;
    in_signature.enclosure = signature->enclosure;
    in_signature.slot = signature->slot;
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_CLEAR_DISK_SIGNATURE,
                                                      &in_signature, sizeof(in_signature));

    return status;
}



fbe_status_t FBE_API_CALL fbe_api_database_set_fru_descriptor_info(fbe_database_control_set_fru_descriptor_t* in_fru_descriptor,
                                                                                  fbe_database_control_set_fru_descriptor_mask_t mask)
{
    fbe_database_control_set_fru_descriptor_t set_fru_descriptor = {0};
    fbe_database_control_get_fru_descriptor_t get_fru_descriptor = {0};
    fbe_status_t status = FBE_STATUS_INVALID;

    get_fru_descriptor.disk_mask = FBE_FRU_DISK_ALL;
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_FRU_DESCRIPTOR,
                                                    &get_fru_descriptor, sizeof(get_fru_descriptor));
    
    if (status != FBE_STATUS_OK)
        return status;

    set_fru_descriptor.wwn_seed = (mask & FBE_FRU_WWN_SEED)? \
        in_fru_descriptor->wwn_seed : get_fru_descriptor.descriptor.wwn_seed;
    
    set_fru_descriptor.chassis_replacement_movement = (mask & FBE_FRU_CHASSIS_REPLACEMENT)? \
        in_fru_descriptor->chassis_replacement_movement: get_fru_descriptor.descriptor.chassis_replacement_movement;
    
    set_fru_descriptor.structure_version = (mask & FBE_FRU_STRUCTURE_VERSION)? \
        in_fru_descriptor->structure_version: get_fru_descriptor.descriptor.structure_version;
    
    if (mask & FBE_FRU_SERIAL_NUMBER)
        fbe_copy_memory(set_fru_descriptor.system_drive_serial_number,
                        in_fru_descriptor->system_drive_serial_number, 
                        sizeof(set_fru_descriptor.system_drive_serial_number));
    else
        fbe_copy_memory(set_fru_descriptor.system_drive_serial_number,
                        get_fru_descriptor.descriptor.system_drive_serial_number, 
                        sizeof(set_fru_descriptor.system_drive_serial_number));
    
    return send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_FRU_DESCRIPTOR,
                                                    &set_fru_descriptor, sizeof(set_fru_descriptor));
    
}

fbe_status_t FBE_API_CALL fbe_api_database_set_chassis_replacement_flag(fbe_bool_t whether_set)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_database_control_set_fru_descriptor_t descriptor = {0};
    fbe_database_control_set_fru_descriptor_mask_t modify = FBE_FRU_CHASSIS_REPLACEMENT;
    
    if(whether_set == FBE_TRUE)
        descriptor.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_TRUE;
    else
        descriptor.chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_INVALID;
        
    status = fbe_api_database_set_fru_descriptor_info(&descriptor, modify);
    return status;
}



/*****************************************************************/
/** fbe_api_database_persist_prom_cmd
 ****************************************************************
 * @brief
 *  This function send packet to database service to persist prom.
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/4/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_system_persist_prom_wwnseed_cmd(fbe_u32_t *cmd_buffer)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    if (cmd_buffer == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_PERSIST_PROM_WWNSEED,
                                                    cmd_buffer, sizeof(fbe_u32_t));

    
    return status;
    
}

/*****************************************************************/
/** fbe_api_database_obtain_prom_cmd
 ****************************************************************
 * @brief
 *  This function send packet to database service to get prom.
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/4/11 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_system_obtain_prom_wwnseed_cmd(fbe_u32_t *cmd_buffer)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    if (cmd_buffer == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_OBTAIN_PROM_WWNSEED,
                                                    cmd_buffer, sizeof(fbe_u32_t));

    
    return status;
}

/*****************************************************************/
/** fbe_api_database_lookup_lun_by_wwid
 ****************************************************************
 * @brief
 *  This function looks up a lun object id by using it's wwid
 *
 * @param lun_number(in) - lun_number
 * @param object_id(out)    - object id
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_lookup_lun_by_wwid(fbe_assigned_wwid_t wwid, fbe_object_id_t *object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_lookup_lun_by_wwn_t lookup_lun;
    
    if (object_id == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lookup_lun.lun_wwid  = wwid;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_WWID,
                                                    &lookup_lun, sizeof(lookup_lun));

    /* return the discovered object id */
    *object_id = lookup_lun.object_id;
    
    return status;
}

/*****************************************************************/
/** fbe_api_database_get_compat_mode
 ****************************************************************
 * @brief
 *  This function return the version of SEP package and the committed version of the SEP package 
 *
 * @param compat_mode (output) -  the version information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_compat_mode(fbe_compatibility_mode_t *compat_mode)
{
    fbe_status_t    status = FBE_STATUS_OK;

    if (compat_mode == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_COMPAT_MODE,
                                                       compat_mode, sizeof(fbe_compatibility_mode_t));
    return status;
}

/*****************************************************************/
/** fbe_api_database_commit 
 ****************************************************************
 * @brief
 *  This function delivers the "NDU commit" command to database service 
 *
 * @param NULL 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_commit(void)
{
    fbe_status_t    status = FBE_STATUS_OK;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_COMMIT,
                                                       NULL, 0);
    return status;
}

/*****************************************************************/
/** fbe_api_database_set_ndu_state 
 ****************************************************************
 * @brief
 *  This function delivers the ndu state to database service 
 *
 * @param  ndu_state : the current ndu state
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_set_ndu_state(fbe_set_ndu_state_t ndu_state)
{
    fbe_status_t    status = FBE_STATUS_OK;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_NDU_STATE,
                                                       &ndu_state, sizeof(fbe_set_ndu_state_t));
    return status;
}

/*!***************************************************************
 *  @fn fbe_api_database_get_all_luns(fbe_database_lun_info_t *lun_list, fbe_u32_t expected_count, fbe_u32_t *actual_count)
 ****************************************************************
 * @brief
 *  This function fills a buffer with the fbe_database_lun_info_t for each existing lun in the system
 *  The user is in charge of allocating the buffer of lun_list and releasing it.
 *  Be ware this function return private space LUNs as well.
 *
 * @param lun_list - a buffer big enough to hold (fbe_database_all_luns_info_t * expected_count)
 * @param expected_count - how many luns the user buffer can contain. Usually the user would run fbe_api_get_total_objects_of_class before
 * @param actual_count - how many luns SEP actually returned
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_all_luns(fbe_database_lun_info_t *lun_list,
                                                        fbe_u32_t expected_count,
                                                        fbe_u32_t *actual_count)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_database_control_get_all_luns_t		get_all_luns;

    if (expected_count == 0 || actual_count == NULL || lun_list == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)lun_list;
    sg_list[0].count = expected_count * sizeof(fbe_database_lun_info_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    get_all_luns.number_of_luns_requested = expected_count;
    get_all_luns.number_of_luns_returned = 0;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_DATABASE_CONTROL_CODE_GET_ALL_LUNS,
                                                                        &get_all_luns,
                                                                        sizeof (fbe_database_control_get_all_luns_t),
                                                                        FBE_SERVICE_ID_DATABASE,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_list,
                                                                        2,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_SEP_0);


    fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_count = get_all_luns.number_of_luns_returned;
    
    return FBE_STATUS_OK;

}

/*****************************************************************/
/** fbe_api_database_get_peer_sep_version(fbe_u64_t *version)
 ****************************************************************
 * @brief
 *  This function will get the peer sep verion
 *
 * @param version - structure with needed information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_peer_sep_version(fbe_u64_t *version)
{
    fbe_status_t                          status;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_PEER_SEP_VERSION,
                                                      version, sizeof(fbe_u64_t));
    
    return status;
}


/*****************************************************************/
/** fbe_api_database_set_peer_sep_version(fbe_u64_t *version)
 ****************************************************************
 * @brief
 *  This function will set the peer sep verion
 *
 * @param version - structure with needed information
 *
 * @NOTE : Should be used for TESTING purposes ONLY
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_set_peer_sep_version(fbe_u64_t *version)
{
    fbe_status_t                          status;
    
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_PEER_SEP_VERSION,
                                                      version, sizeof(fbe_u64_t));
    
    return status;
}


/*****************************************************************/
/** fbe_api_database_set_emv_params(fbe_database_emv_t *emv)
 ****************************************************************
 * @brief
 *  This function will set the ENV parameters in the MCR DB
 *
 * @param emv - structure with needed information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_set_emv_params(fbe_database_emv_t *emv)
{
    fbe_status_t                          status;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_EMV,
                                                    emv, sizeof(fbe_database_emv_t));
    
    return status;
}


/*****************************************************************/
/** fbe_api_database_get_emv_params(fbe_database_emv_t *emv)
 ****************************************************************
 * @brief
 *  This function gets the EMV information from the MCR DB
 *
 * @param emv - structure with needed information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_emv_params(fbe_database_emv_t *emv)
{
    fbe_status_t                          status;
    
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_EMV,
                                                    emv, sizeof(fbe_database_emv_t));
    
    return status;
}

/*!***************************************************************
 *  @fn fbe_api_database_get_all_raid_groups(fbe_database_raid_group_info_t *rg_list, fbe_u32_t expected_count, fbe_u32_t *actual_count)
 ****************************************************************
 * @brief
 *  This function fills a buffer with the fbe_database_raid_group_info_t for each existing raid group in the system
 *  The user is in charge of allocating the buffer of rg_list and releasing it.
 *  Be ware this function return private raid groups as well.
 *
 * @param rg_list - a buffer big enough to hold (fbe_database_raid_group_info_t * expected_count)
 * @param expected_count - how many raid groups the user buffer can contain.
 *                         Usually the user would run fbe_api_get_total_objects_of_all_raid_groups before
 * @param actual_count - how many raid groups SEP actually returned
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 * 
 * @version
 *  04/30/12 - Vera Wang Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_all_raid_groups(fbe_database_raid_group_info_t *rg_list,
                                                               fbe_u32_t expected_count,
                                                               fbe_u32_t *actual_count)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_database_control_get_all_rgs_t		get_all_rgs;

    if (expected_count == 0 || actual_count == NULL || rg_list == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)rg_list;
    sg_list[0].count = expected_count * sizeof(fbe_database_raid_group_info_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    get_all_rgs.number_of_rgs_requested = expected_count;
    get_all_rgs.number_of_rgs_returned = 0;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_DATABASE_CONTROL_CODE_GET_ALL_RAID_GROUPS,
                                                                        &get_all_rgs,
                                                                        sizeof (fbe_database_control_get_all_rgs_t),
                                                                        FBE_SERVICE_ID_DATABASE,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_list,
                                                                        2,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_SEP_0);


    fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_count = get_all_rgs.number_of_rgs_returned;
    
    return FBE_STATUS_OK;

}

/*!***************************************************************
 *  @fn fbe_api_database_get_all_pvds(fbe_database_pvd_info_t *pvd_list, fbe_u32_t expected_count, fbe_u32_t *actual_count)
 ****************************************************************
 * @brief
 *  This function fills a buffer with the fbe_database_pvdinfo_t for each existing raid group in the system
 *  The user is in charge of allocating the buffer of pvd_list and releasing it.
 *  Be ware this function return private PVDS as well.
 *
 * @param pvd_list - a buffer big enough to hold (fbe_database_pvd_info_t * expected_count)
 * @param expected_count - how many PVDs the user buffer can contain.
 *                         Usually the user would run fbe_api_get_total_objects_of_class(FBE_CLASS_ID_PVD) before
 * @param actual_count - how many PVDs SEP actually returned
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 * 
 * @version
 *  05/02/12 - Vera Wang Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_all_pvds(fbe_database_pvd_info_t *pvd_list,
                                                        fbe_u32_t expected_count,
                                                        fbe_u32_t *actual_count)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_database_control_get_all_pvds_t		get_all_pvds;

    if (expected_count == 0 || actual_count == NULL || pvd_list == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)pvd_list;
    sg_list[0].count = expected_count * sizeof(fbe_database_pvd_info_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    get_all_pvds.number_of_pvds_requested = expected_count;
    get_all_pvds.number_of_pvds_returned = 0;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_DATABASE_CONTROL_CODE_GET_ALL_PVDS,
                                                                        &get_all_pvds,
                                                                        sizeof (fbe_database_control_get_all_pvds_t),
                                                                        FBE_SERVICE_ID_DATABASE,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_list,
                                                                        2,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_SEP_0);


    fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_count = get_all_pvds.number_of_pvds_returned;
    
    return FBE_STATUS_OK;

}

/*!***************************************************************
 *  @fn fbe_api_database_get_raid_group(fbe_object_id_t rg_id, fbe_database_raid_group_info_t *rg)
 ****************************************************************
 * @brief
 *  Get information from the DB about a single Raid GRoup, using it's object id.
 *
 * @param rg_id - Object Id of the RG
 * @param rg - pointer to data
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_raid_group(fbe_object_id_t rg_id, fbe_database_raid_group_info_t *rg)
{
    fbe_status_t status;

    if (rg_id > FBE_OBJECT_ID_INVALID || rg == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    rg->rg_object_id = rg_id;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_RAID_GROUP,
                                                      rg, sizeof(fbe_database_raid_group_info_t));

    return status;

}

/*!***************************************************************
 *  @fn fbe_api_database_get_pvd(fbe_object_id_t pvd_id, fbe_database_pvd_info_t *pvd)
 ****************************************************************
 * @brief
 *  Get information from the DB about a single PVD, using it's object id.
 *
 * @param pvd_id - Object Id of the PVD
 * @param pvd - pointer to data
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_pvd(fbe_object_id_t pvd_id, fbe_database_pvd_info_t *pvd)
{
    fbe_status_t status;

    if (pvd_id > FBE_OBJECT_ID_INVALID || pvd == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    pvd->pvd_object_id = pvd_id;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_PVD,
                                                      pvd, sizeof(fbe_database_pvd_info_t));

    return status;

}

/*!***************************************************************
 *  @fn fbe_api_database_stop_all_background_service(fbe_bool_t update_bgs_on_both_sp)
 ****************************************************************
 * @brief
 *  This function stops all background operations in SEP as a response to a request
 *  from MCC needing to initiate a vault dump
 *
 * @param update_bgs_on_both_sp - FBE_TRUE for both SPs; FBE_FALSE for local SP only
 * 
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_stop_all_background_service(fbe_bool_t update_bgs_on_both_sp)
{
    fbe_status_t status;
    fbe_database_control_update_system_bg_service_t update_system_bg_service;

    update_system_bg_service.update_bgs_on_both_sp = update_bgs_on_both_sp;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_STOP_ALL_BACKGROUND_SERVICES, 
                                                      &update_system_bg_service, sizeof(fbe_database_control_update_system_bg_service_t));

    return status;

}

/*!***************************************************************
 *  @fn fbe_api_database_restart_all_background_service()void
 ****************************************************************
 * @brief
 *  This function restarts all background operations after a dump is done on not needed
 *
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_restart_all_background_service(void)
{
    fbe_status_t status;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_RESTART_ALL_BACKGROUND_SERVICES, NULL, 0);

    return status;

}


/*****************************************************************/
/** fbe_api_database_set_power_save
 ****************************************************************
 * @brief
 *  This function sets load balancing
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_set_load_balance(fbe_bool_t is_enable)                                            
{
    fbe_status_t status = FBE_STATUS_OK;	

    if(is_enable){
        status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_ENABLE_LOAD_BALANCE, NULL, 0);
    } else {
        status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_DISABLE_LOAD_BALANCE, NULL, 0);
    }

    return status;
}

/*****************************************************************/
/** fbe_api_database_init_system_db_header
 ****************************************************************
 * @brief
 *  This function initializes the system db header
 *
 * @param 
    database_system_db_header_t * out_db_header
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @date
 *  May 7, 2013 - Created. Yingying Song
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_init_system_db_header(database_system_db_header_t *out_db_header)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (out_db_header == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_INIT_SYSTEM_DB_HEADER,
                                                      out_db_header, sizeof(database_system_db_header_t));

    return status;
} 


/*****************************************************************/
/** fbe_api_database_get_system_db_header
 ****************************************************************
 * @brief
 *  This function gets the system db header
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_system_db_header(database_system_db_header_t *out_db_header)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (out_db_header == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_DB_HEADER,
                                                      out_db_header, sizeof(database_system_db_header_t));

    return status;
} 

/*****************************************************************/
/** fbe_api_database_set_system_db_header
 ****************************************************************
 * @brief
 *  This function sets the system db header
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_set_system_db_header(database_system_db_header_t *in_db_header)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (in_db_header == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_SYSTEM_DB_HEADER,
                                                      in_db_header, sizeof(database_system_db_header_t));

    return status;
} 

/*****************************************************************/
/** fbe_api_database_persist_system_db_header
 ****************************************************************
 * @brief
 *  This function persist the system db header into the disk
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_persist_system_db_header(database_system_db_header_t *in_db_header)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (in_db_header == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_PERSIST_SYSTEM_DB_HEADER,
                                                      in_db_header, sizeof(database_system_db_header_t));

    return status;
}  

/*****************************************************************/
/** fbe_api_database_get_system_object_recreate_flags
 ****************************************************************
 * @brief
 *  This function gets the system object recreate flags
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_system_object_recreate_flags(fbe_database_system_object_recreate_flags_t *system_object_recreate_flags)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (system_object_recreate_flags == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_OBJECT_RECREATE_FLAGS,
                                                      system_object_recreate_flags, sizeof(fbe_database_system_object_recreate_flags_t));

    return status;
} 

/*****************************************************************/
/** fbe_api_database_persist_system_object_recreate_flags
 ****************************************************************
 * @brief
 *  This function persists the system object recreate flags
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_persist_system_object_recreate_flags(fbe_database_system_object_recreate_flags_t *system_object_recreate_flags)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (system_object_recreate_flags == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_PERSIST_SYSTEM_OBJECT_OP_FLAGS,
                                                      system_object_recreate_flags, sizeof(fbe_database_system_object_recreate_flags_t));

    return status;
} 

/*****************************************************************/
/** fbe_api_database_set_system_object_recreation
 ****************************************************************
 * @brief
 *  This function set system object to be recreated
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_set_system_object_recreation(fbe_object_id_t object_id, fbe_u8_t recreate_flags)
{
    fbe_database_system_object_recreate_flags_t   system_object_recreate_flags;
    fbe_status_t    status;

    fbe_zero_memory(&system_object_recreate_flags, sizeof(fbe_database_system_object_recreate_flags_t));

    status = fbe_api_database_get_system_object_recreate_flags(&system_object_recreate_flags);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: get system object recreation flags failed\n", __FUNCTION__);
        return status;
    }
    
    system_object_recreate_flags.system_object_op[object_id] |= recreate_flags;
    system_object_recreate_flags.valid_num++;

    status = fbe_api_database_persist_system_object_recreate_flags(&system_object_recreate_flags);
    return status;
}

/*****************************************************************/
/** fbe_api_database_reset_system_object_recreation
 ****************************************************************
 * @brief
 *  This function reset system object to be recreated
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_reset_system_object_recreation(void)
{
    fbe_database_system_object_recreate_flags_t   system_object_recreate_flags;
    fbe_status_t    status;

    fbe_zero_memory(&system_object_recreate_flags, sizeof(fbe_database_system_object_recreate_flags_t));

    status = fbe_api_database_persist_system_object_recreate_flags(&system_object_recreate_flags);
    return status;
}

/*****************************************************************/
/** fbe_api_database_generate_system_object_config_and_persist
 ****************************************************************
 * @brief
 *  This function reinitialize system object config for the specified object
 *  and persist the config to disk.
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @date
 *   May 24, 2013   Created: Yingying Song
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_generate_system_object_config_and_persist(fbe_object_id_t object_id)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t system_object_id = object_id;
    
    if(system_object_id == FBE_OBJECT_ID_INVALID)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GENERATE_SYSTEM_OBJECT_CONFIG_AND_PERSIST,
                                                      &system_object_id, sizeof(fbe_object_id_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_generate_all_system_rg_and_lun_and_persist
 ****************************************************************
 * @brief
 *  This function reinitialize system object config for all system lun and rg
 *  and persist all config to disk.
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @date
 *   May 24, 2013   Created: Yingying Song
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_generate_config_for_all_system_rg_and_lun_and_persist(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GENERATE_ALL_SYSTEM_OBJECTS_CONFIG_AND_PERSIST,
                                                      NULL, 0);

    return status;
}


/*****************************************************************/
/** fbe_api_database_generate_configuration_for_system_parity_rg_and_lun
 ****************************************************************
 * @brief
 *  This function generate the configuration for system parity RG and LUN from the 
 *  PSL.
 *
 * @param
 *  fbe_bool_t ndb - generate Lun in ndb or non-ndb way.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_generate_configuration_for_system_parity_rg_and_lun(fbe_bool_t ndb)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t ndb_b = ndb;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GENERATE_CONFIGURATION_FOR_SYSTEM_PARITY_RG_AND_LUN, &ndb_b, sizeof(fbe_bool_t));

    return status;
}

/***************************************************************************************/
/* fbe_api_database_online_planned_drive
 ***************************************************************************************
 * @brief
 *  This function sends packet to database service to make a planned drive online.
 *
 * @param[in] online_drive - fbe_database_control_online_planned_drive_t structure,
 *                          where the drive location specified by user is input
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/06/2012 Created. Zhipeng Hu
 *
 ***************************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_online_planned_drive(fbe_database_control_online_planned_drive_t* online_drive)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    if (NULL == online_drive)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_MAKE_PLANNED_DRIVE_ONLINE,
                                                    online_drive, sizeof(*online_drive));
    
    return status;    
}



/*****************************************************************/
/** fbe_api_database_cleanup_reinit_persit_service
 ****************************************************************
 * @brief
 *  This function clear all the data in the database lun and
 *  re-initialize the persist service on a clean and empty data
 *  base lun
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_cleanup_reinit_persit_service(void)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_CLEANUP_RE_INIT_PERSIST_SERVICE, NULL, 0);

    return status;
} 


/*****************************************************************/
/** fbe_api_database_restore_user_configuration
 ****************************************************************
 * @brief
 *  This function restores the user configuraiton to the database
 *  LUN
 *
 * @param fbe_database_config_entries_restore_t  input of config required restored
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_restore_user_configuration(fbe_database_config_entries_restore_t *restore_op_p)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (!restore_op_p) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:NULL input\n", __FUNCTION__);
    return FBE_STATUS_OK;
    }

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_RESTORE_USER_CONFIGURATION, 
                                                      restore_op_p, sizeof (fbe_database_config_entries_restore_t));
    
    return status;
}

/*!***************************************************************
 *  @fn fbe_api_database_get_object_tables_in_range
 ****************************************************************
 * @brief
 *  This function fills a buffer with object enrites whithin
 *  the range user specified.
 *  The user is in charge of allocating the buffer and releasing it.
 *  Be ware this function return private object entries as well.
 *
 * @param in_buffer - a buffer big enough to hold all the entries user want
 * @param in_buffer_size -  
 * @param start_obj_id - the first object id that the user want to get
 * @param end_obj_id - the last object id that the user want to get
 * @param actual_count - how many objects' entry actually returned
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 * 
 * @version
 *  07/10/12 - Yang Zhang Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_object_tables_in_range(void * in_buffer,
                                                                           fbe_u32_t in_buffer_size,
                                                                           fbe_object_id_t start_object_id,
                                                                           fbe_object_id_t end_object_id,
                                                                           fbe_u32_t *actual_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_sg_element_t *                              sg_list = NULL;
    fbe_database_control_get_tables_in_range_t		get_tables_in_range;
    fbe_u32_t                                       expected_count = 0;

    if(end_object_id < start_object_id)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    expected_count = (end_object_id - start_object_id) + 1;
    if ( expected_count == 0 || actual_count == NULL || in_buffer == NULL
         || (in_buffer_size < expected_count *sizeof(database_object_entry_t)))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)in_buffer;
    sg_list[0].count =  expected_count *sizeof(database_object_entry_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    get_tables_in_range.start_object_id = start_object_id;
    get_tables_in_range.end_object_id = end_object_id;
    get_tables_in_range.number_of_objects_returned = 0;
    get_tables_in_range.table_type = FBE_DATABASE_TABLE_TYPE_OBJECT;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_DATABASE_CONTROL_CODE_GET_TABLES_IN_RANGE_WITH_TYPE,
                                                                        &get_tables_in_range,
                                                                        sizeof (fbe_database_control_get_tables_in_range_t),
                                                                        FBE_SERVICE_ID_DATABASE,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_list,
                                                                        2,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_SEP_0);


    fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_count = get_tables_in_range.number_of_objects_returned;
    
    return FBE_STATUS_OK;

}

/*!***************************************************************
 *  @fn fbe_api_database_get_user_tables_in_range
 ****************************************************************
 * @brief
 *  This function fills a buffer with user entries whithin
 *  the range user specified.
 *  The user is in charge of allocating the buffer and releasing it.
 *  Be ware this function return private object entries as well.
 *
 * @param in_buffer - a buffer big enough to hold all the entries user want
 * @param in_buffer_size -  
 * @param start_obj_id - the first object id that the user want to get
 * @param end_obj_id - the last object id that the user want to get
 * @param actual_count - how many objects' entries(user,object and edges) 
 *                                          actually returned
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 * 
 * @version
 *  07/10/12 - Yang Zhang Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_user_tables_in_range(void * in_buffer,
                                                                           fbe_u32_t in_buffer_size,
                                                                           fbe_object_id_t start_object_id,
                                                                           fbe_object_id_t end_object_id,
                                                                           fbe_u32_t *actual_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_sg_element_t *                              sg_list = NULL;
    fbe_database_control_get_tables_in_range_t		get_tables_in_range;
    fbe_u32_t                                       expected_count = 0;

    if(end_object_id < start_object_id)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    expected_count = (end_object_id - start_object_id) + 1;
    if ( expected_count == 0 || actual_count == NULL || in_buffer == NULL
         || (in_buffer_size < expected_count *sizeof(database_user_entry_t)))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)in_buffer;
    sg_list[0].count = expected_count *sizeof(database_user_entry_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    get_tables_in_range.start_object_id = start_object_id;
    get_tables_in_range.end_object_id = end_object_id;
    get_tables_in_range.number_of_objects_returned = 0;
    get_tables_in_range.table_type = FBE_DATABASE_TABLE_TYPE_USER;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_DATABASE_CONTROL_CODE_GET_TABLES_IN_RANGE_WITH_TYPE,
                                                                        &get_tables_in_range,
                                                                        sizeof (fbe_database_control_get_tables_in_range_t),
                                                                        FBE_SERVICE_ID_DATABASE,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_list,
                                                                        2,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_SEP_0);


    fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_count = get_tables_in_range.number_of_objects_returned;
    
    return FBE_STATUS_OK;

}

/*!***************************************************************
 *  @fn fbe_api_database_get_edge_tables_in_range
 ****************************************************************
 * @brief
 *  This function fills a buffer with edge entries whithin
 *  the range user specified.
 *  The user is in charge of allocating the buffer and releasing it.
 *  Be ware this function return private object entries as well.
 *
 * @param in_buffer - a buffer big enough to hold all the entries user want
 * @param in_buffer_size -
 * @param start_obj_id - the first object id that the user want to get
 * @param end_obj_id - the last object id that the user want to get
 * @param actual_count - how many objects' entries(user,object and edges) 
 *                                          actually returned
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 * 
 * @version
 *  07/10/12 - Yang Zhang Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_edge_tables_in_range(void * in_buffer,
                                                                           fbe_u32_t in_buffer_size,
                                                                           fbe_object_id_t start_object_id,
                                                                           fbe_object_id_t end_object_id,
                                                                           fbe_u32_t *actual_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_sg_element_t *                              sg_list = NULL;
    fbe_database_control_get_tables_in_range_t		get_tables_in_range;
    fbe_u32_t                                       expected_count = 0;

    
    if(end_object_id < start_object_id)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    expected_count = (end_object_id - start_object_id) + 1;
    if ( expected_count == 0 || actual_count == NULL || in_buffer == NULL
         || (in_buffer_size < expected_count * DATABASE_MAX_EDGE_PER_OBJECT * sizeof(database_edge_entry_t)))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)in_buffer;
    sg_list[0].count = expected_count * DATABASE_MAX_EDGE_PER_OBJECT * sizeof(database_edge_entry_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    get_tables_in_range.start_object_id = start_object_id;
    get_tables_in_range.end_object_id = end_object_id;
    get_tables_in_range.number_of_objects_returned = 0;
    get_tables_in_range.table_type = FBE_DATABASE_TABLE_TYPE_EDGE;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_DATABASE_CONTROL_CODE_GET_TABLES_IN_RANGE_WITH_TYPE,
                                                                        &get_tables_in_range,
                                                                        sizeof (fbe_database_control_get_tables_in_range_t),
                                                                        FBE_SERVICE_ID_DATABASE,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_list,
                                                                        2,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_SEP_0);


    fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_count = get_tables_in_range.number_of_objects_returned;
    
    return FBE_STATUS_OK;

}

/*!***************************************************************
 *  @fn fbe_api_database_persist_system_db
 ****************************************************************
 * @brief
 *  This function persist the content in the buffer which user specified to DB raw mirror
 *  The user is in charge of allocating the buffer and releasing it.
 *
 * @param in_buffer - a buffer big enough to hold all the entries in system db
 * @param in_buffer_size -  must bigger than the size of all entries for all system objects
*  @param system_object_count - must be system objects counts!!
 * @param actual_count - how many objects' entries(user,object and edges) 
 *                                          actually returned
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 * 
 * @version
 *  07/10/12 - Yang Zhang Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_persist_system_db(void * in_buffer,
                                                                           fbe_u32_t in_buffer_size,
                                                                           fbe_u32_t system_object_count,
                                                                           fbe_u32_t *actual_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_sg_element_t *                              sg_list = NULL;
    fbe_database_control_persist_sep_objets_t		persist_system_db;
    
    if ( system_object_count == 0 || actual_count == NULL || in_buffer == NULL
         || (in_buffer_size < system_object_count *(sizeof(database_object_entry_t) + sizeof(database_user_entry_t) + DATABASE_MAX_EDGE_PER_OBJECT * sizeof(database_edge_entry_t))))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)in_buffer;
    sg_list[0].count = in_buffer_size;
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    persist_system_db.number_of_objects_requested = system_object_count;
    persist_system_db.number_of_objects_returned = 0;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_DATABASE_CONTROL_CODE_PERSIST_SYSTEM_DB,
                                                                        &persist_system_db,
                                                                        sizeof (fbe_database_control_persist_sep_objets_t),
                                                                        FBE_SERVICE_ID_DATABASE,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_list,
                                                                        2,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_SEP_0);


    fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_count = persist_system_db.number_of_objects_returned;
    
    return FBE_STATUS_OK;

}
/*****************************************************************
 * fbe_api_database_get_max_configurable_objects
 *****************************************************************
 * @brief
 *  This function get the max configurable bojects in database service.
 *
 * @param fbe_u32_t - the max configurable objects of database service
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/17/12 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_max_configurable_objects(fbe_u32_t *max_configurable_objects)
{
    fbe_status_t                          status;
    fbe_database_control_get_max_configurable_objects_t control_get_max_configurable_objects;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_MAX_CONFIGURABLE_OBJECTS,
                                                    &control_get_max_configurable_objects, 
                                                    sizeof(fbe_database_control_get_max_configurable_objects_t));

    *max_configurable_objects = control_get_max_configurable_objects.max_configurable_objects;
    
    return status;
}
/*!***************************************************************
 *  @fn fbe_api_database_set_invalidate_config_flag()
 ****************************************************************
 * @brief
 *  This function sets invalidate configuration flag in SEP to true.
 *
 * @param None
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 *
 * @version
 *  10/11/12 - Vera Wang Created. 
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_set_invalidate_config_flag(void)
{
    fbe_status_t status;
    fbe_database_control_set_invalidate_config_flag_t invalidate_config_flag;

    invalidate_config_flag.flag = FBE_IMAGING_FLAGS_TRUE;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_INVALIDATE_CONFIG_FLAG, 
                                                      &invalidate_config_flag, sizeof(fbe_database_control_set_invalidate_config_flag_t));

    return status;

}
/*!***************************************************************
 *  @fn fbe_api_database_clear_invalidate_config_flag()
 ****************************************************************
 * @brief
 *  This function sets invalidate configuration flag in SEP to false.
 *
 * @param None
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 * 
 * @version
 *  10/11/12 - Vera Wang Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_clear_invalidate_config_flag(void)
{
    fbe_status_t status;
    fbe_database_control_set_invalidate_config_flag_t invalidate_config_flag;

    invalidate_config_flag.flag = FBE_IMAGING_FLAGS_FALSE;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_INVALIDATE_CONFIG_FLAG, 
                                                      &invalidate_config_flag, sizeof(fbe_database_control_set_invalidate_config_flag_t));

    return status;
}

/*!***************************************************************
 *  @fn fbe_api_database_versioning_operation(fbe_database_control_versioning_op_code_t *)
 ****************************************************************
 * @brief
 *  This function checks versioning related information in database serivce.
 *
 * @param fbe_database_control_versioning_op_code_t * - the versioning operation structure. 
 *    The caller of this function should fill the content.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 * 
 * @version
 *  11/28/12 - Wenxuan Yin Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_versioning_operation(fbe_database_control_versioning_op_code_t  *versioning_op_code_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (versioning_op_code_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL versioning op code.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_VERSIONING,
                                                      versioning_op_code_p,
                                                      sizeof(fbe_database_control_versioning_op_code_t));
    
    return status;
}


/*!***************************************************************
 *  @fn fbe_api_database_get_disk_ddmi_data ()
 ****************************************************************
 * @brief
 *  This function gets data from disk DDMI PSL region.
 *   It fills a buffer with bytes for DDMI data.
 *  The user is in charge of allocating the buffer and releasing it.
 *
 * @param out_ddmi_data - a buffer big enough to hold ddmi data
 * @param in_bus - input bus number
 * @param in_enclosure - input enclosure number
 * @param in_slot - input slot number
 * @param expected_count - how many bytes the user buffer can contain.
 * @param actual_count - how many bytes actually returned
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK if good
 * 
 * @version
 *  11/28/12 - Wenxuan Yin Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_disk_ddmi_data(fbe_u8_t *out_ddmi_data_p,
                                                    fbe_u32_t in_bus,
                                                    fbe_u32_t in_enclosure,
                                                    fbe_u32_t in_slot,
                                                    fbe_u32_t expected_count,
                                                    fbe_u32_t *actual_count)                                            
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sg_element_t                        *sg_list = NULL;
    fbe_database_control_ddmi_data_t        get_ddmi_data;

    if (expected_count == 0 || actual_count == NULL || out_ddmi_data_p == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = out_ddmi_data_p;
    sg_list[0].count = expected_count * sizeof(fbe_u8_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    get_ddmi_data.bus = in_bus;
    get_ddmi_data.enclosure = in_enclosure;
    get_ddmi_data.slot = in_slot;
    get_ddmi_data.number_of_bytes_requested = expected_count;
    get_ddmi_data.number_of_bytes_returned= 0;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_DATABASE_CONTROL_CODE_GET_DISK_DDMI_DATA,
                                                                        &get_ddmi_data,
                                                                        sizeof (fbe_database_control_ddmi_data_t),
                                                                        FBE_SERVICE_ID_DATABASE,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_list,
                                                                        2,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_SEP_0);


    fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_count = get_ddmi_data.number_of_bytes_returned;
    
    return FBE_STATUS_OK;

}

/****************************************************************/
/* fbe_api_database_add_hook()
 ****************************************************************
 * @brief
 *  This API sets hook in database service for testability
 *
 * @param hook_type - the hook type we want to add
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/24/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_add_hook(fbe_database_hook_type_t hook_type)
{
    fbe_status_t                                status;
    fbe_database_control_hook_t                 hook;

    hook.hook_type = hook_type;
    hook.hook_op_code = FBE_DATABASE_HOOK_OPERATION_SET_HOOK;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_HOOK, 
                                                      &hook, sizeof(hook));    
    return status;
}

/******************************************************************************/
/* fbe_api_database_remove_hook()
 ******************************************************************************
 * @brief
 *  This API removes an already set hook in database service  for testability
 *
 * @param hook_type - the hook type we want to remove
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/24/2012 - Created. Zhipeng Hu
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_remove_hook(fbe_database_hook_type_t hook_type)
{
    fbe_status_t                                status;
    fbe_database_control_hook_t                 hook;

    hook.hook_type = hook_type;
    hook.hook_op_code = FBE_DATABASE_HOOK_OPERATION_REMOVE_HOOK;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_HOOK, 
                                                      &hook, sizeof(hook));    
    return status;

}

/****************************************************************/
/* fbe_api_database_wait_hook()
 ****************************************************************
 * @brief
 *  This API wait for a hook in database service to be triggered
 *  The hook must be set using fbe_api_database_add_hook before
 *  for testability
 *
 * @param hook_type - the hook type we want to wait
 * @param timeout_ms - how many milliseconds do we want to wait
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/24/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_wait_hook(fbe_database_hook_type_t hook_type, fbe_u32_t timeout_ms)
{
    fbe_status_t                                status;
    fbe_database_control_hook_t                 hook;
    fbe_u32_t                                   wait_count = 0;
    fbe_bool_t                                  do_infinite_wait = FBE_FALSE;

    wait_count = (timeout_ms + 100 - 1)/100;
    if(wait_count == 0)
    {
        do_infinite_wait = FBE_TRUE;
    }

    hook.hook_type = hook_type;
    hook.hook_op_code = FBE_DATABASE_HOOK_OPERATION_GET_STATE;
    hook.is_set = FBE_FALSE;
    hook.is_triggered = FBE_FALSE;

    while(1)
    {
        status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_HOOK, 
                                                          &hook, sizeof(hook));
        if(status != FBE_STATUS_OK)
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: fail to send cmd to db srv. status:%d",
                           __FUNCTION__, status);
            return status;
        }

        if(hook.is_triggered == FBE_TRUE)
        {
            break;
        }

        if(do_infinite_wait == FBE_FALSE)
        {
            wait_count--;
            if(wait_count == 0)
                break;
        }

        fbe_api_sleep(100);
    }

    if(hook.is_triggered == FBE_FALSE)
    {
        return FBE_STATUS_TIMEOUT;
    }

    return FBE_STATUS_OK;
}

/*****************************************************************/
/** fbe_api_database_lun_operation_ktrace_warning
 ****************************************************************
 * @brief
 *  This function sends a warning ktrace message when LU is created/destroyed via fbecli.
 *
 * @param lun_id - the lun id
 * @param operation - create/destroy
 * @param result    - the result of create/destroy operation
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01/23/13 - Preethi Poddatur. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_lun_operation_ktrace_warning(fbe_object_id_t lun_id, 
                                              fbe_database_lun_operation_code_t operation,
                                              fbe_status_t result)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_set_lu_operation_t lu_operation;
    
    lu_operation.lun_id = lun_id;
    lu_operation.operation  = operation;
    lu_operation.status = result;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_MARK_LU_OPERATION_FROM_CLI,
                                                    &lu_operation, sizeof(lu_operation));

    if (status != FBE_STATUS_OK) {
       fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: fail to send control packet to database service status:%d",
                           __FUNCTION__, status);
       return status;
    }

    return FBE_STATUS_OK;
}
/*****************************************************************/
/** fbe_api_database_get_lun_raid_info
 ****************************************************************
 * @brief
 *  This function looks up a LUN's raid info.
 *
 * @param lun_object_id 
 * @param raid_info_p
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  4/17/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_lun_raid_info(fbe_object_id_t lun_object_id,
                                   fbe_database_get_sep_shim_raid_info_t *raid_info_p)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_get_sep_shim_raid_info_t raid_info;

    raid_info.lun_object_id = lun_object_id;
    if (raid_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_GET_LU_RAID_INFO,
                                                     &raid_info, sizeof(fbe_database_get_sep_shim_raid_info_t));

    /* return the discovered object id */
    *raid_info_p = raid_info;
    
    return status;
}
/*****************************************************************/
/** fbe_api_database_get_be_port_info
 ****************************************************************
 * @brief
 *  This function returns information about the BE ports.
 *
 * @param be_port_info 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01/17/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_get_be_port_info(fbe_database_control_get_be_port_info_t *be_port_info)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (be_port_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_GET_BE_PORT_INFO,
                                                     be_port_info, sizeof(fbe_database_control_get_be_port_info_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_setup_encryption_key
 ****************************************************************
 * @brief
 *  This sets up the object with the encryption keys.
 *
 * @param - Buffer the encryption Info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_setup_encryption_key(fbe_database_control_setup_encryption_key_t *encryption_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (encryption_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_SETUP_ENCRYPTION_KEY,
                                                     encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_setup_kek
 ****************************************************************
 * @brief
 *  This sets up the object with the key encryption keys(KEK).
 *
 * @param - Buffer the encryption Info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_setup_kek(fbe_database_control_setup_kek_t *kek_encryption_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (kek_encryption_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_SETUP_KEK,
                                                     kek_encryption_info, sizeof(fbe_database_control_setup_kek_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_destroy_kek
 ****************************************************************
 * @brief
 *  This initiates the destroy of the key encryption keys(KEK)
 *
 * @param - Buffer the encryption Info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_destroy_kek(fbe_database_control_destroy_kek_t *kek_encryption_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_DESTROY_KEK,
                                                     kek_encryption_info, sizeof(fbe_database_control_destroy_kek_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_setup_kek_kek
 ****************************************************************
 * @brief
 *  This sets up the object with the Key for key encryption keys(KEK of KEKs).
 *
 * @param - Buffer the encryption Info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_setup_kek_kek(fbe_database_control_setup_kek_kek_t *kek_encryption_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (kek_encryption_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_SETUP_KEK_KEK,
                                                     kek_encryption_info, sizeof(fbe_database_control_setup_kek_kek_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_destroy_kek_kek
 ****************************************************************
 * @brief
 *  This sets the unregistration of the key encryption keys(KEK)
 *
 * @param - Buffer the encryption Info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_destroy_kek_kek(fbe_database_control_destroy_kek_kek_t *kek_kek_destroy_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;

     if (kek_kek_destroy_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_DESTROY_KEK_KEK,
                                                     kek_kek_destroy_info, sizeof(fbe_database_control_destroy_kek_kek_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_reestablish_kek_kek
 ****************************************************************
 * @brief
 *  This reestablishes the KEK of KEKs
 *
 * @param - Buffer the encryption Info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_reestablish_kek_kek(fbe_database_control_reestablish_kek_kek_t *kek_kek_handle_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;

     if (kek_kek_handle_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_REESTABLISH_KEK_KEK,
                                                     kek_kek_handle_info, sizeof(fbe_database_control_reestablish_kek_kek_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_set_port_encryption_mode
 ****************************************************************
 * @brief
 *  This sets the port to a particular encryption mode
 *
 * @param - Buffer the encryption Info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_set_port_encryption_mode(fbe_database_control_port_encryption_mode_t *mode_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;

     if (mode_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_SET_PORT_ENCRYPTION_MODE,
                                                     mode_info, sizeof(fbe_database_control_port_encryption_mode_t));

    return status;
}

/*****************************************************************/
/** fbe_api_database_get_port_encryption_mode
 ****************************************************************
 * @brief
 *  This gets the encryption mode of the port
 *
 * @param - Buffer the encryption Info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/03/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_port_encryption_mode(fbe_database_control_port_encryption_mode_t *mode_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;

     if (mode_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_GET_PORT_ENCRYPTION_MODE,
                                                     mode_info, sizeof(fbe_database_control_port_encryption_mode_t));

    return status;
}

/*!***************************************************************
 * @fn fbe_api_database_get_total_locked_objects_of_class(
 *     fbe_u32_t *total_objects,
 *     database_class_id_t db_class_id )
 *****************************************************************
 * @brief
 *   This function returns the total number of objects that are in 
 *  the locked state 
 *
 * @param total_objects             - pointer to total objects
 * @param db_class_id
 * 
 * @return fbe_status_t - success or failure.
 *
 * @version
 *    10/23/13: Ashok Tamilarasan  created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_total_locked_objects_of_class(database_class_id_t db_class_id,
                                                                             fbe_u32_t *total_objects )
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_total_locked_objects_of_class_t      get_total;/* this structure is small enough to be on the stack*/
    fbe_api_control_operation_status_info_t              status_info = {0};

    get_total.number_of_objects = 0;
    get_total.class_id = db_class_id;

    status = fbe_api_common_send_control_packet_to_service (FBE_DATABASE_CONTROL_CODE_GET_TOTAL_LOCKED_OBJECTS_OF_CLASS,
                                                            &get_total,
                                                            sizeof (fbe_database_control_get_total_locked_objects_of_class_t),
                                                            FBE_SERVICE_ID_DATABASE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *total_objects = get_total.number_of_objects;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_database_get_locked_objects_info_of_class(
 *     fbe_database_control_get_locked_objects_info_t * info_buffer, 
 *     fbe_u32_t total_objects, 
 *     fbe_u32_t *actual_objects)
 *****************************************************************
 * @brief
 *  This function get information about objects that are in the locked
 *  state 
 *
 * @param info_buffer - The array to return results in.
 * @param total_objects - how many object we allocated for.
 * @param actual_objects - how many objects were actually copied to memory
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/23/13: Ashok Tamilarasan    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_locked_object_info_of_class(database_class_id_t db_class_id,
                                                                           fbe_database_control_get_locked_object_info_t * info_buffer, 
                                                                           fbe_u32_t total_objects, 
                                                                           fbe_u32_t *actual_objects)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_database_control_get_locked_info_of_class_header_t   locked_object_header;
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_sg_element_t *                      temp_sg_list = NULL;

    if (info_buffer == NULL || total_objects == 0) {
        *actual_objects = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);/*one for the entry and one for the NULL termination*/
    temp_sg_list = sg_list;

    /*set up the sg list to point to the list of objects the user wants to get*/
    temp_sg_list->address = (fbe_u8_t *)info_buffer;
    temp_sg_list->count = total_objects * sizeof(fbe_database_control_get_locked_object_info_t);
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    locked_object_header.number_of_objects = total_objects;
    locked_object_header.class_id = db_class_id;
    locked_object_header.get_locked_only = FBE_TRUE;

    /*upon completion, the user memory will be filled with the data, this is taken care of by the FBE API common code*/
    status = fbe_api_common_send_control_packet_to_service_with_sg_list (FBE_DATABASE_CONTROL_CODE_GET_LOCKED_OBJECT_INFO_OF_CLASS,
                                                                         &locked_object_header,
                                                                         sizeof (fbe_database_control_get_locked_info_of_class_header_t),
                                                                         FBE_SERVICE_ID_DATABASE,
                                                                         FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                         sg_list,
                                                                         2,/*2 sg entries*/
                                                                         &status_info,
                                                                         FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        fbe_api_free_memory (sg_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (actual_objects != NULL) {
        *actual_objects = locked_object_header.number_of_objects_copied;
    }

    fbe_api_free_memory (sg_list);
    return FBE_STATUS_OK;
}


/*****************************************************************/
/** fbe_api_database_setup_encryption_rekey
 ****************************************************************
 * @brief
 *  This sets up the object with another set of encryption keys
 *  when a rekey is needed.
 *
 * @param - Buffer the encryption Info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_setup_encryption_rekey(fbe_database_control_setup_encryption_key_t *encryption_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (encryption_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_SETUP_ENCRYPTION_REKEY,
                                                     encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));

    return status;
}


/*****************************************************************/
/** fbe_api_database_set_capacity_limit
 ****************************************************************
 * @brief
 *  This function sets the capacity limit for pvd
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_set_capacity_limit(fbe_database_capacity_limit_t * capacity_limit)                                            
{
    fbe_status_t status = FBE_STATUS_OK;	

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_CAPACITY_LIMIT,
                                                      capacity_limit, sizeof(fbe_database_capacity_limit_t));

    return status;
}


/*****************************************************************/
/** fbe_api_database_get_capacity_limit
 ****************************************************************
 * @brief
 *  This function gets the capacity limit for pvd
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_capacity_limit(fbe_database_capacity_limit_t *capacity_limit)                                            
{
    fbe_status_t status = FBE_STATUS_OK;

    if (capacity_limit == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_CAPACITY_LIMIT,
                                                      capacity_limit, sizeof(fbe_database_capacity_limit_t));

    return status;
}  


/*****************************************************************/
/** fbe_api_database_remove_encryption_keys
 ****************************************************************
 * @brief
 *  This function deletes the object with the encryption keys.
 *
 * @param - object ID.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/13/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_remove_encryption_keys(fbe_database_control_remove_encryption_keys_t * encryption_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (encryption_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_REMOVE_ENCRYPTION_KEYS,
                                                     encryption_info, sizeof(fbe_database_control_remove_encryption_keys_t));

    return status;
}

/*****************************************************************
 * fbe_api_database_get_system_encryption_progress
 ****************************************************************
 * @brief
 *  This function returns information about the progress of encryption
 *  on the system as a whole
 *
 * @param 
 *   encryption_progress - Buffer to store the encryption progress 
 *                         related info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01/07/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_get_system_encryption_progress(fbe_database_system_encryption_progress_t *encryption_progress)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (encryption_progress == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_PROGRESS,
                                                     encryption_progress, sizeof(fbe_database_system_encryption_progress_t));

    return status;
}

/*****************************************************************
 * fbe_api_database_set_poll_interval
 ****************************************************************
 * @brief
 *  This function sets the frequency database updating in progress status
 *
 * @param 
 *   poll_interval - new interval
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_set_poll_interval(fbe_u32_t poll_interval)
{
    fbe_database_system_poll_interval_t 	new_poll_interval;
    fbe_status_t                            status = FBE_STATUS_OK;
    

    new_poll_interval.poll_interval = poll_interval;

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_SET_POLL_INTERVAL,
                                                     &new_poll_interval, sizeof(fbe_database_system_poll_interval_t));

    return status;
}


/*****************************************************************
 * fbe_api_database_get_system_scrub_progress
 ****************************************************************
 * @brief
 *  This function returns information about the progress of scrubbing
 *  on the system as a whole
 *
 * @param 
 *   scrub_progress - Buffer to store the scrubbing progress 
 *                         related info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_get_system_scrub_progress(fbe_database_system_scrub_progress_t *scrub_progress)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (scrub_progress == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_SCRUB_PROGRESS,
                                                     scrub_progress, sizeof(fbe_database_system_scrub_progress_t));

    return status;
}

/*****************************************************************
 * fbe_api_database_get_system_encryption_info
 ****************************************************************
 * @brief
 *  This function returns information related to System Encryption
 *
 * @param 
 *   sys_encryption_info - Buffer to store the system encryption status
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01/07/2014 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_database_get_system_encryption_info(fbe_database_system_encryption_info_t *sys_encryption_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (sys_encryption_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_INFO,
                                                     sys_encryption_info, sizeof(fbe_database_system_encryption_info_t));

    return status;
}

fbe_status_t FBE_API_CALL 
fbe_api_database_get_encryption_paused(fbe_bool_t * paused)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (paused == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_GET_ENCRYPTION_PAUSED,
                                                     paused,
                                                     sizeof(fbe_bool_t));

    return status;
}



/*****************************************************************/
/** fbe_api_database_update_drive_keys
 ****************************************************************
 * @brief
 *  This function updates the object with the encryption keys.
 *
 * @param - encryption_info.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01/03/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_update_drive_keys(fbe_database_control_update_drive_key_t * encryption_info)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (encryption_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_UPDATE_DRIVE_KEYS,
                                                     encryption_info, sizeof(fbe_database_control_update_drive_key_t));

    return status;
}


fbe_status_t FBE_API_CALL fbe_api_database_get_backup_info(fbe_database_backup_info_t * backup_info)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (backup_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_BACKUP_INFO,
                                                      backup_info, sizeof(fbe_database_backup_info_t));

    return status;
}  

fbe_status_t FBE_API_CALL fbe_api_database_set_backup_info(fbe_database_backup_info_t * backup_info)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (backup_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_BACKUP_INFO,
                                                      backup_info, sizeof(fbe_database_backup_info_t));

    return status;
}  

fbe_status_t FBE_API_CALL fbe_api_database_set_update_db_on_peer_op(fbe_database_control_db_update_on_peer_op_t update_op)
{
    fbe_database_control_db_update_peer_t   update_peer;
    fbe_status_t status = FBE_STATUS_OK;

    update_peer.update_op = update_op;
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_UPDATE_DB_ON_PEER,
                                                      &update_peer, sizeof(fbe_database_control_db_update_peer_t));

    return status;
}  


/*****************************************************************/
/** fbe_api_database_update_drive_keys
 ****************************************************************
 * @brief
 *  This function updates the object with the encryption keys.
 *
 * @param - encryption_info.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01/03/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_get_drive_sn_for_raid(fbe_database_control_get_drive_sn_t *drive_sn)
{

    fbe_status_t                            status = FBE_STATUS_OK;
    
    if (drive_sn == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_GET_DRIVE_SN_FOR_RAID,
                                                     drive_sn, sizeof(fbe_database_control_get_drive_sn_t));

    return status;
}

/*!***************************************************************
 * @fn fbe_api_database_get_encryption_info_of_class(
 *     fbe_database_control_get_locked_objects_info_t * info_buffer, 
 *     fbe_u32_t total_objects, 
 *     fbe_u32_t *actual_objects)
 *****************************************************************
 * @brief
 *  This function get information about objects that are in the locked
 *  state 
 *
 * @param info_buffer - The array to return results in.
 * @param total_objects - how many object we allocated for.
 * @param actual_objects - how many objects were actually copied to memory
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  01/27/2014: Lili Chen    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_encryption_info_of_class(database_class_id_t db_class_id,
                                                                           fbe_database_control_get_locked_object_info_t * info_buffer, 
                                                                           fbe_u32_t total_objects, 
                                                                           fbe_u32_t *actual_objects)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_database_control_get_locked_info_of_class_header_t   locked_object_header;
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_sg_element_t *                      temp_sg_list = NULL;

    if (info_buffer == NULL || total_objects == 0) {
        *actual_objects = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);/*one for the entry and one for the NULL termination*/
    temp_sg_list = sg_list;

    /*set up the sg list to point to the list of objects the user wants to get*/
    temp_sg_list->address = (fbe_u8_t *)info_buffer;
    temp_sg_list->count = total_objects * sizeof(fbe_database_control_get_locked_object_info_t);
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    locked_object_header.number_of_objects = total_objects;
    locked_object_header.class_id = db_class_id;
    locked_object_header.get_locked_only = FBE_FALSE;

    /*upon completion, the user memory will be filled with the data, this is taken care of by the FBE API common code*/
    status = fbe_api_common_send_control_packet_to_service_with_sg_list (FBE_DATABASE_CONTROL_CODE_GET_LOCKED_OBJECT_INFO_OF_CLASS,
                                                                         &locked_object_header,
                                                                         sizeof (fbe_database_control_get_locked_info_of_class_header_t),
                                                                         FBE_SERVICE_ID_DATABASE,
                                                                         FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                         sg_list,
                                                                         2,/*2 sg entries*/
                                                                         &status_info,
                                                                         FBE_PACKAGE_ID_SEP_0);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        fbe_api_free_memory (sg_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (actual_objects != NULL) {
        *actual_objects = locked_object_header.number_of_objects_copied;
    }

    fbe_api_free_memory (sg_list);
    return FBE_STATUS_OK;
}


fbe_status_t FBE_API_CALL fbe_api_database_get_peer_state(fbe_database_state_t *state)
{
    fbe_status_t                          status;
    fbe_database_control_get_state_t control_state;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_PEER_STATE,
                                                    &control_state, sizeof(control_state));

    *state = control_state.state;
    
    return status;
}

fbe_status_t FBE_API_CALL fbe_api_database_set_drive_tier_enabled(fbe_bool_t enabled)
{
    fbe_status_t                          status;
    fbe_database_control_bool_t           drive_tier_enabled;

    drive_tier_enabled.is_enabled =  enabled;
    
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_EXTENDED_CACHE_ENABLED,
                                                      &drive_tier_enabled, sizeof(fbe_database_control_bool_t));
    
    return status;
}

/*!*************************************************************************** 
 * @fn fbe_api_database_set_debug_flags(
 *     fbe_database_debug_flags_t database_debug_flags) 
 *****************************************************************************
 *
 * @brief   This function sets the database debug flags to the requested value.
 *
 * @param   database_debug_flags - The database debug flags to set (or clear).
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @version
 *  06/30/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_set_debug_flags(fbe_database_debug_flags_t database_debug_flags)
{
    fbe_status_t                            status;
    fbe_database_control_set_debug_flags_t  set_debug_flags;
    
    set_debug_flags.set_db_debug_flags = database_debug_flags;
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_SET_DEBUG_FLAGS,
                                                      &set_debug_flags, 
                                                      sizeof(fbe_database_control_set_debug_flags_t));
    
    return status;
}
/**************************************** 
 * end fbe_api_database_set_debug_flags()
 ****************************************/

/*!*************************************************************************** 
 * @fn fbe_api_database_get_debug_flags(
 *     fbe_database_debug_flags_t *database_debug_flags_p) 
 *****************************************************************************
 *
 * @brief   This function gets the database debug flags to the requested value.
 *
 * @param   database_debug_flags - The database debug flags to set (or clear).
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @version
 *  06/30/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_debug_flags(fbe_database_debug_flags_t *database_debug_flags_p)
{
    fbe_status_t                            status;
    fbe_database_control_get_debug_flags_t  get_debug_flags;
    
    fbe_zero_memory(&get_debug_flags, sizeof(fbe_database_control_get_debug_flags_t));
    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_GET_DEBUG_FLAGS,
                                                      &get_debug_flags, 
                                                      sizeof(fbe_database_control_get_debug_flags_t));
    if (status == FBE_STATUS_OK) {
        *database_debug_flags_p = get_debug_flags.get_db_debug_flags;
    }
    return status;
}
/**************************************** 
 * end fbe_api_database_get_debug_flags()
 ****************************************/

/*!*************************************************************************** 
 * @fn fbe_api_database_validate_database(
 *     fbe_database_validate_failure_action_t failure_action) 
 ***************************************************************************** 
 * 
 * @brief   This function sends a request to the database service to `validate'
 *          the database.  It reads the on-disk records and compares them to
 *          the in-memory records.  If there is any discrepancy it takes the
 *          action specified.
 * 
 * @param   caller - Identifies caller for debug
 * @param   failure_action - What action to take if there is a validation failure:
 *              FBE_DATABASE_VALIDATE_FAILURE_ACTION_ERROR_TRACE - Simply generate
 *                  an error trace.
 *              FBE_DATABASE_VALIDATE_FAILURE_ACTION_CORRECT_ENTRY - Correct the
 *                  entry. Currently this option isn't supported.
 *              FBE_DATABASE_VALIDATE_FAILURE_ACTION_PANIC_SP - Generate a
 *                  Critical error trace which will PANIC the SP.
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @note    Currently FBE_DATABASE_VALIDATE_FAILURE_ACTION_CORRECT_ENTRY is not
 *          supported and will result in an error if requested.
 *
 * @version
 *  06/30/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_validate_database(fbe_database_validate_request_type_t caller,
                                                             fbe_database_validate_failure_action_t failure_action)
{
    fbe_status_t                            status;
    fbe_api_job_service_validate_database_t validate_request;
    fbe_database_control_is_validation_allowed_t is_validation_allowed;
    fbe_status_t                            job_status;
    fbe_job_service_error_type_t            job_error_code;
    
    /* Populate the API request.
     */
    fbe_zero_memory(&validate_request, sizeof(fbe_api_job_service_validate_database_t));
    validate_request.validate_caller = caller;
    validate_request.failure_action = failure_action;

    /* We always attempt User requests, but for any other type of request
     * we will only attempt the validation if it is enabled.
     */
    fbe_zero_memory(&is_validation_allowed, sizeof(fbe_database_control_is_validation_allowed_t));
    is_validation_allowed.validate_caller = caller;
    if (caller != FBE_DATABASE_VALIDATE_REQUEST_TYPE_USER) {
            status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_IS_VALIDATION_ALLOWED,
                                                             &is_validation_allowed, 
                                                             sizeof(fbe_database_control_is_validation_allowed_t));
            if (status != FBE_STATUS_OK) {
                fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s: caller: %d failure action: %d is allowed failed - status: 0x%x\n", 
                       __FUNCTION__, 
                       validate_request.validate_caller,
                       validate_request.failure_action,
                       status);
                return status;
            }
            if (!is_validation_allowed.b_allowed) {
                fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                       "%s: caller: %d failure action: %d validation not allowed reason: %d\n", 
                       __FUNCTION__, 
                       validate_request.validate_caller,
                       validate_request.failure_action,
                       is_validation_allowed.not_allowed_reason);
                return FBE_STATUS_FAILED;
            }
    }

    /* Start the job.
     */
    status = fbe_api_job_service_validate_database(&validate_request);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s: caller: %d failure action: %d job: 0x%llx failed - status: 0x%x\n", 
                       __FUNCTION__, 
                       validate_request.validate_caller,
                       validate_request.failure_action,
                       validate_request.job_number,
                       status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Wait for the job to complete.
     */
    status = fbe_api_common_wait_for_job(validate_request.job_number,
                                         FBE_JOB_SERVICE_WAIT_VALIDATION_TIMEOUT, 
                                         &job_error_code, &job_status, NULL);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: failed waiting for job - status: 0x%x\n", 
                      __FUNCTION__, status);
        return status;
    }

    if (job_status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: job failed with status: %d error code: %d\n", 
                      __FUNCTION__, job_status, job_error_code);
    }
    
    return job_status;
}
/******************************************* 
 * end fbe_api_database_validate_database()
 *******************************************/

fbe_status_t FBE_API_CALL fbe_api_database_set_garbage_collection_debouncer(fbe_u32_t timeout_ms)
{
    fbe_status_t                          status;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_SET_GARBAGE_COLLECTION_DEBOUNCER,
                                                      &timeout_ms, sizeof(fbe_u32_t));
    
    return status;
}

fbe_status_t FBE_API_CALL fbe_api_database_check_bootflash_mode(fbe_bool_t *bootflash_mode_flag)
{
    fbe_status_t                          status;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_CODE_CHECK_BOOTFLASH_MODE,
                                                      bootflash_mode_flag, sizeof(fbe_bool_t));
    
    return status;
}


/*****************************************************************/
/** fbe_api_database_lookup_lun_by_number
 ****************************************************************
 * @brief
 *  This function looks up a ext pool lun object id by using it's number
 *
 * @param lun_number(in) - lun_number
 * @param object_id(out)    - object id
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_lookup_ext_pool_lun_by_number(fbe_u32_t lun_id, 
                                               fbe_object_id_t *object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_lookup_ext_pool_lun_t lookup_lun;
    
    if (object_id == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lookup_lun.pool_id  = FBE_POOL_ID_INVALID; /* For user ext pool LUN we don't need pool id */
    lookup_lun.lun_id  = lun_id;

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_LOOKUP_EXT_POOL_LUN_BY_NUMBER,
                                                     &lookup_lun, sizeof(fbe_database_control_lookup_ext_pool_lun_t));

    /* return the discovered object id */
    *object_id = lookup_lun.object_id;
    
    return status;
}

/*****************************************************************/
/** fbe_api_database_lookup_ext_pool_by_number
 ****************************************************************
 * @brief
 *  This function looks up a pool object id by using it's number
 *
 * @param pool_id(in)       - pool id
 * @param object_id(out)    - object id
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_database_lookup_ext_pool_by_number(fbe_u32_t pool_id,
                                           fbe_object_id_t *object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_lookup_ext_pool_t lookup_pool;
    
    if (object_id == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lookup_pool.pool_id  = pool_id;

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_LOOKUP_EXT_POOL_BY_NUMBER,
                                                     &lookup_pool, sizeof(fbe_database_control_lookup_ext_pool_t));

    /* return the discovered object id */
    *object_id = lookup_pool.object_id;
    
    return status;
}

fbe_status_t FBE_API_CALL 
fbe_api_database_update_mirror_pvd_map(fbe_object_id_t rg_obj_id, fbe_u16_t edge_index, fbe_u8_t *sn, fbe_u32_t size)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_database_control_mirror_update_t mirror_update;

    fbe_zero_memory(&mirror_update, sizeof(fbe_database_control_mirror_update_t));
    mirror_update.rg_obj_id = rg_obj_id;
    mirror_update.edge_index = edge_index;
    strncpy(mirror_update.sn, sn, size);

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_UPDATE_MIRROR_PVD_MAP,
                                                     &mirror_update,
                                                     sizeof(fbe_database_control_mirror_update_t));

    return status;
}

fbe_status_t FBE_API_CALL 
fbe_api_database_get_mirror_pvd_map(fbe_object_id_t rg_obj_id, fbe_u16_t edge_index, fbe_u8_t *sn, fbe_u32_t size)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_database_control_mirror_update_t mirror_update;

    fbe_zero_memory(&mirror_update, sizeof(fbe_database_control_mirror_update_t));
    mirror_update.rg_obj_id = rg_obj_id;
    mirror_update.edge_index = edge_index;

    status = send_control_packet_to_database_service(FBE_DATABASE_CONTROL_CODE_GET_MIRROR_PVD_MAP,
                                                     &mirror_update,
                                                     sizeof(fbe_database_control_mirror_update_t));
    if (status == FBE_STATUS_OK)
    {
        strncpy(sn, mirror_update.sn, size);
    }

    return status;
}


/*!*************************************************************************** 
 * @fn fbe_api_database_get_additional_supported_drive_types(
 *     fbe_database_additional_drive_types_supported_t *types_p) 
 ***************************************************************************** 
 * 
 * @brief   This function get the additional supported drive types
 * 
 * @param   types_p - drive types
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @version
 *  08/19/2016  Wayne Garrett  - Created.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_get_additional_supported_drive_types(fbe_database_additional_drive_types_supported_t *types_p)
{
    fbe_status_t                          status;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_GET_ADDL_SUPPORTED_DRIVE_TYPES,
                                                      types_p, sizeof(*types_p));    
    return status;
}

/*!*************************************************************************** 
 * @fn fbe_api_database_set_supported_drive_types(
 *     fbe_database_control_set_supported_drive_type_t *set_drive_type_p) 
 ***************************************************************************** 
 * 
 * @brief   This function sets the additional supported drive types
 * 
 * @param   set_drive_type_p - drive type to set
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @version
 *  08/19/2016  Wayne Garrett  - Created.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_database_set_additional_supported_drive_types(fbe_database_control_set_supported_drive_type_t *set_drive_type_p)
{
    fbe_status_t                          status;

    status = send_control_packet_to_database_service (FBE_DATABASE_CONTROL_SET_ADDL_SUPPORTED_DRIVE_TYPES,
                                                      set_drive_type_p, sizeof(*set_drive_type_p));    
    return status;
}

/**************************************** 
 * end file fbe_api_database_interface.c 
 ***************************************/ 
