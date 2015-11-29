#ifndef FBE_PAYLOAD_PRE_READ_DESCRIPTOR_H
#define FBE_PAYLOAD_PRE_READ_DESCRIPTOR_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_payload_pre_read_descriptor.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the fbe_payload_pre_read_descriptor_t
 *  structure and related api functions.
 * 
 *  The fbe_payload_pre_read_descriptor_t is a part of the
 *  fbe_payload_block_operation_t.  The pre-read descriptor provides a
 *  description of a range of blocks that is being passed in with a write
 *  operation as read data.  This pre-read data is used on write operations that
 *  are not aligned to the optimum block size.
 *
 * @version
 *   9/2008:  Created. Peter Puhov.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_sg_element.h"

/*! @defgroup fbe_payload_pre_read_descriptor FBE Payload Pre-Read Descriptor 
 *  
 *  This is the set of definitions that comprise the fbe payload's pre-read
 *  descriptor.  The pre-read descriptor provides a
 *  description of a range of blocks that is being passed in with a write
 *  operation as read data.  This pre-read data is used on write operations that
 *  are not aligned to the optimum block size.
 *  
 *  @ingroup fbe_payload_block_operation
 * @{
 */ 

/*! @struct fbe_payload_pre_read_descriptor_t 
 *  @brief This is the pre-read descriptor needed for writes to the logical
 *         drive object when a write is not aligned to the optimal block size.
 *         This pre-read descriptor allows us to pass data that can be used as
 *         part of the write operation to the physical media.
 *  
 *         In the example of a 520 exported and 512 exported below in order to
 *         write a single 520 we might need to update two underlying 512s.
 *         Thus, there is data in the first edge and last edge that is required
 *         in order to complete the write.  This data is the pre-read data.
 *         The client will pass in the pre-read range and then the server will
 *         use this data in order to complete the write.
 *  
 *  @verbatim
 * |-----------pre-read data--------------------|
 *\|/                                          \|/ 
 * |--------------|ioioioioioioio|--------------|   <-- Exported 520
 *     |--------------|--------------|------------- <-- Imported 512
 *    /|\        /|\            /|\ /|\
 *     |__________|              |___|
 *     First edge.                 Last edge.
 *  
 *  @endverbatim
 */
typedef struct fbe_payload_pre_read_descriptor_s
{
    /*! Start logical block address address of this pre-read range. 
     */
    fbe_lba_t lba;

    /*! Blocks in this pre-read range. 
     */
    fbe_block_count_t block_count;

    /*! Pointer to the scatter gather list with pre-read data for this lba  
     *  range.  
     */
    fbe_sg_element_t *sg_list;
}fbe_payload_pre_read_descriptor_t;

/*!**************************************************************
 * @fn fbe_payload_pre_read_descriptor_get_lba(
 *       fbe_payload_pre_read_descriptor_t * pre_read_descriptor,
 *       fbe_lba_t * lba)
 ****************************************************************
 * @brief
 *  This will return the pre-read descriptor's lba.
 *
 * @param pre_read_descriptor - ptr to the pre-read descriptor
 * @param lba - ptr to the lba value to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_pre_read_descriptor_t::lba
 *
 ****************************************************************/

static __forceinline fbe_status_t
fbe_payload_pre_read_descriptor_get_lba(fbe_payload_pre_read_descriptor_t * pre_read_descriptor, fbe_lba_t * lba)
{
	* lba = pre_read_descriptor->lba;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_pre_read_descriptor_set_lba(
 *       fbe_payload_pre_read_descriptor_t * pre_read_descriptor,
 *       fbe_lba_t lba)
 ****************************************************************
 * @brief
 *  This will set the pre-read descriptor's lba.
 *
 * @param pre_read_descriptor - ptr to the pre-read descriptor
 * @param lba - ptr to the lba value to set.
 *
 * @return Always FBE_STATUS_OK   
 * 
 * @see fbe_payload_pre_read_descriptor_t::lba
 *
 ****************************************************************/

static __forceinline fbe_status_t 
fbe_payload_pre_read_descriptor_set_lba(fbe_payload_pre_read_descriptor_t * pre_read_descriptor, fbe_lba_t lba)
{
	pre_read_descriptor->lba = lba;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_pre_read_descriptor_get_block_count(
 *       fbe_payload_pre_read_descriptor_t * pre_read_descriptor,
 *       fbe_block_count_t * block_count)
 ****************************************************************
 * @brief
 *  This will return the pre-read descriptor's block_count.
 *
 * @param pre_read_descriptor - ptr to the pre-read descriptor
 * @param block_count - ptr to the block_count value to return.
 *
 * @return Always FBE_STATUS_OK   
 * 
 * @see fbe_payload_pre_read_descriptor_t::block_count
 *
 ****************************************************************/

static __forceinline fbe_status_t
fbe_payload_pre_read_descriptor_get_block_count(fbe_payload_pre_read_descriptor_t * pre_read_descriptor,
												fbe_block_count_t * block_count)
{
	* block_count = pre_read_descriptor->block_count;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_pre_read_descriptor_set_block_count(
 *       fbe_payload_pre_read_descriptor_t * pre_read_descriptor,
 *       fbe_block_count_t block_count)
 ****************************************************************
 * @brief
 *  This will set the pre-read descriptor's block_count.
 *
 * @param pre_read_descriptor - ptr to the pre-read descriptor
 * @param block_count - ptr to the block_count value to set.
 *
 * @return Always FBE_STATUS_OK   
 * 
 * @see fbe_payload_pre_read_descriptor_t::block_count
 *
 ****************************************************************/

static __forceinline fbe_status_t 
fbe_payload_pre_read_descriptor_set_block_count(fbe_payload_pre_read_descriptor_t * pre_read_descriptor, 
                                                fbe_block_count_t block_count)
{
	pre_read_descriptor->block_count = block_count;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_pre_read_descriptor_get_sg_list(
 *       fbe_payload_pre_read_descriptor_t * pre_read_descriptor,
 *       fbe_sg_element_t ** sg_list)
 ****************************************************************
 * @brief
 *  This will return the pre-read descriptor's
 *  scatter gather list pointer.
 *
 * @param pre_read_descriptor - ptr to the pre-read descriptor
 * @param sg_list - ptr to the scatter gather list pointer to return.
 *
 * @return Always FBE_STATUS_OK   
 * 
 * @see fbe_payload_pre_read_descriptor_t::sg_list
 *
 ****************************************************************/

static __forceinline fbe_status_t
fbe_payload_pre_read_descriptor_get_sg_list(fbe_payload_pre_read_descriptor_t * pre_read_descriptor,
											fbe_sg_element_t ** sg_list)
{
	* sg_list = pre_read_descriptor->sg_list;
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_payload_pre_read_descriptor_set_sg_list(
 *       fbe_payload_pre_read_descriptor_t * pre_read_descriptor,
 *       fbe_sg_element_t *sg_list)
 ****************************************************************
 * @brief
 *  This will set the pre-read descriptor's scatter gather pointer.
 * 
 * @param pre_read_descriptor - ptr to the pre-read descriptor
 * @param sg_list - ptr to the sg_list value to set.
 *
 * @return Always FBE_STATUS_OK   
 * 
 * @see fbe_payload_pre_read_descriptor_t::sg_list
 *
 ****************************************************************/

static __forceinline fbe_status_t 
fbe_payload_pre_read_descriptor_set_sg_list(fbe_payload_pre_read_descriptor_t * pre_read_descriptor, 
                                            fbe_sg_element_t * sg_list)
{
	pre_read_descriptor->sg_list = sg_list;
	return FBE_STATUS_OK;
}

/*! @} */ /* end of group fbe_payload_pre_read_descriptor */

#endif /* FBE_PAYLOAD_PRE_READ_DESCRIPTOR_H */
