/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_block_transport_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_block_transport interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_block_transport_interface
 *
 * @version
 *   11/05/2009:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_block_transport_interface.h"

#include "fbe/fbe_data_pattern.h"
#include "fbe_block_transport.h"

/**************************************
                Local structure
**************************************/
#define FBE_API_BLOCK_TRANSPORT_SG_COUNT_FOR_SUB_OPERATIONS  2

struct fbe_api_block_transport_sub_operation_structure_s;

typedef struct fbe_api_block_transport_split_io_context_s
{
    /* user request */
    fbe_payload_block_operation_opcode_t block_op;
    fbe_object_id_t   object_id;
    fbe_package_id_t  package_id;
    fbe_u8_t         *original_data;
    fbe_lba_t         start_lba; 
    fbe_lba_t         data_length;
    fbe_u32_t         queue_depth;
    fbe_block_count_t io_size;
    /* data related to local operations */
    struct fbe_api_block_transport_sub_operation_structure_s  *op_array;
    fbe_lba_t         current_offset;
    fbe_u32_t         outstanding_io_count;
    fbe_status_t      completion_status;
    fbe_spinlock_t    context_lock;
    fbe_semaphore_t   context_completed;
}fbe_api_block_transport_split_io_context_t;

typedef struct fbe_api_block_transport_sub_operation_structure_s
{
    fbe_packet_t							   *sub_packet;
    fbe_api_block_transport_split_io_context_t *io_context;
    fbe_block_transport_block_operation_info_t	block_op_info;
	fbe_block_transport_io_operation_info_t		io_info;
    fbe_sg_element_t							sg_elements[FBE_API_BLOCK_TRANSPORT_SG_COUNT_FOR_SUB_OPERATIONS];
}fbe_api_block_transport_sub_operation_structure_t;

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/
static fbe_status_t 
fbe_api_block_transport_initialize_split_io_context(fbe_api_block_transport_split_io_context_t *io_context,
                                                    fbe_payload_block_operation_opcode_t block_op,
                                                    fbe_object_id_t   object_id,
                                                    fbe_package_id_t  package_id,
                                                    fbe_u8_t         *original_data,
                                                    fbe_lba_t         data_length,
                                                    fbe_lba_t         start_lba, 
                                                    fbe_u32_t         queue_depth,
                                                    fbe_block_count_t io_size);
static fbe_status_t 
fbe_api_block_transport_destroy_split_io_context(fbe_api_block_transport_split_io_context_t *io_context);

static fbe_status_t 
fbe_api_block_transport_sub_operation_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t 
fbe_api_block_transport_io_operation_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t 
fbe_api_block_transport_setup_next_operation(fbe_api_block_transport_split_io_context_t *io_context,
                                             fbe_api_block_transport_sub_operation_structure_t *current_op);

static fbe_status_t 
fbe_api_block_transport_split_block_operation(fbe_object_id_t   object_id, 
                                              fbe_package_id_t  package_id,
                                              fbe_payload_block_operation_opcode_t block_op,
                                              fbe_u8_t        * data, 
                                              fbe_lba_t         data_length, 
                                              fbe_lba_t         start_lba, 
                                              fbe_block_count_t io_size,
                                              fbe_u32_t         queue_depth);

/*!***************************************************************
 * @fn fbe_api_block_transport_create_edge(
 *      fbe_object_id_t client_object_id,
 *      fbe_api_block_transport_control_create_edge_t *edge,
 *      fbe_packet_attr_t attribute)
 ****************************************************************
 * @brief
 *  This function create edge with the given params.
 *
 * @param client_object_id - client object ID
 * @param edge - pointer to the create edge
 * @param attribute - attribute
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/05/09 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_block_transport_create_edge(fbe_object_id_t client_object_id,
                                    fbe_api_block_transport_control_create_edge_t *edge,
                                    fbe_packet_attr_t attribute)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_block_transport_control_create_edge_t       create_edge;

    if (edge == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    create_edge.server_id = edge->server_id;
    create_edge.client_index = edge->client_index;
    create_edge.capacity = edge->capacity;
    create_edge.offset = edge->offset;
    create_edge.edge_flags = edge->edge_flags;

    status = fbe_api_common_send_control_packet (FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE,
                                                 &create_edge,
                                                 sizeof(fbe_block_transport_control_create_edge_t),
                                                 client_object_id,
                                                 attribute,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/**************************************
 * end fbe_api_block_transport_create_edge()
 **************************************/

/*!***************************************************************************
 * fbe_api_block_transport_send_block_operation()
 *****************************************************************************
 *
 * @brief   This method sends the block operation to raid group object.
 *
 * @param   object_id - Base config Object 
 * @param   package_id - Package id
 * @param   block_op_info_p - Ptr to the block payload struct.
 * @param   sg_list - the pointer to the sg list allocated while sending cmd.
*
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/01/2010  Swati Fursule  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_block_transport_send_block_operation(fbe_object_id_t object_id, 
                               fbe_package_id_t package_id,
                               fbe_block_transport_block_operation_info_t *block_op_info_p, 
                               fbe_sg_element_t *sg_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sg_element_t                        sg_elements[FBE_DATA_PATTERN_MAX_SG_DATA_ELEMENTS+1] = {0};
    fbe_sg_element_t *                      temp_sg_ptr = sg_p;
    fbe_u8_t                                sg_count = 0;
    while (temp_sg_ptr != NULL)
    {
        if((temp_sg_ptr->count == 0 ) || 
           (temp_sg_ptr->address == NULL))
        {
            break;
        }
        /* FBE_API_MAX_SG_DATA_ELEMENTS buffer element + 1 termination element = 2*/
        /* set next sg element size and address for the response buffer*/
        fbe_sg_element_init(&sg_elements[sg_count], 
                            temp_sg_ptr->count, 
                            temp_sg_ptr->address);

        sg_count++;
        temp_sg_ptr++;
    }
    /* no more elements*/
    fbe_sg_element_terminate(&sg_elements[sg_count]);
    sg_count++;
    /* send FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND
     * usurper command to base config object
     */

	/* To support legacy behavior we need to "unmark" is_checksum_requiered */
	block_op_info_p->is_checksum_requiered = FBE_FALSE;

    status = fbe_api_common_send_control_packet_with_sg_list(
        FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND,
        block_op_info_p,
        sizeof (fbe_block_transport_block_operation_info_t),
        object_id,
        FBE_PACKET_FLAG_NO_ATTRIB,
        (fbe_sg_element_t*)sg_elements,
        sg_count,
        &status_info,
        package_id);

    if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: line: %d packet error:%d, packet qualifier:%d,"
                      "payload error:%d, payload qualifier:%d\n", 
                      __FUNCTION__, __LINE__,
                      status, status_info.packet_qualifier, 
                      status_info.control_operation_status, 
                      status_info.control_operation_qualifier);
    }
    /* Only change the status we return if the block status is bad. 
     * Otherwise we return the packet status. 
     */
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    /* Always return the execution status.
     */
    return(status);
}
/************************************************
 * end fbe_api_block_transport_send_block_operation()
 ************************************************/

/*!***************************************************************************
 * fbe_api_block_transport_negotiate_info()
 *****************************************************************************
 *
 * @brief   This method gets negotiate information for block transport.
 *
 * @param   object_id - Base config Object 
 * @param   package_id - Package id
 * @param   negotiate_info_p - Ptr to the negotiate info structure.
*
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/27/2010  Swati Fursule  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_block_transport_negotiate_info(fbe_object_id_t object_id, 
                               fbe_package_id_t package_id,
                               fbe_block_transport_negotiate_t *negotiate_info_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    /* send FBE_BLOCK_TRANSPORT_CONTROL_CODE_NEGOTIATE_BLOCK_SIZE
     * usurper command to base config object
     */
    status = fbe_api_common_send_control_packet(
        FBE_BLOCK_TRANSPORT_CONTROL_CODE_NEGOTIATE_BLOCK_SIZE,
        negotiate_info_p,
        sizeof (fbe_block_transport_negotiate_t),
        object_id,
        FBE_PACKET_FLAG_NO_ATTRIB,
        &status_info,
        package_id);

    if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: line: %d packet error:%d, packet qualifier:%d,"
                      "payload error:%d, payload qualifier:%d\n", 
                      __FUNCTION__, __LINE__,
                      status, status_info.packet_qualifier, 
                      status_info.control_operation_status, 
                      status_info.control_operation_qualifier);
        status = FBE_STATUS_GENERIC_FAILURE;
    }


    /* Always return the execution status.
     */
    return(status);
}

/*!***************************************************************************
 * fbe_api_block_transport_default_trace_func()
 *****************************************************************************
 *
 * @brief   This method traces the information.
 *
 * @param   trace_context - 
 * @param   fmt - formatted string
*
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/01/2010  Swati Fursule  - Created.
 *
 *****************************************************************************/
static void fbe_api_block_transport_default_trace_func(
    fbe_trace_context_t trace_context, const fbe_char_t * fmt, ...)
    __attribute__((format(__printf_func__,2,3)));

static void fbe_api_block_transport_default_trace_func(
    fbe_trace_context_t trace_context, const fbe_char_t * fmt, ...)
{
    va_list ap;
    fbe_char_t string_to_print[256];

    va_start(ap, fmt);
    _vsnprintf(string_to_print, (sizeof(string_to_print) - 1), fmt, ap);
    va_end(ap);
    fbe_printf("%s", string_to_print);
    return;
}

/*!***************************************************************************
 *          fbe_api_block_transport_status_to_string
 *****************************************************************************
 * 
 * @brief   Returns a description for the block operation status
 *  
 * @param   status  - operation status that the description is required for.
 * @param   status_string_p - Pointer of status string to update
 * @param   string_length - Length of input string
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  08/31/2010: Swati Fursule - Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_block_transport_status_to_string(
                            fbe_payload_block_operation_status_t status,
                            fbe_u8_t* status_string_p,
                            fbe_u32_t string_length)
{
    fbe_status_t    ret_status = FBE_STATUS_OK;
    fbe_u8_t       *ret_string;
    
    /* The goal of all this code is to have a single location that 
     * needs to change when a flag is added or removed!!
     */
    switch(status)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID:
            ret_string = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
            ret_string = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST";
            break;
        default:
            ret_string = "Invalid Value";
            break;
    }

    /* Return the description string and status
     */
    if (strlen(ret_string) >= string_length)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    strncpy(status_string_p, ret_string, strlen(status_string_p) + 1); 

    fbe_api_block_transport_default_trace_func(NULL,
        "Status Description: %d  %s \n",
        status, ret_string);
    return(ret_status);
} /* fbe_api_block_transport_status_to_string */

/*!***************************************************************************
 *          fbe_api_block_transport_qualifier_to_string
 *****************************************************************************
 * 
 * @brief   Returns a description for the block operation qualifier
 *  
 * @param   qualifier  - operation qualifier that the description is required for.
 * @param   status_string_p - Pointer of qualifier string to update
 * @param   string_length - Length of input string
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  08/31/2010: Swati Fursule - Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_block_transport_qualifier_to_string(
                            fbe_payload_block_operation_qualifier_t qualifier,
                            fbe_u8_t* qualifier_string_p,
                            fbe_u32_t string_length)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u8_t       *ret_string;
    
    /* The goal of all this code is to have a single location that 
     * needs to change when a flag is added or removed!!
     */
    switch(qualifier)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID:
            ret_string = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE:
            ret_string = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_OPTIONAL_ABORTED_LEGACY:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_OPTIONAL_ABORTED_LEGACY";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG:
            ret_string = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED:
            ret_string
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INSUFFICIENT_RESOURCES:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INSUFFICIENT_RESOURCES";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR_SG:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR_SG";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MINIPORT_ABORTED_IO:
            ret_string 
            = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MINIPORT_ABORTED_IO";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED:
            ret_string = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR:
            ret_string = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR";
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_ZERO_REQUEST:
            ret_string = "FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_ZERO_REQUEST";
            break;
        default:
            ret_string = "Invalid Value";
            break;
    }

    /* Return the description string and status
     */
    if (strlen(ret_string) >= string_length)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    strncpy(qualifier_string_p, ret_string, strlen(qualifier_string_p) + 1); 

    fbe_api_block_transport_default_trace_func(NULL,
        "Qualifier Description: %d  %s \n",
        qualifier, ret_string);

    return(status);
} /* fbe_api_block_transport_qualifier_to_string */

/*!***************************************************************
 * @fn fbe_api_block_transport_get_throttle_info()
 ****************************************************************
 * @brief
 *  Fetch the parameters for throttling the object.
 *
 * @param client_object_id - client object ID
 * @param package_id - 
 * @param get_throttle_info - 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  3/20/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_block_transport_get_throttle_info(fbe_object_id_t client_object_id,
                                          fbe_package_id_t package_id,
                                          fbe_block_transport_get_throttle_info_t *get_throttle_info)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (get_throttle_info == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_THROTTLE_INFO,
                                                 get_throttle_info,
                                                 sizeof(fbe_block_transport_get_throttle_info_t),
                                                 client_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 package_id);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/**************************************
 * end fbe_api_block_transport_get_throttle_info()
 **************************************/
/*!***************************************************************
 * @fn fbe_api_block_transport_set_throttle_info()
 ****************************************************************
 * @brief
 *  Set the parameters for throttling the object.
 *
 * @param client_object_id - client object ID
 * @param edge - pointer to the create edge
 * @param attribute - attribute
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  3/20/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_block_transport_set_throttle_info(fbe_object_id_t client_object_id,
                                          fbe_block_transport_set_throttle_info_t *set_throttle_info)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (set_throttle_info == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BLOCK_TRANSPORT_CONTROL_CODE_SET_THROTTLE_INFO,
                                                 set_throttle_info,
                                                 sizeof(fbe_block_transport_set_throttle_info_t),
                                                 client_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/**************************************
 * end fbe_api_block_transport_set_throttle_info()
 **************************************/

/*!***************************************************************************
 * fbe_api_block_transport_read_data()
 *****************************************************************************
 *
 * @brief   This method build read block operations with 
 *          limited io size and q depth, and send to object.
 *
 * @param   object_id   - Object 
 * @param   package_id  - Package id
 * @param   data        - Ptr to user data.
 * @param   data_length - size of user data.
 * @param   io_size     - size of the generated IO.
 * @param   sg_list     - the pointer to the sg list allocated while sending cmd.
 * @param   queue_depth - number of concurrent IO.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/02/2013  nchiu  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_block_transport_read_data(fbe_object_id_t   object_id, 
                                  fbe_package_id_t  package_id,
                                  fbe_u8_t        * data, 
                                  fbe_lba_t         data_length, 
                                  fbe_lba_t         start_lba, 
                                  fbe_block_count_t io_size,
                                  fbe_u32_t         queue_depth)
{
    fbe_status_t status;
    status = fbe_api_block_transport_split_block_operation(object_id,
                                                           package_id,
                                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                           data,
                                                           data_length,
                                                           start_lba,
                                                           io_size,
                                                           queue_depth);
    return status;
}
/**************************************
 * end fbe_api_block_transport_read_data()
 **************************************/

/*!***************************************************************************
 * fbe_api_block_transport_write_data()
 *****************************************************************************
 *
 * @brief   This method build write block operations with 
 *          limited io size and q depth, and send to object.
 *
 * @param   object_id   - Object 
 * @param   package_id  - Package id
 * @param   data        - Ptr to user data.
 * @param   data_length - size of user data.
 * @param   io_size     - size of the generated IO.
 * @param   sg_list     - the pointer to the sg list allocated while sending cmd.
 * @param   queue_depth - number of concurrent IO.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/02/2013  nchiu  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_block_transport_write_data(fbe_object_id_t   object_id, 
                                   fbe_package_id_t  package_id,
                                   fbe_u8_t        * data, 
                                   fbe_lba_t         data_length, 
                                   fbe_lba_t         start_lba, 
                                   fbe_block_count_t io_size,
                                   fbe_u32_t         queue_depth)
{
    fbe_status_t status;
    status = fbe_api_block_transport_split_block_operation(object_id,
                                                           package_id,
                                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                                           data,
                                                           data_length,
                                                           start_lba,
                                                           io_size,
                                                           queue_depth);
    return status;
}
/**************************************
 * end fbe_api_block_transport_write_data()
 **************************************/

/*!***************************************************************************
 * fbe_api_block_transport_split_block_operation()
 *****************************************************************************
 *
 * @brief   This method build block operations with 
 *          limited io size and q depth, and send to object.
 *
 * @param   object_id   - Object 
 * @param   package_id  - Package id
 * @param   block_op    - block operation
 * @param   data        - Ptr to user data.
 * @param   data_length - size of user data.
 * @param   io_size     - size of the generated IO.
 * @param   sg_list     - the pointer to the sg list allocated while sending cmd.
 * @param   queue_depth - number of concurrent IO.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/02/2013  nchiu  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_api_block_transport_split_block_operation(fbe_object_id_t   object_id, 
                                              fbe_package_id_t  package_id,
                                              fbe_payload_block_operation_opcode_t block_op,
                                              fbe_u8_t        * data, 
                                              fbe_lba_t         data_length, 
                                              fbe_lba_t         start_lba, 
                                              fbe_block_count_t io_size,
                                              fbe_u32_t         queue_depth)
{
    fbe_status_t status;
    fbe_api_block_transport_split_io_context_t io_context;
    fbe_u32_t index;
    fbe_u32_t io_count; 
    status = fbe_api_block_transport_initialize_split_io_context(&io_context, 
                                                                 block_op,
                                                                 object_id,
                                                                 package_id,
                                                                 data,
                                                                 data_length,
                                                                 start_lba,
                                                                 queue_depth,
                                                                 io_size);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s: failed to initialize io context, status:%d\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* set up all outstanding operations */
    while ((io_context.current_offset < data_length) && 
           (io_context.outstanding_io_count < queue_depth))
    {
        fbe_api_block_transport_setup_next_operation(&io_context,
                                                     &io_context.op_array[io_context.outstanding_io_count]);
    }
    io_count = io_context.outstanding_io_count;

    /* dispatch all operations */
    for(index = 0; index < io_count; index ++)
    {
#if 0
        fbe_api_common_send_control_packet_with_sg_list_async(io_context.op_array[index].sub_packet,
                                                            FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND,
                                                            &io_context.op_array[index].block_op_info,
                                                            sizeof (fbe_block_transport_block_operation_info_t),
                                                            object_id,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            (fbe_sg_element_t*)io_context.op_array[index].sg_elements,
                                                            FBE_API_BLOCK_TRANSPORT_SG_COUNT_FOR_SUB_OPERATIONS,
                                                            fbe_api_block_transport_sub_operation_asynch_callback,
                                                            &io_context.op_array[index],
                                                            io_context.package_id);
#else
        fbe_api_common_send_control_packet_with_sg_list_async(io_context.op_array[index].sub_packet,
                                                            FBE_BLOCK_TRANSPORT_CONTROL_CODE_IO_COMMAND,
                                                            &io_context.op_array[index].io_info,
                                                            sizeof (fbe_block_transport_io_operation_info_t),
                                                            object_id,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            (fbe_sg_element_t*)io_context.op_array[index].sg_elements,
                                                            FBE_API_BLOCK_TRANSPORT_SG_COUNT_FOR_SUB_OPERATIONS,
                                                            fbe_api_block_transport_io_operation_asynch_callback,
                                                            &io_context.op_array[index],
                                                            io_context.package_id);

#endif
    }

    /* we have to wait if we send anything out */
    if (index > 0)
    {
        fbe_semaphore_wait(&io_context.context_completed, NULL);
    }

    /* destroy io context */
    fbe_api_block_transport_destroy_split_io_context(&io_context);

    return io_context.completion_status;
}

/*!***************************************************************************
 * fbe_api_block_transport_setup_next_operation()
 *****************************************************************************
 *
 * @brief   This method build the next sub block operations 
 *
 * @param   io_context   - io tracking context 
 * @param   current_op   - current operation
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/02/2013  nchiu  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_api_block_transport_setup_next_operation(fbe_api_block_transport_split_io_context_t *io_context,
                                             fbe_api_block_transport_sub_operation_structure_t *current_op)
{
    fbe_lba_t current_op_size;
    fbe_lba_t current_lba;

        
    current_op_size = io_context->data_length - io_context->current_offset;
    if (current_op_size > io_context->io_size)
    {
        current_op_size = io_context->io_size;
    }
    current_lba = io_context->start_lba + io_context->current_offset;

    current_op->sg_elements[0].address = io_context->original_data + io_context->current_offset * FBE_BE_BYTES_PER_BLOCK;
    current_op->sg_elements[0].count   = (fbe_u32_t)current_op_size * FBE_BE_BYTES_PER_BLOCK;
       
    io_context->current_offset += current_op_size;
    io_context->outstanding_io_count ++;

	/* Inform MCR that checksum is needed */
	if(io_context->block_op == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE){
		current_op->block_op_info.is_checksum_requiered = FBE_TRUE;
		current_op->io_info.is_checksum_requiered = FBE_TRUE;
	}

    fbe_payload_block_build_operation(&current_op->block_op_info.block_operation,
                                      io_context->block_op,
                                      current_lba,
                                      current_op_size,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      1,
                                      NULL);

	current_op->io_info.block_opcode = io_context->block_op;
	current_op->io_info.lba = current_lba;
    current_op->io_info.block_count = current_op_size;
    current_op->io_info.block_size = FBE_BE_BYTES_PER_BLOCK;
	current_op->io_info.status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    current_op->io_info.status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;

    fbe_transport_reuse_packet(current_op->sub_packet);

    return FBE_STATUS_OK;
}

/*!***************************************************************************
 * fbe_api_block_transport_initialize_split_io_context()
 *****************************************************************************
 *
 * @brief   This method initializes io tracking context and necessary resource
 *
 * @param   io_context  - io tracking context 
 * @param   object_id   - Object 
 * @param   package_id  - Package id
 * @param   block_op    - block operation
 * @param   data        - Ptr to user data.
 * @param   data_length - size of user data.
 * @param   io_size     - size of the generated IO.
 * @param   sg_list     - the pointer to the sg list allocated while sending cmd.
 * @param   queue_depth - number of concurrent IO.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/02/2013  nchiu  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_api_block_transport_initialize_split_io_context(fbe_api_block_transport_split_io_context_t *io_context,
                                                    fbe_payload_block_operation_opcode_t block_op,
                                                    fbe_object_id_t   object_id,
                                                    fbe_package_id_t  package_id,
                                                    fbe_u8_t         *original_data,
                                                    fbe_lba_t         data_length,
                                                    fbe_lba_t         start_lba, 
                                                    fbe_u32_t         queue_depth,
                                                    fbe_block_count_t io_size)
{
    fbe_u32_t index;

    io_context->op_array = (fbe_api_block_transport_sub_operation_structure_t *)
        fbe_api_allocate_memory( queue_depth * sizeof(fbe_api_block_transport_sub_operation_structure_t));

    if (io_context->op_array == NULL)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    for(index = 0; index < queue_depth; index ++)
    {
        io_context->op_array[index].sub_packet = fbe_api_get_contiguous_packet();
        io_context->op_array[index].io_context = io_context;
        fbe_sg_element_terminate(&io_context->op_array[index].sg_elements[1]);
    }

    io_context->block_op       = block_op;
    io_context->object_id      = object_id;
    io_context->package_id      = package_id,
    io_context->original_data  = original_data;
    io_context->start_lba      = start_lba;
    io_context->data_length    = data_length;
    io_context->queue_depth    = queue_depth;
    io_context->io_size        = io_size;
    io_context->current_offset = 0;
    io_context->outstanding_io_count = 0;
    io_context->completion_status    = FBE_STATUS_OK;

    fbe_spinlock_init(&io_context->context_lock);
    fbe_semaphore_init(&io_context->context_completed, 0, 1);
                                                                                
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 * fbe_api_block_transport_destroy_split_io_context()
 *****************************************************************************
 *
 * @brief   clean up pre-allocated resources.
 *
 * @param   io_context   - io tracking context 

 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/02/2013  nchiu  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_api_block_transport_destroy_split_io_context(fbe_api_block_transport_split_io_context_t *io_context)
{
    fbe_u32_t index;

    for(index = 0; index < io_context->queue_depth; index ++)
    {
        fbe_api_return_contiguous_packet(io_context->op_array[index].sub_packet);
    }

    fbe_api_free_memory(io_context->op_array);
    fbe_spinlock_destroy(&io_context->context_lock);
    fbe_semaphore_destroy(&io_context->context_completed);
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 * fbe_api_block_transport_sub_operation_asynch_callback()
 *****************************************************************************
 *
 * @brief   callback function handles error and dispatching additional IO.
 *
 * @param   packet   - packet 
 * @param   context  - completion context
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/02/2013  nchiu  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_api_block_transport_sub_operation_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_api_block_transport_sub_operation_structure_t *sub_op = (fbe_api_block_transport_sub_operation_structure_t *)context;
    fbe_api_block_transport_split_io_context_t *io_context = sub_op->io_context;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    /* we need to handle error retry later
     * we also need to handle error status where raid is overloaded 
     */
    if ((status != FBE_STATUS_OK) || 
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:status:%d, payload status:%d & qulf:%d\n", __FUNCTION__,
                       status, control_status, control_status_qualifier);

        io_context->completion_status = FBE_STATUS_GENERIC_FAILURE;
    }      

    fbe_spinlock_lock(&io_context->context_lock);

    io_context->outstanding_io_count --;

    if (io_context->data_length > io_context->current_offset)
    {
        fbe_api_block_transport_setup_next_operation(io_context, sub_op);
        fbe_spinlock_unlock(&io_context->context_lock);

        fbe_api_common_send_control_packet_with_sg_list_async(sub_op->sub_packet,
                                                            FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND,
                                                            &(sub_op->block_op_info),
                                                            sizeof (fbe_block_transport_block_operation_info_t),
                                                            io_context->object_id,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            (fbe_sg_element_t*)sub_op->sg_elements,
                                                            FBE_API_BLOCK_TRANSPORT_SG_COUNT_FOR_SUB_OPERATIONS,
                                                            fbe_api_block_transport_sub_operation_asynch_callback,
                                                            sub_op,
                                                            io_context->package_id);
    }
    else
    {
        if (io_context->outstanding_io_count ==0)
        {
            fbe_spinlock_unlock(&io_context->context_lock);
            fbe_semaphore_release(&io_context->context_completed, 0, 1, FALSE);
        }
        else
        {
            fbe_spinlock_unlock(&io_context->context_lock);
        }

    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*!***************************************************************************
 * fbe_api_block_transport_io_operation_asynch_callback()
 *****************************************************************************
 *
 * @brief   callback function handles error and dispatching additional IO.
 *
 * @param   packet   - packet 
 * @param   context  - completion context
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/10/2013  PuhovP  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_api_block_transport_io_operation_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_api_block_transport_sub_operation_structure_t *sub_op = (fbe_api_block_transport_sub_operation_structure_t *)context;
    fbe_api_block_transport_split_io_context_t *io_context = sub_op->io_context;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL) {
        control_status =  FBE_STATUS_INSUFFICIENT_RESOURCES;
        control_status_qualifier = 0;
    } else {
        fbe_payload_control_get_status(control_operation, &control_status);
        fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);
    }

    /* we need to handle error retry later
     * we also need to handle error status where raid is overloaded 
     */
    if ((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:status:%d, payload status:%d & qulf:%d\n", __FUNCTION__,
                       status, control_status, control_status_qualifier);

	    fbe_spinlock_lock(&io_context->context_lock);

		io_context->outstanding_io_count --;

        if(io_context->completion_status != FBE_STATUS_GENERIC_FAILURE)
        {
            if(control_status == (fbe_payload_control_status_t)FBE_STATUS_INSUFFICIENT_RESOURCES)
            {
                io_context->completion_status = FBE_STATUS_INSUFFICIENT_RESOURCES;
            } else {
                io_context->completion_status = FBE_STATUS_GENERIC_FAILURE;
            }
        }
        

        if (io_context->outstanding_io_count == 0) {
            fbe_spinlock_unlock(&io_context->context_lock);
            fbe_semaphore_release(&io_context->context_completed, 0, 1, FALSE);
        } else {
            fbe_spinlock_unlock(&io_context->context_lock);
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }      

    fbe_spinlock_lock(&io_context->context_lock);

    io_context->outstanding_io_count --;

    if (io_context->data_length > io_context->current_offset)
    {
        fbe_api_block_transport_setup_next_operation(io_context, sub_op);
        fbe_spinlock_unlock(&io_context->context_lock);

        fbe_api_common_send_control_packet_with_sg_list_async(sub_op->sub_packet,
                                                            FBE_BLOCK_TRANSPORT_CONTROL_CODE_IO_COMMAND,
                                                            &(sub_op->io_info),
                                                            sizeof (fbe_block_transport_io_operation_info_t),
                                                            io_context->object_id,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            (fbe_sg_element_t*)sub_op->sg_elements,
                                                            FBE_API_BLOCK_TRANSPORT_SG_COUNT_FOR_SUB_OPERATIONS,
                                                            fbe_api_block_transport_io_operation_asynch_callback,
                                                            sub_op,
                                                            io_context->package_id);
    }
    else
    {
        if (io_context->outstanding_io_count == 0)
        {
            fbe_spinlock_unlock(&io_context->context_lock);
            fbe_semaphore_release(&io_context->context_completed, 0, 1, FALSE);
        }
        else
        {
            fbe_spinlock_unlock(&io_context->context_lock);
        }

    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}



/*************************
 * end file fbe_api_block_transport_interface.c
 *************************/
