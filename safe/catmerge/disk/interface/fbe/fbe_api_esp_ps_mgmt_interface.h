#ifndef FBE_API_ESP_PS_MGMT_INTERFACE_H
#define FBE_API_ESP_PS_MGMT_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_ps_mgmt_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the ESP PS Mgmt object.
 * 
 * @ingroup fbe_api_esp_interface_class_files
 * 
 * @version
 *   03/16/2010:  Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_esp_ps_mgmt.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_esp_base_environment.h"

FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the top level group for the FBE Storage Extent Package APIs
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class FBE ESP APIs
 *  @brief 
 *    This is the set of definitions for FBE ESP APIs.
 *
 *  @ingroup fbe_api_esp_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API ESP PS Mgmt Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_ps_mgmt_interface_usurper_interface FBE API ESP PS Mgmt Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API ESP PS Mgmt Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

// FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_COUNT

/*!********************************************************************* 
 * @struct fbe_esp_ps_mgmt_ps_count_t 
 *  
 * @brief 
 *   Contains the ps count.
 *
 * @ingroup fbe_api_esp_ps_mgmt_interface
 * @ingroup fbe_esp_ps_mgmt_psStatus
 **********************************************************************/
typedef struct
{
    fbe_device_physical_location_t  location;
    fbe_u32_t                       psCount;
} fbe_esp_ps_mgmt_ps_count_t;

// FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_STATUS

/*!********************************************************************* 
 * @struct fbe_esp_ps_mgmt_ps_info_t 
 *  
 * @brief 
 *   Contains the ps status.
 *
 * @ingroup fbe_api_esp_ps_mgmt_interface
 * @ingroup fbe_esp_ps_mgmt_psStatus
 **********************************************************************/
typedef struct
{
    fbe_device_physical_location_t  location;
    fbe_power_supply_info_t         psInfo;
    fbe_bool_t                      psLogicalFault;
    fbe_bool_t                      dataStale;
    fbe_bool_t                      environmentalInterfaceFault;
    // applies to processor enclosure only
    fbe_ps_mgmt_expected_ps_type_t expected_ps_type;
} fbe_esp_ps_mgmt_ps_info_t;

/*! @} */ /* end of group fbe_api_esp_ps_mgmt_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API ESP PS Mgmt Interface.  
// This is where all the function prototypes for the FBE API ESP PS Mgmt.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_ps_mgmt_interface FBE API ESP PS Mgmt Interface
 *  @brief 
 *    This is the set of FBE API ESP PS Mgmt Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_esp_ps_mgmt_interface.h.
 *
 *  @ingroup fbe_api_esp_interface_class
 *  @{
 */
//----------------------------------------------------------------

fbe_status_t FBE_API_CALL 
fbe_api_esp_ps_mgmt_getPsCount(fbe_device_physical_location_t * pLocation,
                               fbe_u32_t * pPsCount);

fbe_status_t FBE_API_CALL 
fbe_api_esp_ps_mgmt_getPsInfo(fbe_esp_ps_mgmt_ps_info_t *psInfo);
fbe_status_t FBE_API_CALL 
fbe_api_esp_ps_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus);
fbe_status_t FBE_API_CALL fbe_api_esp_ps_mgmt_powerdown(void);
fbe_status_t FBE_API_CALL fbe_api_esp_ps_mgmt_set_expected_ps_type(fbe_ps_mgmt_expected_ps_type_t expected_ps_type);

/*! @} */ /* end of group fbe_api_esp_ps_mgmt_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE ESP Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API ESP 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class_files FBE ESP APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE ESP Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------


#endif /* FBE_API_ESP_PS_MGMT_INTERFACE_H */

/*************************
 * end file fbe_api_esp_ps_mgmt_interface.h
 *************************/
