/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_error_region.c
 ***************************************************************************
 *
 * @brief
 *       This file contains functions for processing the fbe_xor_error_t_REGION.
 *       This structure contains the information about the error that is 
 *       encountered. It gives the PBA, error type, disk position on which 
 *       error has occurred and number of consecutive blocks.
 *       This structure is present in the VCTS under fbe_raid_siots_t.
 *       Moved the functions related to the fbe_xor_error_t_REGION from xor_proc0.c
 *       to this file.
 *
 * @author
 *   11/21/2008 Created.  PP.
 *
 ***************************************************************************/

/************************
 * INCLUDE FILES
 ************************/
#include "fbe_xor_private.h"
#include "fbe_xor_epu_tracing.h"

fbe_bool_t fbe_xor_search_for_disk_pos_to_replace(fbe_xor_error_type_t error,
                                        fbe_u16_t error_bitmask,
                                        fbe_lba_t lba,
                                        fbe_xor_error_regions_t *error_region_p,
                                        fbe_u16_t width,
                                        fbe_u16_t parity_pos,
                                        fbe_u16_t *log_index);

fbe_bool_t fbe_xor_check_for_overlap_error_entry(fbe_xor_error_type_t error,
                                       fbe_u16_t error_bitmask,
                                       fbe_lba_t lba,
                                       fbe_xor_error_regions_t *error_region_p);

static fbe_status_t fbe_xor_save_raw_mirror_error_region(const fbe_xor_error_t * const eboard_p,
                                                         fbe_xor_error_regions_t * const regions_p,
                                                         const fbe_lba_t lba,
                                                         const fbe_u32_t parity_drives,
                                                         const fbe_u16_t width,
                                                         const fbe_u16_t parity_pos);


/*!*********************************************************************
 * fbe_xor_create_error_region
 **********************************************************************
 * 
 * @brief
 *  For the given inputs, create a new error region within
 *  the input error regions structure.
 *

 * @param error - The error type to add.
 * @param error_bitmask - The error bitmask where these errors occurred.
 * @param lba - The lba where this error occurred.
 *  error_region_p,[I] - The pointer to the error regions structure
 *                       that we should fill out.
 *  width          [I] - The width of the lun.
 *  parity_pos     [I] - The parity position of the lun.
 *
 * ASSUMPTIONS:
 *  All parameters must be set to valid values.
 *  error must be valid (greater than NONE and less than unknown)
 *  error_region_p must not be NULL.
 *  error_bitmask may be zero, and if it is we will just return.
 *      
 * @return fbe_status_t
 *  
 * @author
 *  03/14/06 - Created. Rob Foley
 *  08/25/08 - Modified for new error logging mechanism  PP
 *
 *********************************************************************/
fbe_status_t fbe_xor_create_error_region( fbe_xor_error_type_t error,
                                          fbe_u16_t error_bitmask,
                                          fbe_lba_t lba,
                                          fbe_xor_error_regions_t *error_region_p,
                                          fbe_u16_t width,
                                          fbe_u16_t parity_pos)
{
    fbe_xor_error_region_t *current_entry_p = NULL;
    fbe_lba_t current_lba = lba;
    fbe_u32_t current_blocks = 1;
    fbe_u32_t current_pos_bitmask = error_bitmask;

   /* This flag is used to track whether we have found the empty space in the 
    * disk wise pool of error region array .
    */

   /* This is used to keep track of the position in error regions array. 
    */
   fbe_u16_t log_index;

   fbe_bool_t log_error = FBE_TRUE;

   fbe_u32_t array_size = FBE_MIN(FBE_XOR_MAX_ERROR_REGIONS, error_region_p->next_region_index);

   /* Check to be sure we have something to do.
     */
   if ( error_bitmask == 0 )
   {
       /* Nothing to do so return.
        */
       return FBE_STATUS_OK;
   }

    /* Validate inputs.
     */
    if (XOR_COND(error_region_p == NULL ))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "error_region_p is NULL.\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (XOR_COND(((error & ~FBE_XOR_ERR_TYPE_FLAG_MASK) <= FBE_XOR_ERR_TYPE_NONE) || 
                 ((error & ~FBE_XOR_ERR_TYPE_FLAG_MASK) >= FBE_XOR_ERR_TYPE_UNKNOWN)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "error type = 0x%x, lba = 0x%llx\n",
                              error,
                              (unsigned long long)lba);

        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    /* Initialize our pointer to the first element in the array.
     */
    current_entry_p = &error_region_p->regions_array[0];


    if (error_region_p->next_region_index == 0)
    {
         /* Nothing used yet, just create a new one at the first entry below.
          */
    }
    else
    {
        /* There are valid entries in the error regions structure.
         * Search for overlap by looping over all the entries.
         */
        if( fbe_xor_check_for_overlap_error_entry(error,error_bitmask,lba,error_region_p) == FBE_TRUE)
        {
            /* we have found an overlapped error entry and updated the error entry in the
             * error region array. Now just return from here.
             */
            return FBE_STATUS_OK;
        }        
    } /* end else if(error_region_p->next_region_index == 0) */

    /* Check to see if we have to fill out a brand new entry.
     */
    if (array_size < FBE_XOR_MAX_ERROR_REGIONS)
    {
        log_index = error_region_p->next_region_index;
        error_region_p->next_region_index++;
    }
    else
    {
        /* We don't have room for this entry.
         * search for the disk with maximum error entries.
         */
        log_error = fbe_xor_search_for_disk_pos_to_replace(error,error_bitmask,lba,
                                                       error_region_p,width,parity_pos,&log_index);
        if (log_error == FBE_FALSE)
        {
            return FBE_STATUS_OK;
        }
    }

    current_entry_p = &error_region_p->regions_array[log_index];

    /* We have to fill out a brand new entry.
     * Set the local variables into this structure.
     */
    current_entry_p->lba = current_lba;
    current_entry_p->blocks = current_blocks;
    current_entry_p->pos_bitmask = current_pos_bitmask;
    current_entry_p->error = error;
#if 0
    FLARE_XOR_VB_TRC((TRC_K_STD,"XOR: Next Error region index : 0x%x \n", error_region_p->next_region_index));
    FLARE_XOR_VB_TRC((TRC_K_STD,"XOR: Error region index : 0x%x \n",log_index));
    FLARE_XOR_VB_TRC((TRC_K_STD,"XOR: LBA = 0x%llX, blk = 0x%x, pos_bitmask = 0x%x, error = 0x%x \n",
                     current_entry_p->lba,
                     current_entry_p->blocks,
                     current_entry_p->pos_bitmask,
                     current_entry_p->error));
#endif

    /* Make sure next_region_index is a legal value.
     */
    if (XOR_COND(error_region_p->next_region_index > FBE_XOR_MAX_ERROR_REGIONS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "error_region_p->next_region_index 0x%x > FBE_XOR_MAX_ERROR_REGIONS 0x%x\n",
                              error_region_p->next_region_index,
                              FBE_XOR_MAX_ERROR_REGIONS);

        return FBE_STATUS_GENERIC_FAILURE;
    }
#if 0
    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_INFO,
                          "XOR: Create error region error: 0x%x pos_b: 0x%x lba: 0x%llx bl: 0x%x", 
                          current_entry_p->error, current_entry_p->pos_bitmask, 
                          current_entry_p->lba, current_entry_p->blocks);
#endif
    return FBE_STATUS_OK;
}
/* END fbe_xor_create_error_region */

/*!*********************************************************************
 * fbe_xor_check_for_overlap_error_entry()
 **********************************************************************
 * 
 * @brief
 * There are valid entries in the error regions structure.
 * Search for overlap by looping over all the entries.
 *
 *

 * @param error - The error type to add.
 * @param error_bitmask - The error bitmask where these errors occurred.
 * @param lba - The lba where this error occurred.
 *  error_region_p,[I] - The pointer to the error regions structure
 *                       that we should fill out.
 *  
 * ASSUMPTIONS:
 *  All parameters must be set to valid values.
 *  error must be valid (greater than NONE and less than unknown)
 *  error_region_p must not be NULL.
 *  error_bitmask may be zero, and if it is we will just return.
 *      
 * @return
 *  FBE_TRUE : if the we are overlapping with the exiting error entry.
 *  FBE_FALSE : if we are not overlapping.
 *
 * @author
 *  08/25/08 - Created. PP.
 *  
 *********************************************************************/
fbe_bool_t fbe_xor_check_for_overlap_error_entry(fbe_xor_error_type_t error,
                                       fbe_u16_t error_bitmask,
                                       fbe_lba_t lba,
                                       fbe_xor_error_regions_t *error_region_p)
{
    fbe_xor_error_region_t *current_entry_p = NULL;

    fbe_u16_t entry_index;

    fbe_u32_t array_size = FBE_MIN(FBE_XOR_MAX_ERROR_REGIONS, error_region_p->next_region_index);

    /* Initialize our pointer to the first element in the array.
     */
    current_entry_p = &error_region_p->regions_array[0];
    /* There are valid entries in the error regions structure.
     * Search for overlap by looping over all the entries.
     */
    for (entry_index = 0; entry_index < array_size; entry_index++)
    {
        if ((current_entry_p->lba + current_entry_p->blocks == lba ) &&
            (current_entry_p->error == error ) &&
            (current_entry_p->pos_bitmask == error_bitmask ))
        {
            /* The new entry is just after this entry, 
             * so just increase the size of the current 
             * entry so it includes the new entry.
             */
            current_entry_p->blocks++;

#if 0
            FLARE_XOR_VB_TRC((TRC_K_STD,"XOR: Error region index : 0x%x \n", entry_index));
            FLARE_XOR_VB_TRC((TRC_K_STD,"XOR: LBA = 0x%llX, blk = 0x%x, pos_bitmask = 0x%x, error = 0x%x \n",
                             current_entry_p->lba,
                             current_entry_p->blocks,
                             current_entry_p->pos_bitmask,
                             current_entry_p->error));
#endif

            return FBE_TRUE;
        }
        else if ((current_entry_p->lba != 0) && 
                 (current_entry_p->lba - 1 == lba) &&
                 (current_entry_p->error == error) &&
                 (current_entry_p->pos_bitmask == error_bitmask))
        {
            /* The new entry is just before this entry, so 
             * change the start of the current entry, and 
             * increase the size of the current entry so 
             * it includes the new entry.
             */
            current_entry_p->lba = lba;
            current_entry_p->blocks++;

#if 0
            FLARE_XOR_VB_TRC((TRC_K_STD,"XOR: Error region index : 0x%x \n", entry_index));
            FLARE_XOR_VB_TRC((TRC_K_STD,"XOR: LBA = 0x%llX, blk = 0x%x, pos_bitmask = 0x%x, error = 0x%x \n",
                             current_entry_p->lba,
                             current_entry_p->blocks,
                             current_entry_p->pos_bitmask,
                             current_entry_p->error));
#endif

            return FBE_TRUE;
        }
        else if ((lba >= current_entry_p->lba) &&
                 (lba < current_entry_p->lba + current_entry_p->blocks) &&
                 (current_entry_p->error == error) &&
                 (current_entry_p->pos_bitmask == error_bitmask ))
        {
            /* The new entry is contained in this entry already,
             * so there is nothing to do.
             */
            return FBE_TRUE;
        }

        current_entry_p++;
    } /* end for (entry_index = 0; entry_index < array_size; entry_index++ ) */

    return FBE_FALSE;
}
/* END fbe_xor_check_for_overlap_error_entry() */

/*!*********************************************************************
 * fbe_xor_search_for_disk_pos_to_replace()
 **********************************************************************
 * 
 * @brief
 *  The XOR error regions array is fully filled and we don't have 
 *  room for this entry.
 *  search for the disk with maximum error entries.
 *        
 *

 * @param error - The error type to add.
 * @param error_bitmask - The error bitmask where these errors occurred.
 * @param lba - The lba where this error occurred.
 *  error_region_p,[I] - The pointer to the error regions structure
 *                       that we should fill out.
 *  width..........[I] - The width of the lun.
 *  parity_pos.....[I] - The parity position of the lun.
 *  log_index      [I] - the entry index in the error regions array that will be 
 *                       replaced with new entry.
 *
 * ASSUMPTIONS:
 *  All parameters must be set to valid values.
 *  error must be valid (greater than NONE and less than unknown)
 *  error_region_p must not be NULL.
 *  error_bitmask may be zero, and if it is we will just return.
 *      
 * @return
 *  FBE_TRUE : if we need to log this entry.
 *  FLASE : The entry is already present for this disk position, 
 *          So we are not logging. 
 *
 * @author
 *  08/25/08 - Created. PP
 * 
 *********************************************************************/
fbe_bool_t fbe_xor_search_for_disk_pos_to_replace(fbe_xor_error_type_t error,
                                        fbe_u16_t error_bitmask,
                                        fbe_lba_t lba,
                                        fbe_xor_error_regions_t *error_region_p,
                                        fbe_u16_t width,
                                        fbe_u16_t parity_pos,
                                        fbe_u16_t *log_index)
{
    /* This is used to track whether the error is present for the 
     * data position or not. 
     */
    fbe_bool_t error_present = FBE_FALSE;

    /* This is used to track whether the error is on the parity drive or not.
     */
    fbe_bool_t error_on_parity_drive = FBE_FALSE;

    /* This is used to store the bitmap showing data positions with the error. 
     */
    fbe_u16_t error_on_data_position = 0;

    /* This is used to keep track of the error count for each disk.
     */
    fbe_u32_t error_count[FBE_XOR_MAX_FRUS];

    /* This is used to store the latest error entry position for the disk 
     * in the XOR error region array.
     */
    fbe_u16_t latest_error_pos[FBE_XOR_MAX_FRUS];

    /* This is used to store the disk position having the maximum number 
     * of error entries.
     */
    fbe_u32_t disk_pos_with_max_error = 0;

    /* This is used to get the position bitmask of the disk.
     */
    fbe_u16_t pos;

    fbe_u16_t region_index, disk_count;

    fbe_bool_t error_sim_enabled;
    #ifdef RG_ERROR_SIM_ENABLED
        fbe_bool_t rg_error_sim_enabled(void);
        error_sim_enabled = rg_error_sim_enabled();
    #else
        error_sim_enabled = FBE_FALSE;
    #endif


    for (region_index = 0; region_index < FBE_XOR_MAX_FRUS; region_index++)
    {
        error_count[region_index] = 0;
        latest_error_pos[region_index] = 0;
    }

    for (region_index = 0; region_index < FBE_XOR_MAX_ERROR_REGIONS; region_index++)
    {
        error_on_data_position = 0;
        error_on_parity_drive = FBE_FALSE;

        /* Check whether error is on parity drive or not 
         * depending on correctable or uncorrectable errors.
         */
        for (disk_count = 0 ; disk_count < width ; disk_count++)
        {
            pos = (1 << disk_count );
            if (error_region_p->regions_array[region_index].pos_bitmask & pos)
            {
                if (disk_count == parity_pos)
                {
                    if (error_region_p->regions_array[region_index].error & 
                        FBE_XOR_ERR_TYPE_UNCORRECTABLE )
                    {
                        error_on_parity_drive = FBE_FALSE;
                    }
                    else
                    {
                        error_on_parity_drive = FBE_TRUE;
                    }
                }
                else
                {
                    error_on_data_position |= pos;
                }
            } /* end if( error_region_p->regions_array[region_index].pos_bitmask & pos) */
        } /* end for(disk_count = 0 ; disk_count < width ; disk_count++) */

        /* Check for the entry if present for the disk or not in the
         * error region array.
         */
        if (error_bitmask & 
            error_region_p->regions_array[region_index].pos_bitmask)
        {
            if ((error_on_parity_drive == FBE_FALSE) && 
                (error_bitmask & error_on_data_position))
            {
                error_present = FBE_TRUE;
                break;
            }
            
        }

        /* Count the number of errors entries for each disk.
         */
        for (disk_count = 0 ; disk_count < width ; disk_count++)
        {
            pos = 1 << disk_count;
            if (error_region_p->regions_array[region_index].pos_bitmask & pos)
            {
                if (disk_count == parity_pos)
                {
                    if (error_on_parity_drive == FBE_TRUE)
                    {
                        error_count[disk_count]++; 
                        latest_error_pos[disk_count] = region_index;
                    }
                }
                else
                {
                    error_count[disk_count]++; 
                    latest_error_pos[disk_count] = region_index;
                }
            } /* end if( error_region_p->regions_array[region_index].pos_bitmask & pos) */
        } /* end for(disk_count = 0 ; disk_count < width ; disk_count++) */
    }/* end for(region_index = 0; region_index < FBE_XOR_MAX_ERROR_REGIONS; region_index++) */

    /* Find out which disk position has the most errors, which will be 
     * the candidate for storing new error entry.
     */
    disk_pos_with_max_error = 0;
    for (disk_count = 0; disk_count < width; disk_count++)
    {
        if (error_count[disk_count] >= error_count[disk_pos_with_max_error])
        {
            if (error_count[disk_count] == error_count[disk_pos_with_max_error])
            {
                if (latest_error_pos[disk_count] > 
                    latest_error_pos[disk_pos_with_max_error])
                {
                    disk_pos_with_max_error = disk_count;
                }
            }
            else
            {
                disk_pos_with_max_error = disk_count;
            }
        } /* end if (error_count[disk_count] >= error_count[disk_pos_with_max_error]) */
    } /* end for(disk_count = 0; disk_count < width; disk_count++) */

    if(error_present == FBE_FALSE)
    {
        if (error_sim_enabled == FBE_TRUE)
        {
            /* When rderr is enabled, then we do not want to overwrite records.
            * Once the error region is filled we stop overwriting. 
            * The rderr validation code cna get very confused if is sees records being 
            * overwritten because the logical order of the errors gets disturbed. 
            * This is expecially true when we overwrite a media error and then get 
            * an uncorrectable on a different drive since the reason for the uncorrectable is 
            * lost. 
            */
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                  "All error regions are filled.\n");

            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                  "Disk position 0x%x has the most errors.\n",
                                  disk_pos_with_max_error);

            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                  "We will NOT replace 0x%x error region. pos:0x%x lba:0x%llx bl:0x%x err:0x%x\n",
                                  latest_error_pos[disk_pos_with_max_error],
                                  error_region_p->regions_array[region_index].pos_bitmask,
                                  (unsigned long long)error_region_p->regions_array[region_index].lba,
                                   error_region_p->regions_array[region_index].blocks,
                                   error_region_p->regions_array[region_index].error);
            return FBE_FALSE;
        }/*end if (error_sim_enabled == FBE_TRUE)*/
        else
        {
            *log_index = latest_error_pos[disk_pos_with_max_error];

            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                  "All error regions are filled.\n");
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                  "Disk position 0x%x has the most errors.\n",
                                  disk_pos_with_max_error);
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                  "The latest error entry is at region 0x%x \n",
                                  (*log_index));
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                   "We will replace 0x%x error region. pos:0x%x lba:0x%llx bl:0x%x err:0x%x\n",
                                   (*log_index),
                                   error_region_p->regions_array[region_index].pos_bitmask,
                                   (unsigned long long)error_region_p->regions_array[region_index].lba,
                                   error_region_p->regions_array[region_index].blocks,
                                   error_region_p->regions_array[region_index].error);
            return FBE_TRUE;
        }/* end else-if if (error_sim_enabled == FBE_TRUE)*/
    }/*end if(error_present == FBE_FALSE)*/
    else
    {
        /* we will not log the error as we are having the entry already.
         */
        return FBE_FALSE;
    }
}/* end fbe_xor_search_for_disk_pos_to_replace */

/*!*********************************************************************
 * fbe_xor_save_crc_error_region()
 **********************************************************************
 * 
 * @brief
 *  Map a crc error to an error in the enumeration 
 *  fbe_xor_error_type_t.
 *
 * @param regions_p - The pointer to the error regions structure 
 *                       that we should fill out.
 * @param eboard_p - This eboard has errors to be categorized.
 * @param lba - The lba where this error occurred.
 * @param pos_bitmask - This is the bitmask of positions
 *                       to check for errors.
 * @param e_err_type - The type of error we believe has occurred
 *                       at this particular position bitmask.
 * @param b_correctable - FBE_TRUE if error is correctable, FBE_FALSE otherwise.
 *  width          [I] - width of the raid group.
 *  parity_pos     [I] - parity position of the lun.
 *
 *      
 * @return fbe_status_t
 *
 * @author
 *  07/12/06 - Created. Rob Foley
 *  06/16/08 - Modified  PP
 *
 *********************************************************************/
fbe_status_t fbe_xor_save_crc_error_region( fbe_xor_error_regions_t * const regions_p,
                                const fbe_xor_error_t * const eboard_p, 
                                const fbe_lba_t lba, 
                                const fbe_u16_t pos_bitmask,
                                const fbe_xor_error_type_t input_err_type,
                                const fbe_bool_t b_correctable,
                                const fbe_u32_t parity_drives,
                                fbe_u16_t width,
                                fbe_u16_t parity_pos)
{
    fbe_xor_error_type_t local_error_type;
    fbe_xor_error_type_t e_crc_type;
    fbe_status_t status;
    
    /* This is the set of positions that were found in the
     * associated correctable/uncorrectable bitmask.
     */
    fbe_u16_t current_err_bitmask;
    
    /* Get the bits that match this position bitmask in either
     * the appropriate crc error bitmask.
     * Remember, the c_crc_bitmap is the union of all the correctable 
     * errors across all the different reasons.
     * u_crc_bitmap is the union of all the un correctable crc errs.
     * across all the different reasons.
     */
    if ( b_correctable )
    {
        current_err_bitmask = pos_bitmask & eboard_p->c_crc_bitmap;
    }
    else
    {
        current_err_bitmask = pos_bitmask & eboard_p->u_crc_bitmap;
    }
    
    /* Now, using the above information determine the error type
     * by adding bits as appropriate.
     */
    status = fbe_xor_eboard_get_error(eboard_p, 
                                      current_err_bitmask,
                                      input_err_type,
                                      b_correctable,
                                      parity_drives,
                                      &local_error_type);

    if (status != FBE_STATUS_OK) {  return status; }

    e_crc_type = local_error_type & FBE_XOR_ERR_TYPE_MASK;
    
    if ( e_crc_type != FBE_XOR_ERR_TYPE_INVALIDATED &&
         e_crc_type != FBE_XOR_ERR_TYPE_RAID_CRC &&
         e_crc_type != FBE_XOR_ERR_TYPE_CORRUPT_CRC &&
         ( current_err_bitmask & eboard_p->crc_invalid_bitmap ||
           current_err_bitmask & eboard_p->crc_raid_bitmap    ||
           current_err_bitmask & eboard_p->corrupt_crc_bitmap    ) )
    {
        /* We took a RAID or Corrupt CRC error on this position too.
         * It's useful to know that this position took more than one error.
         */
        local_error_type |= FBE_XOR_ERR_TYPE_RB_INV_DATA;
    }

    /* If this is a generic CRC error, determine if it is single or multi bit.
     * Modify the error bits we will pass on accordingly.
     */
    if( e_crc_type == FBE_XOR_ERR_TYPE_CRC )
    {
        /* pick out any bits added above */
        fbe_xor_error_type_t new_error_type = local_error_type & (~FBE_XOR_ERR_TYPE_MASK);

        /* Set the error type based on the multi-bit error bit mask */
        if( pos_bitmask & eboard_p->crc_multi_bitmap )
        {
            new_error_type |= FBE_XOR_ERR_TYPE_MULTI_BIT_CRC;
        }
        else
        {
            new_error_type |= FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC;
        } 
        local_error_type = new_error_type;
    }

    status  = fbe_xor_create_error_region( local_error_type, current_err_bitmask, lba, regions_p, width, parity_pos);
    return status;
}
/* end fbe_xor_save_crc_error_region */

/*!*********************************************************************
 * fbe_xor_save_error_region
 **********************************************************************
 * 
 * @brief
 *  For the given inputs add the necessary error regions to the
 *  input error regions structure.
 *
 * @param eboard_p - The error board with errors.
 * @param regions_p - The pointer to the error regions structure
 *                       that we should fill out.
 * @param b_invalidate - FBE_TRUE if we are invalidating already or
 *                       FBE_FALSE if we still have retries to do 
 *                       before invalidating.
 * @param lba - The lba where this error occurred.
 * @param parity_drives - The number of parity drives.
 *  width          [I] - width of the lun.
 *  parity_pos.....[I] - parity position for the lun.
 *
 *                      
 * ASSUMPTIONS:
 *  We don't assume the eboard has any errors.
 *  We will check it here for errors and return if no errors found.
 *  All raid types are allowed to use this function.
 *  We don't assume the regions_p is valid.  If it is NULL we return.
 *      
 * @return fbe_status_t
 *  
 * @author
 *  03/14/06 - Created. Rob Foley
 *  06/16/08 - Modified  PP
 *
 *********************************************************************/
fbe_status_t fbe_xor_save_error_region( const fbe_xor_error_t * const eboard_p, 
                            fbe_xor_error_regions_t * const regions_p, 
                            const fbe_bool_t b_invalidate,
                            const fbe_lba_t lba,
                            const fbe_u32_t parity_drives,
                            const fbe_u16_t width,
                            const fbe_u16_t parity_pos)
{
    fbe_u16_t total_errors_bitmask;
    fbe_status_t status;
    fbe_xor_error_type_t error_type;
    
    /* Calculate the sum of all errors.
     */
    total_errors_bitmask = 
        eboard_p->c_crc_bitmap | eboard_p->u_crc_bitmap |
        eboard_p->c_ws_bitmap | eboard_p->u_ws_bitmap |
        eboard_p->c_ts_bitmap | eboard_p->u_ts_bitmap |
        eboard_p->c_ss_bitmap | eboard_p->u_ss_bitmap |
        eboard_p->c_coh_bitmap | eboard_p->u_coh_bitmap |
        eboard_p->c_rm_magic_bitmap | eboard_p->u_rm_magic_bitmap |
        eboard_p->c_rm_seq_bitmap;

    if ( (total_errors_bitmask == 0) || 
         (regions_p == NULL) || 
         ((b_invalidate == FBE_FALSE) && 
          (eboard_p->media_err_bitmap == 0)) )
    {
        /* We don't have errors, or we have errors, but we're not done retrying.
         * When we're not done retrying we don't update the error regions
         * structure since we will come through this path again when the 
         * retries are exhausted.
         * We wouldn't want to update the error counts more than required. 
         * We always record errors for media errors. 
         */
        return FBE_STATUS_OK;
    }
    
    /* Since each bit in the "crc reason bitmasks" is associated with either
     * correctable or uncorrectable bitmasks (not both)
     * Thus, we check correctable and uncorrectable separately.
     */

    /* Create error regions for "Data Lost errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_invalid_bitmap,
                               FBE_XOR_ERR_TYPE_INVALIDATED, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_invalid_bitmap,
                               FBE_XOR_ERR_TYPE_INVALIDATED, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    /* Create error regions for "Raid crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_raid_bitmap,
                               FBE_XOR_ERR_TYPE_RAID_CRC, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_raid_bitmap,
                               FBE_XOR_ERR_TYPE_RAID_CRC, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    /* Create error regions for "DH crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_dh_bitmap,
                               FBE_XOR_ERR_TYPE_DH_CRC, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_dh_bitmap,
                               FBE_XOR_ERR_TYPE_DH_CRC, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    /* Create error regions for "Klondike crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_klondike_bitmap,
                               FBE_XOR_ERR_TYPE_KLOND_CRC, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_klondike_bitmap,
                               FBE_XOR_ERR_TYPE_KLOND_CRC, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    /* Create error regions for "Media error caused crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->media_err_bitmap,
                               FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->media_err_bitmap,
                               FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    /* Create error regions for "IORB_OP_CORRUPT_CRC caused crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->corrupt_crc_bitmap,
                               FBE_XOR_ERR_TYPE_CORRUPT_CRC, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->corrupt_crc_bitmap,
                               FBE_XOR_ERR_TYPE_CORRUPT_CRC, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    /* Create error regions for "IORB_OP_CORRUPT_DATA caused crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->corrupt_data_bitmap,
                               FBE_XOR_ERR_TYPE_CORRUPT_DATA, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->corrupt_data_bitmap,
                               FBE_XOR_ERR_TYPE_CORRUPT_DATA, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }


    /* Create error regions for "LBA stamp errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_lba_stamp_bitmap,
                               FBE_XOR_ERR_TYPE_LBA_STAMP, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_lba_stamp_bitmap,
                               FBE_XOR_ERR_TYPE_LBA_STAMP, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }
    
    /* Create error regions for "single bit crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_single_bitmap,
                               FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_single_bitmap,
                               FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

        /* Create error regions for "multi bit crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_multi_bitmap,
                               FBE_XOR_ERR_TYPE_MULTI_BIT_CRC, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_multi_bitmap,
                               FBE_XOR_ERR_TYPE_MULTI_BIT_CRC, FBE_FALSE, /* UNcorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    /* Create error regions for "COPY crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_copy_bitmap,
                               FBE_XOR_ERR_TYPE_COPY_CRC, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_copy_bitmap,
                               FBE_XOR_ERR_TYPE_COPY_CRC, FBE_FALSE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    /* Create error regions for "PVD Metadata crc errors".
     */
    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_pvd_metadata_bitmap,
                               FBE_XOR_ERR_TYPE_PVD_METADATA, FBE_TRUE, /* Correctable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                               eboard_p->crc_pvd_metadata_bitmap,
                               FBE_XOR_ERR_TYPE_PVD_METADATA, FBE_FALSE, /*UNCorrectable */ 
                               parity_drives,
                               width,
                               parity_pos);
    if (status != FBE_STATUS_OK) { return status; }

    /* At this point the "unknown" bitmap contains crc errors that were
     * detected.  
     * There may also be crc errors logged against uncorrectable
     * rebuild positions (or media errors).
     * Any of these "extra" errors need to be pushed into the error region also.
     */
    {
        fbe_u16_t crc_errors_union = eboard_p->u_crc_bitmap | eboard_p->c_crc_bitmap;
        fbe_u16_t crc_reason_union = 
            (eboard_p->crc_invalid_bitmap  | eboard_p->crc_raid_bitmap      | 
             eboard_p->crc_dh_bitmap       | eboard_p->crc_klondike_bitmap  | 
             eboard_p->media_err_bitmap    | eboard_p->corrupt_crc_bitmap   | 
             eboard_p->corrupt_data_bitmap | eboard_p->crc_single_bitmap    | 
             eboard_p->crc_multi_bitmap    | eboard_p->crc_lba_stamp_bitmap |
			 eboard_p->crc_copy_bitmap);
        
        /* Calculate the unaccounted for crc errors.
         */
        fbe_u16_t leftover_crc_errors = crc_errors_union & ~crc_reason_union;
     
        /* Now log these errors.
         */
        status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                                   leftover_crc_errors & eboard_p->c_crc_bitmap,
                                   FBE_XOR_ERR_TYPE_CRC, FBE_TRUE, /* Correctable */ 
                                   parity_drives,
                                   width,
                                   parity_pos);
        if (status != FBE_STATUS_OK) { return status; }

        /* Log RB_FAILED errors.
         * This used to be called only for RAID6 groups, and the rebuild failed error 
         * for RAID3/5 groups would be generated from the error regions later. 
         * But this failed to deal with write or timestamp errors due to incomplete 
         * writes which did not cause uncorrectable errors elsewhere in the stripe. 
         * Now we generate an error region for the failed rebuild position, as did the 
         * RAID6 case before, and we no longer generate rebuild failed messages after the fact. 
         */
        /* We log this as a rebuild failed to differentiate it from
         * the standard crc errors in the validation code. 
         * It gets turned into VR_NO_ERROR in the ELOG error_info field.
         */
        status = fbe_xor_save_crc_error_region( regions_p, eboard_p, lba,
                                   leftover_crc_errors & eboard_p->u_crc_bitmap,
                                   FBE_XOR_ERR_TYPE_RB_FAILED, FBE_FALSE, /* UNcorrectable */ 
                                   parity_drives,
                                   width,
                                   parity_pos);
        if (status != FBE_STATUS_OK) { return status; }
    }

    /* Coherency errors.
     */
    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->c_coh_bitmap,
                                       FBE_XOR_ERR_TYPE_COH,
                                       FBE_TRUE /* Correctable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->c_coh_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);
    
    if (status != FBE_STATUS_OK) { return status; }

    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->u_coh_bitmap,
                                       FBE_XOR_ERR_TYPE_COH,
                                       FBE_FALSE /* UNcorrectable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->u_coh_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) { return status; }

    /* Timestamp errors.
     */
    {
        fbe_u16_t ts_errs = FBE_XOR_ERR_TYPE_TS;
        fbe_u32_t error;
        
        /* Log time stamp errors caused by zeroing.
         */
        status = fbe_xor_epu_trc_get_flags(FBE_XOR_ERR_TYPE_TS, & error);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        if ( error & (1 <<  9) )
        {
            /* Flag zero happens to be a TS error caused by an initial timestamp.
             * Indicate this in the error regions structure.
             */
            ts_errs |= FBE_XOR_ERR_TYPE_INITIAL_TS;
        }

        status =  fbe_xor_eboard_get_error(eboard_p,
                                           eboard_p->c_ts_bitmap,
                                           ts_errs,
                                           FBE_TRUE /* Correctable */, 
                                           parity_drives,
                                           &error_type);

        if (status != FBE_STATUS_OK) { return status; }

        status = fbe_xor_create_error_region(error_type,
                                    eboard_p->c_ts_bitmap,
                                    lba,
                                    regions_p,
                                    width,
                                    parity_pos);

        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        status =  fbe_xor_eboard_get_error(eboard_p,
                                           eboard_p->u_ts_bitmap,
                                           ts_errs,
                                           FBE_FALSE /* Uncorrectable */, 
                                           parity_drives,
                                           &error_type);
        
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }


        status = fbe_xor_create_error_region(error_type,
                                    eboard_p->u_ts_bitmap,
                                    lba,
                                    regions_p,
                                    width,
                                    parity_pos);

        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }

    /* Writestamp errors.
     */
    
    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->c_ws_bitmap,
                                       FBE_XOR_ERR_TYPE_WS,
                                       FBE_TRUE/* Correctable */, 
                                       parity_drives,
                                       &error_type);
    
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->c_ws_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->u_ws_bitmap,
                                       FBE_XOR_ERR_TYPE_WS,
                                       FBE_FALSE /* Uncorrectable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->u_ws_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* Shedstamp errors. 
     * This function is allowed to be used by all raid types including R5/R3,
     * which check for shed stamp errors.
     */
    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->c_ss_bitmap,
                                       FBE_XOR_ERR_TYPE_SS,
                                       FBE_TRUE/* Correctable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->c_ss_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status;
    }

    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->u_ss_bitmap,
                                       FBE_XOR_ERR_TYPE_SS,
                                       FBE_FALSE /* UNcorrectable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    
    /* N POC Coherency errors, multiple issues with parity of checksums on R6.
     */
    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->c_n_poc_coh_bitmap,
                                       FBE_XOR_ERR_TYPE_N_POC_COH,
                                       FBE_TRUE/* Correctable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->c_n_poc_coh_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    {
        return status; 
    }

    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->u_n_poc_coh_bitmap,
                                       FBE_XOR_ERR_TYPE_N_POC_COH,
                                       FBE_FALSE /* UNcrorectable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->u_n_poc_coh_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* POC Coherency Errors, indicating issues with parity of checksums on R6.
     */
    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->c_poc_coh_bitmap,
                                       FBE_XOR_ERR_TYPE_POC_COH,
                                       FBE_TRUE/* Correctable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->c_poc_coh_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->u_poc_coh_bitmap,
                                       FBE_XOR_ERR_TYPE_POC_COH,
                                       FBE_FALSE /* UNcorrectable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) 
    {
        return status;
    }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->u_poc_coh_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status;
    }

    /* Unknown coherency errors on R6.
     */
    status =  fbe_xor_eboard_get_error(eboard_p,
                                        eboard_p->c_coh_unk_bitmap,
                                        FBE_XOR_ERR_TYPE_COH_UNKNOWN,
                                        FBE_TRUE/* Correctable */, 
                                        parity_drives,
                                        &error_type);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }


    status = fbe_xor_create_error_region(error_type,
                                eboard_p->c_coh_unk_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->u_coh_unk_bitmap,
                                       FBE_XOR_ERR_TYPE_COH_UNKNOWN,
                                       FBE_FALSE /* UNcorrectable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    
    status = fbe_xor_create_error_region(error_type,
                                eboard_p->u_coh_unk_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* Raw mirror errors.
     */
    status = fbe_xor_save_raw_mirror_error_region(eboard_p,
                                                  regions_p,
                                                  lba,
                                                  parity_drives,
                                                  width,
                                                  parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    return FBE_STATUS_OK;
}
/* END fbe_xor_save_error_region */

/*!*********************************************************************
 * fbe_xor_save_raw_mirror_error_region
 **********************************************************************
 * 
 * @brief
 *  Add the necessary error regions to the input error regions structure for raw mirror I/O.
 *
 * @param eboard_p - Pointer to the error board with errors.
 * @param regions_p - Pointer to the error regions structure that we should fill out.
 * @param lba - LBA where this error occurred.
 * @param parity_drives - Number of parity drives.
 * @param width - Width of the raw mirror.
 * @param parity_pos - Parity position for the raw mirror.
 *
 * @return fbe_status_t
 *  
 * @author
 *  11/2011 - Created. Susan Rundbaken
 *
 *********************************************************************/
static fbe_status_t fbe_xor_save_raw_mirror_error_region(const fbe_xor_error_t * const eboard_p,
                                                         fbe_xor_error_regions_t * const regions_p,
                                                         const fbe_lba_t lba,
                                                         const fbe_u32_t parity_drives,
                                                         const fbe_u16_t width,
                                                         const fbe_u16_t parity_pos)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t raw_mir_magic_errs = FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM;
    fbe_u16_t raw_mir_seq_errs = FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM;
    fbe_xor_error_type_t error_type;
    

    /* Raw mirror errors for bad magic number positions.
     */
    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->c_rm_magic_bitmap,
                                       raw_mir_magic_errs,
                                       FBE_TRUE /* Correctable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->c_rm_magic_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->u_rm_magic_bitmap,
                                       raw_mir_magic_errs,
                                       FBE_FALSE /* Uncorrectable */, 
                                       parity_drives,
                                       &error_type);
    
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }


    status = fbe_xor_create_error_region(error_type,
                                eboard_p->u_rm_magic_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* Raw mirror errors for bad sequence number positions.
     */
    status =  fbe_xor_eboard_get_error(eboard_p,
                                       eboard_p->c_rm_seq_bitmap,
                                       raw_mir_seq_errs,
                                       FBE_TRUE /* Correctable */, 
                                       parity_drives,
                                       &error_type);

    if (status != FBE_STATUS_OK) { return status; }

    status = fbe_xor_create_error_region(error_type,
                                eboard_p->c_rm_seq_bitmap,
                                lba,
                                regions_p,
                                width,
                                parity_pos);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    return FBE_STATUS_OK;
}
/* END fbe_xor_save_raw_mirror_error_region */


/*!*********************************************************************
 * fbe_xor_eboard_get_error
 **********************************************************************
 * 
 * @brief
 *  Map an error in the eboard to an error in the enumeration 
 *  fbe_xor_error_type_t.
 *
 * @param eboard_p - This eboard has errors to be categorized.
 * @param pos_bitmask - This is the bitmask of positions
 *                     to check for errors.
 * @param b_correctable - FBE_TRUE if error is correctable, FBE_FALSE otherwise.
 * @param e_err_type - The type of error we believe has occurred
 *                       at this particular position bitmask.
 * @param parity_drives - Number of parity drives
 * @param xor_error_type - [O] used to return xor error type
 *                      
 * ASSUMPTIONS:
 *  We assume the eboard is not NULL.
 *  We assume nothing about the pos_bitmask.  If it is 0, then
 *  we return FBE_XOR_ERR_TYPE_NONE
 *      
 * @return fbe_status_t
 *
 * @author
 *  03/14/06 - Created. Rob Foley
 *  
 *********************************************************************/
fbe_status_t fbe_xor_eboard_get_error(const fbe_xor_error_t * const eboard_p, 
                                      const fbe_u16_t pos_bitmask,
                                      const fbe_xor_error_type_t e_err_type,
                                      const fbe_bool_t b_correctable,
                                      const fbe_u16_t parity_drives,
                                      fbe_xor_error_type_t * const xor_error_type_p)
{
    /* Initialize the return value to the passed in error.
     * We will only add "flags" to this error type.
     */
    fbe_xor_error_type_t error = e_err_type;
    
    /* Make sure pos_bitmask is sane, at least one position should be set.
     * Make sure also that the eboard is sane (not null).
     */
    if (XOR_COND(eboard_p == NULL))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "eboard_p == NULL\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if ( pos_bitmask == 0 )
    {
        /* Nothing to do, just return no errors.
         */
        *xor_error_type_p = FBE_XOR_ERR_TYPE_NONE;
        return FBE_STATUS_OK;
    }
    
    /* Check if the error was uncorrectable,
     * and if so, set the high bit.
     */
    if ( !b_correctable )
    {
        error |= FBE_XOR_ERR_TYPE_UNCORRECTABLE;
    }
    
    /* If the zeroed error is set, OR in this flag also.
     */
    if ( eboard_p->zeroed_bitmap & pos_bitmask )
    {
        error |= FBE_XOR_ERR_TYPE_ZEROED;
    }
    
    if ((parity_drives == 1) &&
        (!b_correctable ) &&
        e_err_type != FBE_XOR_ERR_TYPE_INVALIDATED &&
        e_err_type != FBE_XOR_ERR_TYPE_RAID_CRC &&
        e_err_type != FBE_XOR_ERR_TYPE_CORRUPT_CRC &&
        e_err_type != FBE_XOR_ERR_TYPE_CORRUPT_DATA && 
        ( ~pos_bitmask & eboard_p->crc_invalid_bitmap  ||
          ~pos_bitmask & eboard_p->crc_raid_bitmap     ||
          ~pos_bitmask & eboard_p->corrupt_crc_bitmap  ||
          ~pos_bitmask & eboard_p->corrupt_data_bitmap    )   )
    {
        /* There is another invalidated block in this  strip. Set the
         * flag to let the validation code know that there will may be
         * uncorrectable errors. But we will do so only if current 
         * error is uncorrectable in nature. In case of RAID 6, we 
         * do not expect to be here we do reconstruct parity if it 
         * strip is having sectors invalidated in past because of corrupt
         * CRC operation.
         * 
         * Example:
         * Let us assume that strip is already having corrupt CRC pattern
         * and  checksum error is injected. Since, we have two errors in
         * same strip, errors will become uncorrectable but validation
         * code need to conveyed that its expected. Otherwise, it will be
         * treated as mismatch.
         */
        error |= FBE_XOR_ERR_TYPE_OTHERS_INVALIDATED;
    }
    else if ((parity_drives == 2) &&
             (!b_correctable ) &&
             (e_err_type != FBE_XOR_ERR_TYPE_CORRUPT_DATA) && 
             (~pos_bitmask & eboard_p->corrupt_data_bitmap))
    {
        /* In case of RAID 6, we can have strips which is having corrupt data 
         * pattern at two different position and another error is injected at 
         * third position. As, we do write invalid patternon -ONLY- on data  
         * drives and we do not recosntruct parity. So, strip will have  
         * uncorrectable errors though we injected error at only one position. 
         * In this case, it is possible to have uncorrectable. So, convey this 
         * to validation code to avoid mismatch.
         */
        error |= FBE_XOR_ERR_TYPE_OTHERS_INVALIDATED;
    }
    
    /* We should have found an error if we made it this far.
     */
    if (XOR_COND(error == FBE_XOR_ERR_TYPE_UNKNOWN))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "error type 0x%x is unknown\n",
                              error);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *xor_error_type_p = error;
    return FBE_STATUS_OK;
}
/* END fbe_xor_eboard_get_error */

/*****************************************
 * End of  fbe_xor_error_region.c
 *****************************************/
