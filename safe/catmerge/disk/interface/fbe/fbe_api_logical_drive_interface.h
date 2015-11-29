#ifndef FBE_API_LOGICAL_DRIVE_INTERFACE_H
#define FBE_API_LOGICAL_DRIVE_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_logical_drive_interface.h
 ***************************************************************************
 *
 * @brief
 *   This file contains the Logical Drive APIs.
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
#include "fbe/fbe_logical_drive.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure.h"

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
// Define the FBE API Logical Drive Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_logical_drive_usurper_interface FBE API Logical Drive Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Logical Drive class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!********************************************************************* 
 * @def FBE_API_DEFAULT_FLARE_BLOCK_SIZE
 *  
 * @brief 
 *   This represents the default flare block size.
 *
 * @ingroup fbe_api_logical_drive_usurper_interface
 **********************************************************************/
#define FBE_API_DEFAULT_FLARE_BLOCK_SIZE        520

/*!********************************************************************* 
 * @struct fbe_api_block_status_values_t 
 *  
 * @brief 
 *   The purpose of this structure is to contain all the status we need 
 *   on a block operation.
 *
 * @ingroup fbe_api_logical_drive_interface
 * @ingroup fbe_api_block_status_values
 **********************************************************************/
typedef struct fbe_api_block_status_values_s
{
   fbe_payload_block_operation_status_t status;         /*!< Block status */
   fbe_payload_block_operation_qualifier_t qualifier;   /*!< Block qualifier */
   fbe_lba_t media_error_lba;                           /*!< Media error */
   fbe_object_id_t pdo_object_id;                       /*!< object id of the physical drive to which the IO request is sent.*/
} fbe_api_block_status_values_t;

/*! @} */ /* end of group fbe_api_logical_drive_usurper_interface */


//----------------------------------------------------------------
// Define the group for the FBE API Logical Drive Interface.  
// This is where all the function prototypes for the Logical Drive API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_logical_drive_interface FBE API Logical Drive Interface
 *  @brief 
 *    This is the set of FBE API Logical Drive.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_logical_drive_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_logical_drive_get_drive_block_size(const fbe_object_id_t object_id,
                                                        fbe_block_transport_negotiate_t *const negotiate_p,
                                                        fbe_payload_block_operation_status_t *const block_status_p,
                                                        fbe_payload_block_operation_qualifier_t *const block_qualifier);

fbe_status_t FBE_API_CALL fbe_api_logical_drive_get_attributes(fbe_object_id_t object_id, fbe_logical_drive_attributes_t * const attributes_p);
fbe_status_t FBE_API_CALL fbe_api_logical_drive_set_attributes(fbe_object_id_t object_id, fbe_logical_drive_attributes_t * const attributes_p);
fbe_status_t FBE_API_CALL fbe_api_get_logical_drive_object_id_by_location(fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_logical_drive_set_pvd_eol(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_logical_drive_clear_pvd_eol(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL 
fbe_api_logical_drive_interface_send_block_payload(fbe_object_id_t object_id,
                                                   fbe_payload_block_operation_opcode_t opcode,
                                                   fbe_lba_t lba,
                                                   fbe_block_count_t blocks,
                                                   fbe_block_size_t block_size,
                                                   fbe_block_size_t opt_block_size,
                                                   fbe_sg_element_t * const sg_p,
                                                   fbe_payload_pre_read_descriptor_t * const pre_read_desc_p,
                                                   fbe_packet_attr_t * const transport_qualifier_p,
                                                   fbe_api_block_status_values_t * const block_status_p);
fbe_status_t FBE_API_CALL fbe_api_logical_drive_get_id_by_serial_number(const fbe_u8_t *sn, fbe_u32_t sn_array_size, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_logical_drive_negotiate_and_validate_block_size(fbe_object_id_t object_id,
                                                                                  fbe_block_size_t exported_block_size,
                                                                                  fbe_block_size_t exported_optimal_block_size,
                                                                                  fbe_block_size_t expected_imported_block_size,
                                                                                  fbe_block_size_t expected_imported_optimal_block_size);
fbe_status_t  FBE_API_CALL 
fbe_api_logical_drive_get_phy_info(fbe_object_id_t object_id, 
                                   fbe_drive_phy_info_t * drive_phy_info_p);


fbe_status_t FBE_API_CALL fbe_api_logical_drive_interface_generate_ica_stamp(fbe_object_id_t object_id, fbe_bool_t preserve_ddbs);
fbe_status_t FBE_API_CALL fbe_api_logical_drive_interface_read_ica_stamp(fbe_object_id_t object_id, fbe_imaging_flags_t *ica_stamp);

fbe_status_t FBE_API_CALL fbe_api_logical_drive_interface_zero_firmware_luns(fbe_object_id_t object_id, fbe_packet_t* request_packet); 


/*! @} */ /* end of group fbe_api_logical_drive_interface */

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
 *  @ingroup fbe_classes
 */
//----------------------------------------------------------------


#endif /*FBE_API_LOGICAL_DRIVE_INTERFACE_H*/



