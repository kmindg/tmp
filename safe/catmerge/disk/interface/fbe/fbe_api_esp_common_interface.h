#ifndef FBE_API_ESP_COMMON_INTERFACE_H
#define FBE_API_ESP_COMMON_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_common_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the ESP base environment object.
 * 
 * @ingroup fbe_api_esp_interface_class_files
 * 
 * @version
 *   25-Jul-2010:  PHE - Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
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
// Define the group for the FBE API ESP Base Environment Interface.   
// This is where all the function prototypes for the FBE API ESP Base Environment. 
//---------------------------------------------------------------- 
/*! @defgroup fbe_api_esp_common_interface FBE API ESP common Interface  
 *  @brief   
 *    This is the set of FBE API ESP common Interface.   
 *  
 *  @details   
 *    In order to access this library, please include fbe_api_esp_common_interface.h.  
 *  
 *  @ingroup fbe_api_esp_interface_class  
 *  @{  
 */
//---------------------------------------------------------------- 
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_object_id_by_device_type(fbe_u64_t deviceType, fbe_object_id_t * pOjectId);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_any_upgrade_in_progress(fbe_u64_t deviceType, fbe_bool_t *pAnyUpgradeInProgress);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_initiate_upgrade(fbe_u64_t deviceType,
                                    fbe_device_physical_location_t * pLocation,
                                    fbe_base_env_fup_force_flags_t forceFlags);
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_terminate_upgrade(void);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_abort_upgrade(void);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_resume_upgrade(void);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_terminate_upgrade_by_class_id(fbe_class_id_t classId);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_abort_upgrade_by_class_id(fbe_class_id_t classId);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_resume_upgrade_by_class_id(fbe_class_id_t classId);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_fup_work_state(fbe_u64_t deviceType,
                                      fbe_device_physical_location_t * pLocation,
                                      fbe_base_env_fup_work_state_t  * pWorkState);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_fup_info(fbe_esp_base_env_get_fup_info_t * pGetFupInfo);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_sp_id(fbe_class_id_t classId,fbe_base_env_sp_t *sp_id);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_resume_prom_info(fbe_esp_base_env_get_resume_prom_info_cmd_t * pGetResumePromInfo);

fbe_status_t FBE_API_CALL
fbe_api_esp_common_write_resume_prom(fbe_resume_prom_cmd_t * pWriteResumeCmd);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_initiate_resume_prom_read(fbe_u64_t deviceType,
                                          fbe_device_physical_location_t * pLocation);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_any_resume_prom_read_in_progress(fbe_class_id_t classId, fbe_bool_t *pAnyReadInProgress);

fbe_status_t FBE_API_CALL
fbe_api_esp_common_get_is_service_mode(fbe_class_id_t classId,
                                       fbe_bool_t *pIsServiceMode);

fbe_status_t FBE_API_CALL
fbe_api_esp_common_getCacheStatus(fbe_common_cache_status_info_t *cacheStatusPtr);
fbe_status_t FBE_API_CALL fbe_api_esp_common_powerdown(void);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_fup_manifest_info(fbe_u64_t deviceType, 
                                         fbe_base_env_fup_manifest_info_t  * pFupManifestInfo);

fbe_status_t FBE_API_CALL 
fbe_api_esp_common_modify_fup_delay(fbe_u64_t deviceType, 
                                    fbe_u32_t delayInSec);

/*! @} */ /* end of group fbe_api_esp_common_interface */

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



#endif /* FBE_API_ESP_COMMON_INTERFACE_H */

/*************************
 * end file fbe_api_esp_common_interface.h
 *************************/
