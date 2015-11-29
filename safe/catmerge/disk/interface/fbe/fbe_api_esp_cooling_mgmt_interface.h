#ifndef FBE_API_ESP_COOLING_MGMT_INTERFACE_H
#define FBE_API_ESP_COOLING_MGMT_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_encl_mgmt_interface.h
 ***************************************************************************
 *
 * @brief 
 *  This header file defines the FBE API for the ESP ENCL Mgmt object.  
 *   
 * @ingroup fbe_api_esp_interface_class_files  
 *   
 * @version  
 *   03/16/2010:  Created. Joe Perry  
 ****************************************************************************/

/*************************  
 *   INCLUDE FILES  
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_pe_types.h"

FBE_API_CPP_EXPORT_START

//---------------------------------------------------------------- 
// Define the top level group for the FBE Environment Service Package APIs
// ----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class FBE ESP APIs 
 *
 *  @brief 
 *    This is the set of definitions for FBE ESP APIs.
 * 
 *  @ingroup fbe_api_esp_interface
 */ 
//----------------------------------------------------------------

//---------------------------------------------------------------- 
// Define the FBE API ESP ENCL Mgmt Interface for the Usurper Interface. 
// This is where all the data structures defined.  
//---------------------------------------------------------------- 
/*! @defgroup fbe_api_esp_encl_mgmt_interface_usurper_interface FBE API ESP ENCL Mgmt Interface Usurper Interface  
 *  @brief   
 *    This is the set of definitions that comprise the FBE API ESP ENCL Mgmt Interface class  
 *    usurper interface.  
 *  
 *  @ingroup fbe_api_classes_usurper_interface  
 *  @{  
 */ 
//----------------------------------------------------------------


typedef struct fbe_esp_cooling_mgmt_fan_info_s
{
    fbe_env_inferface_status_t envInterfaceStatus;
    fbe_bool_t          inProcessorEncl;
    SP_ID               associatedSp;                      //This is only for PE fans
    fbe_u8_t            slotNumOnSpBlade;          //This is only for PE fans
    fbe_mgmt_status_t   inserted;
    fbe_fan_state_t     state;
    fbe_fan_subState_t  subState;
    fbe_mgmt_status_t   fanFaulted;
    fbe_mgmt_status_t   fanDegraded;
    fbe_mgmt_status_t   multiFanModuleFaulted;
    fbe_bool_t          fwUpdatable;
    fbe_bool_t          resumePromSupported;
    HW_MODULE_TYPE      uniqueId;
    fbe_fan_location_t  fanLocation;
    /* Only for Processor enclosure fans */
    fbe_bool_t          isFaultRegFail;
    fbe_u8_t            firmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_bool_t          dataStale;
    fbe_bool_t          environmentalInterfaceFault;
    fbe_bool_t          resumePromReadFailed;
    fbe_bool_t          fupFailure;
} fbe_esp_cooling_mgmt_fan_info_t;

/*! @} */ /* end of group fbe_api_esp_encl_mgmt_interface_usurper_interface */

//---------------------------------------------------------------- 
// Define the group for the FBE API ESP Cooling Mgmt Interface.   
// This is where all the function prototypes for the FBE API ESP Cooling Mgmt. 
//---------------------------------------------------------------- 
/*! @defgroup fbe_api_esp_encl_mgmt_interface FBE API ESP ENCL Mgmt Interface  
 *  @brief   
 *    This is the set of FBE API ESP ENCL Mgmt Interface.   
 *  
 *  @details   
 *    In order to access this library, please include fbe_api_esp_encl_mgmt_interface.h.  
 *  
 *  @ingroup fbe_api_esp_interface_class  
 *  @{  
 */
//---------------------------------------------------------------- 

fbe_status_t FBE_API_CALL fbe_api_esp_cooling_mgmt_get_fan_info(fbe_device_physical_location_t * pLocation,
                                                             fbe_esp_cooling_mgmt_fan_info_t * pFanInfo);
fbe_status_t FBE_API_CALL 
fbe_api_esp_cooling_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus);

/*! @} */ /* end of group fbe_api_esp_cooling_mgmt_interface */

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

#endif /* FBE_API_ESP_COOLING_MGMT_INTERFACE_H */

/**********************************************
 * end file fbe_api_esp_encl_mgmt_interface.h
 **********************************************/

