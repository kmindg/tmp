/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_spare_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the drive swap library utility function.
 *
 * @version
 *  8/26/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_spare.h"
#include "fbe_spare_lib_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_event_log_api.h"                      //  for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                    //  for message codes
#include "fbe_private_space_layout.h"
#include "fbe_cmi.h"



/*Initialize the tier table configuration */
static fbe_performance_tier_drive_information_t tier_table_configuration[FBE_PERFORMANCE_TIER_LAST] = 

{
    /*  Performance Tier Identifier     Primary drive type for this tier    Drive types supported for this performance tier */
    {   FBE_PERFORMANCE_TIER_ZERO,      FBE_DRIVE_TYPE_SAS_FLASH_HE,        FBE_DRIVE_TYPE_SATA_FLASH_HE, 
    },
    {   FBE_PERFORMANCE_TIER_ONE,       FBE_DRIVE_TYPE_SAS_NL,
    },
    {   FBE_PERFORMANCE_TIER_TWO,       FBE_DRIVE_TYPE_SAS,
    },
    {   FBE_PERFORMANCE_TIER_THREE,     FBE_DRIVE_TYPE_SATA_FLASH_HE,          FBE_DRIVE_TYPE_SAS_FLASH_HE,
    },
    {   FBE_PERFORMANCE_TIER_FOUR,      FBE_DRIVE_TYPE_SATA_PADDLECARD,
    },
    {   FBE_PERFORMANCE_TIER_FIVE,      FBE_DRIVE_TYPE_SAS_FLASH_ME,
    },
    {   FBE_PERFORMANCE_TIER_SIX,       FBE_DRIVE_TYPE_SAS_FLASH_LE,
    },
    {   FBE_PERFORMANCE_TIER_SEVEN,     FBE_DRIVE_TYPE_SAS_FLASH_RI,
    },
    {   FBE_PERFORMANCE_TIER_EIGHT,     FBE_DRIVE_TYPE_INVALID,
    }
};

/*!****************************************************************************
 * fbe_spare_lib_get_performance_table_info()
 ******************************************************************************
 * @brief
 *  This function is used to to get the drive type for a given tier
 *  number and tier drive index
 *
 * @param performance_tier_number - Performance tier number.
 * @param tier_drive_info_p - Pointer to supported drive information
 *
 * @return status - FBE_STATUS_OK.
 *
 * @author
 *  07/28/2010 - Created. Shanmugam.
 *
 ******************************************************************************/

fbe_status_t
fbe_spare_lib_get_performance_table_info(fbe_u32_t performance_tier_number, 
                                         fbe_performance_tier_drive_information_t *tier_drive_info_p)
{
    /* Validate the performance tier.
     */
    if (((fbe_s32_t)performance_tier_number < FBE_PERFORMANCE_TIER_ZERO) ||
        (performance_tier_number >= FBE_PERFORMANCE_TIER_LAST)              )
    {
        /* Fail the request
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s invalid performance tier: %d\n",
                               __FUNCTION__, performance_tier_number);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note In order to protect the global table from modification
     *        we copy the table into the pointer passed.
     */
    *tier_drive_info_p = tier_table_configuration[performance_tier_number];
    return FBE_STATUS_OK;

}

/******************************************************************************
 * end fbe_spare_lib_get_performance_table_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_send_control_packet()
 ******************************************************************************
 * @brief
 * This function is used to send a control packet from the spare library.
 *
 * @param control_code  - Control code to send to other services.
 * @param buffer        - Buffer containing data for service.
 * @param buffer_length - Length of buffer.
 * @param service_id    - Service to send buffer.
 * @param class_id      - Class-id where service belongs.
 * @param object_id     - Object-id on which service will work on.
 * @param attr          - Attributes of packet 
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/15/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_send_control_packet(fbe_payload_control_operation_opcode_t control_code,
                                        fbe_payload_control_buffer_t buffer,
                                        fbe_payload_control_buffer_length_t buffer_length,
                                        fbe_service_id_t service_id,
                                        fbe_class_id_t class_id,
                                        fbe_object_id_t object_id,
                                        fbe_packet_attr_t attr)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                  packet_p = NULL;
    fbe_payload_ex_t *             sep_payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_status_t    payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_cpu_id_t cpu_id;

    /* Allocate the packet. */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s packet allocation failed.\n",
                               __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    /* Initialize the packet. */
    fbe_transport_initialize_sep_packet(packet_p);

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet_p, cpu_id);

    sep_payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    fbe_payload_control_build_operation(control_operation,
                                        control_code,
                                        buffer,
                                        buffer_length);
    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              service_id,
                              class_id,
                              object_id); 

    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet_p, attr);

    fbe_transport_set_sync_completion_type(packet_p,
            FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);

    /* wait for the completion.*/
    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s payload control operation failed! Control Code:%u\n",
                               __FUNCTION__, control_code);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s packet failed\n",
                               __FUNCTION__);
    }

    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    fbe_transport_release_packet(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_send_control_packet()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_check_capacity()
 ******************************************************************************
 * @brief
 *  This function is used to to find out whether current hot spare
 *  gross capacity is greather than desired capacity for the 
 *  selection of the spare or not.
 *  It returns TRUE if current spare gross capacity is greater
 *  than or equal to desired spare capacity otherwise it 
 *  returns FALSE.
 *
 * @param desired_spare_capacity - Desired spare capacity.
 * @param curr_spare_gross_capacity - Current spare gross capacity.
 * @param hs_capacity_check_p - Returns TRUE/FALSE based on input data.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/28/2009 - Created. Dhaval Patel.
 *
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_check_capacity(fbe_lba_t desired_spare_capacity,
                                   fbe_lba_t curr_spare_gross_capacity,
                                   fbe_bool_t *hs_capacity_check_p)
{

    /* Return false if current spare gross capacity is invalid.
     */
    if (curr_spare_gross_capacity == FBE_LBA_INVALID)
    {
        *hs_capacity_check_p = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /* Return TRUE if current spare gross capacity is greather than desired
     * spare capacity else return FALSE.
     */
    if(curr_spare_gross_capacity >= desired_spare_capacity)
    {
        *hs_capacity_check_p = FBE_TRUE;
    }
    else
    {
        *hs_capacity_check_p = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_utils_check_capacity()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_is_drive_type_valid_for_performance_tier()
 ******************************************************************************
 * @brief
 *  This function is used to check if current hot spare drive type is in the same performance tier
 *  table of the desired drive type spare.
 *
 * @param original_drive_performance_tier - The performance tier of the original drive
 * @param proposed_spare_drive_type - Drive type of the proposed spare
 * @param b_is_proposed_spare_in_performance_tier_p - FBE_TRUE - The proposed spare
 *          is in the same performance tier.
 *                                                  FBE_FALSE - The proposed spare
 *          is not in the same performance tier as the orignal drive.
 *
 * @return status - FBE_STATUS_OK.
 *
 * @author
 *  02/28/2013  Ron Proulx  - Updated.
 *
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_is_drive_type_valid_for_performance_tier(fbe_performance_tier_number_t  performance_tier_number,
                                                                          fbe_drive_type_t proposed_spare_drive_type,
                                                                          fbe_bool_t *b_is_proposed_spare_in_performance_tier_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_performance_tier_drive_information_t    tier_drive_info;
    fbe_u32_t                                   tier_drive_index;

    /* By default it doesn't match.
     */
    *b_is_proposed_spare_in_performance_tier_p = FBE_FALSE;

    /* Return false if current spare gross capacity is invalid.
     */
    if ((proposed_spare_drive_type == FBE_DRIVE_TYPE_INVALID) ||
        (proposed_spare_drive_type >= FBE_DRIVE_TYPE_LAST)       )
    {
        /* This is an error */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "%s invalid proposed spare drive type: %d\n",
                               __FUNCTION__, proposed_spare_drive_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the performance tier information for the tier indicated
     */
    status = fbe_spare_lib_get_performance_table_info(performance_tier_number, &tier_drive_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "%s get perf tier info for tier: %d failed - status: 0x%x\n",
                               __FUNCTION__, performance_tier_number, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Find the first compatible drive type in the tier specified.
     */
    for (tier_drive_index = 0; tier_drive_index < FBE_DRIVE_TYPE_LAST; tier_drive_index++)
    {
        /* Check if the proposed spare drive 
         */
        if (tier_drive_info.supported_drive_type_array[tier_drive_index] == proposed_spare_drive_type)
        {
            /* We found a match.  Trace some information if it wasn't the first
             * entry in the table.
             */
            if (tier_drive_index != 0)
            {
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO, 
                                       "spare: original tier: %d drive type: %d being replaced by drive type: %d\n",
                                       performance_tier_number, tier_drive_info.supported_drive_type_array[0],
                                       tier_drive_info.supported_drive_type_array[tier_drive_index]);
            }
            *b_is_proposed_spare_in_performance_tier_p = FBE_TRUE;
            break;
        }
    }

    /* Even if the proposed spare was not compatible return success
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_utils_check_if_hot_spare_is_in_same_performance_tier()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_complete_packet()
 ******************************************************************************
 * @brief
 *  Complete the control packet with a given status.
 *
 * @param packet_p - packet to complete.
 * @parram packet_status - Status to put in packet.
 *
 * @return None.
 *
 * @author
 *  10/28/2009 - Created. Dhaval Patel.
 ******************************************************************************/
void fbe_spare_lib_utils_complete_packet(fbe_packet_t *packet_p, 
                                         fbe_status_t packet_status)
{
    fbe_transport_set_status(packet_p, packet_status, 0);
    fbe_transport_complete_packet(packet_p);
    return;
}
/******************************************************************************
 * end fbe_spare_lib_utils_complete_packet()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_start_database_transaction()
 ******************************************************************************
 * @brief
 * This function starts a transaction operation with the database service
 *
 * @param transaction_id - transaction id for database service
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t 
fbe_spare_lib_utils_start_database_transaction(fbe_database_transaction_id_t *transaction_id, fbe_u64_t job_number)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_transaction_info_t transaction_info;
    transaction_info.transaction_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    transaction_info.job_number = job_number;
    transaction_info.transaction_type = FBE_DATABASE_TRANSACTION_RECOVERY;

    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_CODE_TRANSACTION_START,
                                                     &transaction_info,
                                                     sizeof(transaction_info),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s Transaction start failed.\n",
                               __FUNCTION__);
         return FBE_STATUS_GENERIC_FAILURE;
    }

    *transaction_id = transaction_info.transaction_id;
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_start_database_transaction()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_commit_database_transaction()
 ******************************************************************************
 * @brief
 * This function commits a transaction with the database
 * service
 *
 * @param transaction_id - transaction id for database service
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_commit_database_transaction(fbe_database_transaction_id_t transaction_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 

    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_CODE_TRANSACTION_COMMIT,
                                                     &transaction_id,
                                                     sizeof(transaction_id),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s Transaction commit failed.\n",
                               __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_commit_database_transaction()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_abort_database_transaction()
 ******************************************************************************
 * @brief
 * This function aborts a transaction with the database service.
 *
 * @param transaction_id - transaction id for database service
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_abort_database_transaction(fbe_database_transaction_id_t transaction_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 

    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_CODE_TRANSACTION_ABORT,
                                                     &transaction_id,
                                                     sizeof(transaction_id),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s Transaction abort failed.\n",
                               __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_abort_database_transaction()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_get_block_edge_info()
 ******************************************************************************
 * @brief
 * This function is used to get the block edge information for the specific
 * object and specific downstream edge index.
 *
 * @param object_id         - Object id for which we need edge information.
 * @param edge_index        - Edge index for which we need information.
 * @param block_edge_info_p - Pointer to the block edge information.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/29/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_get_block_edge_info(fbe_object_id_t object_id,
                                        fbe_edge_index_t edge_index,
                                        fbe_block_transport_control_get_edge_info_t * block_edge_info_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE; 

    /* we need to get the edge information */
    block_edge_info_p->base_edge_info.client_index = edge_index;

    /* send the control packet to get the block edge information. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
                                                     block_edge_info_p,
                                                     sizeof(fbe_block_transport_control_get_edge_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO failed\n", __FUNCTION__);
    }

    return status;
}

/******************************************************************************
 * end fbe_spare_lib_utils_get_block_edge_info()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_wait_for_edge_swap_in()
 ***************************************************************************** 
 * 
 * @brief   This method waits for the virtual drive (only on the SP where the
 *          job service is running) to enter the configuration mode expected.
 *
 * @param   vd_object_id  - Object id for virtual drive or spare drive
 * @param   swap_in_edge_index - Virtual drive edge we are waiting to swap-in
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/22/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_utils_wait_for_edge_swap_in(fbe_object_id_t vd_object_id,
                                                              fbe_edge_index_t swap_in_edge_index)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_virtual_drive_get_info_t    vd_get_info;
    fbe_u32_t                       wait_for_swap_in_secs = fbe_spare_main_get_operation_timeout_secs();
    fbe_semaphore_t                 wait_for_swap_in_sem;
    fbe_u32_t                       wait_secs = 1;
    fbe_u32_t                       total_secs_to_wait;
    fbe_u32_t                       total_secs_waited = 0;

    /* Only the following edge indexs are supported
     */
    switch (swap_in_edge_index)
    {
        case FIRST_EDGE_INDEX:
        case SECOND_EDGE_INDEX:
            /* Supported
             */
            break;

        default:
            /* Unsupported
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x edge index: %d not supported\n",
                                   __FUNCTION__, vd_object_id, swap_in_edge_index);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now wait at least (6) seconds for the virtual drive to complete the 
     * request
     */
    fbe_semaphore_init(&wait_for_swap_in_sem, 0, 1);
    if (wait_for_swap_in_secs < 6)
    {
        total_secs_to_wait = 6;
    }
    else
    {
        total_secs_to_wait = wait_for_swap_in_secs;
    }

    /* Get the virtual drive information and the edge info.
     * Since we just issued the request, wait a short period before
     * we check.
     */
    fbe_semaphore_wait_ms(&wait_for_swap_in_sem, 100);
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                     &vd_get_info,
                                                     sizeof (fbe_virtual_drive_get_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* We expect success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x edge index: %d get info failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, swap_in_edge_index,
                               status);
        fbe_semaphore_destroy(&wait_for_swap_in_sem);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Trace some information.
     */
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare: vd obj: 0x%x edge index: %d wait up to: %d seconds for edge swap\n",
                           vd_object_id, swap_in_edge_index, wait_for_swap_in_secs);

    /* Wait for the virtual drive to acknowledge the swap-in.
     */
    while ((vd_get_info.b_is_swap_in_complete == FBE_FALSE) &&
           (total_secs_waited < total_secs_to_wait)            )
    {
        /* Wait (1) cycle and then check lifecycle state.
         */
        fbe_semaphore_wait_ms(&wait_for_swap_in_sem, (wait_secs * 1000));
        total_secs_waited += wait_secs;

        /* Get the virtual drive information and the edge info
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                         &vd_get_info,
                                                         sizeof (fbe_virtual_drive_get_info_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         vd_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            /* We expect success
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x edge index: %d get info failed - status: 0x%x\n",
                                   __FUNCTION__, vd_object_id, swap_in_edge_index,
                                   status);

            /* Free the semaphore and return an error.
             */
            fbe_semaphore_destroy(&wait_for_swap_in_sem);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    } /* end while request is not complete */

    /* Check for success
     */
    if  (vd_get_info.b_is_swap_in_complete == FBE_FALSE)
    {
        /* Report the error
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x edge index: %d waited: %d seconds - failed\n",
                               __FUNCTION__, vd_object_id, swap_in_edge_index,
                               total_secs_waited);

        status = FBE_STATUS_TIMEOUT;
    }
    else
    {
        /* Report success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: swap in vd obj: 0x%x edge index: %d waited: %d seconds\n",
                               vd_object_id, swap_in_edge_index,
                               total_secs_waited);

        status = FBE_STATUS_OK;
    }

    /* Free the semaphore.
     */
    fbe_semaphore_destroy(&wait_for_swap_in_sem);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_wait_for_edge_swap_in()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_database_service_create_edge()
 ******************************************************************************
 * @brief
 * This function is used to create an edge in spare library.
 *
 * @param transaction_id                - Transaction id.
 * @param server_object_id              - Edge service id.
 * @param client_object_id              - Edge client id.
 * @param client_index                  - Edge index.
 * @param client_imported_capacity      - Edge capacity.
 * @param offset - edge's offset        - Offset.
 * @parma b_is_operation_confirmation_enabled - FBE_TRUE wait for completion
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_database_service_create_edge(fbe_database_transaction_id_t transaction_id, 
                                                      fbe_object_id_t server_object_id,
                                                      fbe_object_id_t client_object_id,
                                                      fbe_u32_t client_index,
                                                      fbe_lba_t client_imported_capacity,
                                                      fbe_lba_t offset,
                                                      fbe_bool_t b_is_operation_confirmation_enabled)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_create_edge_t create_edge;

    create_edge.transaction_id = transaction_id;
    create_edge.object_id = client_object_id;
    create_edge.client_index = client_index;
    create_edge.capacity = client_imported_capacity;
    create_edge.offset = offset;
    create_edge.server_id = server_object_id;
    create_edge.ignore_offset = FBE_EDGE_FLAG_IGNORE_OFFSET;
    fbe_zero_memory(create_edge.serial_num, sizeof(create_edge.serial_num));

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare_db_create_edge:create edge with spare drive,client-id:%d,server-id:%d,client-idx:%d\n",
                           client_object_id, server_object_id, client_index);

    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_CODE_CREATE_EDGE,
                                                     &create_edge,
                                                     sizeof(fbe_database_control_create_edge_t),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_db_create_edge DB_CTRL_CODE_CREATE_EDGE failed\n");
        return status;
    }

    /* Wait for the virtual drive to acknowledge the swap-in and the 
     * edge state to be good.
     */
    if (b_is_operation_confirmation_enabled)
    {
        status = fbe_spare_lib_utils_wait_for_edge_swap_in(client_object_id, client_index);
    }

    /* Return the status
     */
    return status;
}

/******************************************************************************
 * end fbe_spare_lib_utils_database_service_create_edge()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_wait_for_edge_swap_out()
 ***************************************************************************** 
 * 
 * @brief   This method waits for the virtual drive (only on the SP where the
 *          job service is running) to enter the configuration mode expected.
 *
 * @param   vd_object_id  - Object id for virtual drive or spare drive
 * @param   swap_out_edge_index - Virtual drive edge we are waiting to swap-in
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/22/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_utils_wait_for_edge_swap_out(fbe_object_id_t vd_object_id,
                                                               fbe_edge_index_t swap_out_edge_index)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_virtual_drive_get_info_t    vd_get_info;
    fbe_u32_t                       wait_for_swap_out_secs = fbe_spare_main_get_operation_timeout_secs();
    fbe_semaphore_t                 wait_for_swap_out_sem;
    fbe_u32_t                       wait_secs = 1;
    fbe_u32_t                       total_secs_to_wait;
    fbe_u32_t                       total_secs_waited = 0;

    /* Only the following edge indexs are supported
     */
    switch (swap_out_edge_index)
    {
        case FIRST_EDGE_INDEX:
        case SECOND_EDGE_INDEX:
            /* Supported
             */
            break;

        default:
            /* Unsupported
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x edge index: %d not supported\n",
                                   __FUNCTION__, vd_object_id, swap_out_edge_index);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now wait at least (6) seconds for the virtual drive to complete the 
     * request
     */
    fbe_semaphore_init(&wait_for_swap_out_sem, 0, 1);
    if (wait_for_swap_out_secs < 6)
    {
        total_secs_to_wait = 6;
    }
    else
    {
        total_secs_to_wait = wait_for_swap_out_secs;
    }

    /* Get the virtual drive information and the edge info
     * Since we just issued the request, wait a short period before
     * we check.
     */
    fbe_semaphore_wait_ms(&wait_for_swap_out_sem, 100);
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                     &vd_get_info,
                                                     sizeof (fbe_virtual_drive_get_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* We expect success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x edge index: %d get info failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, swap_out_edge_index,
                               status);
        fbe_semaphore_destroy(&wait_for_swap_out_sem);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Trace some information.
     */
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare: vd obj: 0x%x edge index: %d wait up to: %d seconds for edge swap\n",
                           vd_object_id, swap_out_edge_index, wait_for_swap_out_secs);

    /* Wait for the virtual drive to acknowledge the swap-out.
     */
    while ((vd_get_info.b_is_swap_out_complete == FBE_FALSE) &&
           (total_secs_waited < total_secs_to_wait)             )
    {
        /* Wait (1) cycle and then check lifecycle state.
         */
        fbe_semaphore_wait_ms(&wait_for_swap_out_sem, (wait_secs * 1000));
        total_secs_waited += wait_secs;

        /* Get the virtual drive information and the edge info
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                     &vd_get_info,
                                                     sizeof (fbe_virtual_drive_get_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            /* We expect success
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x edge index: %d get info failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, swap_out_edge_index,
                               status);

            /* Free the semaphore and return an error.
             */
            fbe_semaphore_destroy(&wait_for_swap_out_sem);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    } /* end while request is not complete */

    /* Check for success
     */
    if  (vd_get_info.b_is_swap_out_complete == FBE_FALSE)
    {
        /* Report the error
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x edge index: %d waited: %d seconds - failed\n",
                               __FUNCTION__, vd_object_id, swap_out_edge_index,
                               total_secs_waited);

        status = FBE_STATUS_TIMEOUT;
    }
    else
    {
        /* Report success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: swap out vd obj: 0x%x edge index: %d waited: %d seconds \n",
                               vd_object_id, swap_out_edge_index,
                               total_secs_waited);

        status = FBE_STATUS_OK;
    }

    /* Free the semaphore.
     */
    fbe_semaphore_destroy(&wait_for_swap_out_sem);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_wait_for_edge_swap_out()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_database_service_destroy_edge()
 ******************************************************************************
 * @brief
 * This function sends a control packet to the Config Service to destroy an edge.  
 *
 *
 * @param transaction ID    - Config Service transaction id (IN)
 * @param object_id         - upstream object edge connected to 
 * @param client_index      - index of edge to destroy (IN)
 * @param b_operation_confirmation - FBE_TRUE wait for confirmation from virtual
 *          drive
 *
 * @return status - status of the operation
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t 
fbe_spare_lib_utils_database_service_destroy_edge(fbe_database_transaction_id_t transaction_id,
                                                  fbe_object_id_t object_id, 
                                                  fbe_edge_index_t client_index,
                                                  fbe_bool_t b_operation_confirmation)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_destroy_edge_t destroy_edge;

    /* Set the ID of the edge to destroy */
    destroy_edge.transaction_id                             = transaction_id;
    destroy_edge.object_id                                  = object_id;
    destroy_edge.block_transport_destroy_edge.client_index  = client_index;

    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_CODE_DESTROY_EDGE,
                                                     &destroy_edge,
                                                     sizeof(fbe_database_control_destroy_edge_t),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare_db_destroy_edge:Destroy existing edge,client-id: %d,client-idx: %d.\n",
                           object_id, client_index);

    /* If failure status returned, log an error and return immediately */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO, "spare_db_destroy_edge FBE_DATABASE_CONTROL_CODE_DESTROY_EDGE failed.\n");
        return status;
    }

    /* Wait for the virtual drive to acknowledge the swap-out and the 
     * edge state to be invalid.
     */
    if (b_operation_confirmation)
    {
        status = fbe_spare_lib_utils_wait_for_edge_swap_out(object_id, client_index);
    }

    /* Return the status
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_database_service_destroy_edge()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_database_service_update_pvd()
 ******************************************************************************
 * @brief
 * This function is used to update the config of the provision drive
 * object.
 *
 * @param  transaction_id    - Transaction id.
 * @param  pvd_object_id     - pvd object id for which to update the config type.
 * @param  config            - update_pvd structure.
 *
 * @return status     - The status of the operation.
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 *  08/19/2011 - make it generic
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_database_service_update_pvd(fbe_database_control_update_pvd_t *update_provision_drive)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 

    /* Trace as much information as possible.*/
    switch(update_provision_drive->update_type)
    {
        case FBE_UPDATE_PVD_TYPE:
        case FBE_UPDATE_PVD_ENCRYPTION_IN_PROGRESS:
        case FBE_UPDATE_PVD_REKEY_IN_PROGRESS:
        case FBE_UPDATE_PVD_UNENCRYPTED:
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "spare: update pvd obj: 0x%x trans_id: 0x%llx update type: %d new config type: %d\n",
                                       update_provision_drive->object_id,
                                       (unsigned long long)update_provision_drive->transaction_id,
                                       update_provision_drive->update_type,
                                       update_provision_drive->config_type);
                break;
        case FBE_UPDATE_PVD_SNIFF_VERIFY_STATE:
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "spare: update pvd obj: 0x%x trans_id: 0x%llx update type: %d new sniff state: %d\n",
                                       update_provision_drive->object_id,
                                       (unsigned long long)update_provision_drive->transaction_id,
                                       update_provision_drive->update_type,
                                       update_provision_drive->sniff_verify_state);
                break; 
        case FBE_UPDATE_PVD_POOL_ID:
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "spare: update pvd obj: 0x%x trans_id: 0x%llx update type: %d new pool id: 0x%x\n",
                                       update_provision_drive->object_id,
                                       (unsigned long long)update_provision_drive->transaction_id,
                                       update_provision_drive->update_type,
                                       update_provision_drive->pool_id);
                break;
        case FBE_UPDATE_PVD_SERIAL_NUMBER:
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "spare: update pvd obj: 0x%x trans_id: 0x%llx update type: %d new sn: %s\n",
                                       update_provision_drive->object_id,
                                       (unsigned long long)update_provision_drive->transaction_id,
                                       update_provision_drive->update_type,
                                       update_provision_drive->serial_num);
                break;
        case FBE_UPDATE_PVD_CONFIG:
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "spare: update pvd obj: 0x%x trans_id: 0x%llx update type: %d new sn: %s\n",
                                       update_provision_drive->object_id,
                                       (unsigned long long)update_provision_drive->transaction_id,
                                       update_provision_drive->update_type,
                                       update_provision_drive->serial_num);
                break;
        case FBE_UPDATE_PVD_NONPAGED_BY_DEFALT_VALUE:
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "spare: update pvd obj: 0x%x trans_id: 0x%llx update type: %d set non-paged\n",
                                   update_provision_drive->object_id,
                                   (unsigned long long)update_provision_drive->transaction_id,
                                   update_provision_drive->update_type);
                break;
        default:
            /* Trace an error but let the database report the failure */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "spare: update pvd obj: 0x%x trans_id: 0x%llx update type: %d unsupported\n",
                                   update_provision_drive->object_id,
                                   (unsigned long long)update_provision_drive->transaction_id,
                                   update_provision_drive->update_type);
            break;
    }

    /* Send the request to the datbase service*/
    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
                                                     update_provision_drive,
                                                     sizeof(fbe_database_control_update_pvd_t),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_db_update_pvd_cfg DB_CTRL_CODE_UPDATE_PVD failed\n");
    }
    return status;
}

/******************************************************************************
 * end fbe_spare_lib_utils_update_provision_drive_config_type()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_wait_for_configuration_mode()
 ***************************************************************************** 
 * 
 * @brief   This method waits for the virtual drive (only on the SP where the
 *          job service is running) to enter the configuration mode expected.
 *
 * @param   vd_object_id  - Object id for virtual drive or spare drive
 * @param   expected_configuration_mode - Configuration mode to wait for
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/22/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_utils_wait_for_configuration_mode(fbe_object_id_t vd_object_id,
                                                                    fbe_virtual_drive_configuration_mode_t expected_configuration_mode)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_virtual_drive_get_info_t    vd_get_info;
    fbe_u32_t                       wait_for_config_secs = fbe_spare_main_get_operation_timeout_secs();
    fbe_semaphore_t                 wait_for_config_mode_sem;
    fbe_u32_t                       wait_secs = 1;
    fbe_u32_t                       total_secs_to_wait;
    fbe_u32_t                       total_secs_waited = 0;

    /* Only the following configuration modes are supported.
     */
    switch (expected_configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Supported
             */
            break;

        default:
            /* Unsupported
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x mode: %d not supported\n",
                                   __FUNCTION__, vd_object_id, expected_configuration_mode);
            
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now wait at least (6) seconds for the virtual drive to exit hibernation
     */
    fbe_semaphore_init(&wait_for_config_mode_sem, 0, 1);
    if (wait_for_config_secs < 6)
    {
        total_secs_to_wait = 6;
    }
    else
    {
        total_secs_to_wait = wait_for_config_secs;
    }

    /* Send the first command.
     * Since we just issued the request, wait a short period before
     * we check.
     */
    fbe_semaphore_wait_ms(&wait_for_config_mode_sem, 100);
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                     &vd_get_info,
                                                     sizeof (fbe_virtual_drive_get_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* We expect success and there should be a copy request in progress
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x mode: %d - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, expected_configuration_mode,
                               status);
        fbe_semaphore_destroy(&wait_for_config_mode_sem);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Wait for the virtual drive to acknowledge the configuration mode change.
     */
    while ((vd_get_info.b_is_change_mode_complete == FBE_FALSE) &&
           (total_secs_waited < total_secs_to_wait)                )
    {
        /* Wait (1) cycle and then check lifecycle state.
         */
        fbe_semaphore_wait_ms(&wait_for_config_mode_sem, (wait_secs * 1000));
        total_secs_waited += wait_secs;

        /* Check if it is still in progress
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                         &vd_get_info,
                                                         sizeof (fbe_virtual_drive_get_info_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         vd_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            /* We expect success and there should be a copy request in progress
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x mode: %d failed - status: 0x%x\n",
                                   __FUNCTION__, vd_object_id, expected_configuration_mode,
                                   status);

            /* Free the semaphore and return an error.
             */
            fbe_semaphore_destroy(&wait_for_config_mode_sem);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    } /* end while request is not complete */

    /* Check for success
     */
    if  ((vd_get_info.b_is_change_mode_complete == FBE_FALSE)            ||
         (vd_get_info.configuration_mode != expected_configuration_mode)    )
    {
        /* Report the error
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x mode: %d waited: %d seconds - failed\n",
                               __FUNCTION__, vd_object_id, expected_configuration_mode,
                               total_secs_waited);

        status = FBE_STATUS_TIMEOUT;
    }
    else
    {
        /* Report success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: change config vd obj: 0x%x mode: %d waited: %d seconds \n",
                               vd_object_id, expected_configuration_mode,
                               total_secs_waited);

        status = FBE_STATUS_OK;
    }

    /* Free the semaphore.
     */
    fbe_semaphore_destroy(&wait_for_config_mode_sem);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_wait_for_configuration_mode()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_spare_lib_utils_update_virtual_drive_configuration_mode()
 ******************************************************************************
 * @brief
 * This function is used to update the configuration mode of the virtual drive
 * on both SPs.
 *
 * @param   update_virtual_drive_p - Pointer to upddate virtual drive request
 * @param   b_operation_confirmation - FBE_TRUE - wait for virtual drive
 *
 * @return status     - The status of the operation.
 *
 * @author
 *  11/01/2012  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_update_virtual_drive_configuration_mode(fbe_database_control_update_vd_t *update_virtual_drive_p,
                                                                         fbe_bool_t b_operation_confirmation)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE; 

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare: update vd obj: 0x%x mode to: %d transaction id: 0x%llx\n",
                           update_virtual_drive_p->object_id,
                           update_virtual_drive_p->configuration_mode,
                           update_virtual_drive_p->transaction_id);

    /* Issue database request to update the virtual drive configuration mode on
     * both SPs.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_VD,
                                                     update_virtual_drive_p,
                                                     sizeof(*update_virtual_drive_p),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: update vd obj: 0x%x mode to: %d transaction id: 0x%llx failed\n",
                               update_virtual_drive_p->object_id,
                               update_virtual_drive_p->configuration_mode,
                               update_virtual_drive_p->transaction_id);
        return status;
    }

    /* Now `wait' for the completion
     */
    if (b_operation_confirmation)
    {
        status = fbe_spare_lib_utils_wait_for_configuration_mode(update_virtual_drive_p->object_id,
                                                                 update_virtual_drive_p->configuration_mode);
    }

    /* Return the result of waiting for the completion.
     */
    return status;
}

/******************************************************************************
 * end fbe_spare_lib_utils_update_virtual_drive_configuration_mode()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_wait_for_swap_request_complete()
 ***************************************************************************** 
 * 
 * @brief   This method waits for the virtual drive to acknowledge that the
 *          swap request is complete.
 *
 * @param   vd_object_id  - Object id for virtual drive or spare drive
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  10/09/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_utils_wait_for_swap_request_complete(fbe_object_id_t vd_object_id)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_virtual_drive_get_info_t    vd_get_info;
    fbe_u32_t                       wait_for_complete_secs = fbe_spare_main_get_operation_timeout_secs();
    fbe_semaphore_t                 wait_for_complete_sem;
    fbe_u32_t                       wait_secs = 1;
    fbe_u32_t                       total_secs_to_wait;
    fbe_u32_t                       total_secs_waited = 0;

    /* Now wait at least (6) seconds for the virtual drive to exit hibernation
     */
    fbe_semaphore_init(&wait_for_complete_sem, 0, 1);
    if (wait_for_complete_secs < 6)
    {
        total_secs_to_wait = 6;
    }
    else
    {
        total_secs_to_wait = wait_for_complete_secs;
    }

    /* Send the first command
     * Since we just issued the request, wait a short period before
     * we check.
     */
    fbe_semaphore_wait_ms(&wait_for_complete_sem, 100);
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                     &vd_get_info,
                                                     sizeof (fbe_virtual_drive_get_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* We expect success and there should be a copy request in progress
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
        fbe_semaphore_destroy(&wait_for_complete_sem);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Wait for the virtual drive to acknowledge that it has completed the swap
     * request.
     */
    while ((vd_get_info.b_request_in_progress == FBE_TRUE) &&
           (total_secs_waited < total_secs_to_wait)           )
    {
        /* Wait (1) cycle and then check lifecycle state.
         */
        fbe_semaphore_wait_ms(&wait_for_complete_sem, (wait_secs * 1000));
        total_secs_waited += wait_secs;

        /* Check if it is still in progress
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                         &vd_get_info,
                                                         sizeof (fbe_virtual_drive_get_info_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         vd_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            /* We expect success and there should be a copy request in progress
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x - status: 0x%x\n",
                                   __FUNCTION__, vd_object_id, status);

            /* Free the semaphore and return an error.
             */
            fbe_semaphore_destroy(&wait_for_complete_sem);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    } /* end while request is not complete */

    /* Check for success
     */
    if  (vd_get_info.b_request_in_progress == FBE_TRUE)
    {
        /* We have wait the allotted time and the virtual drive still has not
         * completed the swap request.  Trace a warning message and return a
         * failure status.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x progress: %d waited: %d seconds - failed\n",
                               __FUNCTION__, vd_object_id,
                               vd_get_info.b_request_in_progress, 
                               total_secs_waited);

        status = FBE_STATUS_TIMEOUT;
    }
    else
    {
        /* Report success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: swap complete vd obj: 0x%x waited: %d seconds \n",
                               vd_object_id, total_secs_waited);

        status = FBE_STATUS_OK;
    }

    /* Free the semaphore.
     */
    fbe_semaphore_destroy(&wait_for_complete_sem);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_wait_for_swap_request_complete()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_send_swap_completion_status()
 *****************************************************************************
 *
 * @brief   This method sends the `swap request complete' (a.k.a send status)
 *          usurper to the virtual drive.  This usurper requests that the
 *          virtual drive perform any cleanup required for the swap request.
 *          If confirmation is required we wait for the virtual drive to
 *          complete the cleanup before we continue.

 * @param   job_service_drive_swap_request_p - Pointer to swap request
 * @param   b_operation_confirmation - FBE_TRUE - wait for virtual drive
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  10/09/2013  Ron Proulx  - Updated.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_utils_send_swap_completion_status(fbe_job_service_drive_swap_request_t *job_service_drive_swap_request_p,
                                                             fbe_bool_t b_operation_confirmation)

{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_error_type_t            status_code;
    fbe_spare_swap_command_t                command_type = FBE_SPARE_SWAP_INVALID_COMMAND;
    fbe_base_config_upstream_object_list_t  upstream_object_list;
    fbe_raid_group_set_extended_media_error_handling_t parent_set_emeh_request;
    fbe_bool_t                              b_is_proactive_copy = FBE_TRUE;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Get the virtual drive object id from swap request to send the error response. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(job_service_drive_swap_request_p, &vd_object_id);
    fbe_job_service_drive_swap_request_get_swap_command_type(job_service_drive_swap_request_p, &command_type);
    fbe_job_service_drive_swap_request_get_status(job_service_drive_swap_request_p, &status_code);
    fbe_job_service_drive_swap_request_get_is_proactive_copy(job_service_drive_swap_request_p, &b_is_proactive_copy);

    /* Trace the completion */
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s vd obj: 0x%x cmd: %d job status: %d \n", 
                           __FUNCTION__, vd_object_id, command_type, status_code);

    /* If this is a completed or aborted proactive copy request then restore
     * the media error thresholds.
     */
    if (((command_type == FBE_SPARE_COMPLETE_COPY_COMMAND) ||
         (command_type == FBE_SPARE_ABORT_COPY_COMMAND)       ) &&
        (b_is_proactive_copy == FBE_TRUE)                          )
    {
        /* Get the upstream object id.
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                         &upstream_object_list,
                                                         sizeof(fbe_base_config_upstream_object_list_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         vd_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if ((status != FBE_STATUS_OK)                              ||
            (upstream_object_list.number_of_upstream_objects != 1)    )
        {
            /* Trace and error and continue (so that we complete the request)
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: get upstream for emeh failed. vd obj:0x%x status: %d\n",
                                   __FUNCTION__, vd_object_id, status);
        }
        else
        {   
            /* Else restore the thresholds.
             * Now send usurper to have parent raid group enter EMEH `high availability mode'.
             */
            parent_set_emeh_request.set_control = FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE;
            parent_set_emeh_request.b_is_paco = FBE_TRUE;
            parent_set_emeh_request.paco_vd_object_id = vd_object_id;
            parent_set_emeh_request.b_change_threshold = FBE_FALSE;
            parent_set_emeh_request.percent_threshold_increase = 0;
            status = fbe_spare_lib_utils_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_SET_EXTENDED_MEDIA_ERROR_HANDLING,
                                                         &parent_set_emeh_request,
                                                         sizeof(fbe_raid_group_set_extended_media_error_handling_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         upstream_object_list.upstream_object_list[0],
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
            if (status != FBE_STATUS_OK)
            {
                /* This is a warning and we continue (it is not neccessary to 
                 * change the thresholds).
                 */
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: set emeh for rg obj:0x%x vd obj: 0x%x status: %d\n",
                                   __FUNCTION__, upstream_object_list.upstream_object_list[0], 
                                   vd_object_id, status);
            }
        }
    }

    /* Send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_REQUEST_COMPLETE,
                                                     job_service_drive_swap_request_p,
                                                     sizeof(fbe_job_service_drive_swap_request_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: Send Swap Error Failed.\n", __FUNCTION__);
        return status;
    }

    /* Now `wait' for the completion
     */
    if (b_operation_confirmation)
    {
        status = fbe_spare_lib_utils_wait_for_swap_request_complete(vd_object_id);
    }

    /* Return the result of waiting for the completion.
     */
    return status;

}
/******************************************************************************
 * end fbe_spare_lib_utils_send_swap_completion_status()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_get_configuration_mode()
 ******************************************************************************
 * @brief
 * This function is used to get the config mode.
 *
 *
 * @param vd_object_id          - virtual drive object id.
 * @param config_mode_p  - returns current config mode.
 *
 * @return status - status of the operation
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_get_configuration_mode(fbe_object_id_t vd_object_id,
                                           fbe_virtual_drive_configuration_mode_t * configuration_mode_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_virtual_drive_configuration_t       get_configuration;

    /* initialize configuration mode as unknown. */
    *configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    get_configuration.configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;

    /* get the configuration of the virtual drive object. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION,
                                                     &get_configuration,
                                                     sizeof(fbe_virtual_drive_configuration_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    /* If failure status returned, log an error and return immediately */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION failed.\n", __FUNCTION__);
    }
    else
    {
        *configuration_mode_p = get_configuration.configuration_mode;
    }

    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_get_configuration_mode()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_get_permanent_spare_swap_edge_index()
 ******************************************************************************
 * @brief
 * This function is used to get the edge index for the swap-in permanent spare.
 *
 * @param vd_object_id                          - vd object id.
 * @param permanent_spare_swap_edge_index_p     - permanent spare edge index.
 *
 * @return status - status of the operation
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_get_permanent_spare_swap_edge_index(fbe_object_id_t vd_object_id,
                                                        fbe_edge_index_t * permanent_spare_swap_edge_index_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    
    /* initialize proactive spare swap edge index as invalid. */
    *permanent_spare_swap_edge_index_p = FBE_EDGE_INDEX_INVALID;

    /* get the configuration mode of the virtual drive object. */
    fbe_spare_lib_utils_get_configuration_mode(vd_object_id, &configuration_mode);

    /* based on configuration mode return the specific edge index for the permanent spare swap-in. */
    switch(configuration_mode)
    {
    case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            *permanent_spare_swap_edge_index_p = FIRST_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            *permanent_spare_swap_edge_index_p = SECOND_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_get_permanent_spare_swap_edge_index()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_get_copy_swap_edge_index()
 ******************************************************************************
 * @brief
 * This function is used to get the edge index for the swap-in spare.
 *
 *
 * @param object_id         - upstream object edge connected to 
 * @param client_index      - index of edge to destroy (IN)
 *
 * @return status - status of the operation
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_get_copy_swap_edge_index(fbe_object_id_t vd_object_id,
                                                          fbe_edge_index_t *swap_in_edge_index_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    
    /* initialize proactive spare swap edge index as invalid. */
    *swap_in_edge_index_p = FBE_EDGE_INDEX_INVALID;

    /* get the configuration mode of the virtual drive object. */
    fbe_spare_lib_utils_get_configuration_mode(vd_object_id, &configuration_mode);

    /* based on configuration mode return the specific edge index for the proactive spare swap-in. */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            *swap_in_edge_index_p = SECOND_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            *swap_in_edge_index_p = FIRST_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}
/****************************************************
 * end fbe_spare_lib_utils_get_copy_swap_edge_index()
 ****************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_initiate_copy_get_updated_virtual_drive_configuration_mode()
 ******************************************************************************
 * @brief
 * This function is used to get the updated configuration mode for a copy
 * operation.
 *
 *
 * @param   vd_object_id - virtual drive object id to determine updated mode for
 * @param   updated_configuration_mode_p - Pointer to updated configuration mode
 *
 * @return status - status of the operation
 *
 * @author
 *  11/01/2012  Ron Proulx  - Created
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_initiate_copy_get_updated_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id,
                                                                   fbe_virtual_drive_configuration_mode_t *updated_configuration_mode_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    /* Initiatize the configuration mode to invalid.
     */
    *updated_configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;

    /* get the configuration mode of the virtual drive object. 
     */
    fbe_spare_lib_utils_get_configuration_mode(vd_object_id, &configuration_mode);

    /* Based on configuration mode return the updated configuration mode.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            *updated_configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            *updated_configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}
/**************************************************************************************
 * end fbe_spare_lib_utils_initiate_copy_get_updated_virtual_drive_configuration_mode()
 **************************************************************************************

/*!****************************************************************************
 * fbe_spare_lib_utils_get_server_object_id_for_edge()
 ******************************************************************************
 * @brief
 *  This function is used to get the server object id of the edge.
 *
 *
 * @param object_id         - upstream object edge connected to 
 * @param client_index      - index of edge to destroy (IN)
 * @param transaction ID    - Config Service transaction id (IN)
 *
 * @return status - status of the operation
 *
 * @author
 *  06/03/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_get_server_object_id_for_edge(fbe_object_id_t object_id,
                                                  fbe_edge_index_t edge_index,
                                                  fbe_object_id_t * object_id_p)
{
    fbe_block_transport_control_get_edge_info_t edge_info;
    fbe_status_t                                status;

    /* set the edge index on get edge information structure. */
    edge_info.base_edge_info.client_index = edge_index;
    *object_id_p = FBE_OBJECT_ID_INVALID;

    /* get the block edge information for the edge specified by caller. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
                                                     &edge_info,
                                                     sizeof(fbe_block_transport_control_get_edge_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    /* If failure status returned, log an error and return immediately */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO failed.\n", __FUNCTION__);
    }
    else
    {
        /* return the server object-id if status is good. */
        *object_id_p = edge_info.base_edge_info.server_id;
    }

    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_get_server_object_id_for_edge()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_get_provision_drive_info()
 ******************************************************************************
 * @brief
 * This function sends the get drive info control packet to provision drive
 * object and in return it gets exported drive information from provision
 * drive object.
 *
 * @param packet_p                  - The packet requesting this operation.
 * @param in_pvd_object_id          - pvd object id.
 * @param in_provision_drive_info_p - provision drive information.
 *
 * @return status                   - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_get_provision_drive_info(fbe_object_id_t in_pvd_object_id,
                                             fbe_provision_drive_info_t * in_provision_drive_info_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Return with error if input parameters are NULL.
     */
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (in_provision_drive_info_p);

    /*! @note A value of `invalid' is allowed.
     */
    if (in_pvd_object_id == FBE_OBJECT_ID_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note A value of 0 is not allowed.
     */
    if (in_pvd_object_id == 0)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: invalid pvd obj: 0x%x \n", 
                               __FUNCTION__, in_pvd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize provision drive information with default values. */
    in_provision_drive_info_p->capacity = FBE_LBA_INVALID;
    in_provision_drive_info_p->config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;

    /* Send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                     in_provision_drive_info_p,
                                                     sizeof(fbe_provision_drive_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     in_pvd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: pvd obj: 0x%x failed. status: 0x%x \n", 
                               __FUNCTION__, in_pvd_object_id, status);
    }

    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_get_provision_drive_info()
 ******************************************************************************/

/*!*********************************************************************************
 * fbe_spare_lib_utils_complete_copy_get_updated_virtual_drive_configuration_mode()
 ***********************************************************************************
 * @brief
 * This function is used to get the updated configuration mode for a completing a copy
 * operation.
 *
 *
 * @param   vd_object_id - virtual drive object id to determine updated mode for
 * @param   updated_configuration_mode_p - Pointer to updated configuration mode
 *
 * @return status - status of the operation
 *
 * @author
 *  11/01/2012  Ron Proulx  - Created
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_complete_copy_get_updated_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id,
                                                                   fbe_virtual_drive_configuration_mode_t *updated_configuration_mode_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    /* Initiatize the configuration mode to invalid.
     */
    *updated_configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;

    /* get the configuration mode of the virtual drive object. 
     */
    fbe_spare_lib_utils_get_configuration_mode(vd_object_id, &configuration_mode);

    /* Based on configuration mode return the updated configuration mode.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            *updated_configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            *updated_configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE;
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}
/**************************************************************************************
 * end fbe_spare_lib_utils_complete_copy_get_updated_virtual_drive_configuration_mode()
 **************************************************************************************
 
/*!*********************************************************************************
 * fbe_spare_lib_utils_complete_aborted_copy_get_updated_virtual_drive_configuration_mode()
 ***********************************************************************************
 * @brief
 * This function is used to get the updated configuration mode for an aborted copy
 * operation.
 *
 * @param   vd_object_id - virtual drive object id to determine updated mode for
 * @param   swap_out_edge_index - The edge index that failed (could be the source or
 *              destination index).
 * @param   updated_configuration_mode_p - Pointer to updated configuration mode
 *
 * @return status - status of the operation
 *
 * @author
 *  11/07/2012  Ron Proulx  - Created
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_complete_aborted_copy_get_updated_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id,
                                                                   fbe_edge_index_t swap_out_edge_index,
                                                                   fbe_virtual_drive_configuration_mode_t *updated_configuration_mode_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    /* Initiatize the configuration mode to invalid.
     */
    *updated_configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;

    /* Validate the swap-out edge index.
     */
    if ((swap_out_edge_index != FIRST_EDGE_INDEX)  &&
        (swap_out_edge_index != SECOND_EDGE_INDEX)    )
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the configuration mode of the virtual drive object. 
     */
    fbe_spare_lib_utils_get_configuration_mode(vd_object_id, &configuration_mode);

    /* Based on configuration mode return the updated configuration mode.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Use failed edge index to determine the updated configuration mode.
             */
            if (swap_out_edge_index == FIRST_EDGE_INDEX)
            {
                *updated_configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE;
            }
            else
            {
                *updated_configuration_mode_p = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE;
            }
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}
/**********************************************************************************************
 * end fbe_spare_lib_utils_complete_aborted_copy_get_updated_virtual_drive_configuration_mode()
 **********************************************************************************************
 
/*!****************************************************************************
 * fbe_spare_lib_utils_get_control_buffer()
 ******************************************************************************
 * @brief
 * Helper function to get the control operation buffer.
 *
 * @param packet_p                  - packet requesting this operation.
 * @param control_operation_p       - return control operation of the packet.
 * @param payload_data_p            - return payload data to caller.
 * @param payload_data_len          - length of the payload.
 * @param function                  - caller's function pointer.
 *
 * @return status                   - The status of the operation.
 *
 * @author
 *  09/30/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_get_control_buffer(fbe_packet_t * packet_p,
                                                    fbe_payload_control_operation_t ** control_operation_p,
                                                    void ** payload_data_p,
                                                    fbe_u32_t payload_data_len,
                                                    const char * function)
{
    fbe_payload_ex_t * payload_p = NULL;
    fbe_u32_t       buffer_len;
    fbe_job_queue_element_t * job_queue_element_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    *control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(*control_operation_p, &job_queue_element_p);

    /* Get the payload - if we specified a buffer */
    if(payload_data_p != NULL)
    {
        fbe_payload_control_get_buffer(*control_operation_p, payload_data_p);
        if(*payload_data_p == NULL)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s fbe_payload_control_get_buffer failed\n", function);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_payload_control_set_status(*control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_payload_control_get_buffer_length(*control_operation_p, &buffer_len);
        if(buffer_len != payload_data_len)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s invalid len %d != %d\n", function, buffer_len, payload_data_len);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_payload_control_set_status(*control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s input buffer is null\n", function);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(*control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_utils_get_control_buffer()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_send_notification()
 ******************************************************************************
 * @brief
 * Helper function to send the notification for the spare library.
 * 
 * @param js_swap_request_p - Job service swap request.
 * @param job_number        - The job number from original request
 * @param job_status        - The job status (did the job succeed or fail)
 * @param job_action_state  - Job action state.
 * 
 * @return status                   - The status of the operation.
 *
 * @author
 *  09/30/2010 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_send_notification(fbe_job_service_drive_swap_request_t *js_swap_request_p,
                                                   fbe_u64_t job_number,
                                                   fbe_status_t job_status,
                                                   fbe_job_action_state_t job_action_state)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_notification_info_t         notification_info;
    fbe_job_service_error_type_t    error_code;

    /* Get the swap status code.*/
    fbe_job_service_drive_swap_request_get_status(js_swap_request_p, &error_code);

    /* If the error code indicates that the job failed, we expect the status to
     * be failure.  Trace an error and change the job status to failure.
     */
    if ((error_code != FBE_JOB_SERVICE_ERROR_NO_ERROR) &&
        (job_status == FBE_STATUS_OK)                     )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s job status: %d not expected with error code: %d\n",
                               __FUNCTION__, job_status, error_code);
        job_status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* fill up the notification information before sending notification. */
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.source_package = FBE_PACKAGE_ID_SEP_0;
    notification_info.class_id = FBE_CLASS_ID_VIRTUAL_DRIVE;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE;
    notification_info.notification_data.job_service_error_info.object_id = js_swap_request_p->vd_object_id;
    notification_info.notification_data.job_service_error_info.status = job_status;
    notification_info.notification_data.job_service_error_info.error_code = error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_number;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_DRIVE_SWAP;

    /* Always populate the original and spare even if they are invalid.
     */
    notification_info.notification_data.job_service_error_info.swap_info.orig_pvd_object_id = js_swap_request_p->orig_pvd_object_id;
    notification_info.notification_data.job_service_error_info.swap_info.spare_pvd_object_id = js_swap_request_p->spare_object_id;
    notification_info.notification_data.job_service_error_info.swap_info.command_type = js_swap_request_p->command_type;
    notification_info.notification_data.job_service_error_info.swap_info.vd_object_id = js_swap_request_p->vd_object_id;

    /*! @note For internally generated request we do not want to log repeating
     *        errors.  It is up to the virtual drive to generate the notification.
     */
    switch(js_swap_request_p->command_type)
    {
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            /* Do not log repeating errors.  It is assumed that all other
             * errors are terminal (i.e. the request is complete).
             */
            switch(js_swap_request_p->status_code)
            {
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_HAS_COPY_IN_PROGRESS:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_SOURCE_DRIVE_DEGRADED:
                    /* These error repeat.  Do not report them.
                     */
                    return status;
                default:
                    /* For all other error codes fall-thru.
                     */
                    break;
            }
            // Fall-thru
        default:
            /* We have validated the error code above.  Generate the notification.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "SPARE: send notification cmd: %d status: %d vd obj: 0x%x orig: 0x%x spare: 0x%x\n",
                                   js_swap_request_p->command_type, error_code, js_swap_request_p->vd_object_id,
                                   js_swap_request_p->orig_pvd_object_id, js_swap_request_p->spare_object_id);

            status = fbe_notification_send(js_swap_request_p->vd_object_id, notification_info);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_send_notification()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_spare_lib_utils_internal_error_to_job_error()
 ******************************************************************************
 *
 * @brief   Convert from an `internal' error code to a job error code (for
 *          logging the internal error)
 * 
 * @param   internal_error_code - The internal error code for the failure.
 * @param   drive_swap_request_p  Job service swap request.
 * @param   job_error_code_p - Pointer to resultant job error code.
 * 
 * @return  status - The status of the operation.
 *
 * @author
 *  08/08/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_utils_internal_error_to_job_error(fbe_spare_internal_error_t internal_error_code,
                                                                    fbe_job_service_drive_swap_request_t *drive_swap_request_p,
                                                                    fbe_job_service_error_type_t *job_error_code_p)
{
    /* Initialize to `invalid'
     */
    *job_error_code_p = FBE_JOB_SERVICE_ERROR_INVALID;

    /* Simply generate the appropriate job error code.
     */
    switch(internal_error_code)
    {
        case FBE_SPARE_INTERNAL_ERROR_INVALID:
            /*!< Swap status was never populated. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_SWAP_STATUS_NOT_POPULATED;
            break;
        case FBE_SPARE_INTERNAL_ERROR_NONE:
            /*!< The swap request completed successfully. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_NO_ERROR;
            break;
        case FBE_SPARE_INTERNAL_ERROR_NO_SPARE_AVAILABLE:
            /*!< There are no spare drives in the system. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE;
            break;
        case FBE_SPARE_INTERNAL_ERROR_NO_SUITABLE_SPARE_AVAILABLE:
            /*!< There are no suitable spares. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE;
            break;
        case FBE_SPARE_INTERNAL_ERROR_UNEXPECTED_JOB_STATUS:
            /*!< The job status supplied was not expected. */
            *job_error_code_p = drive_swap_request_p->status_code;
            break;
        case FBE_SPARE_INTERNAL_ERROR_SWAP_IN_VALIDATION_FAIL:
            /*!< Error during swap-in validation. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL;
            break;
        case FBE_SPARE_INTERNAL_ERROR_INVALID_SWAP_COMMAND_TYPE:
            /*!< An invalid/unsupported swap command was used. */
             *job_error_code_p = FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND;
             break;
        case FBE_SPARE_INTERNAL_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID:
            /*!< The virtual drive object id is invalid. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID;
            break;
        case FBE_SPARE_INTERNAL_ERROR_INVALID_EDGE_INDEX:
            /*!< The edge index supplied is not valid. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_INVALID_EDGE_INDEX;
            break;
        case FBE_SPARE_INTERNAL_ERROR_INVALID_VIRTUAL_DRIVE_CONFIG_MODE:
            /*!< The virtual drive configuration mode is invalid. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT;
            break;
        case FBE_SPARE_INTERNAL_ERROR_INVALID_PROVISION_DRIVE_CONFIG_MODE:
            /*!< The provision drive configuration mode is invalid. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_PVD_INVALID_CONFIG_TYPE;
            break;
        case FBE_SPARE_INTERNAL_ERROR_INVALID_SPARE_OBJECT_ID:
            /*!< Invalid object id of spare. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID;
            break;
        case FBE_SPARE_INTERNAL_ERROR_CREATE_EDGE_FAILED:
            /*!< Create edge failed. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_SWAP_CREATE_EDGE_FAILED;
            break;
        case FBE_SPARE_INTERNAL_ERROR_DESTROY_EDGE_FAILED:
            /*!< Destroy edge failed. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_SWAP_DESTROY_EDGE_FAILED;
            break;
        case FBE_SPARE_INTERNAL_ERROR_UNSUPPORTED_EVENT_CODE:
            /*!< The event code supplied is not supported. */
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_UNSUPPORTED_EVENT_CODE;
            break;
        case FBE_SPARE_INTERNAL_ERROR_RAID_GROUP_DENIED:
            /*!< The parent raid group denied the spare request.*/
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED;
            break;

        default:
            /* Log an error trace and simply report internal error.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "spare convert internal: %d to job error vd obj: 0x%x unknown error code.\n",
                                   internal_error_code, drive_swap_request_p->vd_object_id);
            *job_error_code_p = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            break;
    }

    /* Always succeeds
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_utils_internal_error_to_job_error()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_spare_lib_utils_log_internal_error()
 ******************************************************************************
 *
 * @brief   This method is invoked to log an internal `inconsistency' or 
 *          `unexpected' error.
 * 
 * @param   internal_error_code - The internal error code for the failure.
 * @param   drive_swap_request_p  Job service swap request.
 * 
 * @return  status - The status of the operation.
 *
 * @author
 *  04/25/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_utils_log_internal_error(fbe_spare_internal_error_t internal_error_code,
                                                           fbe_job_service_drive_swap_request_t *drive_swap_request_p)
{
    fbe_provision_drive_info_t      original_pvd_info = {0};
    fbe_job_service_error_type_t    job_error_code = FBE_JOB_SERVICE_ERROR_INVALID;

    /* Get the associated job error code*/
    fbe_spare_lib_utils_internal_error_to_job_error(internal_error_code, drive_swap_request_p,
                                                    &job_error_code);
    fbe_job_service_drive_swap_request_set_internal_error(drive_swap_request_p, internal_error_code);
    fbe_job_service_drive_swap_request_set_status(drive_swap_request_p, job_error_code);

    /*get povision drive info for desired spare*/
    fbe_spare_lib_utils_get_provision_drive_info(drive_swap_request_p->orig_pvd_object_id, &original_pvd_info);

    /* Simply log the event.  The first parameter is the internal error
     * code.
     */
    fbe_event_log_write(SEP_ERROR_CODE_SPARE_UNEXPECTED_ERROR, NULL, 0, 
                        "%d %d %d %d", job_error_code,
                        original_pvd_info.port_number,
                        original_pvd_info.enclosure_number,
                        original_pvd_info.slot_number);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_utils_log_internal_error()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_write_event_log()
 ******************************************************************************
 * @brief
 * Helper function to log the the event for the spare library.
 * 
 * @param event_code            Desired spare drive info.
 * @param drive_swap_request_p  Job service swap request.
 * 
 * 
 * @return status                     - The status of the operation.
 *
 * @author
 *  11/24/2010 - Created.Vishnu Sharma
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_utils_write_event_log(fbe_u32_t event_code,
                                                        fbe_job_service_drive_swap_request_t *drive_swap_request_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_object_id_t             orig_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             spare_object_id = FBE_OBJECT_ID_INVALID;
    fbe_provision_drive_info_t  original_pvd_info = {0};
    fbe_provision_drive_info_t  spare_pvd_info = {0};
    fbe_job_service_error_type_t job_error_code;
    fbe_spare_internal_error_t  internal_error_status;
    fbe_spare_swap_command_t    command_type;

    /* get the status of the job */
    job_error_code = drive_swap_request_p->status_code;
    internal_error_status = drive_swap_request_p->internal_error;
    fbe_job_service_drive_swap_request_get_swap_command_type(drive_swap_request_p, &command_type);

    /* get povision drive info for original drive*/
    fbe_job_service_drive_swap_request_get_original_pvd_object_id(drive_swap_request_p, &orig_pvd_object_id);
    fbe_spare_lib_utils_get_provision_drive_info(orig_pvd_object_id, &original_pvd_info);

    /*get povision drive info for selected spare*/
    fbe_job_service_drive_swap_request_get_spare_object_id(drive_swap_request_p, &spare_object_id);

    fbe_spare_lib_utils_get_provision_drive_info(spare_object_id, &spare_pvd_info);

    if(spare_pvd_info.port_number == FBE_PORT_NUMBER_INVALID)
    {
        /* The spare object might NOT be in a ready state in the case of encrypted PVD. So wait for the object to go 
         * ready before getting the physical slot information */
        status = fbe_job_service_wait_for_expected_lifecycle_state(spare_object_id, 
                                                                   FBE_LIFECYCLE_STATE_READY, 
                                                                   120000);
        fbe_spare_lib_utils_get_provision_drive_info(spare_object_id, &spare_pvd_info);

    }
    /* Validate that the swap status success.
     */
    switch(event_code)
    {
        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAP_IN_OPERATION_INITIATED:
        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_SWAP_IN_OPERATION_INITIATED:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_USER_COPY_SPARE_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_DRIVE_SWAP_OUT:
        case SEP_INFO_SPARE_USER_COPY_INITIATED:
        case SEP_INFO_SPARE_USER_COPY_DESTINATION_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_COPY_OPERATION_COMPLETED:
            /* The status should be success.
             */
            if (job_error_code != FBE_JOB_SERVICE_ERROR_NO_ERROR)
            {
                /* Log the error and fail the request.
                 */
                fbe_spare_lib_utils_log_internal_error(FBE_SPARE_INTERNAL_ERROR_UNEXPECTED_JOB_STATUS,
                                                       drive_swap_request_p);
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_INFO, 
                                       "%s Unexpected job status: %d cmd: %d orig: 0x%x(%d_%d_%d) spare:0x%x(%d_%d_%d)\n",
                                       __FUNCTION__, job_error_code, command_type,
                                       orig_pvd_object_id, original_pvd_info.port_number,
                                       original_pvd_info.enclosure_number, original_pvd_info.slot_number,
                                       spare_object_id, spare_pvd_info.port_number,
                                       spare_pvd_info.enclosure_number, spare_pvd_info.slot_number);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        default:
            /* For all other event code the job status is not success.
             */
            if (job_error_code == FBE_JOB_SERVICE_ERROR_NO_ERROR)
            {
                /* Log the error and fail the request.
                 */
                fbe_spare_lib_utils_log_internal_error(FBE_SPARE_INTERNAL_ERROR_UNEXPECTED_JOB_STATUS,
                                                       drive_swap_request_p);
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_INFO, 
                                       "%s Unexpected job status: %d cmd: %d orig: 0x%x(%d_%d_%d) spare:0x%x(%d_%d_%d)\n",
                                       __FUNCTION__, job_error_code, command_type,
                                       orig_pvd_object_id, original_pvd_info.port_number,
                                       original_pvd_info.enclosure_number, original_pvd_info.slot_number,
                                       spare_object_id, spare_pvd_info.port_number,
                                       spare_pvd_info.enclosure_number, spare_pvd_info.slot_number);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;
    }

    /* Based on the event code process the event.  All the event with the same
     * format are grouped together.  If you add an event with a different
     * format then you must add a unique case.
     */
    switch(event_code)
    {
        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_INVALID:
        case SEP_ERROR_CODE_SPARE_SOURCE_IN_UNEXPECTED_STATE:
        case SEP_ERROR_CODE_SPARE_DRIVE_VALIDATION_FAILED:
        case SEP_ERROR_CODE_SPARE_OPERATION_IN_PROGRESS:
        case SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_UNCONSUMED:
        case SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_NOT_REDUNDANT:
        case SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN:
        case SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY:
        case SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED:
        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_REMOVED:
        case SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DESTROYED:
        case SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS:
        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED:
        case SEP_INFO_SPARE_DRIVE_SWAP_OUT:
            /* Format is the location of the original drive.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d",original_pvd_info.port_number,
                        original_pvd_info.enclosure_number, original_pvd_info.slot_number);
            break;

        case SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_REMOVED:
        case SEP_ERROR_CODE_COPY_INVALID_DESTINATION_DRIVE:
        case SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY:
        case SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_IN_USE:
        case SEP_ERROR_CODE_DESTINATION_INCOMPATIBLE_RAID_GROUP_OFFSET:
        case SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DENIED:
            /* Format is the location of the replacement/destination drive.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d",spare_pvd_info.port_number,
                        spare_pvd_info.enclosure_number, spare_pvd_info.slot_number);
            break;

        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAP_IN_OPERATION_INITIATED:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_SWAP_IN_OPERATION_INITIATED:
        case SEP_INFO_SPARE_USER_COPY_INITIATED:
        case SEP_INFO_SPARE_COPY_OPERATION_COMPLETED:
        case SEP_ERROR_CODE_COPY_DESTINATION_INCOMPATIBLE_TIER:
        case SEP_ERROR_CODE_SPARE_BLOCK_SIZE_MISMATCH:
        case SEP_ERROR_CODE_SPARE_INSUFFICIENT_CAPACITY:
        case SEP_ERROR_CODE_COPY_DESTINATION_OFFSET_MISMATCH:
            /* Format is replacement disk location followed by original
             * disk location.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d %d %d %d",spare_pvd_info.port_number,
                        spare_pvd_info.enclosure_number,spare_pvd_info.slot_number,
                        original_pvd_info.port_number,original_pvd_info.enclosure_number,
                        original_pvd_info.slot_number);
            break;
            
        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN:
        case SEP_INFO_SPARE_PROACTIVE_SPARE_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_USER_COPY_SPARE_DRIVE_SWAPPED_IN:
        case SEP_INFO_SPARE_USER_COPY_DESTINATION_DRIVE_SWAPPED_IN:
            /* Format is spare disk and serial number followed by failing
             * disk and serial number.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d %s %d %d %d %s",spare_pvd_info.port_number,
                        spare_pvd_info.enclosure_number,spare_pvd_info.slot_number,spare_pvd_info.serial_num,
                        original_pvd_info.port_number,original_pvd_info.enclosure_number,
                        original_pvd_info.slot_number,original_pvd_info.serial_num);
            break;
                            
        case SEP_WARN_SPARE_NO_SPARES_AVAILABLE:
        case SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE:
        case SEP_INFO_SPARE_PROACTIVE_COPY_NO_LONGER_REQUIRED:
        case SEP_INFO_SPARE_PERMANENT_SPARE_NO_LONGER_REQUIRED:
            /* Format is failing disk location and serial number.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d %s",
                        original_pvd_info.port_number,
                        original_pvd_info.enclosure_number,
                        original_pvd_info.slot_number,
                        original_pvd_info.serial_num);
            break;

        case SEP_ERROR_CODE_SPARE_UNEXPECTED_ERROR:
        case SEP_ERROR_CODE_SPARE_UNEXPECTED_COMMAND:
            /* Format is swap status followed by original disk location.
             */
            fbe_event_log_write(event_code, NULL, 0, 
                        "%d %d %d %d",
                        internal_error_status,
                        original_pvd_info.port_number,
                        original_pvd_info.enclosure_number,
                        original_pvd_info.slot_number);
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "spare write event: status: %d job status: %d cmd: %d orig: 0x%x(%d_%d_%d) spare:0x%x(%d_%d_%d)\n",
                                   internal_error_status, job_error_code, command_type,
                                   orig_pvd_object_id, original_pvd_info.port_number,
                                   original_pvd_info.enclosure_number, original_pvd_info.slot_number,
                                   spare_object_id, spare_pvd_info.port_number,
                                   spare_pvd_info.enclosure_number, spare_pvd_info.slot_number);
            break;

        default:
            /* Unsupported event code.  Generate an internal error and then
             * fail the request.
             */
            fbe_spare_lib_utils_log_internal_error(FBE_SPARE_INTERNAL_ERROR_UNSUPPORTED_EVENT_CODE,
                                                   drive_swap_request_p);
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "%s Unexpected job status: %d cmd: %d orig: 0x%x(%d_%d_%d) spare:0x%x(%d_%d_%d)\n",
                                   __FUNCTION__, job_error_code, command_type,
                                   orig_pvd_object_id, original_pvd_info.port_number,
                                   original_pvd_info.enclosure_number, original_pvd_info.slot_number,
                                   spare_object_id, spare_pvd_info.port_number,
                                   spare_pvd_info.enclosure_number, spare_pvd_info.slot_number);
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_write_event_log()
 ******************************************************************************/
 
/*!***************************************************************************
 *          fbe_spare_lib_utils_rollback_write_event_log()
 *****************************************************************************
 *
 * @brief   The swap commmand has failed.  Based on the command type and failure
 *          generate the proper event log.
 * 
 * @param   drive_swap_request_p - Job service swap request.
 * 
 * @return  status - The status of this operation.
 *
 * @author
 *  07/26/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_utils_rollback_write_event_log(fbe_job_service_drive_swap_request_t *drive_swap_request_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_job_service_error_type_t    status_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_u32_t                       event_code;
    fbe_provision_drive_info_t      original_pvd_info = {0};
    fbe_provision_drive_info_t      spare_pvd_info = {0};
    
    /* First get the status of the request
     */
    status_code = drive_swap_request_p->status_code;
    switch(status_code)
    {
        /* Informational error codes (i.e. permanent spare no longer required 
         * since original drive was re-inserted etc.)
         */
        case FBE_JOB_SERVICE_ERROR_SWAP_PROACTIVE_SPARE_NOT_REQUIRED:
            event_code = SEP_INFO_SPARE_PROACTIVE_COPY_NO_LONGER_REQUIRED;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_PERMANENT_SPARE_NOT_REQUIRED:
            event_code = SEP_INFO_SPARE_PERMANENT_SPARE_NO_LONGER_REQUIRED;
            break;

        /* General spare request failure codes.
         */
        case FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND:
            event_code = SEP_ERROR_CODE_SPARE_UNEXPECTED_COMMAND;
            break;
        case FBE_JOB_SERVICE_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID:
        case FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID:
            event_code = SEP_ERROR_CODE_COPY_SOURCE_DRIVE_INVALID;
            break;
        case FBE_JOB_SERVICE_ERROR_INVALID_EDGE_INDEX:
        case FBE_JOB_SERVICE_ERROR_INVALID_PERM_SPARE_EDGE_INDEX:
        case FBE_JOB_SERVICE_ERROR_INVALID_PROACTIVE_SPARE_EDGE_INDEX:
            event_code = SEP_ERROR_CODE_SPARE_SOURCE_IN_UNEXPECTED_STATE;
            break;
        case FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID:
            event_code = SEP_ERROR_CODE_SPARE_DRIVE_VALIDATION_FAILED;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_UNEXPECTED_ERROR:
        case FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL:
            event_code = SEP_ERROR_CODE_SPARE_UNEXPECTED_ERROR;
            break;

        /* Virtual drive denied request.
         */
        case FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN:
            event_code = SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT:
            event_code = SEP_ERROR_CODE_SPARE_OPERATION_IN_PROGRESS;
            break;

        /* Parent raid group errors.
         */
        case FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_UNCONSUMED:
            event_code = SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_UNCONSUMED;
            break;
        case FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_NOT_REDUNDANT:
            event_code = SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_NOT_REDUNDANT;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN:
            event_code = SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED:
            event_code = SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_HAS_COPY_IN_PROGRESS:
            event_code = SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_SOURCE_DRIVE_DEGRADED:
            event_code = SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED:
            event_code = SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DENIED;
            break;
        case FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_DESTROYED:
            event_code = SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DESTROYED;
            break;

        /* Could not locate a suitable spare.*/
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE:
            event_code = SEP_WARN_SPARE_NO_SPARES_AVAILABLE;
            break;
        case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE:
            event_code = SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE;
            break;
        case FBE_JOB_SERVICE_ERROR_INVALID_DESIRED_SPARE_DRIVE_TYPE:
            /* Performance tier mis-match.*/
            event_code = SEP_ERROR_CODE_COPY_DESTINATION_INCOMPATIBLE_TIER;
            break;

        /*! @note Source drive failures.
         */
        case FBE_JOB_SERVICE_ERROR_SWAP_COPY_SOURCE_DRIVE_REMOVED:
            event_code = SEP_ERROR_CODE_COPY_SOURCE_DRIVE_REMOVED;
            break;

        /*! @note Errors for invalid / unhealthy destination drive.
         */
        case FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_REMOVED:
            event_code = SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_REMOVED;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_COPY_INVALID_DESTINATION_DRIVE:
            event_code = SEP_ERROR_CODE_COPY_INVALID_DESTINATION_DRIVE;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY:
            event_code = SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY;
            break;
        case FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_HAS_UPSTREAM_RAID_GROUP:
            event_code = SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_IN_USE;
            break;
        case FBE_JOB_SERVICE_ERROR_OFFSET_MISMATCH:
        case FBE_JOB_SERVICE_ERROR_SYS_DRIVE_MISMATCH:
            event_code = SEP_ERROR_CODE_DESTINATION_INCOMPATIBLE_RAID_GROUP_OFFSET;
            break;

        /*! @note Generic failures.
         */
        case FBE_JOB_SERVICE_ERROR_BLOCK_SIZE_MISMATCH:
            event_code = SEP_ERROR_CODE_SPARE_BLOCK_SIZE_MISMATCH;
            break;
        case FBE_JOB_SERVICE_ERROR_CAPACITY_MISMATCH:
            event_code = SEP_ERROR_CODE_SPARE_INSUFFICIENT_CAPACITY;
            break;

        /*! @todo Currently these errors are unsupported.
         */
        case FBE_JOB_SERVICE_ERROR_SPARE_NOT_READY:
        case FBE_JOB_SERVICE_ERROR_SPARE_REMOVED:
        case FBE_JOB_SERVICE_ERROR_SPARE_BUSY:
            /* Generate an error trace and don't generate the event log.
             */
            fbe_spare_lib_utils_get_provision_drive_info(drive_swap_request_p->spare_object_id, &spare_pvd_info);
            fbe_spare_lib_utils_get_provision_drive_info(drive_swap_request_p->orig_pvd_object_id, &original_pvd_info);
            fbe_spare_lib_utils_log_internal_error(FBE_SPARE_INTERNAL_ERROR_UNEXPECTED_JOB_STATUS,
                                                   drive_swap_request_p);
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "%s Unexpected job status: %d command: %d orig: 0x%x(%d_%d_%d) spare:0x%x(%d_%d_%d)\n",
                                   __FUNCTION__, status_code, drive_swap_request_p->command_type,
                                   drive_swap_request_p->orig_pvd_object_id, original_pvd_info.port_number,
                                   original_pvd_info.enclosure_number, original_pvd_info.slot_number,
                                   drive_swap_request_p->spare_object_id, spare_pvd_info.port_number,
                                   spare_pvd_info.enclosure_number, spare_pvd_info.slot_number);

            /* Since we don't know the proper event log to generate just return 
             * success without generating the event.
             */
            return FBE_STATUS_OK;

        case FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR:
            /* A job timeout can result in an internal error.
             */
            fbe_spare_lib_utils_get_provision_drive_info(drive_swap_request_p->spare_object_id, &spare_pvd_info);
            fbe_spare_lib_utils_get_provision_drive_info(drive_swap_request_p->orig_pvd_object_id, &original_pvd_info);
            fbe_spare_lib_utils_log_internal_error(FBE_SPARE_INTERNAL_ERROR_UNEXPECTED_JOB_STATUS,
                                                   drive_swap_request_p);
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "%s Internal error job status: %d command: %d orig: 0x%x(%d_%d_%d) spare:0x%x(%d_%d_%d)\n",
                                   __FUNCTION__, status_code, drive_swap_request_p->command_type,
                                   drive_swap_request_p->orig_pvd_object_id, original_pvd_info.port_number,
                                   original_pvd_info.enclosure_number, original_pvd_info.slot_number,
                                   drive_swap_request_p->spare_object_id, spare_pvd_info.port_number,
                                   spare_pvd_info.enclosure_number, spare_pvd_info.slot_number);
            return FBE_STATUS_OK;

        /*! @note We should not be here for success or unsupported codes.
         */
        case FBE_JOB_SERVICE_ERROR_NO_ERROR:
        case FBE_JOB_SERVICE_ERROR_INVALID:
        default:
            /*! @note We don't expect success.  Generate an internal error event.
             */
            fbe_spare_lib_utils_get_provision_drive_info(drive_swap_request_p->spare_object_id, &spare_pvd_info);
            fbe_spare_lib_utils_get_provision_drive_info(drive_swap_request_p->orig_pvd_object_id, &original_pvd_info);
            fbe_spare_lib_utils_log_internal_error(FBE_SPARE_INTERNAL_ERROR_UNEXPECTED_JOB_STATUS,
                                                   drive_swap_request_p);
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "%s Unexpected job status: %d command: %d orig: 0x%x(%d_%d_%d) spare:0x%x(%d_%d_%d)\n",
                                   __FUNCTION__, status_code, drive_swap_request_p->command_type,
                                   drive_swap_request_p->orig_pvd_object_id, original_pvd_info.port_number,
                                   original_pvd_info.enclosure_number, original_pvd_info.slot_number,
                                   drive_swap_request_p->spare_object_id, spare_pvd_info.port_number,
                                   spare_pvd_info.enclosure_number, spare_pvd_info.slot_number);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note For internally generated request we do not want to log repeating
     *        errors.  It is up to the virtual drive to generate the event.
     */
    switch(drive_swap_request_p->command_type)
    {
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            /* Do not log repeating errors.  It is assumed that all other
             * errors are terminal (i.e. the request is complete).
             */
            switch(drive_swap_request_p->status_code)
            {
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_HAS_COPY_IN_PROGRESS:
                case FBE_JOB_SERVICE_ERROR_PRESENTLY_SOURCE_DRIVE_DEGRADED:
                    /* These error repeat.  Do not report them.
                     */
                    return status;
                default:
                    /* For all other error codes fall-thru.
                     */
                    break;
            }
            // Fall-thru
        default:
            /* We have validate the error code above.  Generate the event log.
             */
            status = fbe_spare_lib_utils_write_event_log(event_code, drive_swap_request_p);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_rollback_write_event_log()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_commit_complete_write_event_log()
 ******************************************************************************
 * @brief
 * The swap commmand has been commited.  Generate the appropriate event log.
 * Basically generate the SEP event message based on the swap request that
 * completed.
 * 
 * @param drive_swap_request_p             - Job service swap request.
 * 
 * @return status                          - The status of the operation.
 *
 * @author
 *  11/24/2010 - Created. Shanmugam ,Vishnu Sharma
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_commit_complete_write_event_log(fbe_job_service_drive_swap_request_t *drive_swap_request_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_spare_swap_command_t    swap_command_type;
    fbe_u32_t                   event_code;
    
    /* get the swap command that just completed.
     */
    swap_command_type = drive_swap_request_p->command_type;
    switch(swap_command_type)
    {
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
            event_code = SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN;
            break;
        
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            event_code = SEP_INFO_SPARE_PROACTIVE_SPARE_DRIVE_SWAPPED_IN;
            break;
                        
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
            event_code = SEP_INFO_SPARE_USER_COPY_SPARE_DRIVE_SWAPPED_IN;
            break;

        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            event_code = SEP_INFO_SPARE_USER_COPY_INITIATED;
            break;

        case FBE_SPARE_COMPLETE_COPY_COMMAND:
        case FBE_SPARE_ABORT_COPY_COMMAND:
            event_code = SEP_INFO_SPARE_DRIVE_SWAP_OUT;
            break;
        
        default:
            /* Generate an internal error event and return failure.
             */
            {
                fbe_provision_drive_info_t  original_pvd_info = {0};
                fbe_provision_drive_info_t  spare_pvd_info = {0};

                fbe_spare_lib_utils_get_provision_drive_info(drive_swap_request_p->spare_object_id, &spare_pvd_info);
                fbe_spare_lib_utils_get_provision_drive_info(drive_swap_request_p->orig_pvd_object_id, &original_pvd_info);
                fbe_spare_lib_utils_log_internal_error(FBE_SPARE_INTERNAL_ERROR_INVALID_SWAP_COMMAND_TYPE,
                                                       drive_swap_request_p);
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_INFO, 
                                       "%s Unexpected swap command: %d orig: 0x%x(%d_%d_%d) spare:0x%x(%d_%d_%d)\n",
                                       __FUNCTION__, drive_swap_request_p->command_type,
                                       drive_swap_request_p->orig_pvd_object_id, original_pvd_info.port_number,
                                       original_pvd_info.enclosure_number, original_pvd_info.slot_number,
                                       drive_swap_request_p->spare_object_id, spare_pvd_info.port_number,
                                       spare_pvd_info.enclosure_number, spare_pvd_info.slot_number);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;
    }

    /* Log the event log message.
     */
    status = fbe_spare_lib_utils_write_event_log(event_code, drive_swap_request_p);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_commit_complete_write_event_log()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_get_original_drive_info()
 ******************************************************************************
 * @brief
 * Helper function to get the pvd object's physical location in spare library.
 * 
 * @param fbe_packet_t          packet requesting this operation.
 * @param drive_swap_request_p  Job service swap request.
 * @param original_pvd_info_p            Desired spare drive info pointer.
 * 
 * 
 * @return status                     - The status of the operation.
 *
 * @author
 *  12/15/2010 - Created.Vishnu Sharma
 ******************************************************************************/
fbe_status_t 
fbe_spare_lib_utils_get_original_drive_info(fbe_packet_t *packet_p,
                                            fbe_job_service_drive_swap_request_t *drive_swap_request_p,
                                            fbe_spare_drive_info_t *original_pvd_info_p)
{
    fbe_provision_drive_info_t         original_pvd_info = {0};
    fbe_object_id_t                    original_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                   permanent_spare_edge_index = FBE_EDGE_INDEX_INVALID;
    
    /* Get the edge-index where we need to swap-in the permanent spare. */
    fbe_spare_lib_utils_get_permanent_spare_swap_edge_index(drive_swap_request_p->vd_object_id,
                                                            &permanent_spare_edge_index);
    /* get the server object id for the swap out edge index. */
    fbe_spare_lib_utils_get_server_object_id_for_edge(drive_swap_request_p->vd_object_id,
                                                      permanent_spare_edge_index,
                                                      &original_pvd_object_id);
    /*Get porvision drive info*/
    fbe_spare_lib_utils_get_provision_drive_info(original_pvd_object_id,&original_pvd_info);
    
    original_pvd_info_p->port_number = original_pvd_info.port_number;
    original_pvd_info_p->enclosure_number = original_pvd_info.enclosure_number;
    original_pvd_info_p->slot_number = original_pvd_info.slot_number;
    
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_utils_get_original_drive_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_enumerate_system_objects()
 ******************************************************************************
 * @brief   Enumerate all the system objects
 *
 * @param   class_id - class to enumerate.
 * @param   system_object_list_p - ptr to structure for enumerating a class.
 * @param   total_objects - Max objects that can be enumerated
 * @param   actual_num_objects_p - Pointer to actual number of objects in list               
 *
 * @return status - The status of the operation.
 *
 * @author
 *  23/02/2011 - Created. Vishnu Sharma
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_enumerate_system_objects(fbe_class_id_t class_id,
                                             fbe_object_id_t *system_object_list_p,
                                             fbe_u32_t total_objects,
                                             fbe_u32_t *actual_num_objects_p)
{
    fbe_status_t       status = FBE_STATUS_OK;
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_database_control_enumerate_system_objects_t enumerate_system_objects;
    fbe_sg_element_t sg[FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT];
    fbe_payload_control_status_t    payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /*! @todo Currently enumerating by class id isn't supported.
     */
    if (class_id != FBE_CLASS_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: class_id: 0x%x currently not supported.\n", __FUNCTION__, class_id);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s packet allocation failed.\n",
                               __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_ex_set_sg_list(sep_payload_p, &sg[0], 2);

    sg[0].address = (fbe_u8_t *)system_object_list_p;
    sg[0].count = total_objects * sizeof(fbe_object_id_t);
    sg[1].address = NULL;
    sg[1].count = 0;

    enumerate_system_objects.class_id = class_id;
    enumerate_system_objects.number_of_objects = total_objects;
    enumerate_system_objects.number_of_objects_copied = 0;

    fbe_payload_control_build_operation(control_p,
                                        FBE_DATABASE_CONTROL_CODE_ENUMERATE_SYSTEM_OBJECTS,
                                        &enumerate_system_objects,
                                        sizeof(fbe_database_control_enumerate_system_objects_t));
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Set packet address (database service lives in SEP_0)*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_DATABASE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);

    
    fbe_service_manager_send_control_packet(packet_p);
    fbe_transport_wait_completion(packet_p);
    
    fbe_payload_control_get_status(control_p, &payload_control_status);
    
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s payload control operation failed! Control Code:%u\n",
                               __FUNCTION__, payload_control_status);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s packet failed\n",
                               __FUNCTION__);
    }
    
    /* release the packet before exit.
     */
    fbe_transport_release_packet(packet_p);

    *actual_num_objects_p = enumerate_system_objects.number_of_objects_copied;
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_enumerate_system_objects()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_get_unused_drive_as_spare_flag()
 ******************************************************************************
 * @brief
 * This function is used to get unused drive as spare flag.
 *
 *
 * @param vd_object_id          - virtual drive object id.
 * @param unused_drive_as_spare_p  - flag.
 *
 * @return status - status of the operation
 *
 * @author
 *  05/05/2011 - Created. Vishnu Sharma
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_get_unused_drive_as_spare_flag(fbe_object_id_t vd_object_id,
                                           fbe_bool_t * unused_drive_as_spare_p)
{
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_virtual_drive_unused_drive_as_spare_flag_t  get_unused_drive_as_spare_flag;

    /* initialize flag as false. */
    get_unused_drive_as_spare_flag.unused_drive_as_spare_flag = 0;
    
    /* get the unused drive as spare flag  of the virtual drive object. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_UNUSED_DRIVE_AS_SPARE_FLAG,
                                                     &get_unused_drive_as_spare_flag,
                                                     sizeof(fbe_virtual_drive_unused_drive_as_spare_flag_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    /* If failure status returned, log an error and return immediately */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_UNUSED_DRIVE_AS_SPARE_FLAG failed.\n",
                               __FUNCTION__);
    }
    else
    {
        *unused_drive_as_spare_p = get_unused_drive_as_spare_flag.unused_drive_as_spare_flag;
    }

    return status;
}

/******************************************************************************
 * end fbe_spare_lib_utils_get_unused_drive_as_spare_flag()
 ******************************************************************************/
 

/*!****************************************************************************
 * fbe_spare_lib_utils_take_object_offline()
 ******************************************************************************
 * @brief
 * This function sets an object to offline state.
 *
 *
 * @param object_id          - object to go offline.
 *
 * @return status - status of the operation
 *
 * @author
 *  08/19/2011 - Created.
 ******************************************************************************/
fbe_status_t
fbe_spare_lib_utils_take_object_offline(fbe_object_id_t object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    status = fbe_spare_lib_utils_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_SET_OFFLINE_COND,
                                                      NULL,
                                                      0,
                                                      FBE_SERVICE_ID_TOPOLOGY,
                                                      FBE_CLASS_ID_INVALID,
                                                      object_id,
                                                      FBE_PACKET_FLAG_NO_ATTRIB);
    /* If failure status returned, log an error and return immediately */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to take obj0x%x offline.\n",
                               __FUNCTION__, object_id);
    }
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_take_object_offline()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_set_vd_nonpaged_metadata()
 ******************************************************************************
 * @brief
 * This function sends a control packet to vd to set the non paged metadta
 * to indicate we have swapped in a new edge.
 *
 * @param packet_p
 * @param js_swap_request_p
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/31/2011 - Created. Ashwin Tamilarasan
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_set_vd_nonpaged_metadata(fbe_packet_t* packet_p,
                                                          fbe_job_service_drive_swap_request_t * js_swap_request_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_object_id_t vd_object_id;

    /* Get the virtual drive object id from the incoming job service drive swap request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(js_swap_request_p, &vd_object_id);
    if (vd_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:update NP failed, invalid vd object\n",
                               __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_NONPAGED_PERM_SPARE_BIT,
                                                     NULL/* not used */,
                                                     -1/* NOT USED */,
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO, "%s Update NP failed.\n",
                               __FUNCTION__);
         return status;
    }

    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_set_vd_nonpaged_metadata()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_spare_lib_utils_switch_pool_ids()
 ******************************************************************************
 * @brief
 * This function marks the new drive is being in the same pool as the old 
 * and marks the old drive as not being in a pool
 *
 *
 * @param js_swap_request_p - the drive swap job request
 * @param selected_spare_id - the id of the selected spare
 * @return status - The status of the operation.
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_switch_pool_ids(fbe_job_service_drive_swap_request_t *  js_swap_request_p, fbe_object_id_t selected_spare_id)
{
    fbe_u32_t                               pool_id;
    fbe_status_t                            status;
    fbe_database_control_update_pvd_t       update_provision_drive;


    /*also, update the new drive with the pool ID of the original PVD*/
    status = fbe_database_get_pvd_pool_id(js_swap_request_p->orig_pvd_object_id, &pool_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:getting POOL Id of original drive failed\n",__FUNCTION__);
        
        return status;
    }

    update_provision_drive.pool_id = pool_id;
    update_provision_drive.object_id = selected_spare_id;
    update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
    update_provision_drive.update_type = FBE_UPDATE_PVD_POOL_ID;
    update_provision_drive.update_opaque_data = FBE_FALSE;

    /* update the configuration type of the spare pvd object to raid. */
    status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:failed to update ID of selected spare, id:0x%x\n",
                               __FUNCTION__, selected_spare_id);
    
        return status;
    }

	/*before copy is complete, the source drive's pool id should not be invalidated.
	 *So here deleted the original logic where we set the src drive's pool id invalid - Zhipeng Hu*/

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_spare_lib_utils_switch_pool_ids()
 ********************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_wait_for_object_to_be_ready()
 ***************************************************************************** 
 * 
 * @brief   We allow copy operations when the virtual drive or spare object
 *          is in the Ready or Hibernate state.  But we cannot start the copy
 *          request until it is in the Ready state.  Take the virtual drive or
 *          spare drive out of hibernation and wait the timeout specified for
 *          it to become Ready.
 *
 * @param   object_id    - Object id for virtual drive or spare drive
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  01/09/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_wait_for_object_to_be_ready(fbe_object_id_t object_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_base_object_mgmt_get_lifecycle_state_t  object_lifecycle;
    fbe_base_config_get_peer_lifecycle_state_t  peer_lifecycle; 
    fbe_bool_t                                  b_is_peer_alive;
    fbe_u32_t                                   wait_for_ready_secs = fbe_spare_main_get_operation_timeout_secs();
    fbe_semaphore_t                             object_wait_for_ready_sem;
    fbe_u32_t                                   wait_secs = 3;
    fbe_u32_t                                   total_secs_to_wait;
    fbe_u32_t                                   total_secs_waited = 0;

    /* The only supported states for the virtual drive are Ready or Hibernate.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                     &object_lifecycle,
                                                     sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if ((status != FBE_STATUS_OK)                                                         ||
        ((object_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_READY)             &&
         (object_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)         &&
         (object_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_HIBERNATE)    )    )
    {
        /* Unexpected virtual drive lifecycle state.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: obj: 0x%x life: %d not supported - status: 0x%x\n",
                               __FUNCTION__, object_id, object_lifecycle.lifecycle_state, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the peer is alive get it's state.
     */
    b_is_peer_alive = fbe_cmi_is_peer_alive();
    if (b_is_peer_alive)
    {
        /* The only supported states for the virtual drive are Ready or Hibernate.
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_PEER_LIFECYCLE_STATE,
                                                         &peer_lifecycle,
                                                         sizeof (fbe_base_config_get_peer_lifecycle_state_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);

        /* The peer can go away at anytime but we don't handle that case.
         * Simply fail the request.
         */
        if ((status != FBE_STATUS_OK)                                                  ||
            ((peer_lifecycle.peer_state != FBE_LIFECYCLE_STATE_READY)             &&
             (peer_lifecycle.peer_state != FBE_LIFECYCLE_STATE_HIBERNATE)         &&
             (peer_lifecycle.peer_state != FBE_LIFECYCLE_STATE_PENDING_HIBERNATE)    )    )
        {
            /* Unexpected peer virtual drive lifecycle state.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: obj: 0x%x peer life: %d not supported - status: 0x%x\n",
                               __FUNCTION__, object_id, peer_lifecycle.peer_state, status);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        /* Else the peer isn't alive.  Simply mark it as ready.
         */
        peer_lifecycle.peer_state = FBE_LIFECYCLE_STATE_READY;
    }
    
    /* Trace some information.
     */
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare: obj: 0x%x life: %d peer: %d wait upto: %d seconds to become Ready\n",
                           object_id, 
                           object_lifecycle.lifecycle_state,
                           (b_is_peer_alive) ? peer_lifecycle.peer_state : FBE_LIFECYCLE_STATE_INVALID,
                           wait_for_ready_secs);


    /* If we are in the Ready state don't issue the `exit hibernation'.
     */
    if ((object_lifecycle.lifecycle_state == FBE_LIFECYCLE_STATE_READY) &&
        (peer_lifecycle.peer_state        == FBE_LIFECYCLE_STATE_READY)    )
    {
        return FBE_STATUS_OK;
    }

    /* Send the request to make the virtual drive / spare drive exit hibernation.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BLOCK_TRANSPORT_CONTROL_CODE_EXIT_HIBERNATION,
                                                     NULL,
                                                     0,
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected virtual drive lifecycle state.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: obj: 0x%x request to exit hibernate failed - status: 0x%x\n",
                               __FUNCTION__, object_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now wait at least (6) seconds for the virtual drive to exit hibernation
     */
    fbe_semaphore_init(&object_wait_for_ready_sem, 0, 1);
    if (wait_for_ready_secs < 6)
    {
        total_secs_to_wait = 6;
    }
    else
    {
        total_secs_to_wait = wait_for_ready_secs;
    }
    while (((object_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_READY) ||
            ((b_is_peer_alive == FBE_TRUE)                           &&
             (peer_lifecycle.peer_state != FBE_LIFECYCLE_STATE_READY)    )     ) &&
           (total_secs_waited < total_secs_to_wait)                                 )
    {
        /* Wait (1) cycle and then check lifecycle state.
         */
        fbe_semaphore_wait_ms(&object_wait_for_ready_sem, (wait_secs * 1000));
        total_secs_waited += wait_secs;

        /* Get the lifecycle state.
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                         &object_lifecycle,
                                                         sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            /* Unexpected virtual drive lifecycle state.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: obj: 0x%x get life failed - status: 0x%x\n",
                                   __FUNCTION__, object_id, status);

            /* Free the semaphore and return an error.
             */
            fbe_semaphore_destroy(&object_wait_for_ready_sem);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* If the peer is alive get it's lifecycle state.
         */
        b_is_peer_alive = fbe_cmi_is_peer_alive();
        if (b_is_peer_alive)
        {
            /* Get the peer lifecycle state.
             */
            status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_PEER_LIFECYCLE_STATE,
                                                             &peer_lifecycle,
                                                             sizeof (fbe_base_config_get_peer_lifecycle_state_t),
                                                             FBE_SERVICE_ID_TOPOLOGY,
                                                             FBE_CLASS_ID_INVALID,
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB);

            /* The peer can go away at anytime but we don't handle that case.
             * Simply fail the request.
             */
            if (status != FBE_STATUS_OK)
            {
                /* Unexpected peer virtual drive lifecycle state.
                 */
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: obj: 0x%x get peer life failed - status: 0x%x\n",
                               __FUNCTION__, object_id, status);

                /* Free the semaphore and return an error.
                 */
                fbe_semaphore_destroy(&object_wait_for_ready_sem);

                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            /* Else the peer isn't alive.  Simply mark it as ready.
             */
            peer_lifecycle.peer_state = FBE_LIFECYCLE_STATE_READY;
        }

    } /* end while the vd is not ready */

    /* Check for success*/
    if ((object_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_READY) ||
        (peer_lifecycle.peer_state        != FBE_LIFECYCLE_STATE_READY)    )
    {
        /* Report the error
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: obj: 0x%x life: %d peer: %d waited: %d seconds - failed\n",
                               __FUNCTION__, object_id, 
                               object_lifecycle.lifecycle_state,
                               (b_is_peer_alive) ? peer_lifecycle.peer_state : FBE_LIFECYCLE_STATE_INVALID,
                               total_secs_waited);

        status = FBE_STATUS_TIMEOUT;
    }
    else
    {
        /* Report success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: obj: 0x%x life: %d peer: %d waited: %d seconds for Ready\n",
                               object_id, 
                               object_lifecycle.lifecycle_state, 
                               (b_is_peer_alive) ? peer_lifecycle.peer_state : FBE_LIFECYCLE_STATE_INVALID,
                               total_secs_waited);

        status = FBE_STATUS_OK;
    }

    /* Free the semaphore.
     */
    fbe_semaphore_destroy(&object_wait_for_ready_sem);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_wait_for_object_to_be_ready()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_wait_for_object_to_be_failed()
 ***************************************************************************** 
 * 
 * @brief   Wait for the specified object to be in the Failed state on both SPs.
 *
 * @param   object_id    - Object id for virtual drive or spare drive
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  06/06/2014  Ron Proulx  - Created.
 * 
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_wait_for_object_to_be_failed(fbe_object_id_t object_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_base_object_mgmt_get_lifecycle_state_t  object_lifecycle;
    fbe_base_config_get_peer_lifecycle_state_t  peer_lifecycle; 
    fbe_bool_t                                  b_is_peer_alive;
    fbe_u32_t                                   wait_for_failed_secs = fbe_spare_main_get_operation_timeout_secs();
    fbe_semaphore_t                             object_wait_for_failed_sem;
    fbe_u32_t                                   wait_secs = 3;
    fbe_u32_t                                   total_secs_to_wait;
    fbe_u32_t                                   total_secs_waited = 0;

    /* Get the current lifecycle state.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                     &object_lifecycle,
                                                     sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        /* Unexpected lifecycle state.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: obj: 0x%x get lifecycle state failed - status: 0x%x\n",
                               __FUNCTION__, object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the peer is alive get it's state.
     */
    b_is_peer_alive = fbe_cmi_is_peer_alive();
    if (b_is_peer_alive) {
        /* If the peer is alive get it's state.
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_PEER_LIFECYCLE_STATE,
                                                         &peer_lifecycle,
                                                         sizeof (fbe_base_config_get_peer_lifecycle_state_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);

        /* If the request failed fail the request.
         */
        if (status != FBE_STATUS_OK) {
            /* Unexpected peer lifecycle state.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: obj: 0x%x get peer life failed - status: 0x%x\n",
                               __FUNCTION__, object_id, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    } else {
        /* Else the peer isn't alive mark it as failed.
         */
        peer_lifecycle.peer_state = FBE_LIFECYCLE_STATE_FAIL;
    }
    
    /* Trace some information.
     */
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare: obj: 0x%x life: %d peer: %d wait upto: %d seconds to become Failed\n",
                           object_id, object_lifecycle.lifecycle_state, 
                           (b_is_peer_alive) ? peer_lifecycle.peer_state : FBE_LIFECYCLE_STATE_INVALID,
                           wait_for_failed_secs);

    /* If the object is Failed don't wait.
     */
    if ((object_lifecycle.lifecycle_state == FBE_LIFECYCLE_STATE_FAIL) &&
        (peer_lifecycle.peer_state        == FBE_LIFECYCLE_STATE_FAIL)    ) {
        return FBE_STATUS_OK;
    }

    /* Now wait at least (6) seconds for the object to become Failed.
     */
    fbe_semaphore_init(&object_wait_for_failed_sem, 0, 1);
    if (wait_for_failed_secs < 6) {
        total_secs_to_wait = 6;
    } else {
        total_secs_to_wait = wait_for_failed_secs;
    }
    while (((object_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_FAIL) ||
            ((b_is_peer_alive == FBE_TRUE)                           &&
             (peer_lifecycle.peer_state != FBE_LIFECYCLE_STATE_FAIL)    )     ) &&
           (total_secs_waited < total_secs_to_wait)                                 ) {
        /* Wait (1) cycle and then check lifecycle state.
         */
        fbe_semaphore_wait_ms(&object_wait_for_failed_sem, (wait_secs * 1000));
        total_secs_waited += wait_secs;

        /* Get the lifecycle state.
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                         &object_lifecycle,
                                                         sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK) {
            /* Unexpected virtual drive lifecycle state.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: obj: 0x%x get life failed - status: 0x%x\n",
                                   __FUNCTION__, object_id, status);

            /* Free the semaphore and return an error.
             */
            fbe_semaphore_destroy(&object_wait_for_failed_sem);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* If the peer is alive get it's lifecycle state.
         */
        b_is_peer_alive = fbe_cmi_is_peer_alive();
        if (b_is_peer_alive) {
            /* Get the peer lifecycle state.
             */
            status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_PEER_LIFECYCLE_STATE,
                                                             &peer_lifecycle,
                                                             sizeof (fbe_base_config_get_peer_lifecycle_state_t),
                                                             FBE_SERVICE_ID_TOPOLOGY,
                                                             FBE_CLASS_ID_INVALID,
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB);

            /* The peer can go away at anytime but we don't handle that case.
             * Simply fail the request.
             */
            if (status != FBE_STATUS_OK) {
                /* Unexpected peer virtual drive lifecycle state.
                 */
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: obj: 0x%x get peer life failed - status: 0x%x\n",
                               __FUNCTION__, object_id, status);

                /* Free the semaphore and return an error.
                 */
                fbe_semaphore_destroy(&object_wait_for_failed_sem);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Currently if the peer is booting it returns a lifecycle state of
             * `specialize'.
             */
        } else {
            /* Else the peer isn't alive.  Simply mark it as failed.
             */
            peer_lifecycle.peer_state = FBE_LIFECYCLE_STATE_FAIL;
        }

    } /* end while the vd is not ready */

    /* Check for success*/
    if ((object_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_FAIL) ||
        (peer_lifecycle.peer_state        != FBE_LIFECYCLE_STATE_FAIL)    )
    {
        /* Report the error
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: obj: 0x%x life: %d peer: %d waited: %d seconds - failed\n",
                               __FUNCTION__, object_id, 
                               object_lifecycle.lifecycle_state,
                               (b_is_peer_alive) ? peer_lifecycle.peer_state : FBE_LIFECYCLE_STATE_INVALID,
                               total_secs_waited);
        status = FBE_STATUS_TIMEOUT;
    }
    else
    {
        /* Report success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: obj: 0x%x life: %d peer: %d waited: %d seconds for Failed\n",
                               object_id, 
                               object_lifecycle.lifecycle_state, 
                               (b_is_peer_alive) ? peer_lifecycle.peer_state : FBE_LIFECYCLE_STATE_INVALID,
                               total_secs_waited);

        status = FBE_STATUS_OK;
    }

    /* Free the semaphore.
     */
    fbe_semaphore_destroy(&object_wait_for_failed_sem);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_wait_for_object_to_be_failed()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_wait_for_copy_complete_or_failed()
 ***************************************************************************** 
 * 
 * @brief   This method waits for either a `copy complete' or `copy failed'
 *          (determined by the `swap command' passed) copy completion.  If
 *          the operation is still in progress BUT the `request in progress'
 *          flag is not set it indicates an unexpected sequence.
 *
 * @param   vd_object_id  - Object id for virtual drive or spare drive
 * @param   swap_command - Either `copy complete' or `abort copy'
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/22/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_utils_wait_for_copy_complete_or_failed(fbe_object_id_t vd_object_id,
                                                                         fbe_spare_swap_command_t swap_command)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_virtual_drive_get_info_t    vd_get_info;
    fbe_u32_t                       wait_for_complete_secs = FBE_JOB_SERVICE_MAXIMUM_SPARE_OPERATION_TIMEOUT_SECONDS;
    fbe_semaphore_t                 wait_for_complete_sem;
    fbe_u32_t                       wait_secs = 1;
    fbe_u32_t                       total_secs_to_wait;
    fbe_u32_t                       total_secs_waited = 0;

    /*! @note For testing purposes we check if the default timeout has been lowered.
     *        If it has then we change the timeout to the lower value.
     */
    if (fbe_spare_main_get_operation_timeout_secs() < FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS) {
        wait_for_complete_secs = fbe_spare_main_get_operation_timeout_secs();
    }

    /* The only supported commands are `copy complete' or `abort copy'
     */
    if ((swap_command != FBE_SPARE_COMPLETE_COPY_COMMAND) &&
        (swap_command != FBE_SPARE_ABORT_COPY_COMMAND)       )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x cmd: %d not supported\n",
                               __FUNCTION__, vd_object_id, swap_command);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now wait at least (6) seconds for the virtual drive to exit hibernation
     */
    fbe_semaphore_init(&wait_for_complete_sem, 0, 1);
    if (wait_for_complete_secs < 6)
    {
        total_secs_to_wait = 6;
    }
    else
    {
        total_secs_to_wait = wait_for_complete_secs;
    }

    /* Send the first command
     * Since we just issued the request, wait a short period before
     * we check.
     */
    fbe_semaphore_wait_ms(&wait_for_complete_sem, 100);
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                     &vd_get_info,
                                                     sizeof (fbe_virtual_drive_get_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if ((status != FBE_STATUS_OK)                        ||
        (vd_get_info.b_request_in_progress == FBE_FALSE)    )
    {
        /* We expect success and there should be a copy request in progress
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x cmd: %d request in progress: %d - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, swap_command,
                               vd_get_info.b_request_in_progress, status);
        fbe_semaphore_destroy(&wait_for_complete_sem);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Wait for the virtual drive to acknowledge the update virtual drive
     * non-paged metadata.
     */
    while ((vd_get_info.b_request_in_progress == FBE_TRUE)                    &&
           (((swap_command == FBE_SPARE_COMPLETE_COPY_COMMAND)      &&
             (vd_get_info.b_copy_complete_in_progress == FBE_FALSE)    ) ||
            ((swap_command == FBE_SPARE_ABORT_COPY_COMMAND)      &&  
             (vd_get_info.b_copy_failed_in_progress == FBE_FALSE)    )      ) &&
           (total_secs_waited < total_secs_to_wait)                              )
    {
        /* Wait (1) cycle and then check lifecycle state.
         */
        fbe_semaphore_wait_ms(&wait_for_complete_sem, (wait_secs * 1000));
        total_secs_waited += wait_secs;

        /* Check if it is still in progress
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                         &vd_get_info,
                                                         sizeof (fbe_virtual_drive_get_info_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         vd_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if ((status != FBE_STATUS_OK)                        ||
            (vd_get_info.b_request_in_progress == FBE_FALSE)    )
        {
            /* We expect success and there should be a copy request in progress
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x cmd: %d request in progress: %d - status: 0x%x\n",
                                   __FUNCTION__, vd_object_id, swap_command,
                                   vd_get_info.b_request_in_progress, status);

            /* Free the semaphore and return an error.
             */
            fbe_semaphore_destroy(&wait_for_complete_sem);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    } /* end while request is not complete */

    /* Check for success
     */
    if  ((vd_get_info.b_request_in_progress == FBE_FALSE)                   ||
         (((swap_command == FBE_SPARE_COMPLETE_COPY_COMMAND)      &&
           (vd_get_info.b_copy_complete_in_progress == FBE_FALSE)    ) ||
          ((swap_command == FBE_SPARE_ABORT_COPY_COMMAND)      &&  
           (vd_get_info.b_copy_failed_in_progress == FBE_FALSE)    )      )     )
    {
        /* Since this is a recoverable (albeit the copy request failed) error
         * this is a warning not an error.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x cmd: %d progress: %d complete: %d failed: %d waited: %d seconds - failed\n",
                               __FUNCTION__, vd_object_id, swap_command,
                               vd_get_info.b_request_in_progress, 
                               vd_get_info.b_copy_complete_in_progress ,
                               vd_get_info.b_copy_failed_in_progress,
                               total_secs_waited);

        status = FBE_STATUS_TIMEOUT;
    }
    else
    {
        /* Report success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: copy compete or failed vd obj: 0x%x cmd: %d waited: %d seconds \n",
                               vd_object_id, swap_command,
                               total_secs_waited);

        status = FBE_STATUS_OK;
    }

    /* Free the semaphore.
     */
    fbe_semaphore_destroy(&wait_for_complete_sem);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_wait_for_copy_complete_or_failed()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_set_vd_checkpoint_to_end_marker()
 *****************************************************************************
 *
 * @brief   Based on the swap command (either `copy complete' or `abort copy')
 *          send the usurper to the virtual drive that will set the checkpoints
 *          appropriately (send the destination to the end marker if the copy
 *          is complete etc).  Since setting the checkpoint to the end marker
 *          requires multiple conditions (quiesce, unquiesce, etc) we need to
 *          `wait' for the completion.

 * @param   vd_object_id - Object id of the virtual drive to update non-paged
 *              for.
 * @param   swap_command - The type of swap request (copy complete or copy failed)
 * @param   b_operation_confirmation - FBE_TRUE - wait for virtual drive
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/22/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_utils_set_vd_checkpoint_to_end_marker(fbe_object_id_t vd_object_id,
                                                                 fbe_spare_swap_command_t swap_command,
                                                                 fbe_bool_t b_operation_confirmation)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_virtual_drive_control_code_t    vd_control_code = FBE_VIRTUAL_DRIVE_CONTROL_CODE_INVALID;

    /* Populate the command based on the swap command
     */
    switch (swap_command)
    {
        case FBE_SPARE_COMPLETE_COPY_COMMAND:
            /* Set the checkpoint to the end marker after the copy completed
             * successfully.
             */
            vd_control_code = FBE_VIRTUAL_DRIVE_CONTROL_CODE_COPY_COMPLETE_SET_CHECKPOINT_TO_END_MARKER;
            break;

        case FBE_SPARE_ABORT_COPY_COMMAND:
            /* Set the checkpoint to the end marker after the copy was
             * aborted.
             */
            vd_control_code = FBE_VIRTUAL_DRIVE_CONTROL_CODE_COPY_FAILED_SET_CHECKPOINT_TO_END_MARKER;
            break;

        default:
            /* The only supported commands are `copy complete' or `abort copy'
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x cmd: %d not supported\n",
                                   __FUNCTION__, vd_object_id, swap_command);

            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send the control packet then wait for its completion. 
     */
    status = fbe_spare_lib_utils_send_control_packet(vd_control_code,
                                                     NULL/* not used */,
                                                     -1/* NOT USED */,
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* If the request failed something is wrong
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x cmd: %d usurper failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, swap_command, status);
        return status;
    }

    /* If enabled wait for the virtual drive to acknowledge the request.
     */
    if (b_operation_confirmation)
    {
        status = fbe_spare_lib_utils_wait_for_copy_complete_or_failed(vd_object_id, swap_command);
    }

    /* Return the result of waiting for the completion.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_set_vd_checkpoint_to_end_marker()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_wait_for_user_copy_start()
 ***************************************************************************** 
 * 
 * @brief   This method waits for the virtual drive (only on the SP where the
 *          job service is running) to start the user copy request.
 *
 * @param   vd_object_id  - Object id for virtual drive or spare drive
 * @param   user_copy_command - Type of user copy requested
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/22/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_utils_wait_for_user_copy_start(fbe_object_id_t vd_object_id,
                                                                 fbe_spare_swap_command_t user_copy_command)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_virtual_drive_get_info_t    vd_get_info;
    fbe_u32_t                       wait_for_user_copy_secs = fbe_spare_main_get_operation_timeout_secs();
    fbe_semaphore_t                 wait_for_user_copy_sem;
    fbe_u32_t                       wait_secs = 1;
    fbe_u32_t                       total_secs_to_wait;
    fbe_u32_t                       total_secs_waited = 0;

    /* Only the following commands are supported
     */
    switch (user_copy_command)
    {
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* Supported
             */
            break;

        default:
            /* Unsupported
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x swap command: %d not supported\n",
                                   __FUNCTION__, vd_object_id, user_copy_command);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now wait at least (6) seconds for the virtual drive to complete the 
     * request
     */
    fbe_semaphore_init(&wait_for_user_copy_sem, 0, 1);
    if (wait_for_user_copy_secs < 6)
    {
        total_secs_to_wait = 6;
    }
    else
    {
        total_secs_to_wait = wait_for_user_copy_secs;
    }

    /* Get the virtual drive information and the edge info
     * Since we just issued the request, wait a short period before
     * we check.
     */
    fbe_semaphore_wait_ms(&wait_for_user_copy_sem, 100);
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                     &vd_get_info,
                                                     sizeof (fbe_virtual_drive_get_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* We expect success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x cmd: %d get info failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, user_copy_command,
                               status);
        fbe_semaphore_destroy(&wait_for_user_copy_sem);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Trace some information.
     */
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare: vd obj: 0x%x cmd: %d wait up to: %d seconds for user copy start\n",
                           vd_object_id, user_copy_command, wait_for_user_copy_secs);

    /* Wait for the virtual drive to acknowledge that the user copy has started.
     */
    while ((vd_get_info.b_user_copy_started == FBE_FALSE) &&
           (total_secs_waited < total_secs_to_wait)           )
    {
        /* Wait (1) cycle and then check lifecycle state.
         */
        fbe_semaphore_wait_ms(&wait_for_user_copy_sem, (wait_secs * 1000));
        total_secs_waited += wait_secs;

        /* Get the virtual drive information and the edge info
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                         &vd_get_info,
                                                         sizeof (fbe_virtual_drive_get_info_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         vd_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            /* We expect success
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x cmd: %d get info failed - status: 0x%x\n",
                                   __FUNCTION__, vd_object_id, user_copy_command,
                                   status);

            /* Free the semaphore and return an error.
             */
            fbe_semaphore_destroy(&wait_for_user_copy_sem);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    } /* end while request is not complete */

    /* Check for success
     */
    if  (vd_get_info.b_user_copy_started == FBE_FALSE)
    {
        /* Report the error
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x cmd: %d waited: %d seconds - failed\n",
                               __FUNCTION__, vd_object_id, user_copy_command,
                               total_secs_waited);

        status = FBE_STATUS_TIMEOUT;
    }
    else
    {
        /* Report success
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: swap in vd obj: 0x%x cmd: %d waited: %d seconds\n",
                               vd_object_id, user_copy_command,
                               total_secs_waited);

        status = FBE_STATUS_OK;
    }

    /* Free the semaphore.
     */
    fbe_semaphore_destroy(&wait_for_user_copy_sem);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_wait_for_user_copy_start()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_start_user_copy_request()
 *****************************************************************************
 *
 * @brief   Send a usurper to the virtual drive to `start' a user copy
 *          operation.
 * @param   vd_object_id - Object id of the virtual drive to update non-paged
 *              for.
 * @param   swap_command - The type of swap request (copy complete or copy failed)
 * @param   b_operation_confirmation - FBE_TRUE - wait for virtual drive
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/26/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_utils_start_user_copy_request(fbe_object_id_t vd_object_id,
                                                         fbe_spare_swap_command_t swap_command,
                                                         fbe_edge_index_t swap_edge_index,
                                                         fbe_bool_t b_operation_confirmation)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_initiate_user_copy_t  virtual_drive_initiate_user_copy;

    /* If this is a user copy we must send a usurper to the the virtual drive
     * to `initiate' the copy request.
     */
    switch(swap_command)
    {
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* These commands are suppoted
             */
            break;

        default:
            /* The only supported commands are `user copy' or `user copy to'
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: vd obj: 0x%x cmd: %d not supported\n",
                                   __FUNCTION__, vd_object_id, swap_command);

            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy the fields from the swap request
     */
    fbe_zero_memory(&virtual_drive_initiate_user_copy, sizeof(fbe_virtual_drive_initiate_user_copy_t));
    virtual_drive_initiate_user_copy.command_type = swap_command;
    virtual_drive_initiate_user_copy.swap_edge_index = swap_edge_index;
    virtual_drive_initiate_user_copy.b_operation_confirmation = b_operation_confirmation;

    /* Inform virtual drive that we are `initiating' a user copy request.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_INITIATE_USER_COPY,
                                                     &virtual_drive_initiate_user_copy,
                                                     sizeof(fbe_virtual_drive_initiate_user_copy_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: initiate user copy failed - status:%d\n", 
                               __FUNCTION__, status);

        return status;
    }

    /* If enabled wait for the virtual drive to acknowledge the request.
     */
    if (b_operation_confirmation)
    {
        status = fbe_spare_lib_utils_wait_for_user_copy_start(vd_object_id, swap_command);
    }

    /* Return the status
     */
    return status;
}
/****************************************************
 * end fbe_spare_lib_utils_start_user_copy_request()
 ****************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_send_job_complete()
 *****************************************************************************
 *
 * @brief   Send a usurper to the virtual drive to notify it that the swap
 *          job is complete.
 * 
 * @param   vd_object_id - Object id of the virtual drive to update non-paged
 *              for.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  10/11/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_utils_send_job_complete(fbe_object_id_t vd_object_id)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Send the control packet then wait for its completion. 
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_SEND_SWAP_COMMAND_COMPLETE,
                                                     NULL/* not used */,
                                                     -1/* NOT USED */,
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* If the request failed something is wrong
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x usurper failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
    }

    /* Return the status
     */
    return status;
}
/****************************************************
 * end fbe_spare_lib_utils_send_job_complete()
 ****************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_utils_send_swap_request_rollback()
 *****************************************************************************
 *
 * @brief   Send a usurper to the virtual drive to notify it that the swap
 *          request failed.  The virtual drive needs to perform any cleanup
 *          (i.e. rollback) necessary.
 * 
 * @param   vd_object_id - Object id of the virtual drive to cleanup.
 * @param   js_swap_request_p - Pointer to swap request used to populate
 *              rollback request.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  05/21/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_utils_send_swap_request_rollback(fbe_object_id_t vd_object_id,
                                                            fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_virtual_drive_control_swap_request_rollback_t swap_request_rollback;

    /* Send the control packet then wait for its completion. 
     */
    swap_request_rollback.orig_pvd_object_id = js_swap_request_p->orig_pvd_object_id;
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_REQUEST_ROLLBACK,
                                                     &swap_request_rollback,
                                                     sizeof (fbe_virtual_drive_control_swap_request_rollback_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        /* If the request failed something is wrong
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x usurper failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
    }

    /* Return the status
     */
    return status;
}
/******************************************************
 * end fbe_spare_lib_utils_send_swap_request_rollback()
 ******************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_database_service_mark_pvd_swap_pending()
 ******************************************************************************
 * @brief
 * This function is used to mark the provision drive that is about to be swapped
 * out `pending'.  This is done when encryption is enabled because the drive scrub
 * process will immediately zero the data (unless the `swap pending' flag is set).
 *
 * @param   js_swap_request_p - Pointer to swap request
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_database_service_mark_pvd_swap_pending(fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_spare_swap_command_t                    swap_command = FBE_SPARE_COMPLETE_COPY_COMMAND;   
    fbe_spare_swap_command_t                    original_swap_command = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND; 
    fbe_bool_t                                  b_is_proactive_copy = FBE_TRUE;  
    fbe_object_id_t                             server_object_id = FBE_OBJECT_ID_INVALID;
    fbe_database_control_mark_pvd_swap_pending_t mark_pvd_swap_pending;

    /* Only need to mark swap pending during a copy complete.
     */
    fbe_job_service_drive_swap_request_get_swap_command_type(js_swap_request_p, &swap_command);
    if (swap_command != FBE_SPARE_COMPLETE_COPY_COMMAND) 
    {
        return FBE_STATUS_OK;
    }

    /* Determine the original swap command.
     */
    fbe_job_service_drive_swap_request_get_original_pvd_object_id(js_swap_request_p, &server_object_id);
    fbe_job_service_drive_swap_request_get_is_proactive_copy(js_swap_request_p, &b_is_proactive_copy);
    if (b_is_proactive_copy == FBE_FALSE)
    {
        original_swap_command = FBE_SPARE_INITIATE_USER_COPY_COMMAND;
    }

    /* Always mark swap-pending.
     */
    fbe_zero_memory(&mark_pvd_swap_pending, sizeof(fbe_database_control_mark_pvd_swap_pending_t));
    mark_pvd_swap_pending.object_id = server_object_id;
    mark_pvd_swap_pending.swap_command = original_swap_command;
    mark_pvd_swap_pending.transaction_id = js_swap_request_p->transaction_id;

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare_mark_pvd_swap_pending: cmd: %d pvd obj: 0x%x trans id: 0x%llx\n",
                           original_swap_command, server_object_id, js_swap_request_p->transaction_id);

    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_MARK_PVD_SWAP_PENDING,
                                                     &mark_pvd_swap_pending,
                                                     sizeof(fbe_database_control_mark_pvd_swap_pending_t),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_mark_pvd_swap_pending: DATABASE_MARK_PVD_SWAP_PENDING failed\n");
        return status;
    }
    
    /* Return the status
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_database_service_mark_pvd_swap_pending()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_utils_database_service_clear_pvd_swap_pending()
 ******************************************************************************
 * @brief
 * This function is used to clear the `mark offline' status of a provision drive
 * that was swapped out but now needs to be brought online (for proactive copy we
 * do not bring the drive back since it EOL).
 *
 * @param   js_swap_request_p - Pointer to swap request
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 ******************************************************************************/
fbe_status_t fbe_spare_lib_utils_database_service_clear_pvd_swap_pending(fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_spare_swap_command_t                    swap_command = FBE_SPARE_COMPLETE_COPY_COMMAND;   
    fbe_spare_swap_command_t                    original_swap_command = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND; 
    fbe_bool_t                                  b_is_proactive_copy = FBE_TRUE;  
    fbe_object_id_t                             server_object_id = FBE_OBJECT_ID_INVALID;
    fbe_database_control_clear_pvd_swap_pending_t clear_pvd_swap_pending; 

    /* Only need to mark swap pending during a copy complete.
     */
    fbe_job_service_drive_swap_request_get_swap_command_type(js_swap_request_p, &swap_command);
    if (swap_command != FBE_SPARE_COMPLETE_COPY_COMMAND) 
    {
        return FBE_STATUS_OK;
    }

    /* Determine the original swap command.
     */
    fbe_job_service_drive_swap_request_get_original_pvd_object_id(js_swap_request_p, &server_object_id);
    fbe_job_service_drive_swap_request_get_is_proactive_copy(js_swap_request_p, &b_is_proactive_copy);
    if (b_is_proactive_copy == FBE_FALSE)
    {
        original_swap_command = FBE_SPARE_INITIATE_USER_COPY_COMMAND;
    }

    /* Clear the swap-pending NP flag.
     */
    fbe_zero_memory(&clear_pvd_swap_pending, sizeof(fbe_database_control_clear_pvd_swap_pending_t));
    clear_pvd_swap_pending.object_id = server_object_id;
    clear_pvd_swap_pending.swap_command = original_swap_command;
    clear_pvd_swap_pending.transaction_id = js_swap_request_p->transaction_id;

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "spare_clear_pvd_swap_pending: cmd: %d pvd obj: 0x%x trans id: 0x%llx\n",
                           original_swap_command, server_object_id, js_swap_request_p->transaction_id);

    status = fbe_spare_lib_utils_send_control_packet(FBE_DATABASE_CONTROL_CLEAR_PVD_SWAP_PENDING,
                                                     &clear_pvd_swap_pending,
                                                     sizeof(fbe_database_control_clear_pvd_swap_pending_t),
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_clear_pvd_swap_pending: DATABASE_CLEAR_OFFLINE failed\n");
        return status;
    }

    /* Return the status
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_utils_database_service_clear_pvd_swap_pending()
 ******************************************************************************/


/************************* 
 * end fbe_spare_utils.c
 *************************/
