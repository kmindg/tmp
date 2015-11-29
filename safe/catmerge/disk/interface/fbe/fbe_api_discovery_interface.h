#ifndef FBE_API_DISCOVERY_INTERFACE_H
#define FBE_API_DISCOVERY_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_discovery_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_discovery_interface.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_system_package_interface_class_files
 * 
 * @version
 *   10/10/08    sharel - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"

FBE_API_CPP_EXPORT_START

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
// Define the top level group for the FBE System APIs.
// This needs to be here because some of the functions define in
// the FBE API Discovery Interface are really used by the FBE API
// System. 
//----------------------------------------------------------------
/*! @defgroup fbe_system_package_class FBE System APIs 
 *  @brief 
 *    This is the set of definitions for FBE System APIs.
 * 
 *  @ingroup fbe_api_system_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Discovery Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_discovery_interface_usurper_interface FBE API Discovery Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Discovery class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!********************************************************************* 
 * @struct fbe_api_get_discovery_edge_info_t 
 *  
 * @brief 
 *   This contains the data info for Discovery Edge structure.
 *
 * @ingroup fbe_api_discovery_interface
 * @ingroup fbe_api_get_discovery_edge_info
 **********************************************************************/
typedef struct fbe_api_get_discovery_edge_info_s{
    fbe_path_state_t    path_state;     /*!< Discover Edge Path State */
    fbe_path_attr_t     path_attr;      /*!< Discover Edge Path Attributes */
    fbe_object_id_t     client_id;      /*!< Discover Edge Client ID */
    fbe_object_id_t     server_id;      /*!< Discover Edge Server ID */
    fbe_edge_index_t    client_index;   /*!< Discover Edge Client Index */
    fbe_edge_index_t    server_index;   /*!< Discover Edge Server Index */
    fbe_transport_id_t  transport_id;   /*!< Discover Edge Transport ID */
}fbe_api_get_discovery_edge_info_t;

/*!********************************************************************* 
 * @struct fbe_api_get_ssp_edge_info_t 
 *  
 * @brief 
 *   This contains the data info for SSP Edge structure.
 *
 * @ingroup fbe_api_discovery_interface
 * @ingroup fbe_api_get_ssp_edge_info
 **********************************************************************/
typedef struct fbe_api_get_ssp_edge_info_s{
    fbe_path_state_t    path_state;     /*!< Discover SSP Edge Path State */
    fbe_path_attr_t     path_attr;      /*!< Discover SSP Edge Path Attributes */
    fbe_object_id_t     client_id;      /*!< Discover SSP Edge Client ID */
    fbe_object_id_t     server_id;      /*!< Discover SSP Edge Server ID */
    fbe_edge_index_t    client_index;   /*!< Discover SSP Edge Client Index */
    fbe_edge_index_t    server_index;   /*!< Discover SSP Edge Server Index */
    fbe_transport_id_t  transport_id;   /*!< Discover SSP Edge Transport ID */
}fbe_api_get_ssp_edge_info_t;

/*!********************************************************************* 
 * @struct fbe_api_get_block_edge_info_t 
 *  
 * @brief 
 *  This contains the data info for Block Edge structure.
 * 
 * @ingroup fbe_api_discovery_interface
 * @ingroup fbe_api_get_block_edge_info
 **********************************************************************/
typedef struct fbe_api_get_block_edge_info_s{
    fbe_path_state_t    path_state;     /*!< Discover Block Edge Path State */
    fbe_path_attr_t     path_attr;      /*!< Discover Block Edge Path Attribute */
    fbe_object_id_t     client_id;      /*!< Discover Block Edge Client ID */
    fbe_object_id_t     server_id;      /*!< Discover Block Edge Server ID */
    fbe_edge_index_t    client_index;   /*!< Discover Block Edge Client Index */
    fbe_edge_index_t    server_index;   /*!< Discover Block Edge Server Index */
    fbe_transport_id_t  transport_id;   /*!< Discover Block Edge Transport ID */
	/*! Capacity in blocks exported by the server.*/
    fbe_lba_t 			capacity;
     /*! This is the block offset of this extent from the start of the 
     *  device.  Currently this is not used since all edges import the entire
     *  capacity.
     */
    fbe_lba_t 			offset;

    /*! Block size in bytes exported by the server. */ 
    fbe_block_size_t 	exported_block_size;

    /*! Block size in bytes imported by the server. */ 
    fbe_block_size_t 	imported_block_size;
}fbe_api_get_block_edge_info_t;

/*!********************************************************************* 
 * @struct fbe_api_get_stp_edge_info_t 
 *  
 * @brief 
 *  This contains the data info for STP Edge structure.
 * 
 * @ingroup fbe_api_discovery_interface
 * @ingroup fbe_api_get_stp_edge_info
 **********************************************************************/
typedef struct fbe_api_get_stp_edge_info_s{
    fbe_path_state_t    path_state;     /*!< Discover STP Edge Path State */
    fbe_path_attr_t     path_attr;      /*!< Discover STP Edge Path Attribute */
    fbe_object_id_t     client_id;      /*!< Discover STP Edge Client ID */
    fbe_object_id_t     server_id;      /*!< Discover STP Edge Server ID */
    fbe_edge_index_t    client_index;   /*!< Discover STP Edge Client Index */
    fbe_edge_index_t    server_index;   /*!< Discover STP Edge Server Index */
    fbe_transport_id_t  transport_id;   /*!< Discover STP Edge Transport ID */
}fbe_api_get_stp_edge_info_t;

/*!********************************************************************* 
 * @struct fbe_api_get_smp_edge_info_t 
 *  
 * @brief 
 *   This contains the data info for SMP Edge structure.
 *
 * @ingroup fbe_api_discovery_interface
 * @ingroup fbe_api_get_smp_edge_info
 **********************************************************************/
typedef struct fbe_api_get_smp_edge_info_s{
    fbe_path_state_t    path_state;     /*!< Discover SMP Edge Path State */
    fbe_path_attr_t     path_attr;      /*!< Discover SMP Edge Path Attribute */
    fbe_object_id_t     client_id;      /*!< Discover SMP Edge Client ID */
    fbe_object_id_t     server_id;      /*!< Discover SMP Edge Server ID */
    fbe_edge_index_t    client_index;   /*!< Discover SMP Edge Client Index */
    fbe_edge_index_t    server_index;   /*!< Discover SMP Edge Server Index */
    fbe_transport_id_t  transport_id;   /*!< Discover SMP Edge Transport ID */
}fbe_api_get_smp_edge_info_t;

/*!********************************************************************* 
 * @struct fbe_api_set_edge_hook_t 
 *  
 * @brief 
 *    This contains the data info for Edge Hook structure.
 * 
 * @ingroup fbe_api_discovery_interface
 * @ingroup fbe_api_set_edge_hook
 **********************************************************************/
typedef struct fbe_api_set_edge_hook_s{
    fbe_edge_hook_function_t    hook;   /*!< Edge Hook Function */
}fbe_api_set_edge_hook_t;
/*! @} */ /* end of group fbe_api_discovery_interface_usurper_interface */


//----------------------------------------------------------------
// Define the group for the FBE API Discovery Interface.  
// This is where all the function prototypes for the Discovery API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_discovery_interface FBE API Discovery Interface
 *  @brief 
 *    This is the set of FBE API Discovery.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_discovery_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_get_object_port_number (fbe_object_id_t object_id, fbe_port_number_t * port_number);
fbe_status_t FBE_API_CALL fbe_api_get_object_enclosure_number (fbe_object_id_t object_id, fbe_enclosure_number_t * enclosure_number);
fbe_status_t FBE_API_CALL fbe_api_get_object_component_id (fbe_object_id_t object_id, fbe_component_id_t * component_id);
fbe_status_t FBE_API_CALL fbe_api_get_object_drive_number(fbe_object_id_t object_id, fbe_enclosure_slot_number_t * drive_number);
fbe_status_t FBE_API_CALL fbe_api_get_discovery_edge_info (fbe_object_id_t object_id, fbe_api_get_discovery_edge_info_t *edge_info);
fbe_status_t FBE_API_CALL fbe_api_get_ssp_edge_info (fbe_object_id_t object_id, fbe_api_get_ssp_edge_info_t *edge_info);
fbe_status_t FBE_API_CALL fbe_api_get_physical_drive_object_state (fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot, fbe_lifecycle_state_t *lifecycle_state_out);
fbe_status_t FBE_API_CALL fbe_api_get_logical_drive_object_state (fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot, fbe_lifecycle_state_t *lifecycle_state_out);
fbe_status_t FBE_API_CALL fbe_api_get_enclosure_object_state (fbe_u32_t port, fbe_u32_t enclosure, fbe_lifecycle_state_t *lifecycle_state_out);
fbe_status_t FBE_API_CALL fbe_api_get_all_enclosure_object_ids( fbe_object_id_t *object_id_list, fbe_u32_t buffer_size, fbe_u32_t *enclosure_count);
fbe_status_t FBE_API_CALL fbe_api_get_enclosure_lcc_object_ids(fbe_object_id_t enclObjectId, fbe_object_id_t *client_lcc_obj_list, fbe_u32_t buffer_size, fbe_u32_t *lcc_count);
fbe_status_t FBE_API_CALL fbe_api_get_all_drive_object_ids( fbe_object_id_t *object_id_list, fbe_u32_t buffer_size, fbe_u32_t *drive_count);

fbe_status_t FBE_API_CALL fbe_api_get_port_object_state (fbe_u32_t port, fbe_lifecycle_state_t *lifecycle_state_out);
fbe_status_t FBE_API_CALL fbe_api_get_stp_edge_info (fbe_object_id_t object_id, fbe_api_get_stp_edge_info_t *edge_info);
fbe_status_t FBE_API_CALL fbe_api_get_smp_edge_info (fbe_object_id_t object_id, fbe_api_get_smp_edge_info_t *edge_info);
fbe_status_t FBE_API_CALL fbe_api_set_ssp_edge_hook (fbe_object_id_t object_id, fbe_api_set_edge_hook_t *hook_info);
fbe_status_t FBE_API_CALL fbe_api_set_stp_edge_hook (fbe_object_id_t object_id, fbe_api_set_edge_hook_t *hook_info);
/*! @} */ /* end of group fbe_api_discovery_interface */

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

//----------------------------------------------------------------
// Define the group for the FBE API System Interface.  
// Even though these functions are defined in the FBE Discovery 
// API, they belong to the FBE API System.
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_interface FBE API Discovery Interface
 *  @brief
 *    This is the set of FBE API Discovery.
 *
 *  @details
 *    In order to access this library, please include fbe_api_discovery_interface.h.
 *
 *  @ingroup fbe_system_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_enumerate_objects(fbe_object_id_t * obj_array, fbe_u32_t total_objects, fbe_u32_t *actual_objects, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_get_object_type (fbe_object_id_t object_id, fbe_topology_object_type_t *object_type, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_get_object_class_id (fbe_object_id_t object_id, fbe_class_id_t *class_id, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_destroy_object (fbe_object_id_t object_id, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_get_block_edge_info (fbe_object_id_t object_id, fbe_edge_index_t edge_idex, fbe_api_get_block_edge_info_t *edge_info, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_get_object_lifecycle_state (fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_get_total_objects(fbe_u32_t *total_objects, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_get_all_objects_in_state (fbe_lifecycle_state_t requested_lifecycle_state, fbe_object_id_t obj_array[], fbe_u32_t array_length, fbe_u32_t *total_objects, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_set_object_to_state(fbe_object_id_t object_id, fbe_lifecycle_state_t requested_lifecycle_state, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_get_object_death_reason(fbe_object_id_t object_id, fbe_object_death_reason_t *death_reason, const fbe_u8_t **death_reason_str, fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_enumerate_class(fbe_class_id_t class_id,  fbe_package_id_t package_id, fbe_object_id_t ** obj_array_p, fbe_u32_t *num_objects_p);
fbe_status_t FBE_API_CALL fbe_api_get_total_objects_of_class(fbe_class_id_t class_id, fbe_package_id_t package_id, fbe_u32_t *total_objects_p);
fbe_status_t FBE_API_CALL fbe_api_get_total_objects_of_all_raid_groups(fbe_package_id_t package_id, fbe_u32_t *total_objects_p);
/*! @} */ /* end of group fbe_api_system_interface */

//----------------------------------------------------------------
// Define the group for the FBE API System Interface class files.  
// This is where all the class files that belong to the FBE API Physical 
// Package define. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_package_interface_class_files FBE System APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE System Package Interface.
 * 
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_END

#endif /*#ifndef FBE_API_DISCOVERY_INTERFACE_H*/
