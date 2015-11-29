/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_resume_prom_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains Resume Prom related APIs.  It may call more
 *  specific FBE_API functions (Board Object, Enclosure Object, ...)
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_reusme_prom_interface
 * 
 * @version
 *   20-Dec-2010    Arun S - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "generic_utils_lib.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

/*!***************************************************************
 * fbe_api_resume_prom_read_async()
 ****************************************************************
 * @brief
 *  This function issues ASYNC the resume prom read by calling the
 *  appropriate API.
 *
 *  @param object_id - The object id to send request
 *  @param pGetResumeProm - pointer to the resume prom info
 *                          passed from the the client
 *
 * @return fbe_status_t
 *
 * @author
 *  20-Dec-2010 - Created. Arun S
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_resume_prom_read_async(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_get_resume_async_t * pGetResumeProm)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_topology_object_type_t  object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;

    /* Determine the type of Object this is directed at */
    status = fbe_api_get_object_type(object_id, &object_type, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type from ID 0x%x, status %d\n", 
                       __FUNCTION__, object_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (object_type)
    {
        case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
            status = fbe_api_board_resume_read_async(object_id, pGetResumeProm);
            break;
    
        case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
            status = fbe_api_enclosure_resume_read_asynch(object_id, pGetResumeProm);
            break;
    
        default:
            fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                           "%s: Object Type %llu not supported\n", 
                           __FUNCTION__, (unsigned long long)object_type);
    
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

/*!***************************************************************
 * fbe_api_resume_prom_read_sync()
 ****************************************************************
 * @brief
 *  This function issues the SYNC resume prom read by calling the
 *  appropriate API.
 *
 *  @param object_id - The object id to send request
 *  @param pReadResumeProm - pointer to fbe_resume_prom_cmd_t
 *                          passed from the the client
 *
 * @return fbe_status_t
 *
 * @author
 *  21-Jul-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_resume_prom_read_sync(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_cmd_t * pReadResumeProm)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_topology_object_type_t  object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;

    /* Determine the type of Object this is directed at */
    status = fbe_api_get_object_type(object_id, &object_type, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type from ID 0x%x, status %d\n", 
                       __FUNCTION__, object_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (object_type)
    {
        case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
            status = fbe_api_board_resume_read_sync(object_id, pReadResumeProm);
            break;
    
        case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
            status = fbe_api_enclosure_resume_read_synch(object_id, pReadResumeProm);
            break;
    
        default:
            fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                           "%s: Object Type %llu not supported\n", 
                           __FUNCTION__, (unsigned long long)object_type);
    
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

/*!***************************************************************
 * fbe_api_resume_prom_write_sync()
 ****************************************************************
 * @brief
 *  This function issues the SYNC resume prom write by calling the
 *  appropriate API.
 *
 *  @param object_id - The object id to send request
 *  @param pWriteResumeProm - pointer to fbe_resume_prom_cmd_t
 *                          passed from the the client
 *
 * @return fbe_status_t
 *
 * @author
 *  20-Jul-2011 PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_resume_prom_write_sync(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_cmd_t * pWriteResumeProm)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_topology_object_type_t  object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;

    /* Determine the type of Object this is directed at */
    status = fbe_api_get_object_type(object_id, &object_type, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type from ID 0x%x, status %d\n", 
                       __FUNCTION__, object_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (object_type)
    {
        case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
            status = fbe_api_board_resume_write_sync(object_id, pWriteResumeProm);
            break;
    
        case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
            status = fbe_api_enclosure_resume_write_synch(object_id, pWriteResumeProm);
            break;
    
        default:
            fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                           "%s: Object Type %llu not supported\n", 
                           __FUNCTION__, (unsigned long long)object_type);
    
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}


/*!***************************************************************
 * fbe_api_resume_prom_write_async()
 ****************************************************************
 * @brief
 *  This function issues the ASYNC resume prom write by calling the
 *  appropriate API.
 *
 *  @param object_id - The object id to send request
 *  @param rpWriteAsynchOp - pointer to fbe_resume_prom_write_resume_async_op_t
 *                          passed from the the client
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Dec-2012 Dipak Patel - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_resume_prom_write_async(fbe_object_id_t object_id, 
                                                          fbe_resume_prom_write_resume_async_op_t *rpWriteAsynchOp)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_topology_object_type_t  object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;

    /* Determine the type of Object this is directed at */
    status = fbe_api_get_object_type(object_id, &object_type, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type from ID 0x%x, status %d\n", 
                       __FUNCTION__, object_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (object_type)
    {
        case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
            status = fbe_api_board_resume_write_async(object_id, rpWriteAsynchOp);
            break;

        case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        default:
            fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                           "%s: Object Type %d not supported\n", 
                           __FUNCTION__, (int)object_type);

            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}
/*!**************************************************************
 * fbe_api_resume_prom_get_tla_num()
 ****************************************************************
 * @brief
 *  This function gets TLA part number.
 * 
 * @param pBaseEnv - 
 * @param device_type -
 * @param device_location -
 * @param bufferPtr (OUTPUT)-
 * @param bufferSize -
 * 
 * @return fbe_status_t 
 * 
 * @note If the function is used to get the tla number from the enclosure resume
 *       prom device, please make sure the memory that bufferPtr points to
 *       is physically contiguous.
 * 
 * @author
 *  05-Jan-2011: PHE - Created
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_resume_prom_get_tla_num(fbe_object_id_t objectId,
                         fbe_u64_t device_type,
                         fbe_device_physical_location_t *device_location,
                         fbe_u8_t *bufferPtr,
                         fbe_u32_t bufferSize)
{
    fbe_resume_prom_cmd_t     readResumeProm = {0};
    fbe_status_t              status = FBE_STATUS_OK;
    fbe_u32_t                 index;

    if(bufferSize > RESUME_PROM_EMC_TLA_PART_NUM_SIZE)
    {
        bufferSize = RESUME_PROM_EMC_TLA_PART_NUM_SIZE;
    }

    readResumeProm.deviceType = device_type;
    readResumeProm.deviceLocation = *device_location;
    readResumeProm.pBuffer = bufferPtr;
    readResumeProm.bufferSize = bufferSize;

    if(device_type == FBE_DEVICE_TYPE_SP) 
    {
        readResumeProm.resumePromField = RESUME_PROM_EMC_SUB_ASSY_PART_NUM;
        readResumeProm.offset = getResumeFieldOffset(RESUME_PROM_EMC_SUB_ASSY_PART_NUM);
    }
    else
    {
        readResumeProm.resumePromField = RESUME_PROM_EMC_TLA_PART_NUM;
        readResumeProm.offset = getResumeFieldOffset(RESUME_PROM_EMC_TLA_PART_NUM);
    }

    readResumeProm.readOpStatus = FBE_RESUME_PROM_STATUS_INVALID;

    status = fbe_api_resume_prom_read_sync(objectId, &readResumeProm);

    if(status == FBE_STATUS_OK)
    {
        // make sure the string is null terminated
        readResumeProm.pBuffer[bufferSize-1] = 0;

        for(index=0;index<strlen(readResumeProm.pBuffer);index++)
        {
            // if it isn't an alphanumeric character or hyphen
            if(!( ((readResumeProm.pBuffer[index] >= 45) && (readResumeProm.pBuffer[index] <= 57)) ||
                  ((readResumeProm.pBuffer[index] >= 65) && (readResumeProm.pBuffer[index] <= 90)) ||
                  (readResumeProm.pBuffer[index] == 95) ||
                  ((readResumeProm.pBuffer[index] >= 97) && (readResumeProm.pBuffer[index] <= 122))))
            {
                /*
                 * A bad string is detected, end the string right here so as to not cause
                 * problems logging with this later.
                 */
                fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
                       "%s: malformed part number detected\n", 
                       __FUNCTION__);
                readResumeProm.pBuffer[index] = 0;
                break;
            }
        }
    }
    
    return status;
}

