/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_bitmap.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code to access the bitmap for the provision drive
 *  object.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   05/20/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_provision_drive.h"
#include "fbe_provision_drive_private.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/


/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/*!****************************************************************************
 * fbe_provision_drive_bitmap_get_chunk_index_for_lba()
 ******************************************************************************
 * @brief
 *  This function is used to get the chunk index for the given lba.
 *
 * @param provision_drive_p                     - Provision drive object.
 * @param lba                                           - lba for which we return the chunk index.
 * @param chunk_size                            - size of the chunk.
 * @param chunk_index_p                         - pointer to chunk index (out).
 *
 * @return fbe_status_t                         - status of the operation.
 *  
 *
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_get_chunk_index_for_lba(fbe_provision_drive_t * provision_drive_p,
                                                   fbe_lba_t lba, 
                                                   fbe_u32_t chunk_size,
                                                   fbe_chunk_index_t * chunk_index_p)
{
    fbe_lba_t capacity = FBE_LBA_INVALID;

    /* If checkpoint is invalid then rethrn error. */
    if(lba == FBE_LBA_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If checkpoint is beyond exported capacity then return error. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &capacity);
    if(lba >= capacity)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the chunk index for the given LBA. */
    *chunk_index_p = lba / chunk_size;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_get_chunk_index_for_lba()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_bitmap_get_lba_for_the_chunk_index()
 ******************************************************************************
 * @brief
 *  This function is used to get the corresponding lba (start_lba) of the 
 *  particular chunk index.
 *
 * @param provision_drive_p                     - Provision drive object.
 * @param chunk_index                           - chunk index.
 * @param chunk_size                            - size of the chunk.
 * @param chunk_index_p                         - pointer to lba (out).
 *
 * @return fbe_status_t                         - status of the operation.
 *
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_get_lba_for_the_chunk_index(fbe_provision_drive_t * provision_drive_p,
                                                       fbe_chunk_index_t chunk_index,
                                                       fbe_u32_t chunk_size,
                                                       fbe_lba_t * checkpoint_lba_p)
{
    fbe_lba_t           capacity = FBE_LBA_INVALID;
    fbe_chunk_index_t   last_chunk_index = FBE_LBA_INVALID;

    /* If checkpoint is beyond exported capacity then return error. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &capacity);
    last_chunk_index = capacity / chunk_size;

    /* If chunk index is greater than last chunk index then return error. */
    if(chunk_index > last_chunk_index) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the chunk index for the given LBA. */
    *checkpoint_lba_p = chunk_index * chunk_size;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_get_lba_for_the_chunk_index()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_bitmap_get_total_number_of_chunks()
 ******************************************************************************
 * @brief
 *  This function is used to get the total number of chunks for the paged 
 *  metadata.
 *
 * @param provision_drive_p             - Provision drive object.
 * @param chunk_index                   - chunk index.
 * @param total_number_of_chunks_p      - total number of chunks (out).
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_get_total_number_of_chunks(fbe_provision_drive_t * provision_drive_p,
                                                      fbe_chunk_size_t chunk_size,
                                                      fbe_chunk_count_t * total_number_of_chunks_p)
{
    fbe_lba_t   capacity = FBE_LBA_INVALID;

    /* Get the total number of chunks based on last chunk index. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &capacity);
    *total_number_of_chunks_p = (fbe_u32_t )(capacity / chunk_size);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_get_total_number_of_chunks()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_bitmap_is_chunk_marked_for_zero()
 ******************************************************************************
 * @brief
 *  This function expects caller to provide the paged metadata bits and number
 *  chunks which represent these metadata bits, it finds out the chunk which
 *  requires zero and returns number of chunk which is marked for zero.
 *  
 * @param paged_metadata_bits           - Provision drive object.
 * @param chunk_index                   - chunk index.
 * @param total_number_of_chunks_p      - total number of chunks (out).
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_is_chunk_marked_for_zero(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                    fbe_bool_t * is_chunk_marked_for_zero_p)
{
    if((paged_data_bits_p->need_zero_bit == 1) || 
       ((paged_data_bits_p->user_zero_bit == 1) && 
        (paged_data_bits_p->consumed_user_data_bit == 1)))
    {
        *is_chunk_marked_for_zero_p = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_is_chunk_marked_for_zero()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_bitmap_get_number_of_chunks_marked_for_zero()
 ******************************************************************************
 * @brief
 *  This function expects caller to provide the paged metadata bits and number
 *  chunks which represent these metadata bits, it finds out the chunk which
 *  requires zero and returns number of chunk which is marked for zero.
 *  
 * @param paged_metadata_bits           - Provision drive object.
 * @param chunk_index                   - chunk index.
 * @param total_number_of_chunks_p      - total number of chunks (out).
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_get_number_of_chunks_marked_for_zero(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                fbe_chunk_count_t chunk_count,
                                                                fbe_chunk_count_t * number_of_chunks_marked_for_zero_p)
{
    fbe_chunk_index_t     chunk_index = 0;

    *number_of_chunks_marked_for_zero_p = 0;
    while(chunk_index < chunk_count)
    {
        if((paged_data_bits_p[chunk_index].need_zero_bit == 1) || 
           ((paged_data_bits_p[chunk_index].user_zero_bit == 1) && 
            (paged_data_bits_p[chunk_index].consumed_user_data_bit == 1)))
        {
            (*number_of_chunks_marked_for_zero_p)++;
        }
        chunk_index++;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_get_number_of_chunks_marked_for_zero()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_bitmap_get_next_zeroed_chunk()
 ******************************************************************************
 * @brief
 *  This function is used to get the next zeroed chunk index from the provided
 *  paged data bits by the caller.
 *  
 * @param paged_metadata_bits           - Provision drive object.
 * @param current_chunk_index           - current chunk index in paged data bits.
 * @param chunk_count                   - chunk count of the paged data bits.
 * @param next_zeroed_chunk_p           - next chunk index in paged data bits 
 *                                        which is zeroed(out).
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_get_next_zeroed_chunk(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                 fbe_chunk_index_t current_chunk_index,
                                                 fbe_chunk_count_t chunk_count,
                                                 fbe_chunk_index_t * next_zeroed_chunk_index_in_paged_bits_p)
{
    fbe_chunk_index_t     chunk_index = current_chunk_index;

    /* initialize the next zeroed chunk index in paged bits as invalid. */
    *next_zeroed_chunk_index_in_paged_bits_p = FBE_CHUNK_INDEX_INVALID;

    while(chunk_index < chunk_count)
    {
        if(!((paged_data_bits_p[chunk_index].need_zero_bit == 1) || 
             ((paged_data_bits_p[chunk_index].user_zero_bit == 1)&&
              (paged_data_bits_p[chunk_index].consumed_user_data_bit == 1))))
        {
            *next_zeroed_chunk_index_in_paged_bits_p = chunk_index;
            break;
        }
        chunk_index++;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_get_next_zeroed_chunk()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_bitmap_get_next_need_zero_chunk()
 ******************************************************************************
 * @brief
 *  This function is used to get the next need zero chunk index from the provided
 *  paged data bits by the caller.
 *  
 * @param paged_metadata_bits           - Provision drive object.
 * @param current_chunk_index           - current chunk index in paged data bits.
 * @param chunk_count                   - chunk count of the paged data bits.
 * @param next_need_zero_chunk_p        - next chunk index in paged data bits 
 *                                        which needs zero(out).
 *
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_get_next_need_zero_chunk(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                    fbe_chunk_index_t current_chunk_index,
                                                    fbe_chunk_count_t chunk_count,
                                                    fbe_chunk_index_t * next_need_zero_index_in_paged_bits_p)
{
    fbe_chunk_index_t     chunk_index = current_chunk_index;

    /* initialize the next need zero index in paged bits as invalid. */
    *next_need_zero_index_in_paged_bits_p = FBE_CHUNK_INDEX_INVALID;

    while(chunk_index < chunk_count)
    {
        if((paged_data_bits_p[chunk_index].need_zero_bit == 1) || 
           ((paged_data_bits_p[chunk_index].user_zero_bit == 1) && 
            (paged_data_bits_p[chunk_index].consumed_user_data_bit == 1)))
        {
            *next_need_zero_index_in_paged_bits_p = chunk_index;
            break;
        }
        chunk_index++;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_get_next_need_zero_chunk()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_bitmap_get_next_invalid_chunk()
 ******************************************************************************
 * @brief
 *  This function is used to get the next zeroed chunk index from the provided
 *  paged data bits by the caller.
 *  
 * @param paged_metadata_bits           - Provision drive object.
 * @param current_chunk_index           - current chunk index in paged data bits.
 * @param chunk_count                   - chunk count of the paged data bits.
 * @param next_invalid_chunk_index_in_paged_bits_p           - next chunk index in paged data bits 
 *                                        which is invalid.
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  03/06/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_get_next_invalid_chunk(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                 fbe_chunk_index_t current_chunk_index,
                                                 fbe_chunk_count_t chunk_count,
                                                 fbe_chunk_index_t * next_invalid_chunk_index_in_paged_bits_p)
{
    fbe_chunk_index_t     chunk_index = current_chunk_index;
    fbe_bool_t  is_invalid = FBE_TRUE;

    /* initialize the next invalid chunk index in paged bits as invalid. */
    *next_invalid_chunk_index_in_paged_bits_p = FBE_CHUNK_INDEX_INVALID;

    while(chunk_index < chunk_count)
    {
        fbe_provision_drive_bitmap_is_paged_data_invalid(&paged_data_bits_p[chunk_index], &is_invalid);
        if(is_invalid)
        {
            *next_invalid_chunk_index_in_paged_bits_p = chunk_index;
            break;
        }
        chunk_index++;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_get_next_invalid_chunk()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_bitmap_is_paged_data_invalid()
 ******************************************************************************
 * @brief
 *  This function checks if the paged data is valid or not by looking at the
 *  valid bits
 *
 * @param paged_metadata_bits           - Provision drive object.
 * @param is_paged_data_valid           - return the paged data valid or not
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  03/06/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_is_paged_data_invalid(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                    fbe_bool_t * is_paged_data_invalid)
{
    *is_paged_data_invalid = FBE_FALSE;
    if(!paged_data_bits_p->valid_bit)
    {
        *is_paged_data_invalid = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_is_paged_data_invalid()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_bitmap_get_number_of_invalid_chunks()
 ******************************************************************************
 * @brief
 *  This function expects caller to provide the paged metadata bits and number
 *  chunks which represent these metadata bits, it finds out the chunk which
 *  is invalid and returns number of chunks that are invalid
 *  
 * @param paged_metadata_bits           - Provision drive object.
 * @param chunk_index                   - chunk index.
 * @param total_number_of_chunks_p      - total number of chunks (out).
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_bitmap_get_number_of_invalid_chunks(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                        fbe_chunk_count_t chunk_count,
                                                        fbe_chunk_count_t * total_number_of_chunks)
{
    fbe_chunk_index_t     chunk_index = 0;
    fbe_bool_t            is_paged_data_invalid;

    *total_number_of_chunks = 0;
    while(chunk_index < chunk_count)
    {
        fbe_provision_drive_bitmap_is_paged_data_invalid(&paged_data_bits_p[chunk_index],
                                                         &is_paged_data_invalid);
        if(is_paged_data_invalid)
        {
            (*total_number_of_chunks)++;
        }
        chunk_index++;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_bitmap_get_number_of_invalid_chunks()
 ******************************************************************************/

/*******************************
 * end fbe_provision_drive_main.c
 *******************************/
