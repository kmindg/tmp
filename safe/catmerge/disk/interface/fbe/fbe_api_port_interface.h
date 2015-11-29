#ifndef FBE_API_PORT_INTERFACE_H
#define FBE_API_PORT_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_port_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the Port APIs.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * 
 * @version
 *   04/08/00 sharel    Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_api_common.h"


//----------------------------------------------------------------
// Define the top level group for the FBE Physical Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_physical_package_class FBE Physical Package APIs 
 *  @brief 
 *    This is the set of definitions for FBE Physical Package APIs.
 *
 *  @ingroup fbe_api_physical_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Port Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_port_usurper_interface FBE API Port Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Port class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

/*! @} */ /* end of group fbe_api_port_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Port Interface.  
// This is where all the function prototypes for the Port API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_port_interface FBE API Port Interface
 *  @brief 
 *    This is the set of FBE API Port.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_port_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------

/* Add any port prototype and functions here*/
fbe_status_t FBE_API_CALL fbe_api_get_port_object_id_by_location(fbe_u32_t port, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_get_port_object_id_by_location_and_role(fbe_u32_t port, fbe_port_role_t role, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_get_port_hardware_information(fbe_object_id_t object_id, fbe_port_hardware_info_t *port_hardware_info);
fbe_status_t FBE_API_CALL fbe_api_get_port_sfp_information(fbe_object_id_t object_id, fbe_port_sfp_info_t *port_sfp_info_p);
fbe_status_t FBE_API_CALL fbe_api_port_get_port_role(fbe_object_id_t object_id, fbe_port_role_t *port_role);
fbe_status_t FBE_API_CALL fbe_api_port_get_link_information(fbe_object_id_t object_id, fbe_port_link_information_t *port_link_info);
fbe_status_t FBE_API_CALL fbe_api_port_set_port_debug_control(fbe_object_id_t object_id, fbe_port_dbg_ctrl_t *port_debug_control);
fbe_status_t FBE_API_CALL fbe_api_port_get_kek_handle(fbe_object_id_t object_id, fbe_key_handle_t *key_handle);
fbe_status_t FBE_API_CALL fbe_api_port_set_port_encryption_mode(fbe_object_id_t object_id,
                                                                fbe_port_encryption_mode_t mode);
fbe_status_t FBE_API_CALL fbe_api_port_debug_register_dek(fbe_object_id_t object_id,
                                                          fbe_base_port_mgmt_debug_register_dek_t *dek_info);
fbe_status_t FBE_API_CALL fbe_api_port_get_port_info(fbe_object_id_t object_id,
                                                     fbe_port_info_t  *port_info);

/*! @} */ /* end of group fbe_api_port_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE Physical Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Physical 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_physical_package_interface_class_files FBE Physical Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Physical Package Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /*FBE_API_PORT_INTERFACE_H*/


