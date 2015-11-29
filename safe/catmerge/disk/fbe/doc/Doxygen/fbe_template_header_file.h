/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_template_header_file.h
 ***************************************************************************
 *
 * @brief
 *  This file contains a template for an fbe header file.
 *
 * @version
 *   mm/dd/yyyy:  Created. Creator Name.
 * 
 * We have an ingroup below to add this header file to a set of files that
 * are related.  This group is typically defined in the definition of a claass,
 * and will show up on the "groups" tab.
 * @ingroup template_class_files
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*! @def FBE_TEST
 *  @brief This is an example of a define comment.
 */
#define FBE_TEST

/*! @struct example_fbe_block_transport_identify_t 
 *  
 *  @brief The identity consists of a string that is used to                                            
 *         uniquely identify this device.
 *         This is the output of the block transport opcode of
 *         @ref FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY
 */
typedef struct example_fbe_block_transport_identify_s
{
    /*! The information used to identify the device is a simple  
     *  character string with two parts, a vendor identifier and
     *  a serial number.
     *  @see FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH
     */
	char example_identify_info[FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH];
}
example_fbe_block_transport_identify_t;

/*! @enum example_fbe_payload_block_operation_opcode_t 
 *  
 *  @brief This enum describes the type of block operation.  
 *
 *  @see fbe_payload_block_operation_t
 */
typedef enum example_fbe_payload_block_operation_opcode_e {
    /*! The block operation opcode is not valid.
     */
    EXAMPLE_FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID,

    /*! A standard read command.  The sg in the fbe_payload_t contains 
     *  the sg list to store this data in. 
     */
    EXAMPLE_FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
}
example_fbe_payload_block_operation_opcode_t;

/*************************
 * end file fbe_template_header_file.h
 *************************/


