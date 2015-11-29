#ifndef FBE_LOGICAL_DRIVE_H
#define FBE_LOGICAL_DRIVE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the logical
 *  drive.
 * 
 * @ingroup logical_drive_class_files
 * 
 * HISTORY
 *   5/29/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe_imaging_structures.h"
#include "fbe/fbe_sector.h"


/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup logical_drive_class Logical Drive Class 
 *  @brief This is the set of definitions for the logical drive.
 *  @ingroup fbe_base_discovered
 */ 
/*! @defgroup logical_drive_usurper_interface Logical Drive Usurper Interface
 *  @brief This is the set of definitions that comprise the logical drive class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */ 

/*!********************************************************************* 
 * @enum fbe_logical_drive_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the logical drive specific control codes. 
 * These are control codes specific to the logical drive, which only 
 * the logical drive object will accept. 
 *         
 **********************************************************************/
typedef enum
{
	FBE_LOGICAL_DRIVE_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_LOGICAL_DRIVE),

    /*! This opcode allows returning a @ref fbe_logical_drive_attributes_t "fbe_logical_drive_attributes_t"
     * structure, that includes the imported block size and capacity.
     */
    FBE_LOGICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES,

    /*! This opcode allows us to set attributes of the logical drive. 
     * Again, this takes a @ref  fbe_logical_drive_attributes_t "fbe_logical_drive_attributes_t" 
     * structure as input. 
     */
    FBE_LOGICAL_DRIVE_CONTROL_CODE_SET_ATTRIBUTES,

	FBE_LOGICAL_DRIVE_CONTROL_GET_DRIVE_INFO, /* This will help topology service to find appropriate drive */
	FBE_LOGICAL_DRIVE_CONTROL_GENERATE_ICA_STAMP, /* stamp the ICA on the drive*/
	FBE_LOGICAL_DRIVE_CONTROL_READ_ICA_STAMP, /* read the ICA stamp from the drive*/
	FBE_LOGICAL_DRIVE_CONTROL_SET_PVD_EOL,
	FBE_LOGICAL_DRIVE_CONTROL_CLEAR_PVD_EOL,

	FBE_LOGICAL_DRIVE_CONTROL_CODE_LAST
}
fbe_logical_drive_control_code_t;

/*!********************************************************************* 
 * @struct fbe_logical_drive_attributes_t
 *  
 * @brief 
 *  These are logical drive attributes we can get/set with control code of
 *  @ref FBE_LOGICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES or
 *  @ref FBE_LOGICAL_DRIVE_CONTROL_CODE_SET_ATTRIBUTES.
 **********************************************************************/
typedef struct
{
    /*! This is the number of bytes in the imported block.
     */
    fbe_block_size_t imported_block_size;

    /*! This is the number of blocks imported. 
     *  Each of these blocks is "imported_block_size" bytes. 
     */
    fbe_lba_t imported_capacity;

    /*! This is the identify information from the logical drive. 
     * This gets fetched at init time. 
     */
    fbe_block_transport_identify_t initial_identify;

    /*! This is the last copy of the identify information that was received. 
     */
    fbe_block_transport_identify_t last_identify;

    struct  
    {
        fbe_bool_t      b_optional_queued;           /*!< True if objects queued to optional queue. */
        fbe_bool_t      b_low_queued;                /*!< True if objects queued to low queue. */
        fbe_bool_t      b_normal_queued;             /*!< True if objects queued to normal queue. */
        fbe_bool_t      b_urgent_queued;             /*!< True if objects queued to urgent queue. */
        fbe_u32_t       number_of_clients;           /*!< Number of client edges attached.        */
        fbe_object_id_t server_object_id;            /*!< Object id of the server object. */
        fbe_u32_t		outstanding_io_count;        /*!< Number of I/Os currently in flight. */
        fbe_u32_t		outstanding_io_max;			 /*!< The maximum number of outstanding IO's */
        fbe_u32_t		tag_bitfield;				 /*!< Used for tag allocations */
        fbe_u32_t       attributes; /*!< Collection of block transport flags. See @ref fbe_block_transport_flags_e. */
    }
    server_info;
} 
fbe_logical_drive_attributes_t;

typedef enum fbe_logical_drive_flags_e
{
	FBE_LOGICAL_DRIVE_FLAG_INVALID,
	FBE_LOGICAL_DRIVE_FLAG_LAST
}
fbe_logical_drive_flags_t;

/*! @} */ /* end of group logical_drive_interface */

/*! @defgroup logical_drive_library Logical Drive Library
 *  
 * @brief 
 *  This is code which is exported to clients so they can perform calculations
 *  to determine the sizes of pre-reads for write operations. 
 *  
 * @details 
 *  In order to access this library, please include fbe_logical_drive.h.
 * 
 *  In order to link with this library, please add
 *  fbe_logical_drive_library.lib to your SOURCES.mak.
 *  
 *  @ingroup logical_drive_class
 *  @{
 */
void fbe_logical_drive_get_pre_read_lba_blocks_for_write( fbe_lba_t exported_block_size,
                                                          fbe_lba_t exported_opt_block_size,
                                                          fbe_lba_t imported_block_size,
                                                          fbe_lba_t *const lba_p,
                                                          fbe_block_count_t *const blocks_p );
void fbe_ldo_map_lba_blocks(fbe_lba_t lba,
                            fbe_block_count_t blocks,
                            fbe_block_size_t exported_block_size,
                            fbe_block_size_t exported_opt_block_size,
                            fbe_block_size_t imported_block_size,
                            fbe_block_size_t imported_opt_block_size,
                            fbe_lba_t *const output_lba_p,
                            fbe_block_count_t *const output_blocks_p);
fbe_lba_t
fbe_ldo_get_media_error_lba( fbe_lba_t media_error_pba,
                             fbe_lba_t io_start_lba,
                             fbe_block_size_t exported_block_size,
                             fbe_block_size_t exported_opt_block_size,
                             fbe_block_size_t imported_block_size,
                             fbe_block_size_t imported_opt_block_size);
/*! @} */ /* end of group logical_drive_library */

/*! @page Logical_drive_page0 Logical Drive Class 
   
  <hr> 
  @section logical_drive_interfaces_sec Logical Drive Interfaces 
   
   The logical drive object provides block conversion functionality for
   converting from the imported physical block size of a device to the exported
   block size that the clients expect.
   
  @subsection logical_drive_interfaces_subsec Logical Drive Usurper Interfaces 
  The logical drive has its own set of usurper commands.  It also supports 
  usurper commands of the block transport and the discovered class. 
  - @ref logical_drive_usurper_interface
  - @ref fbe_block_transport_usurper_interface 
  - FBE Discovered Usurper Interface 
   
  @subsection ldo_functional_interfaces_subsec Logical Drive Functional Transport Interfaces 
  
  - @ref fbe_block_transport_interfaces 
  - FBE Discovery Transport Interface 
   
  <hr> 
  @ingroup logical_drive_class 
*/

/*! @defgroup logical_drive_class_files Logical Drive Class Files 
 *  @brief This is the set of files for the logical drive.
 *  @ingroup fbe_classes
 *  @ingroup logical_drive_class
 */


/* FBE_LOGICAL_DRIVE_CONTROL_GET_DRIVE_INFO */
typedef struct fbe_logical_drive_control_get_drive_info_s{
	fbe_u32_t port_number; 
	fbe_u32_t enclosure_number; 
	fbe_u32_t slot_number; 

	/* Various generic information 
		Such as speed, capacity, partitions, etc. 
	*/
    fbe_block_transport_identify_t identify;

    fbe_lba_t   exported_capacity;
}fbe_logical_drive_control_get_drive_info_t;


/*FBE_LOGICAL_DRIVE_CONTROL_READ_ICA_STAMP*/
typedef struct fbe_logical_drive_read_ica_stamp_s{
	/*technically we need to read only fbe_imaging_flags_t but for simlicty and not having to allocate another block
	in the LDO, we'll use the user memory but it has to be contigus*/
	fbe_u8_t 	read_ica[FBE_BE_BYTES_PER_BLOCK + sizeof(fbe_sg_element_t) * 2];
}fbe_logical_drive_read_ica_stamp_t;

/*FBE_LOGICAL_DRIVE_CONTROL_GENERATE_ICA_STAMP*/
typedef struct fbe_logical_drive_generate_ica_stamp_s{
    fbe_bool_t  preserve_ddbs;
    void *      write_buffer_ptr;
}fbe_logical_drive_generate_ica_stamp_t;


#endif /* FBE_LOGICAL_DRIVE_H */

/*************************
 * end file fbe_logical_drive_api.h
 *************************/
