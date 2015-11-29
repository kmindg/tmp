/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_power_supply_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains Power Supply related APIs.  It may call more
 *  specific FBE_API functions (Board Object, Enclosure Object, ...)
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_power_supply_interface
 * 
 * @version
 *   7/27/10    Joe Perry - Created
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
 * @fn fbe_api_power_supply_get_power_supply_info(
 *     fbe_object_id_t object_id, 
 *     fbe_power_supply_info_t *ps_info) 
 *****************************************************************
 * @brief
 *  This function gets Power Supply Info from the appropriate
 *  Physical Package Object. 
 *
 * @param object_id - The object id to send request to.
 * @param ps_info - Power Supply Info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  The client needs to supply the slotNumOnEncl value to 
 *  specify which power supply's info it is getting.
 *  Physical package expects the client to fill out this value.
 *
 * @version
 *    02/19/10:     Joe Perry  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_power_supply_get_power_supply_info(fbe_object_id_t object_id, 
                                           fbe_power_supply_info_t *ps_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_object_type_t                      object_type;

    if (ps_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: PS Info buffer is NULL\n", __FUNCTION__);
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
        status = fbe_api_board_get_power_supply_info(object_id, ps_info);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        status = fbe_api_enclosure_getPsInfo(object_id, ps_info);
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type %llu not supported\n", 
                       __FUNCTION__, (unsigned long long)object_type);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return status;

}   // end of fbe_api_power_supply_get_power_supply_info

/*!***************************************************************
 * @fn fbe_api_power_supply_get_ps_count(fbe_object_id_t object_id, 
 *                             fbe_u32_t * pPsCount)
 *****************************************************************
 * @brief
 *  This function gets Power Supply Info from the appropriate
 *  Physical Package Object. 
 *
 * @param objectId(INTPUT) - The object id in the physical package to send request to.
 * @param pPsCount(OUTPUT) - Power Supply count reported by the specified object id.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *
 * @version
 *    25-Aug-2010: PHE - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_power_supply_get_ps_count(fbe_object_id_t objectId, 
                                  fbe_u32_t * pPsCount)
{
    fbe_status_t                  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_object_type_t    objectType;

    if (pPsCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: PS Count buffer is NULL\n", __FUNCTION__);
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
        status = fbe_api_board_get_ps_count(objectId, pPsCount);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        status = fbe_api_enclosure_get_ps_count(objectId, pPsCount);
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%llu not supported\n", 
                       __FUNCTION__, (unsigned long long)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return status;

}   // end of fbe_api_ps_get_ps_count
