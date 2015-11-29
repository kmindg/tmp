#ifndef FBE_API_SEP_IO_INTERFACE_H
#define FBE_API_SEP_IO_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_sep_io_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the SEP IO object.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *   5/22/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"

#include "fbe/fbe_payload_block_operation.h"
#include "fbe/fbe_transport.h"

//----------------------------------------------------------------
// Define the top level group for the FBE Storage Extent Package APIs
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_class FBE Storage Extent Package APIs
 *  @brief 
 *    This is the set of definitions for FBE Storage Extent Package APIs.
 *
 *  @ingroup fbe_api_storage_extent_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API SEP IO Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_sep_io_interface_usurper_interface FBE API SEP IO Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API SEP IO Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!*******************************************************************
 * @def FBE_API_DEFAULT_SEP_BLOCK_SIZE  
 *********************************************************************
 * @brief 
 *   This is the max number of SEP Block size.
 *
 * @ingroup fbe_api_sep_io_interface
 *********************************************************************/
#define FBE_API_DEFAULT_SEP_BLOCK_SIZE      520

/*!*******************************************************************
 * @struct fbe_api_sep_block_status_values_t
 *********************************************************************
 * @brief
 *  The purpose of this structure is to contain all the status we need 
 *  on a block operation.
 *
 * @ingroup fbe_api_sep_io_interface
 * @ingroup fbe_api_sep_block_status_values
 *********************************************************************/
typedef struct fbe_api_sep_block_status_values_s
{
   fbe_payload_block_operation_status_t status;       /*!< Block Operation Status */
   fbe_payload_block_operation_qualifier_t qualifier; /*!< Block Operation Qualifier */
   fbe_lba_t media_error_lba;                         /*!< Media Error LBA */
   fbe_object_id_t pdo_object_id;                    /*!< object id of the physical drive to which the IO request is sent.*/
}
fbe_api_sep_block_status_values_t;
/*! @} */ /* end of group fbe_api_sep_io_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API SEP IO Interface.  
// This is where all the function prototypes for the FBE API SEP IO.
//----------------------------------------------------------------
/*! @defgroup fbe_api_sep_io_interface FBE API SEP IO Interface
 *  @brief 
 *    This is the set of FBE API SEP IO Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_sep_io_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t 
fbe_api_sep_io_interface_send_block_payload(fbe_object_id_t object_id,
                                            fbe_payload_block_operation_opcode_t opcode,
                                            fbe_lba_t lba,
                                            fbe_block_count_t blocks,
                                            fbe_block_size_t block_size,
                                            fbe_block_size_t opt_block_size,
                                            fbe_sg_element_t * const sg_p,
                                            fbe_payload_pre_read_descriptor_t * const pre_read_desc_p,
                                            fbe_packet_attr_t * const transport_qualifier_p,
                                            fbe_api_sep_block_status_values_t * const block_status_p);
fbe_status_t 
fbe_api_sep_io_interface_get_block_size(const fbe_object_id_t object_id,
                                        fbe_block_transport_negotiate_t *const negotiate_p,
                                        fbe_payload_block_operation_status_t *const block_status_p,
                                        fbe_payload_block_operation_qualifier_t *const block_qualifier);

/*! @} */ /* end of group fbe_api_sep_io_interface */

//----------------------------------------------------------------
// Define the group for all FBE Storage Extent Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Storage Extent
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_interface_class_files FBE Storage Extent Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Storage Extent Package APIs Interface.
 * 
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /*FBE_API_SEP_IO_INTERFACE_H*/


