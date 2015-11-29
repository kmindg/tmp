/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_sps_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains SPS related APIs.  It may call more
 *  specific FBE_API functions (Board Object, Enclosure Object, ...)
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_sps_interface
 * 
 * @version
 *   6/13/12    Joe Perry - Created
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

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/



/*!***************************************************************
 * @fn fbe_api_sps_get_sps_info(
 *     fbe_object_id_t object_id, 
 *     fbe_sps_info_t *ps_info) 
 *****************************************************************
 * @brief
 *  This function gets SPS Info from the appropriate
 *  Physical Package Object. 
 *
 * @param object_id - The object id to send request to.
 * @param sps_info - SPS Info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  The client needs to supply the slotNumOnEncl value to 
 *  specify which SPS's info it is getting.
 *  Physical package expects the client to fill out this value.
 *
 * @version
 *    06/13/12:     Joe Perry  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_sps_get_sps_info(fbe_object_id_t object_id, 
                         fbe_sps_get_sps_status_t *sps_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_object_type_t                      object_type;

    if (sps_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: SPS Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
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
        status = fbe_api_board_get_sps_status(object_id, sps_info);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        status = fbe_api_enclosure_get_sps_info(object_id, sps_info);
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type %d not supported\n", 
                       __FUNCTION__, (int)object_type);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return status;

}   // end of fbe_api_sps_get_sps_info

/*!***************************************************************
 * @fn fbe_api_sps_get_sps_manuf_info(fbe_object_id_t object_id, 
 *                             fbe_base_board_mgmt_get_sps_manuf_info_t *spsManufInfo)
 *****************************************************************
 * @brief
 *  This function gets SPS Manuf Info from the appropriate
 *  Physical Package Object. 
 *
 * @param objectId(INTPUT) - The object id in the physical package to send request to.
 * @param spsManufInfo(OUTPUT) - SPS Manuf Inforeported by the specified object id.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *
 * @version
 *    06-Aug-2012: Joe Perry - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_sps_get_sps_manuf_info(fbe_object_id_t objectId, 
                               fbe_sps_get_sps_manuf_info_t *spsManufInfo)
{
    fbe_status_t                  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_object_type_t    objectType;

    if (spsManufInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: spsManufInfo buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        status = fbe_api_board_get_sps_manuf_info(objectId, spsManufInfo);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        status = fbe_api_enclosure_get_sps_manuf_info(objectId, spsManufInfo);
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (unsigned int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return status;

}   // end of fbe_api_sps_get_sps_manuf_info


/*!***************************************************************
 * @fn fbe_api_sps_get_sps_manuf_info(fbe_object_id_t object_id, 
 *                             fbe_base_board_mgmt_get_sps_manuf_info_t *spsManufInfo)
 *****************************************************************
 * @brief
 *  This function gets SPS Manuf Info from the appropriate
 *  Physical Package Object. 
 *
 * @param objectId(INTPUT) - The object id in the physical package to send request to.
 * @param spsManufInfo(OUTPUT) - SPS Manuf Inforeported by the specified object id.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *
 * @version
 *    06-Aug-2012: Joe Perry - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_sps_send_sps_command(fbe_object_id_t objectId, 
                             fbe_sps_action_type_t spsCommand)
//                             fbe_sps_in_buffer_cmd_t *spsCommand)
{
    fbe_status_t                  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_object_type_t    objectType;

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        status = fbe_api_board_send_sps_command(objectId, spsCommand);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        status = fbe_api_enclosure_send_sps_command(objectId, spsCommand);
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (unsigned int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return status;

}   // end of fbe_api_sps_get_sps_manuf_info

