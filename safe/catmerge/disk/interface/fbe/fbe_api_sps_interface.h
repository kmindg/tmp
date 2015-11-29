#ifndef FBE_API_SPS_INTERFACE_H
#define FBE_API_SPS_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_sps_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the SPS APIs.
 * 
 * @ingroup fbe_api_sps_interface_class_files
 * 
 * @version
 *   08/03/12   Joe Perry    Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_sps_info.h"
#ifndef C4_INTEGRATED
#include "fbe/fbe_api_common.h"
#endif /* C4_INTEGRATED - C4ARCH - some sort of export/import mess */

//----------------------------------------------------------------
// Define the top level group for the FBE Physical Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_physical_package_class FBE Physical Package APIs 
 *  @brief This is the set of definitions for FBE Physical Package APIs.
 *  @ingroup fbe_api_power_supply_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Power Supply Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_sps_usurper_interface FBE API Power Supply Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API SPS Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

#ifndef C4_INTEGRATED
FBE_API_CPP_EXPORT_START
#else
#ifdef __cplusplus
extern "C"
{
#endif
#endif /* C4_INTEGRATED - C4ARCH - some sort of export/import mess - C4BUG - this needs rationalization */

/*! @} */ /* end of fbe_api_sps_usurper_interface */

typedef struct fbe_sps_get_sps_status_s{
    fbe_u16_t                       spsIndex;           // input specifier
    fbe_base_sps_info_t             spsInfo;
/*
    fbe_bool_t                      spsModuleInserted;
    fbe_bool_t                      spsBatteryInserted;
    SPS_STATE                       spsStatus;
    fbe_sps_fault_info_t            spsFaultInfo;
    SPS_TYPE                        spsModel;
    HW_MODULE_TYPE                  spsModuleFfid;
    HW_MODULE_TYPE                  spsBatteryFfid;
    fbe_eir_input_power_sample_t    spsInputPower;
    fbe_u32_t                       spsBatteryTime;
*/
}fbe_sps_get_sps_status_t;

typedef struct fbe_sps_get_sps_manuf_info_s{
    fbe_u16_t               spsIndex;           // input specifier
    fbe_sps_manuf_info_t    spsManufInfo;
}fbe_sps_get_sps_manuf_info_t;

typedef struct fbe_sps_send_command_s{
    fbe_u16_t                spsIndex;           // input specifier
    fbe_sps_action_type_t    spsAction;
} fbe_sps_send_command_t;

//----------------------------------------------------------------
// Define the group for the FBE API SPS Interface.  
// This is where all the function prototypes for the Power Supply API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_sps_interface FBE API SPS Interface
 *  @brief 
 *    This is the set of FBE API SPS Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_sps_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------
#ifdef C4_INTEGRATED
#ifndef FBE_API_CALL
#if defined(__cplusplus)
#define FBE_API_CALL __stdcall
#else
#define FBE_API_CALL
#endif
#endif
#endif /* C4_INTEGRATED - C4ARCH - some sort of export/import mess - C4BUG - this needs rationalization */

fbe_status_t FBE_API_CALL 
fbe_api_sps_get_sps_info(fbe_object_id_t object_id, 
                         fbe_sps_get_sps_status_t *sps_info);
fbe_status_t FBE_API_CALL 
fbe_api_sps_get_sps_manuf_info(fbe_object_id_t objectId, 
                               fbe_sps_get_sps_manuf_info_t *spsManufInfo);
fbe_status_t FBE_API_CALL 
fbe_api_sps_send_sps_command(fbe_object_id_t objectId, 
                             fbe_sps_action_type_t spsCommand);

//fbe_status_t FBE_API_CALL 
//fbe_api_sps_get_sps_count(fbe_object_id_t objectId, 
//                          fbe_u32_t * pSpsCount);

/*! @} */ /* end of group fbe_api_sps_interface */

#ifndef C4_INTEGRATED
FBE_API_CPP_EXPORT_END
#else
#ifdef __cplusplus
}
#endif
#endif /* C4_INTEGRATED - C4ARCH - some sort of export/import mess - C4BUG - this needs rationalization */

//----------------------------------------------------------------
// Define the group for all FBE SPS APIs Interface class files.  
// This is where all the class files that belong to the FBE API Power
// Supply define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_sps_interface_class_files FBE SPS APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE SPS Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /*FBE_API_SPS_INTERFACE_H*/
