#ifndef FBE_API_BLOCK_TRANSPORT_INTERFACE_H
#define FBE_API_BLOCK_TRANSPORT_INTERFACE_H

/*!**************************************************************************
 * @file fbe_api_block_transport_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_block_transport_interface.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *   11/05/2009:  Created. guov
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe_block_transport.h"

FBE_API_CPP_EXPORT_START

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
// Define the FBE API Block Transport Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_block_transport_interface_usurper_interface FBE API Block Transport Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Block Transport Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!*******************************************************************
 * @struct fbe_api_block_transport_control_create_edge_t 
 *********************************************************************
 *  @brief 
 *     This is the fbe_api structure passed as input with the block transport
 *     control code of FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE.
 *
 * @ingroup fbe_api_block_transport_interface
 * @ingroup fbe_api_block_transport_control_create_edge
 ********************************************************************/
typedef struct fbe_api_block_transport_control_create_edge_s
{
    fbe_object_id_t   server_id;        /* object id of the functional transport server */

    fbe_edge_index_t client_index;     /* index of the client edge (second edge of mirror object for example) */
    /*! This is the capacity in blocks of the extent exported by the server on 
     *  this edge.  This capacity is in terms of the block size that is exported
     *  by the server.
     *  refer to fbe_block_edge_t::exported_block_size
     */
    fbe_lba_t capacity;

    /*! This is the block offset of this extent from the start of the device. */
    fbe_lba_t offset;

    /*! This is the edge attributes.  When creating an edge between the virtual 
     *  drive and the provision drive the edge flag:
     *      o FBE_EDGE_FLAG_IGNORE_OFFSET
     *  should be set!
     */
    fbe_edge_flags_t edge_flags;

} fbe_api_block_transport_control_create_edge_t;
/*! @} */ /* end of group fbe_api_block_transport_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Block Transport Interface.  
// This is where all the function prototypes for the FBE API Block Transport.
//----------------------------------------------------------------
/*! @defgroup fbe_api_block_transport_interface FBE API Block Transport Interface
 *  @brief 
 *    This is the set of FBE API Block Transport Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_block_transport_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------

fbe_status_t FBE_API_CALL
fbe_api_block_transport_create_edge(fbe_object_id_t client_object_id,
                                    fbe_api_block_transport_control_create_edge_t *edge,
                                    fbe_packet_attr_t attribute);


fbe_status_t FBE_API_CALL
fbe_api_block_transport_send_block_operation(fbe_object_id_t object_id, 
                               fbe_package_id_t package_id,
                               fbe_block_transport_block_operation_info_t *block_op_info_p, 
                               fbe_sg_element_t *sg_p);

fbe_status_t FBE_API_CALL
fbe_api_block_transport_negotiate_info(fbe_object_id_t object_id, 
                               fbe_package_id_t package_id,
                               fbe_block_transport_negotiate_t *negotiate_info_p);

fbe_status_t FBE_API_CALL
fbe_api_block_transport_status_to_string(
                            fbe_payload_block_operation_status_t status,
                            fbe_u8_t* status_string_p,
                            fbe_u32_t string_length);

fbe_status_t FBE_API_CALL
fbe_api_block_transport_qualifier_to_string(
                            fbe_payload_block_operation_qualifier_t qualifier,
                            fbe_u8_t* qualifier_string_p,
                            fbe_u32_t string_length);

fbe_status_t FBE_API_CALL
fbe_api_block_transport_set_throttle_info(fbe_object_id_t client_object_id,
                                          fbe_block_transport_set_throttle_info_t *set_throttle_info);
fbe_status_t FBE_API_CALL
fbe_api_block_transport_get_throttle_info(fbe_object_id_t client_object_id,
                                          fbe_package_id_t package_id,
                                          fbe_block_transport_get_throttle_info_t *get_throttle_info);

fbe_status_t FBE_API_CALL
fbe_api_block_transport_read_data(fbe_object_id_t   object_id, 
                                  fbe_package_id_t  package_id,
                                  fbe_u8_t        * data, 
                                  fbe_lba_t         data_length, 
                                  fbe_lba_t         start_lba, 
                                  fbe_block_count_t io_size,
                                  fbe_u32_t         queue_depth);

fbe_status_t FBE_API_CALL
fbe_api_block_transport_write_data(fbe_object_id_t   object_id, 
                                   fbe_package_id_t  package_id,
                                   fbe_u8_t        * data, 
                                   fbe_lba_t         data_length, 
                                   fbe_lba_t         start_lba, 
                                   fbe_block_count_t io_size,
                                   fbe_u32_t         queue_depth);

/*! @} */ /* end of group fbe_api_block_transport_interface */

FBE_API_CPP_EXPORT_END

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

#endif /*FBE_API_BLOCK_TRANSPORT_INTERFACE_H*/


