#ifndef FBE_API_RESUME_PROM_INTERFACE_H
#define FBE_API_RESUME_PROM_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_resume_prom_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the Resume Prom APIs.
 * 
 * @ingroup fbe_api_resume_prom_interface_class_files
 * 
 * @version
 *   20-Dec-2010   Arun S    Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_common.h"

//----------------------------------------------------------------
// Define the top level group for the FBE Physical Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_physical_package_class FBE Physical Package APIs 
 *  @brief This is the set of definitions for FBE Physical Package APIs.
 *  @ingroup fbe_api_resume_prom_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Resume Prom Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_resume_prom_usurper_interface FBE API Resume Prom Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Resume Prom Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

/*! @} */ /* end of fbe_api_resume_prom_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Resume Prom Interface.  
// This is where all the function prototypes for the Resume Prom API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_resume_prom_interface FBE API Power Supply Interface
 *  @brief 
 *    This is the set of FBE API Resume Prom Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_resume_prom_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------

fbe_status_t FBE_API_CALL fbe_api_resume_prom_read_async(fbe_object_id_t objectId, 
                                                               fbe_resume_prom_get_resume_async_t * pGetResumeProm);

fbe_status_t FBE_API_CALL fbe_api_resume_prom_read_sync(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_cmd_t * pReadResumeCmd);
                          
fbe_status_t FBE_API_CALL fbe_api_resume_prom_write_sync(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_cmd_t * pWriteResumeCmd);

fbe_status_t FBE_API_CALL fbe_api_resume_prom_write_async(fbe_object_id_t object_id, 
                                                          fbe_resume_prom_write_resume_async_op_t *rpWriteAsynchOp);

fbe_status_t FBE_API_CALL
fbe_api_resume_prom_get_tla_num(fbe_object_id_t objectId,
                         fbe_u64_t device_type,
                         fbe_device_physical_location_t *device_location,
                         fbe_u8_t *bufferPtr,
                         fbe_u32_t bufferSize);

/*! @} */ /* end of group fbe_api_resume_prom_interface */

FBE_API_CPP_EXPORT_END

#endif /*FBE_API_RESUME_PROM_INTERFACE_H*/

