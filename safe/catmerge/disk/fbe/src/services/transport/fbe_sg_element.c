/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sg_element.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions related to sg element and sg list
 *  manipulations.
 *
 * HISTORY
 *   11/10/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_sg_element.h"

/****************************************
 * LOCALLY DEFINED FUNCTIONS
 ****************************************/

/*!***************************************************************
 * @fn fbe_sg_element_count_entries(fbe_sg_element_t * sg_element_p)
 ****************************************************************
 * @brief
 *  This function counts the number of sg entries in the input
 *  sg list.
 *
 * @param sg_element_p - The sg list to count.
 *
 * @return
 *  The number of sg entries in this sg list.
 *
 * HISTORY:
 *  01/31/08 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t 
fbe_sg_element_count_entries(fbe_sg_element_t * sg_element_p)
{
    fbe_u32_t sg_entries = 0;
    
    /* If the sg list passed in is NULL, then we simply return
     * 0, which indicates error.
     */
    if (sg_element_p == NULL)
    {
        return sg_entries;
    }
    
    /* Simply iterate over the sg list, counting the number of entries.
     * We stop counting if we hit a zero count or NULL address.
     * We also stop counting sgs when we have exhausted the byte count.
     */
    while (fbe_sg_element_count(sg_element_p) != 0 && 
           fbe_sg_element_address(sg_element_p) != NULL)
    {
        sg_entries++;
        sg_element_p++;
    }
    
    return sg_entries;
}
/* end fbe_sg_element_count_entries() */

/*!***************************************************************
 * fbe_sg_element_count_entries_and_bytes()
 ****************************************************************
 * @brief
 *  This function counts the number of sg entries in the input
 *  sg list with a maximum bytes to count.
 *
 * @param sg_element_p - The sg list to count.
 * @param bytes - Max bytes to count
 * @param bytes_p - Total bytes found
 *
 * @return
 *  The number of sg entries in this sg list.
 *
 * @author
 *  10/12/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_u32_t 
fbe_sg_element_count_entries_and_bytes(fbe_sg_element_t * sg_element_p,
                                       fbe_u64_t max_bytes,
                                       fbe_u64_t *bytes_found_p)
{
    fbe_u32_t sg_entries = 0;
    fbe_u64_t total_bytes = 0;
    
    /* If the sg list passed in is NULL, then we simply return
     * 0, which indicates error.
     */
    if (sg_element_p == NULL)
    {
        *bytes_found_p = 0;
        return sg_entries;
    }
    
    /* Simply iterate over the sg list, counting the number of entries.
     * We stop counting if we hit a zero count or NULL address.
     * We also stop counting sgs when we have exhausted the byte count.
     */
    while ( (fbe_sg_element_count(sg_element_p) != 0) && 
            (fbe_sg_element_address(sg_element_p) != NULL) &&
            (total_bytes < max_bytes))
    {
        total_bytes += fbe_sg_element_count(sg_element_p);
        sg_entries++;
        sg_element_p++;
    }
    *bytes_found_p = total_bytes;
    return sg_entries;
}
/* end fbe_sg_element_count_entries_and_bytes() */
/*!***************************************************************
 * fbe_sg_element_count_entries_for_bytes()
 ****************************************************************
 * @brief
 *  This function counts the number of sg entries in the input
 *  sg list that represent the input number of bytes.
 *
 * @param sg_element_p - The sg list to count.
 * @param bytes - Bytes to count at the most.
 *
 * @return
 *  The number of sg entries required to represent the
 *  input number of bytes in this sg list.
 *  Zero is always an error, since at least one sg entry is
 *  needed to represent any non-zero number of bytes.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t 
fbe_sg_element_count_entries_for_bytes(fbe_sg_element_t *sg_element_p,
                                       fbe_u32_t bytes)
{
    fbe_u32_t sg_entries = 0;

    sg_entries = fbe_sg_element_count_entries_for_bytes_with_remainder(
        sg_element_p,
        bytes,
        NULL /* no remainder used. */ );
    
    return sg_entries;
}
/* end fbe_sg_element_count_entries_for_bytes() */

/*!***************************************************************
 * fbe_sg_element_count_entries_for_bytes_with_remainder()
 ****************************************************************
 * @brief
 *  This function counts the number of sg entries in the input
 *  sg list that represent the input number of bytes.
 *
 * @param sg_element_p - The sg list to count.
 * @param bytes - Bytes to count at the most.
 * @param b_remainder_p - pointer to boolean indicating remainder.
 *
 * @return
 *  The number of sg entries required to represent the
 *  input number of bytes in this sg list.
 *  Note that if there is an error and the sg list is
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t 
fbe_sg_element_count_entries_for_bytes_with_remainder(fbe_sg_element_t *sg_element_p,
                                                      fbe_u32_t bytes,
                                                      fbe_bool_t *b_remainder_p)
{
    fbe_u32_t sg_entries = 0;

    if (b_remainder_p != NULL)
    {
        *b_remainder_p = FBE_FALSE;
    }
    
    /* If the sg list passed in is NULL, then we simply return
     * 0, which indicates error.
     */
    if (sg_element_p == NULL)
    {
        return sg_entries;
    }
    
    /* Simply iterate over the sg list, counting the number of entries.
     * We stop counting if we hit a zero count or NULL address.
     * We also stop counting sgs when we have exhausted the byte count.
     */
    while (fbe_sg_element_count(sg_element_p) != 0 && 
           fbe_sg_element_address(sg_element_p) != NULL &&
           bytes > 0)
    {
        /* Check if this sg entry has more than we have left to count for.
         */
        if (fbe_sg_element_count(sg_element_p) > bytes)
        {
            /* Don't allow bytes to go negative, since there is a remainder.
             */
            bytes = 0;
            if (b_remainder_p != NULL)
            {
                *b_remainder_p = FBE_TRUE;
            }
        }
        else
        {
            bytes -= fbe_sg_element_count(sg_element_p);
        }
        sg_entries++;
        sg_element_p++;
    }

    if (b_remainder_p != NULL &&
        fbe_sg_element_count(sg_element_p) != 0)
    {
        /* There is something left in the sg.
         */
        *b_remainder_p = FBE_TRUE;
    }

    /* If we have bytes remaining, then the sg list is too short.
     * Set sg_entries to zero to indicate error.
     */
    if (bytes > 0)
    {
        sg_entries = 0;
    }
    
    return sg_entries;
}
/* end fbe_sg_element_count_entries_for_bytes_with_remainder() */

/*!***************************************************************
 * fbe_sg_element_count_entries_with_offset()
 ****************************************************************
 * @brief
 *  This function counts the number of sg entries in the input
 *  sg list that represent the input number of bytes.
 *
 * @param sg_element_p - The sg list to count.
 * @param bytes - Bytes to count at the most.
 * @param offset_bytes - Offset into this sg in bytes to start.
 *
 * @return
 *  The number of sg entries required to represent the
 *  input number of bytes in this sg list.
 *  Note that if there is an error and the sg list is
 *
 * HISTORY:
 *  11/21/07 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t 
fbe_sg_element_count_entries_with_offset(fbe_sg_element_t *sg_element_p,
                                         fbe_u32_t bytes,
                                         fbe_u32_t offset_bytes)
{
    fbe_u32_t sg_entries = 0;
    
    /* If the sg list passed in is NULL, then we simply return
     * 0, which indicates error.
     */
    if (sg_element_p == NULL)
    {
        return sg_entries;
    }
    
    /* Apply the offset to the start of the area to count.
     */
    while (sg_element_p->count != 0 && 
           sg_element_p->address != NULL &&
           offset_bytes > 0)
    {
        /* Check if this sg entry has more than we have left to count for.
         */
        if (fbe_sg_element_count(sg_element_p) > offset_bytes)
        {
            /* This is the remainder in the current sg entry.
             */
            fbe_u32_t remainder =
                fbe_sg_element_count(sg_element_p) - offset_bytes;
            
            /* This entry satisfied our offset request.
             * Add one sg for this sg element and set our offset to zero.
             */
            sg_entries++;
            offset_bytes = 0;
            
            if ( remainder >= bytes )
            {
                /* If our remainder is beyond what the user asked for return now.
                 */
                return sg_entries;
            }
            /* Remove remainder from the total, so
             * that we'll call fbe_sg_element_count_entries_for_bytes() below with
             * the amount remaining.
             */
            bytes -= remainder;
        }
        else
        {
            offset_bytes -= fbe_sg_element_count(sg_element_p);
        }
        sg_element_p++;
    }

    /* If we have bytes remaining, then the sg list is too short.
     * Set sg_entries to zero to indicate error.
     */
    if (offset_bytes > 0)
    {
        sg_entries = 0;
    }
    else
    {
        /* Count the additional sg entries required for this
         * number of bytes from this point in the sg list forward.
         */
        fbe_u32_t additional_sg_entries = 
            fbe_sg_element_count_entries_for_bytes(sg_element_p, bytes);
        
        if (additional_sg_entries == 0)
        {
            /* Some error occurred, return 0 to indicate this.
             */
            sg_entries = 0;
        }
        else
        {
            /* Success.  Add on the rest of the sgs to our count.
             */
            sg_entries += additional_sg_entries;
        }
    }
    return sg_entries;
}
/* end fbe_sg_element_count_entries_with_offset() */

/*!***************************************************************
 * fbe_sg_element_copy_list(fbe_sg_element_t *source_sg_p,
 *                          fbe_sg_element_t dest_sg_p,
 *                          fbe_u32_t bytes)
 ****************************************************************
 * @brief
 *  This function copies the given number of bytes from
 *  one sg list to another.
 *
 * @param source_sg_p - Sg list to copy from.
 * @param dest_sg_p - Sg list to copy to.
 * @param bytes - Total number of bytes to copy into the sg.
 *
 * @return sg ptr to the end of the destination sg list.
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
fbe_sg_element_t * fbe_sg_element_copy_list(fbe_sg_element_t *source_sg_p,
                                            fbe_sg_element_t *dest_sg_p,
                                            fbe_u32_t bytes)
{
    /* If one of the lists is NULL, then we simply return
     * NULL, which indicates error.
     */
    if (source_sg_p == NULL || dest_sg_p == NULL)
    {
        return NULL;
    }

    /* Until we have finished with the sg, and the bytes,
     * copy all the source to the destination.
     */
    while (source_sg_p->count > 0 &&
           bytes > 0 )
    {
        fbe_sg_element_copy_bytes(source_sg_p, dest_sg_p, bytes);
        
        bytes -= dest_sg_p->count;
        fbe_sg_element_increment(&source_sg_p);
        fbe_sg_element_increment(&dest_sg_p);
    }

    /* If the source sg list was not long enough, then we might have bytes 
     * remaining. 
     */
    if (bytes > 0)
    {
        /* We did not copy enough bytes, just return NULL to indicate error.
         */
        return NULL;
    }
    return dest_sg_p;
}
/* end fbe_sg_element_copy_list() */

/*!***************************************************************
 * fbe_sg_element_copy_list_with_offset(fbe_sg_element_t source_sg_p,
 *                                      fbe_sg_element_t dest_sg_p,
 *                                      fbe_u32_t bytes,
 *                                      fbe_u32_t offset_bytes)
 ****************************************************************
 * @brief
 *  This function copies the given number of bytes from
 *  one sg list to another.
 *
 * @param source_sg_p - Sg list to copy from.
 * @param dest_sg_p - Sg list to copy to.
 * @param bytes - Total number of bytes to copy into the sg.
 * @param offset_bytes - Offset to the point to copy.
 *
 * @return fbe_sg_element_t * to the end of the destination sg list.
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
fbe_sg_element_t * fbe_sg_element_copy_list_with_offset(fbe_sg_element_t *source_sg_p,
                                                        fbe_sg_element_t *dest_sg_p,
                                                        fbe_u32_t bytes,
                                                        fbe_u32_t offset_bytes)
{
    /* If one of the lists is NULL, then we simply return
     * NULL, which indicates error.
     */
    if (source_sg_p == NULL || dest_sg_p == NULL)
    {
        return NULL;
    }
    
    /* Apply the offset to the start of the area to copy.
     */
    while (fbe_sg_element_count(source_sg_p) != 0 && offset_bytes > 0)
    {
        /* Check if this sg entry has more than we have left to offset.
         */
        if (fbe_sg_element_count(source_sg_p) > offset_bytes)
        {
            /* We have passed the offset, but we have a 
             * partial sg element to copy.
             * This is the remainder in the current sg entry.
             */
            fbe_u32_t remainder =
                fbe_sg_element_count(source_sg_p) - offset_bytes;
            fbe_u32_t bytes_to_copy;
            
            if ( remainder > bytes )
            {
                /* If our remainder is beyond what the user asked to copy,
                 * then we will copy the remainder.
                 */
                bytes_to_copy = bytes;
                bytes = 0;
            }
            else
            {
                bytes -= remainder;
                bytes_to_copy = remainder;
            }

            /* Copy the rest of this sg element.
             */
            fbe_sg_element_init(dest_sg_p,
                                bytes_to_copy,
                                (fbe_sg_element_address(source_sg_p) +
                                 offset_bytes));
            fbe_sg_element_increment(&dest_sg_p);
            
            /* This entry satisfied our offset request.
             */
            offset_bytes = 0;
        }
        else
        {
            offset_bytes -= fbe_sg_element_count(source_sg_p);
        }
        fbe_sg_element_increment(&source_sg_p);
    }

    /* If we have bytes remaining, then the source sg list is too short.
     * Return NULL to indicate error.
     */
    if (offset_bytes > 0)
    {
        return NULL;
    }
    else if (bytes > 0)
    {
        /* Finish copying the list with the standard copy function.
         */
        dest_sg_p = fbe_sg_element_copy_list(source_sg_p, 
                                             dest_sg_p,
                                             bytes);
    }
    
    return dest_sg_p;
}
/* end fbe_sg_element_copy_list_with_offset() */

/*!***************************************************************
 * fbe_sg_element_copy_bytes(fbe_sg_element_t * const source_sg_p,   
 *                           fbe_sg_element_t * const dest_sg_p,
 *                           fbe_u32_t bytes)
 ****************************************************************
 * @brief
 *  This function copies the given number of bytes from
 *  one sg list to another.  This copies the sg entries, including
 *  the memory addresses in one source sg list to a destination sg list.
 *
 * @param source_sg_p - Sg list to copy from.
 * @param dest_sg_p - Sg list to copy to.
 * @param bytes - Total number of bytes to copy in this
 *               sg element.
 *
 * @return fbe_status_t - status of the operation.
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_sg_element_copy_bytes(fbe_sg_element_t * const source_sg_p,
                                       fbe_sg_element_t * const dest_sg_p,
                                       fbe_u32_t bytes )
{
    fbe_status_t status = FBE_STATUS_OK;
        
    /* Just perform the copy from source to destination.
     */
    status = fbe_sg_element_copy(source_sg_p, dest_sg_p);
    
    if ( dest_sg_p->count > bytes )
    {
        /* If we copied more than we need in this element,
         * then simply reduce this element's count.
         */
        dest_sg_p->count = bytes;
    }
    return status;
}
/* end fbe_sg_element_copy_bytes() */

/*!***************************************************************
 * fbe_sg_element_count_list_bytes(fbe_sg_element_t *sg_element_p)
 ****************************************************************
 * @brief
 *  This function counts the number of bytes in the input
 *  sg list.
 *
 * @param sg_element_p - The sg list to count.
 *
 * @return fbe_u32_t The number of bytes in this sg list.
 *
 * HISTORY:
 *  11/21/07 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t 
fbe_sg_element_count_list_bytes(fbe_sg_element_t *sg_element_p)
{    
    fbe_u32_t bytes = 0;
    
    /* If the sg list passed in is NULL, then we simply return
     * 0, which indicates error.
     */
    if (sg_element_p == NULL)
    {
        return 0;
    }
    
    /* Simply iterate over the sg list, counting the number of entries.
     * We stop counting if we hit a zero count or NULL address.
     */
    while (sg_element_p->count != 0 && sg_element_p->address != NULL)
    {
        bytes += sg_element_p->count;
        sg_element_p++;
    }
    
    return bytes;
}
/* end fbe_sg_element_count_list_bytes() */


/*!***************************************************************
 * fbe_sg_element_bytes_is_exact_match()
 ****************************************************************
 * @brief
 *  This function validates that sg list has the expected number of
 *  bytes.
 *
 * @param sg_element_p - The sg list to count.
 * @param expected_byte_count - expected byte count
 *
 * @return fbe_bool_t -  FBE_TRUE if exact match.
 *
 * HISTORY:
 *  6/05/14 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_bool_t 
fbe_sg_element_bytes_is_exact_match(fbe_sg_element_t *sg_element_p, fbe_u64_t expected_byte_count)
{        
    fbe_u32_t bytes = 0;
    
        
    if (sg_element_p == NULL)
    {
        return FBE_FALSE;
    }
    
    /* Simply iterate over the sg list, counting the number of entries.
     * We stop counting if we hit a zero count or NULL address.
     */
    while (sg_element_p->count != 0 && 
           sg_element_p->address != NULL)
    {
        bytes += sg_element_p->count;
        sg_element_p++;

        /* if we've read more than expected, then stop. This could be a malformed sgl. */
        if (bytes > expected_byte_count)
        {
            break;
        }
    }
    
    if (bytes == expected_byte_count)
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}


/*!**************************************************************
 * fbe_sg_element_adjust_offset()
 ****************************************************************
 * @brief
 *  Adjust the sg element's sg ptr and offset.
 * 
 *  Given an input offset that is within the sg list,
 *  we adjust the sg_p to point to the sg containing the block we
 *  need.
 * 
 *  We also adjust the offset to be an offset into that sg element
 *  of the given offset.
 *
 * @param sg_pp - Pointer to sg to adjust.
 * @param offset_p - Pointer to input/output offset in bytes.
 *
 * @return None.
 *
 * @author
 *  5/16/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_sg_element_adjust_offset(fbe_sg_element_t **sg_pp, fbe_u32_t *offset_p)
{
    fbe_sg_element_t *sg_p = *sg_pp;
    fbe_u32_t offset = *offset_p;

    if ((offset != 0) &&
        (offset >= (fbe_s32_t) sg_p->count))
    {
        /* Simply loop until the offset is valid 
         * for this sg entry. 
         */
        while (offset >= (fbe_s32_t) sg_p->count)
        {
            offset -= sg_p->count;
            sg_p++;
        }
    }
    *sg_pp = sg_p;
    *offset_p = offset;
}
/******************************************
 * end fbe_sg_element_adjust_offset()
 ******************************************/
/*******************************
 * fbe_sg_element.c
 ******************************/
