/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_interface.c
 ***************************************************************************
 *
 * @brief
 *       This file contains functions used by proc0 to execute XOR commands.
 *   Proc0 will execute XOR xommands based on Flare's xor library
 *   functions.  The commands are executed in-line. The operation and any
 *   necessary error recovery are all completed before the function returns
 *   to the calling driver. This component provides synchronous behavior
 *   and is assumed to provide the best performace in a single processor
 *   architecture.
 *
 * @notes
 *
 * @author
 *   8/1/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/************************
 * INCLUDE FILES
 ************************/
#include "xorlib_api.h"
#include "fbe_xor_private.h"
#include "fbe_xor_epu_tracing.h"
#include "fbe/fbe_time.h"

/*****************************************************
 * static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/
static fbe_status_t fbe_xor_execute_stamp_options(fbe_sector_t * const blkptr,
                                                  const fbe_u16_t position_mask, 
                                                  const fbe_lba_t seed,
                                                  const fbe_xor_option_t option, 
                                                  const fbe_u16_t ts,
                                                  const fbe_u16_t ss, 
                                                  const fbe_u16_t ws,
                                                  fbe_xor_status_t *const status_p, 
                                                  fbe_u16_t width,
                                                  fbe_xor_error_t * const eboard_p,
                                                  fbe_xor_error_regions_t *eregions_p);

static fbe_status_t fbe_xor_fill_invalid_sectors(fbe_sg_element_t * sg_ptr,
                                                 fbe_lba_t seed,
                                                 fbe_block_count_t blocks,
                                                 fbe_u32_t offset,
                                                 xorlib_sector_invalid_reason_t reason,
                                                 xorlib_sector_invalid_who_t who);
static fbe_status_t fbe_xor_execute_invalidate_sector(fbe_xor_invalidate_sector_t *xor_invalidate_p);
static fbe_u16_t fbe_xor_calculate_checksum(const fbe_u32_t * srcptr);
static fbe_status_t fbe_xor_zero_buffer(fbe_xor_zero_buffer_t *zero_buffer_p);
/****************************
 *      GLOBAL VARIABLES
 ****************************/

/*************************************
 *      EXTERNAL GLOBAL VARIABLES
 *************************************/
/**************************************************
 * Start of xor interface functions.
 ***************************************************/
/*! @todo eventually we should move the rest of the functions here to other files.
 *        The only functions that should be here are ones in the xor interface.
 */

/***************************************************************************
 *          fbe_xor_get_invalidation_time()
 *************************************************************************** 
 * 
 * @brief   Simply get the `FBE' system time and convert to the `xorlib'
 *          system time.
 *
 * @param   system_time_p - Address of system time to populate.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 ***************************************************************************/
static fbe_status_t fbe_xor_get_invalidation_time(xorlib_system_time_t *const system_time_p)
{
    fbe_system_time_t  systemtime;

    /* Get system time and copy to structure expected by xorlib.
     */
    fbe_get_system_time((fbe_system_time_t *)&systemtime);
    system_time_p->year = systemtime.year;
    system_time_p->month = systemtime.month;
    system_time_p->weekday = systemtime.weekday;
    system_time_p->day = systemtime.day;
    system_time_p->hour = systemtime.hour;
    system_time_p->minute = systemtime.minute;
    system_time_p->second = systemtime.second;
    system_time_p->milliseconds = systemtime.milliseconds;

    return FBE_STATUS_OK;   
}
/****************************************
 * end of fbe_xor_get_invalidation_time()
 ****************************************/

/***************************************************************************
 * fbe_xor_lib_fill_invalid_sector()
 ***************************************************************************
 * @brief
 *  Fills a buffer with invalidated sector information.
 *  It first zeroes out the sector (backend size).
 *  Then, It puts in a caller ID, time, and other assorted goodies to
 *  allow us to determine when a sector was invalidated and why when we
 *  diagnose lab/field issues.
 *
 * @param pSector - pointer to a buffer in which we will put all this data
 * @param seed - essentially the physical address where we will put the
 *               invalidated sector. Used to calculate the checksum
 * @param reason - Reason for invalidating.
 * @param who  information passed from caller
 *
 * @return NONE
 *
 * @notes
 *
 * @author
 *  12/18/2000 - Moved from fl_csum_util.c.  Mike Gordon
 *  01/21/2011 - Ported to xor library. Rob Foley
 ***************************************************************************/
fbe_status_t fbe_xor_lib_fill_invalid_sector(void *data_p,
                                             fbe_lba_t seed,
                                             xorlib_sector_invalid_reason_t reason,
                                             xorlib_sector_invalid_who_t who)
{
    fbe_status_t            status;
    fbe_s32_t               xor_status;
    xorlib_system_time_t    xorlib_systemtime;

    /* Get system time and copy to structure expected by xorlib.
     */
    fbe_xor_get_invalidation_time(&xorlib_systemtime);
    xor_status = xorlib_fill_invalid_sector(data_p, seed, reason, who,
                                            &xorlib_systemtime);
    status = (xor_status == 0) ? FBE_STATUS_OK : FBE_STATUS_GENERIC_FAILURE;

    return status;   
}
/**************************************
 * end fbe_xor_lib_fill_invalid_sector()
 **************************************/

/***************************************************************************
 * fbe_xor_lib_fill_update_invalid_sector()
 ***************************************************************************
 * @brief
 *  Fills a buffer with invalidated sector information if it is first created.
 *  It first zeroes out the sector (backend size).  Then,
 *  It puts in a caller ID, time, and other assorted goodies
 *  to allow us to determine when a sector was invalidated
 *  and why when we diagnose lab/field issues.
 *  If the invalid sector is existed will only update the hit_invalid_count 
 *  and current timestamp.
 *
 * @param pSector - pointer to a buffer in which we will put all this data
 * @param seed - essentially the physical address where we will put the
 *               invalidated sector. Used to calculate the checksum
 * @param reason - Reason for invalidating.
 * @param who - information passed from caller
 *
 * @return NONE
 *
 * @notes
 *
 * @author
 *  1/21/2011 - Created. Rob Foley
 * 
 ***************************************************************************/
fbe_status_t fbe_xor_lib_fill_update_invalid_sector(void *data_p,
                                                    fbe_lba_t seed,
                                                    xorlib_sector_invalid_reason_t reason,
                                                    xorlib_sector_invalid_who_t who)
{
    fbe_status_t            status;
    fbe_s32_t               xor_status;
    xorlib_system_time_t    xorlib_systemtime;

    /* Get system time
     */
    fbe_xor_get_invalidation_time(&xorlib_systemtime);
    xor_status = xorlib_fill_update_invalid_sector(data_p, seed, reason, who,
                                                   &xorlib_systemtime);
    status = (xor_status == 0) ? FBE_STATUS_OK : FBE_STATUS_GENERIC_FAILURE;

    return status;   
}
/**************************************
 * end fbe_xor_lib_fill_update_invalid_sector()
 **************************************/

/****************************************************************
 * fbe_xor_lib_fill_invalid_sectors()
 ****************************************************************
 *
 * @brief
 *  This function is called to fill in the invalid sectors.
 *
 * @param sg_ptr - Pointer to the scatter-gather list.
 * @param seed   - Seed for checksum.
 * @param blocks - Blocks to invalidate.
 * @param offset - Offset into the SG list. 
 * @param reason - reason passed from caller.
 * @param who    - who passed from caller
 *
 * @return fbe_status_t
 * 
 * @notes
 *  we will invalidate blocks of sectors starting from the beginning
 *  of the sg list.
 *
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_fill_invalid_sectors(fbe_sg_element_t * sg_ptr,
                                              fbe_lba_t seed,
                                              fbe_block_count_t blocks,
                                              fbe_u32_t offset,
                                              xorlib_sector_invalid_reason_t reason,
                                              xorlib_sector_invalid_who_t who)
{
    fbe_status_t status;

    status = fbe_xor_fill_invalid_sectors(sg_ptr, seed, blocks, offset, reason, who);
    return status;
}
/**************************************
 * end fbe_xor_lib_fill_invalid_sectors()
 **************************************/

/****************************************************************
 * fbe_xor_lib_execute_invalidate_sector       
 ****************************************************************
 *
 * @brief
 *   This function invalidates sectors within the passed in
 *   sg lists according to the details in the invalidate
 *   sector structure.
 *
 * @param xor_invalidate_p - Structure with all info we need.
 *
 * @return fbe_status_t
 *
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_lib_execute_invalidate_sector(fbe_xor_invalidate_sector_t *xor_invalidate_p)
{
    fbe_status_t status;

    status = fbe_xor_execute_invalidate_sector(xor_invalidate_p);
    return status;
}
/**************************************
 * end fbe_xor_lib_execute_invalidate_sector()
 **************************************/

/*!**************************************************************
 * fbe_xor_lib_calculate_checksum()
 ****************************************************************
 * @brief
 *  Interface function to calculate the checksum
 *  See fbe_xor_calculate_checksum() for further details on the interface.
 *
 * @param sector_p - sector to calc checksum.
 *
 * @return fbe_u16_t checksum
 *
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_u16_t fbe_xor_lib_calculate_checksum(const fbe_u32_t * sector_p)
{
    fbe_u16_t chksum;

    chksum = fbe_xor_calculate_checksum(sector_p);
    return chksum;
}
/******************************************
 * end fbe_xor_lib_calculate_checksum()
 ******************************************/

/*!**************************************************************
 * fbe_xor_lib_calculate_and_set_checksum()
 ****************************************************************
 * @brief
 *  Interface function to calculate the checksum
 *  See fbe_xor_calculate_checksum() for further details on the interface.
 *
 * @param sector_p - sector to calc checksum.
 *
 * @return fbe_u16_t checksum
 *
 * @author
 *  6/12/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_xor_lib_calculate_and_set_checksum(fbe_u32_t * sector_p)
{
    fbe_u16_t chksum;
    fbe_sector_t *blkptr = (fbe_sector_t*)sector_p;

    chksum = fbe_xor_calculate_checksum(sector_p);
    blkptr->crc = chksum;
}
/******************************************
 * end fbe_xor_lib_calculate_and_set_checksum()
 ******************************************/

/*!**************************************************************
 *          fbe_xor_lib_validate_data()
 ****************************************************************
 * @brief
 *  Interface function that validates that if the sector has been
 *  invalidated the checksum better be invalid.  If the sector has
 *  been invalidated 
 *
 * @param   sector_p - sector to validate
 * @param   seed - lba that contains this sector data
 * @param   option - Xor options
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Otherwise the failure status 
 *  
 * @author
 *  08/03/2011  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_validate_data(const fbe_sector_t *const sector_p,
                                       const fbe_lba_t seed,
                                       const fbe_xor_option_t option)
{
    fbe_status_t    status;

    status = fbe_xor_validate_data(sector_p, seed, option);
    return status;
}
/******************************************
 * end fbe_xor_lib_validate_data()
 ******************************************/

/*!**************************************************************
 * fbe_xor_lib_is_lba_stamp_valid()
 ****************************************************************
 * @brief
 *  Interface to check the lba stamp of a sector.
 *
 * @param sector_p  - Sector to check.
 * @param lba - the lba of this block.
 * @param offset - The raid group offset to add to lba
 *
 * @return FBE_TRUE - Success FBE_FALSE if stamp does not match.
 *
 * @author
 *  5/11/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t fbe_xor_lib_is_lba_stamp_valid(fbe_sector_t * const sector_p, fbe_lba_t lba, fbe_lba_t offset)
{
    return(xorlib_is_valid_lba_stamp(&sector_p->lba_stamp, lba, offset));
}
/******************************************
 * end of fbe_xor_lib_is_lba_stamp_valid()
 ******************************************/

/*!**************************************************************
 *          fbe_xor_lib_is_sector_invalidated()
 ****************************************************************
 * @brief
 *  Interface function that checks if a sector is invalidated.  If
 *  the sector was invalidated it populates the `reason' with the
 *  invalidate reason.
 *
 * @param   sector_p - sector to validate
 * @param   reason_p - If the sector was invalidated, this is the
 *                     invalidated reason.
 *
 * @return  bool - FBE_TRUE - Sector was invalidated and the reason
 *                            is updated
 *                 FBE_FALSE - Sector was not invalidated 
 *  
 * @author
 *  08/17/2011  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_bool_t fbe_xor_lib_is_sector_invalidated(const fbe_sector_t *const sector_p,
                                             xorlib_sector_invalid_reason_t *const reason_p)
{
    fbe_bool_t  b_sector_invalidated = FBE_FALSE;

    b_sector_invalidated = fbe_xor_is_sector_invalidated(sector_p, reason_p);
    return b_sector_invalidated;
}
/*********************************************
 * end of fbe_xor_lib_is_sector_invalidated()
 *********************************************/

/****************************************************************
 *                     fbe_xor_mem_move       
 ****************************************************************
 *
 * @brief
 *  This function is called to carry out memory move operations. 
 *
 * @param mem_move_p - Structure describing move.
 * @param move_cmd - specific type of memory move
 * @param option - Only option available is to set lba stamps.
 * @param src_dst_pairs  - number of sources/destinations
 * @param ts - programmed ts
 * @param ss - programmed ss
 * @param ws - programmed ws
 *
 * @return FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 *
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_lib_mem_move(fbe_xor_mem_move_t *mem_move_p,
                                  fbe_xor_move_command_t move_cmd,
                                  fbe_xor_option_t option,
                                  fbe_u16_t src_dst_pairs, 
                                  fbe_u16_t ts,
                                  fbe_u16_t ss,
                                  fbe_u16_t ws)
{
    fbe_status_t status;

    status = fbe_xor_mem_move(mem_move_p, move_cmd, option, src_dst_pairs,
                              ts, ss, ws);
    return status;
}
/******************************************
 * end fbe_xor_lib_mem_move()
 ******************************************/

/****************************************************************
 * fbe_xor_lib_zero_buffer          
 ****************************************************************
 *
 * @brief
 *  This function is called to zero out blocks.
 *  The passed in structure describes the operation in detail.
 *
 * @param zero_buffer_p - describes the zero operation.
 *
 * @return FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 *
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_zero_buffer(fbe_xor_zero_buffer_t *zero_buffer_p)
{
    fbe_status_t status;

    status = fbe_xor_zero_buffer(zero_buffer_p);
    return status;
}
/******************************************
 * end fbe_xor_lib_zero_buffer()
 ******************************************/

/*************************************************************** 
 * fbe_xor_lib_raid0_verify 
 ****************************************************************
 *
 * @brief
 *   This function executes the raid0 verify operation.
 *
 * @param xor_verify_p - Structure with all info we need.
 *
 * @return FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 * 
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_raid0_verify(fbe_xor_striper_verify_t *xor_verify_p)
{
    fbe_status_t status;

    status = fbe_xor_raid0_verify(xor_verify_p);
    return status;
}
/**************************************
 * end fbe_xor_lib_raid0_verify() 
 **************************************/
/*!**************************************************************
 * fbe_xor_lib_validate_raid0_stamps()
 ****************************************************************
 * @brief
 *  Determine if the stamps are ok for a raid 0.
 *
 * @param sector_p - the block we will be validating.
 * @param lba - the disk lba of this block. 
 * @param offset - The raid group offset to use to check lba stamp              
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 *
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_lib_validate_raid0_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset)
{
    fbe_status_t status;

    status = fbe_xor_validate_raid0_stamps(sector_p, lba, offset);
    return status;
}
/**************************************
 * end fbe_xor_lib_validate_raid0_stamps() 
 **************************************/

/*!**************************************************************
 * fbe_xor_lib_validate_mirror_stamps()
 ****************************************************************
 * @brief
 *  Determine if the stamps are ok for all mirror raid group types.
 *
 * @param sector_p - the block we will be validating.
 * @param lba - the disk lba of this block. 
 * @param offset - The raid group offset to use to check lba stamp
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 *
 * @author
 *  11/29/2011  Ron Proulx  - Created.
 *
 ****************************************************************/

fbe_status_t fbe_xor_lib_validate_mirror_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset)
{
    fbe_status_t status;

    status = fbe_xor_validate_mirror_stamps(sector_p, lba, offset);
    return status;
}
/******************************************
 * end fbe_xor_lib_validate_mirror_stamps() 
 ******************************************/

/*!**************************************************************
 * fbe_xor_lib_validate_raw_mirror_stamps()
 ****************************************************************
 * @brief
 *  Determine if the stamps are ok for raw mirror raid group types.
 *
 * @param sector_p - the block we will be validating.
 * @param lba - the disk lba of this block. 
 * @param offset - The raid group offset to use to check lba stamp
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 *
 * @author
 *  6/21/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_lib_validate_raw_mirror_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset)
{
    fbe_status_t status;
    fbe_raw_mirror_sector_data_t *reference_p = NULL;
    reference_p = (fbe_raw_mirror_sector_data_t*)sector_p;

    status = fbe_xor_validate_mirror_stamps(sector_p, lba, offset);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    if (reference_p->magic_number != FBE_MAGIC_NUMBER_RAW_MIRROR_IO)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_xor_lib_validate_raw_mirror_stamps() 
 ******************************************/

/*!***************************************************************
 * fbe_xor_lib_parity_rebuild       
 ****************************************************************
 * @brief
 *   This function executes a parity rebuild.
 *
 * @param rebuild_p - Ptr to struct with all info we need to rebuild.
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.
 * 
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_parity_rebuild(fbe_xor_parity_reconstruct_t *rebuild_p)
{
    fbe_status_t status;

    status = fbe_xor_parity_rebuild(rebuild_p);
    return status;
}
/**************************************
 * end fbe_xor_lib_parity_rebuild() 
 **************************************/
/*!***************************************************************
 * fbe_xor_lib_parity_reconstruct       
 ****************************************************************
 * @brief
 *   This function executes a parity reconstruct.
 *
 * @param rc_p - Ptr to struct with all info we need to reconstruct.
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.
 * 
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_parity_reconstruct(fbe_xor_parity_reconstruct_t *rc_p)
{
    fbe_status_t status;

    status = fbe_xor_parity_reconstruct(rc_p);
    return status;
}
/**************************************
 * end fbe_xor_lib_parity_reconstruct() 
 **************************************/
/*!***************************************************************
 * fbe_xor_lib_parity_verify       
 ****************************************************************
 * @brief
 *   This function executes a parity verify.
 *
 * @param verify_p - Ptr to struct with all info we need to verify.
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.
 * 
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_parity_verify(fbe_xor_parity_reconstruct_t *verify_p)
{
    fbe_status_t status;

    status = fbe_xor_parity_verify(verify_p);
    return status;
}
/**************************************
 * end fbe_xor_lib_parity_verify() 
 **************************************/
/*!***************************************************************
 * fbe_xor_lib_parity_verify_copy_user_data       
 ****************************************************************
 * @brief
 *   This function executes a user data copy to copy user data
 *   for a write operation into a stripe we just verified.
 *
 * @param mem_move_p - Ptr to struct describing the operation.
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.
 * 
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_parity_verify_copy_user_data(fbe_xor_mem_move_t *mem_move_p)
{
    fbe_status_t status;

    status = fbe_xor_parity_verify_copy_user_data(mem_move_p);
    return status;
}
/**************************************
 * end fbe_xor_lib_parity_verify_copy_user_data() 
 **************************************/
/*!***************************************************************
 * fbe_xor_lib_parity_recovery_verify_const_parity       
 ****************************************************************
 * @brief
 *   This function executes a reconstruct of parity following
 *   a verify and copying new write data into the stripe..
 *
 * @param xor_recovery_verify_p - Ptr to struct describing the operation.
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.
 * 
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_parity_recovery_verify_const_parity(fbe_xor_recovery_verify_t *xor_recovery_verify_p)
{
    fbe_status_t status;

    status = fbe_xor_parity_recovery_verify_const_parity(xor_recovery_verify_p);
    return status;
}
/**************************************
 * end fbe_xor_lib_parity_recovery_verify_const_parity() 
 **************************************/
/*!***************************************************************
 * fbe_xor_lib_mirror_verify       
 ****************************************************************
 * @brief
 *   This function executes a mirror verify.
 *
 * @param verify_p - Ptr to struct with all info we need to verify.
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.
 * 
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_mirror_verify(fbe_xor_mirror_reconstruct_t *verify_p)
{
    fbe_status_t status;

    status = fbe_xor_mirror_verify(verify_p);
    return status;
}
/**************************************
 * end fbe_xor_lib_mirror_verify() 
 **************************************/
/*!***************************************************************
 * fbe_xor_lib_mirror_rebuild       
 ****************************************************************
 * @brief
 *   This function executes a mirror rebuild.
 *
 * @param rebuild_p - Ptr to struct with all info we need to rebuild.
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.
 * 
 * @author
 *  1/21/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_mirror_rebuild(fbe_xor_mirror_reconstruct_t *rebuild_p)
{
    fbe_status_t status;

    status = fbe_xor_mirror_rebuild(rebuild_p);
    return status;
}
/**************************************
 * end fbe_xor_lib_mirror_rebuild() 
 **************************************/
/**************************************************
 * End of xor interface functions.
 ***************************************************/
/****************************************************************
 * fbe_xor_eboard_init()
 ****************************************************************
 * @brief
 *  Check if eboard is initialized and if not, then init it.
 *
 * @param eboard_p - pointer to eboard.       
 *
 * @return
 *  None.             
 *
 * @author
 *  4/1/2008 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_xor_lib_eboard_init(fbe_xor_error_t *const eboard_p)
{
    if ( (eboard_p != NULL) && (fbe_xor_is_eboard_setup(eboard_p) == FBE_FALSE) )
    {
        /* The caller assumes that the eboard is initialized and then
         * filled out by this operation. 
         */
        fbe_xor_setup_eboard(eboard_p, 
                             0 /* No Crc Errors.*/ );
    }
    return;
}
/******************************************
 * end fbe_xor_eboard_init()
 ******************************************/

/****************************************************************
 * fbe_xor_lib_eboard_get_raid_group_id()
 ****************************************************************
 * @brief
 *  Check if eboard is initialized and if not.  If it is return
 *  the raid group object identifier.
 *
 * @param eboard_p - pointer to eboard.       
 *
 * @return  object id           
 *
 ****************************************************************/
fbe_object_id_t fbe_xor_lib_eboard_get_raid_group_id(fbe_xor_error_t *const eboard_p)
{
    fbe_object_id_t raid_group_object_id = FBE_MAX_SEP_OBJECTS;

    /* Validate eboard before retrieving object id.
     */
    if (fbe_xor_is_valid_eboard(eboard_p) == FBE_TRUE)
    {
        /* Return the valid raid group object id.
         */
        raid_group_object_id = fbe_xor_error_get_raid_group_id(eboard_p);
    }

    /* Return the riad group object id.
     */
    return raid_group_object_id;
}
/******************************************************
 * end of fbe_xor_lib_eboard_get_raid_group_id()
 ******************************************************/

/****************************************************************
 * fbe_xor_fill_invalid_sectors()
 ****************************************************************
 *
 * @brief
 *    This function is called to fill in the invalid sectors.
 *
 * @param sg_ptr - Pointer to the scatter-gather list.
 * @param seed   - Seed for checksum.
 * @param blocks - Blocks to invalidate.
 * @param offset - Offset into the SG list. 
 * @param reason - reason passed from caller.
 * @param who - who passed from caller.
 *
 * @return fbe_status_t
 * 
 * @notes
 *      we will invalidate blocks of sectors starting from the beginning
 *      of the sg list.
 *
 * @author
 *      08/18/00 JJIN    Created
 *
 ****************************************************************/
static fbe_status_t fbe_xor_fill_invalid_sectors(fbe_sg_element_t * sg_ptr,
                                          fbe_lba_t seed,
                                          fbe_block_count_t blocks,
                                          fbe_u32_t offset,
                                          xorlib_sector_invalid_reason_t reason,
                                          xorlib_sector_invalid_who_t who)
{
    fbe_status_t status;
    fbe_s32_t   xor_status;
    fbe_sg_element_t *cur_sg_ptr = sg_ptr;
    fbe_block_count_t j;
    fbe_u8_t *byte_ptr;
    fbe_s32_t sg_block_count;
    fbe_block_count_t temp_blocks;
    xorlib_system_time_t xorlib_systemtime;

    /* Get system time and copy to structure expected by xorlib.
     */
    fbe_xor_get_invalidation_time(&xorlib_systemtime);

    /* Fill all blocks
     */
    while (blocks > 0)
    {
        if (XOR_COND(cur_sg_ptr->count == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "cur_sg_ptr->count == 0\n");  
            return FBE_STATUS_GENERIC_FAILURE;
        }

        sg_block_count = cur_sg_ptr->count / FBE_BE_BYTES_PER_BLOCK;
        temp_blocks = FBE_MIN(blocks, sg_block_count - offset);

        for (j = 0,
             byte_ptr = (fbe_u8_t *) cur_sg_ptr->address + offset * FBE_BE_BYTES_PER_BLOCK;
             j < temp_blocks;
             j++, byte_ptr += FBE_BE_BYTES_PER_BLOCK, seed++)
        {
            xor_status = xorlib_fill_invalid_sector((void *) byte_ptr,
                                                    seed, /* LBA. */
                                                    reason, /* Reason for invalidating. */
                                                    who /* Who requested the invalidation*/,
                                                    &xorlib_systemtime);
            status = (xor_status == 0) ? FBE_STATUS_OK : FBE_STATUS_GENERIC_FAILURE;
            if (status != FBE_STATUS_OK)
            { 
                return status;
            }
        }

        blocks -= temp_blocks;

        cur_sg_ptr++, offset = 0;
    }

    return FBE_STATUS_OK;
}
/* fbe_xor_fill_invalid_sectors() */

/****************************************************************
 * fbe_xor_execute_invalidate_sector       
 ****************************************************************
 *
 * @brief
 *   This function is called by xor_execute_processor_command
 *   to carry out the sector invalidation.  
 *
 * @param xor_invalidate_p - Structure with all info we need.
 *
 * @return VALUES/ERRORS:
 *   fbe_status_t
 *
 * @author
 *  17-Jan-02: Rob Foley Created.
 *
 ****************************************************************/

static fbe_status_t fbe_xor_execute_invalidate_sector(fbe_xor_invalidate_sector_t *xor_invalidate_p)
{
    fbe_u32_t fru_cnt;
    fbe_u32_t sg_count;
    fbe_status_t status;
    xorlib_system_time_t    xorlib_systemtime;

    FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_INV_BUFFER]);

    xor_invalidate_p->status = FBE_XOR_STATUS_NO_ERROR;

    /* There are only (2) supported invalidate reasons:
     *  o XORLIB_SECTOR_INVALID_REASON_DATA_LOST
     *  o XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA
     */
    switch(xor_invalidate_p->invalidated_reason)
    {
        case XORLIB_SECTOR_INVALID_REASON_DATA_LOST:
        case XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA:
            break;

        default:
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "invalidate for lba: 0x%llx blocks: 0x%llx unsupported reason: %d \n",
                                  (unsigned long long)xor_invalidate_p->fru[0].seed,
				  (unsigned long long)xor_invalidate_p->fru[0].count,
                                  xor_invalidate_p->invalidated_reason);
            xor_invalidate_p->status = FBE_XOR_STATUS_UNEXPECTED_ERROR;  
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get system time
     */
    fbe_xor_get_invalidation_time(&xorlib_systemtime);

    /* Walk thru the frus and mark each of the sectors invalid
     */
    for (fru_cnt = 0; fru_cnt < xor_invalidate_p->data_disks; fru_cnt++)
    {
        fbe_xor_sgl_descriptor_t sgd;

        /* Initialize the sgd including an offset.
         */
        FBE_XOR_SGLD_INIT_WITH_OFFSET(sgd, 
                                      xor_invalidate_p->fru[fru_cnt].sg_p,
                                      xor_invalidate_p->fru[fru_cnt].offset,
                                      FBE_BE_BYTES_PER_BLOCK);


        /* Verify that the Calling driver has work for the XOR driver, otherwise
         * this may be an unnecessary call into the XOR driver and will effect
         * system performance.
         */
        if (XOR_COND(sgd.bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "xor: %s (sgd.bytcnt == 0)\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Invalidate the blocks required in this sgd.
         */
        {
            fbe_block_count_t blocks = xor_invalidate_p->fru[fru_cnt].count;
            fbe_lba_t seed = xor_invalidate_p->fru[fru_cnt].seed;

            while (blocks-- > 0)
            {
                if (XOR_COND(sgd.bytcnt == 0))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "sgd.bytcnt == 0\n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /*! @note Currently we don't invalidate the metadata (i.e. it is
                 *        assumed that the time, write and lba stamps are valid).
                 */
                xorlib_fill_update_invalid_sector((fbe_u32_t *)FBE_XOR_SGLD_ADDR(sgd),
                                                  seed,                                 /* Seed of this sector. */
                                                  xor_invalidate_p->invalidated_reason, /* Reason */
                                                  xor_invalidate_p->invalidated_by,     /* Who */
                                                  &xorlib_systemtime);

                status = FBE_XOR_SGLD_INC_BE(sgd, sg_count);
                if (status != FBE_STATUS_OK)
                {
                    return status; 
                }
                seed++;
            }
        }

        /* Unmap any remaining mapped memory.
         */
        status = FBE_XOR_SGLD_UNMAP(sgd);
    }

    return FBE_STATUS_OK;
}  /* end fbe_xor_execute_invalidate_sector() */

/****************************************************************
 *  fbe_xor_lib_execute_stamps       
 ****************************************************************
 *
 * @brief
 *   This function carries out check and set stamp operations
 *   according to the options flags.
 *
 * @param execute_stamps_p - Ptr to info for this operation.
 *
 * @return fbe_status_t
 *
 *  @notes
 * @param option - 6 modes:
 *                         check checksums only.
 *                          (FBE_XOR_OPTION_CHK_CRC)
 *                         check checksums and lba stamps.
 *                          (FBE_XOR_OPTION_CHK_CRC | FBE_XOR_OPTION_CHK_LBA_STAMPS)
 *                         check lba stamps only.
 *                          (FBE_XOR_OPTION_CHK_LBA_STAMPS)
 *                         set stamps.
 *                          (FBE_XOR_OPTION_GEN_CRC)
 *                         set stamps and lba stamps.
 *                          (FBE_XOR_OPTION_GEN_CRC | FBE_XOR_OPTION_GEN_LBA_STAMPS)
 *                         set lba stamps only.
 *                          (FBE_XOR_OPTION_GEN_LBA_STAMPS)
 *
 * @author
 *   03/18/08:  Rob Foley -- Created.
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_execute_stamps(fbe_xor_execute_stamps_t * const execute_stamps_p)
{
    fbe_u32_t fru_cnt;
    fbe_xor_option_t option = execute_stamps_p->option;
    fbe_xor_error_t *eboard_p = execute_stamps_p->eboard_p;
    fbe_u32_t prefetch_fru = 0;    /* Current fru we are prefetching on. */
    fbe_status_t status;
    fbe_u32_t sg_count;

    /* Total blocks prefetched so far.
     */
    fbe_u32_t prefetch_blocks = FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS;
    fbe_u32_t prefetch_fru_blocks = 0;    /* Blocks remaining to prefetch on this fru. */

    /* Sgd to determine where to start our prefetch.
     */
    fbe_xor_sgl_descriptor_t prefetch_sgd;
    fbe_block_count_t total_blocks = 0;
    fbe_lba_t           raid_group_offset;

    if (eboard_p != NULL)
    {
        raid_group_offset = eboard_p->raid_group_offset;
    }
    else
    {
        raid_group_offset = 0;
    }

    /* eboard must be initialized
     */
    if (XOR_COND((eboard_p != NULL) && fbe_xor_is_valid_eboard(eboard_p) == FBE_FALSE))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "eboard_p state: 0x%x is not valid",
                              fbe_xor_error_get_state(eboard_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We assume the command has been initialized before entry.
     */
    if (XOR_DBG_COND(execute_stamps_p->status != FBE_XOR_STATUS_INVALID))
    {   
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "execute_stamps_p->status: 0x%x is not FBE_XOR_STATUS_INVALID",
                              execute_stamps_p->status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine total blocks.
     */
    for ( fru_cnt = 0; fru_cnt < execute_stamps_p->data_disks; fru_cnt++ )
    {
        total_blocks += execute_stamps_p->fru[fru_cnt].count;
    }

    /* We should only touch the status if we get an error.
     */
        execute_stamps_p->status = FBE_XOR_STATUS_NO_ERROR;

    /* Execute the prefetch if we are only checking the lba stamp.
     */
    if ( (option == FBE_XOR_OPTION_CHK_LBA_STAMPS) &&
         (total_blocks > FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS) )
    {
        fbe_u32_t offset_blocks = prefetch_blocks;

        /* If we need to find the location in a fru other than the first,
         * then increment our prefetch fru and decrement the blocks remaining to
         * skip over this fru. 
         */
        while (offset_blocks >= execute_stamps_p->fru[prefetch_fru].count)
        {

            /* just make sure that we are not crossing the 32bit limit here before we cast it
             */
            if (execute_stamps_p->fru[prefetch_fru].count > FBE_U32_MAX)
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "execute_stamps_p->fru[prefetch_fru].count (0x%llx)> FBE_U32_MAX\n",
                                      (unsigned long long)execute_stamps_p->fru[prefetch_fru].count );
                return FBE_STATUS_GENERIC_FAILURE;
            }

            offset_blocks -= (fbe_u32_t)execute_stamps_p->fru[prefetch_fru].count;
            prefetch_fru++;
        }

        /* just make sure that we are not crossing the 32bit limit here before we cast it
         */
        if (execute_stamps_p->fru[prefetch_fru].count > FBE_U32_MAX)
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "execute_stamps_p->fru[prefetch_fru].count (0x%llx)> FBE_U32_MAX\n",
                                  (unsigned long long)execute_stamps_p->fru[prefetch_fru].count );
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Calculate how many blocks are left on this fru.
         */
        prefetch_fru_blocks = (fbe_u32_t)(execute_stamps_p->fru[prefetch_fru].count - 
                              offset_blocks);

        /* Now that we know which SG we should be located in, init our prefetch 
         * sg with the correct offset. 
         */
        FBE_XOR_SGLD_INIT_WITH_OFFSET(prefetch_sgd, 
                                      execute_stamps_p->fru[prefetch_fru].sg,
                                      execute_stamps_p->fru[prefetch_fru].offset +
                                      offset_blocks,
                                      FBE_BE_BYTES_PER_BLOCK);
    }

    for ( fru_cnt = 0; 
        fru_cnt < execute_stamps_p->data_disks; 
        fru_cnt++ )
    {
        fbe_u32_t block;
        fbe_xor_sgl_descriptor_t sgd;
        fbe_lba_t seed = execute_stamps_p->fru[fru_cnt].seed;

        FBE_XOR_SGLD_INIT_WITH_OFFSET(sgd, 
                                      execute_stamps_p->fru[fru_cnt].sg,
                                      execute_stamps_p->fru[fru_cnt].offset,
                                      FBE_BE_BYTES_PER_BLOCK);

        /* Verify that the Calling driver has work for the XOR driver, otherwise
         * this may be an unnecessary call into the XOR driver and will effect
         * system performance.
         */
        if (XOR_COND(execute_stamps_p->fru[fru_cnt].count == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_WARNING,
                                  "execute_stamps_p->fru[%d].count == 0\n",
                                  fru_cnt);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        for (block = 0; block < execute_stamps_p->fru[fru_cnt].count; block++)
        {
            fbe_sector_t *blkptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd);

            /* If we are only checking the lba stamps, then handle prefetch.
             * If we have not reached the end of prefetching, then our prefetch
             * blocks is less than our total blocks. 
             */
            if ( (option == FBE_XOR_OPTION_CHK_LBA_STAMPS) &&
                 (prefetch_blocks < total_blocks) )
            {
                /* Perform a prefetch of the next block we are likely to
                 * reference. 
                 */
                FBE_XOR_PREFETCH_LBA_STAMP_PAUSE( (fbe_sector_t*) FBE_XOR_SGLD_ADDR(prefetch_sgd) );

                prefetch_blocks++;
                if (XOR_DBG_COND(prefetch_fru_blocks == 0))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "prefetch_fru_blocks == 0\n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }


                prefetch_fru_blocks--;

                /* Only increment to the next if we have blocks remaining to
                 * prefetch. 
                 */
                if (prefetch_blocks < total_blocks)
                {
                    if (prefetch_fru_blocks == 0)
                    {
                        /* We have reached the end of the blocks for this fru. 
                         * Continue with the next fru. 
                         */
                        prefetch_fru++;

                        /* just make sure that we are not crossing the 32bit limit here before we cast it
                         */
                        if (execute_stamps_p->fru[prefetch_fru].count > FBE_U32_MAX)
                        {
                            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                                  "execute_stamps_p->fru[prefetch_fru].count (0x%llx)> FBE_U32_MAX\n",
                                                  (unsigned long long)execute_stamps_p->fru[prefetch_fru].count );
                            return FBE_STATUS_GENERIC_FAILURE;
                        }

                        prefetch_fru_blocks = (fbe_u32_t)execute_stamps_p->fru[prefetch_fru].count;

                        /* Assume there are more frus to process.
                         */
                        if (XOR_DBG_COND(prefetch_fru >= execute_stamps_p->data_disks))
                        {
                            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                                  "prefetch_fru 0x%x >= execute_stamps_p->data_disk 0x%x\n",
                                                  prefetch_fru,
                                                  execute_stamps_p->data_disks);

                            return FBE_STATUS_GENERIC_FAILURE;
                        }

                        FBE_XOR_SGLD_INIT_BE(prefetch_sgd, 
                                             execute_stamps_p->fru[prefetch_fru].sg);
                    }
                    else
                    {
                        /* Increment to the next. 
                         */
                        status = FBE_XOR_SGLD_INC_BE_N(prefetch_sgd, 1, sg_count);
                        if (status != FBE_STATUS_OK)
                        {
                            return status; 
                        }
                    }
                }    /* End if more blocks to prefetch. (after increment) */
            }    /* End if more blocks to prefetch. */

            /* If we are only checking lba stamps, do the quick check here. 
             * Otherwise if we get an lba stamp erorr go through the execute stamp options function. 
             */
            if ((option != FBE_XOR_OPTION_CHK_LBA_STAMPS) ||
                ((blkptr->lba_stamp != 0) &&
                 (!xorlib_is_valid_lba_stamp(&blkptr->lba_stamp, seed, raid_group_offset))))
            {
                /* Now check or set the stamps according to the options.
                 */
                status = fbe_xor_execute_stamp_options(blkptr,
                                                       execute_stamps_p->fru[fru_cnt].position_mask, 
                                                       seed,
                                                       option, 
                                                       execute_stamps_p->ts,
                                                       execute_stamps_p->ss,
                                                       execute_stamps_p->ws,
                                                       &execute_stamps_p->status, 
                                                       execute_stamps_p->array_width,
                                                       execute_stamps_p->eboard_p,
                                                       execute_stamps_p->eregions_p);

                /* If the request failed flag an unexpected status
                 */
                if (status != FBE_STATUS_OK)
                {
                    fbe_xor_set_status(&execute_stamps_p->status, FBE_XOR_STATUS_UNEXPECTED_ERROR);
                    return status; 
                }
            }

            if ((block + 1) < execute_stamps_p->fru[fru_cnt].count)
            {
                /* Goto the next block */
                status = FBE_XOR_SGLD_INC_BE(sgd, sg_count);
                if (status != FBE_STATUS_OK)
                {
                    return status; 
                }
            }

            /* Increment seed for the next block */
            seed++;
        }    /* for each block */
    }    /* For each fru. */

    if (execute_stamps_p->status == FBE_XOR_STATUS_NO_ERROR)
    {
        return FBE_STATUS_OK;
    }
    else
    {
        if (eboard_p != NULL)
        {
            /* Since we took an error, we must reset the eboard so that when
             * it is used again it is setup.
             */
            fbe_xor_reset_eboard(eboard_p);
        }

        if (!(option & (FBE_XOR_OPTION_CHK_CRC | FBE_XOR_OPTION_CHK_LBA_STAMPS)))
        {
            /* Not told to check for checksum errors, 
             * clear any checksum error status.
             */
            execute_stamps_p->status &= ~FBE_XOR_STATUS_CHECKSUM_ERROR;
        }
        /* We took some error, clear out the no-error bit. 
         */
        execute_stamps_p->status &= ~FBE_XOR_STATUS_NO_ERROR;
    }

    return FBE_STATUS_OK;
}
/**********************************
 * end fbe_xor_lib_execute_stamps()
 **********************************/

/****************************************************************
 *  fbe_xor_lib_check_lba_stamp       
 ****************************************************************
 * @brief
 *   This function just checks the lba stamp.
 *   Only one of the fru vectors is used from fbe_xor_execute_stamps_t
 * 
 *   These are the fields we expect to be passed in for execute_stamps_p:
 * 
 *   execute_stamps_p->fru[0].sg
 *   execute_stamps_p->fru[0].seed
 *   execute_stamps_p->fru[0].count
 * 
 *   Output is execute_stamps_p->status
 *         FBE_XOR_STATUS_NO_ERROR       on success
 *         FBE_XOR_STATUS_CHECKSUM_ERROR on error
 *
 * @param execute_stamps_p - Ptr to info for this operation.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/23/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_check_lba_stamp(fbe_xor_execute_stamps_t * const execute_stamps_p)
{
    fbe_status_t status;

    /* Total blocks prefetched so far.
     */
    fbe_u32_t prefetch_blocks = FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS;
    fbe_u32_t prefetch_fru_blocks = 0;    /* Blocks remaining to prefetch on this fru. */
    fbe_u32_t block;
    fbe_sg_element_t *sg_p = execute_stamps_p->fru[0].sg;
    fbe_sector_t *sector_p = (fbe_sector_t *) sg_p->address;
    fbe_sg_element_t *prefetch_sg_p = execute_stamps_p->fru[0].sg;
    fbe_sector_t *prefetch_sector_p = NULL;
    fbe_lba_t seed = execute_stamps_p->fru[0].seed;

    /* We should only touch the status if we get an error.
     */
    execute_stamps_p->status = FBE_XOR_STATUS_NO_ERROR;

    /* Execute the prefetch if we are only checking the lba stamp.
     */
    if (execute_stamps_p->fru[0].count > FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS)
    {
        prefetch_fru_blocks = (fbe_u32_t)(execute_stamps_p->fru[0].count - FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS);
        status = fbe_xor_init_sg_and_sector_with_offset(&prefetch_sg_p, &prefetch_sector_p, prefetch_blocks);
    }

    /* Verify that the Calling driver has work for the XOR driver, otherwise
     * this may be an unnecessary call into the XOR driver and will effect
     * system performance.
     */
    if (XOR_DBG_COND(execute_stamps_p->fru[0].count == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_WARNING, "execute_stamps_p->fru[0].count 0x%x == 0\n", (unsigned int)execute_stamps_p->fru[0].count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (block = 0; block < execute_stamps_p->fru[0].count; block++)
    {
        /* If we are only checking the lba stamps, then handle prefetch.
         * If we have not reached the end of prefetching, then our prefetch
         * blocks is less than our total blocks. 
         */
        if (prefetch_blocks < execute_stamps_p->fru[0].count)
        {
            /* Perform a prefetch of the next block we are likely to
             * reference. 
             */
            FBE_XOR_PREFETCH_LBA_STAMP_PAUSE( prefetch_sector_p );

            prefetch_blocks++;
            if (XOR_DEBUG_ENABLED)
            {
                if (prefetch_fru_blocks == 0)
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR, "prefetch_fru_blocks == 0\n");
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                prefetch_fru_blocks--;
            }

            if (prefetch_blocks < execute_stamps_p->fru[0].count)
            {
                /* Increment to the next. 
                 */
                fbe_xor_inc_sector_for_sg(&prefetch_sg_p, &prefetch_sector_p);
            } /* End if more blocks to prefetch. (after increment) */
        } /* End if more blocks to prefetch. */

        /* If we are only checking lba stamps, do the quick check here. 
         * Otherwise if we get an lba stamp erorr go through the execute stamp options function. 
         */
        if ((sector_p->lba_stamp != 0) &&
            (sector_p->lba_stamp != fbe_xor_get_lba_stamp(seed)))
        {
            execute_stamps_p->status = FBE_XOR_STATUS_CHECKSUM_ERROR;
        }

        /* Goto the next block */
        fbe_xor_inc_sector_for_sg(&sg_p, &sector_p);

        /* Increment seed for the next block */
        seed++;
    }    /* for each block */

    if (execute_stamps_p->status == FBE_XOR_STATUS_NO_ERROR)
    {
        return FBE_STATUS_OK;
    }
    else
    {
        /* We took some error, clear out the no-error bit. 
         */
        execute_stamps_p->status &= ~FBE_XOR_STATUS_NO_ERROR;
    }

    return FBE_STATUS_OK;
}
/**********************************
 * end fbe_xor_lib_check_lba_stamp()
 **********************************/
/****************************************************************
 * fbe_xor_execute_stamp_options()
 ****************************************************************
 * @brief
 *  Execute the input stamp options for the given block.
 *
 * @param blkptr - Block to execute stamps for. 
 * @param position_mask - The mask for this position.
 * @param seed - The lba based seed.
 * @param option - The option we are using to execute stamps.
 * @param ts - Time stamp.
 * @param ss - Shed stamp.    
 * @param ws - Write stamp.
 * @param status_p - The pointer to status.
 * @param width - Width of lun.
 * @param eboard_p - The error board pointer.
 * @param eregions_p - Error regions for reporting corrupt crc and corrupt data errors.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/31/2008 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_xor_execute_stamp_options(fbe_sector_t * const blkptr,
                                                  const fbe_u16_t position_mask, 
                                                  const fbe_lba_t seed,
                                                  const fbe_xor_option_t option, 
                                                  const fbe_u16_t ts,
                                                  const fbe_u16_t ss, 
                                                  const fbe_u16_t ws,
                                                  fbe_xor_status_t *const status_p, 
                                                  fbe_u16_t width,
                                                  fbe_xor_error_t * const eboard_p,
                                                  fbe_xor_error_regions_t *eregions_p)
{
    fbe_u16_t           cooked_csum = 0;
    fbe_status_t        fbe_status;
    xorlib_sector_invalid_reason_t InvalidSectorType = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;
    fbe_xor_status_t    xor_status = FBE_XOR_STATUS_NO_ERROR;   /* Default status is `no error' */
    fbe_bool_t          b_check_invalidated_lba = FBE_TRUE;     /* Default is to check seed */
    fbe_lba_t           raid_group_offset;

    if (eboard_p != NULL)
    {
        raid_group_offset = eboard_p->raid_group_offset;
    }
    else
    {
        raid_group_offset = 0;
    }
    /* Determine if we need to validate the lba written to invalidated blocks
     * or not.
     */
    if (option & FBE_XOR_OPTION_LOGICAL_REQUEST)
    {
        /* This is a logical request, we cannot confirm the lba written in any
         * invalidated blocks to the seed passed.
        */
        b_check_invalidated_lba = FBE_FALSE;
    }

    /* If we are checking checksums, then calculate the checksum and compare
     * it to the checksum found in the block.  
     */
    if ((option & FBE_XOR_OPTION_CHK_CRC)                                                        &&
        (blkptr->crc != (cooked_csum = xorlib_cook_csum(xorlib_calc_csum(blkptr->data_word),0)))    )
    {
        /* Check for invalids (use different method for reads and writes)
         */
        if (option & FBE_XOR_OPTION_WRITE_REQUEST)
        {
            xor_status = fbe_xor_handle_bad_crc_on_write(blkptr, 
                                                         seed, 
                                                         position_mask,
                                                         option, 
                                                         eboard_p, 
                                                         eregions_p,
                                                         width);
        }
        else
        {
            /* Else handle checksum errors for the read request.
             */
            xor_status = fbe_xor_handle_bad_crc_on_read(blkptr, 
                                                        seed, 
                                                        position_mask,
                                                        option, 
                                                        eboard_p, 
                                                        eregions_p,
                                                        width);
        }

        /* If this was a `client invalidated' status is good (i.e. no error)
         */
        if (xor_status == FBE_XOR_STATUS_NO_ERROR)
        {
            /* Just continue.
             */
        }
        else if (xor_status & FBE_XOR_STATUS_BAD_MEMORY)
        {
            /* We have already reported the error.  Just continue.
             */
        }
        else
        {
            /* Checksum error found.
             */
            if ( eboard_p != NULL )
            {
                /* Init eboard if it is not initted.
                 * We delay the init of the eboard for performance.  
                 */
                fbe_xor_lib_eboard_init(eboard_p);

                /* Determine the types of errors.
                 * We need to do this in cases where the
                 * user has specified an eboard.
                 */
                fbe_status = fbe_xor_determine_csum_error( eboard_p,    /* Eboard struct to save results. */
                                                           blkptr,    /* Current block. */
                                                           /* Current bitmask of this position */
                                                           position_mask,
                                                           seed,
                                                           blkptr->crc,
                                                           cooked_csum,
                                                           b_check_invalidated_lba);
                if (fbe_status != FBE_STATUS_OK)
                {
                    return fbe_status; 
                }

                /* We also need to specify an uncorrectable
                 * checksum error in this case in order to classify
                 * the type of error.
                 */
                eboard_p->u_crc_bitmap |= position_mask;
            }
        }

        /* Invalidate the block if told to do so.
         */
        if (option & FBE_XOR_OPTION_INVALIDATE_BAD_BLOCKS)
        {
            /* Only if the sector isn't already invalidated do we need to
             * invalidate it.
             */
            if (fbe_xor_is_sector_invalidated(blkptr, &InvalidSectorType) == FBE_FALSE)
            {
                /* If the sector wasn't already invalidated, invalidate it now.
                 */
                fbe_xor_lib_fill_invalid_sector(blkptr,
                                                seed,
                                                XORLIB_SECTOR_INVALID_REASON_DATA_LOST,
                                                XORLIB_SECTOR_INVALID_WHO_RAID);
            }
        }

    }    /* End if check checksum. */
    /* If we are checking lba stamps, then call the validate function.
     */
    else if ((option & FBE_XOR_OPTION_CHK_LBA_STAMPS) &&
             (blkptr->lba_stamp != 0) &&
             (!xorlib_is_valid_lba_stamp(&blkptr->lba_stamp, seed, raid_group_offset)))
    {
        /* We were told to check the lba stamps and
         * the lba stamp is not correct, log this exactly the same as
         * a checksum error.
         */
        xor_status = FBE_XOR_STATUS_CHECKSUM_ERROR;

        if ( eboard_p != NULL )
        {
            fbe_u16_t cooked_csum = 0;

            /* Init eboard if it is not initted.
             * We delay the init of the eboard for performance.  
             */
            fbe_xor_lib_eboard_init(eboard_p);

            /* We already know the 'type' of checksum error, 
             * just set the type and log the error.
             */
            if (!(option & FBE_XOR_OPTION_CHK_CRC) && 
                (blkptr->crc != (cooked_csum = xorlib_cook_csum(xorlib_calc_csum(blkptr->data_word),
                                                                 0))))
            {
                /* We were told not to check the checksum, but we found
                 * a lba stamp error. But the checksum is actually wrong too. We
                 * should report this as a checksum error, since this indicates 
                 * that the entire sector could be wrong, the lba stamp error is 
                 * just a side affect of that. 
                 */
                fbe_status = fbe_xor_determine_csum_error(eboard_p,    /* Eboard struct to save results. */
                                                          blkptr,      /* Current block. */
                                                          /* Current bitmask of this position */
                                                          position_mask,
                                                          seed,
                                                          blkptr->crc,
                                                          cooked_csum,
                                                          b_check_invalidated_lba);
                if (fbe_status != FBE_STATUS_OK)
                {
                    return fbe_status; 
                }
            } else
            {
                /* Log the Error 
                 */
                fbe_status = fbe_xor_record_invalid_lba_stamp(seed, 
                                                              position_mask, 
                                                              eboard_p,
                                                              blkptr);
                if (fbe_status != FBE_STATUS_OK)
                {
                    return fbe_status; 
                }

            }

            /* We also need to specify an uncorrectable
             * checksum error in this case in order to classify
             * the type of error.
             */
            eboard_p->u_crc_bitmap |= position_mask;
        }

        /* Invalidate the block if told to do so.
         */
        if (option & FBE_XOR_OPTION_INVALIDATE_BAD_BLOCKS)
        {
            /* Only if the sector isn't already invalidated do we need to
             * invalidate it.
             */
            if (fbe_xor_is_sector_invalidated(blkptr, &InvalidSectorType) == FBE_FALSE)
            {
                /* If the sector wasn't already invalidated, invalidate it now.
                 */
                fbe_xor_lib_fill_invalid_sector(blkptr,
                                                seed,
                                                XORLIB_SECTOR_INVALID_REASON_DATA_LOST,
                                                XORLIB_SECTOR_INVALID_WHO_RAID);
            }
        }

    }    /* End if check lba stamp. */
    else
    {
        /* Set stamps if this was asked for. 
         * Note that if we are generating lba or crc, then 
         * these will overwrite the values we set now. 
         */
        if (option & FBE_XOR_OPTION_SET_STAMPS)
        {
            /* Simply program the stamps.
             */
            blkptr->time_stamp = ts;
            blkptr->write_stamp = ws;

            /* Set the shed stamp to the provided value.
             */
            blkptr->lba_stamp = ss;
        }

        /* Must be a 'generate' stamps request.
         */
        if (option & FBE_XOR_OPTION_GEN_CRC)
        {
            blkptr->crc = xorlib_cook_csum(xorlib_calc_csum(blkptr->data_word),
                                            0);
            blkptr->time_stamp = ts;
            blkptr->write_stamp = ws;

            /* Set the shed stamp to the provided value.
             */
            blkptr->lba_stamp = ss;
        }

        /* Generate the lba stamp.
         */
        if ((option & FBE_XOR_OPTION_GEN_LBA_STAMPS))
        {
            blkptr->lba_stamp = xorlib_generate_lba_stamp(seed, raid_group_offset);
        }

    }    /* End if generate stamps.  */

    /* Update the status of the xor request
     */
    fbe_xor_set_status(status_p, xor_status);

    /* If the XOR debug option is specified log any errors
     * found in this iteration. 
     */
    if ((option & FBE_XOR_OPTION_DEBUG)              &&
        (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)    )
    {
        /* Trace information about the CRC error.
         */
        fbe_status = fbe_xor_trace_checksum_errored_sector(blkptr,
                                                           position_mask,
                                                           seed,
                                                           option,
                                                           *status_p);

        if (fbe_status != FBE_STATUS_OK)
        {
            return fbe_status; 
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_xor_execute_stamp_options()
 ******************************************/

/****************************************************************
 *                     fbe_xor_mem_move       
 ****************************************************************
 *
 * @brief
 *   This function is called to carry out memory move operations. 
 *
 * @param mem_move_p - Structure describing move.
 * @param move_cmd - specific type of memory move
 * @param option - Only option available is to set lba stamps.
 * @param src_dst_pairs  - number of sources/destinations
 * @param ts - programmed ts
 * @param ss - programmed ss
 * @param ws - programmed ws
 *
 * @return FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 *
 * @author
 *      11/06/00:  MPG -- Copied from xor_proc1.c.
 *
 ****************************************************************/

fbe_status_t fbe_xor_mem_move(fbe_xor_mem_move_t *mem_move_p,
                              fbe_xor_move_command_t move_cmd,
                              fbe_xor_option_t option,
                              fbe_u16_t src_dst_pairs, 
                              fbe_u16_t ts,
                              fbe_u16_t ss,
                              fbe_u16_t ws)
{
    fbe_bool_t continue_operation = FBE_TRUE;
    fbe_u32_t array_pos;
    fbe_u32_t block;
    fbe_u32_t src_buf_size = 0;
    fbe_u32_t dst_buf_size = 0;
    fbe_status_t status;
    fbe_u32_t sg_bytes = 0;

    mem_move_p->status = FBE_XOR_STATUS_NO_ERROR;

    switch (move_cmd)
    {
        case FBE_XOR_MOVE_COMMAND_MEM_TO_MEM512:
            FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_MEM_MEM512]);
            src_buf_size = FBE_BYTES_PER_BLOCK;
            dst_buf_size = FBE_BYTES_PER_BLOCK;
            break;
        case FBE_XOR_MOVE_COMMAND_MEM512_TO_MEM520:
            FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_MEM512_MEM520]);
            src_buf_size = FBE_BYTES_PER_BLOCK;
            dst_buf_size = FBE_BE_BYTES_PER_BLOCK;
            break;
        case FBE_XOR_MOVE_COMMAND_MEM_TO_MEM520:
            FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_MEM_MEM520]);
            src_buf_size = FBE_BE_BYTES_PER_BLOCK;
            dst_buf_size = FBE_BE_BYTES_PER_BLOCK;
            break;
        case FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM512:
            FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_MEM520_MEM512]);
            src_buf_size = FBE_BE_BYTES_PER_BLOCK;
            dst_buf_size = FBE_BYTES_PER_BLOCK;
            break;
        case FBE_XOR_MOVE_COMMAND_MEM512_8_TO_MEM520:
            FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_MEM512_8_MEM520]);
            src_buf_size = FBE_BE_BYTES_PER_BLOCK;
            dst_buf_size = FBE_BE_BYTES_PER_BLOCK;
            break;
        case FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM520:
            FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_MEM520_MEM520]);
            src_buf_size = FBE_BE_BYTES_PER_BLOCK;
            dst_buf_size = FBE_BE_BYTES_PER_BLOCK;
            break;

        default:
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "unexpected move command %d\n", move_cmd);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    array_pos = 0;
    while ((array_pos < src_dst_pairs) && continue_operation)
    {
        fbe_xor_sgl_descriptor_t src_sgd, dst_sgd;
        fbe_lba_t seed = 0;

        FBE_XOR_SGLD_INIT_WITH_OFFSET( src_sgd, 
                                       mem_move_p->fru[array_pos].source_sg_p,
                                       mem_move_p->fru[array_pos].source_offset,
                                       src_buf_size );

        FBE_XOR_SGLD_INIT_WITH_OFFSET( dst_sgd, 
                                       mem_move_p->fru[array_pos].dest_sg_p,
                                       mem_move_p->fru[array_pos].dest_offset,
                                       dst_buf_size );
        /* Initialize seed if we were asked to generate lba stamps.
         */
        if (option & FBE_XOR_OPTION_GEN_LBA_STAMPS)
        {
            seed = mem_move_p->fru[array_pos].seed;
        }

        block = 0;
        while ((block <  mem_move_p->fru[array_pos].count) && continue_operation)
        {
            fbe_sector_t       *srcblk=NULL;
            fbe_sector_t       *dstblk=NULL;
            fbe_u32_t           dcsum = 0;
            fbe_xor_status_t    xor_status = FBE_XOR_STATUS_NO_ERROR;

            srcblk = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(src_sgd);
            dstblk = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(dst_sgd);

            if (srcblk != dstblk)    /* This is an optimization to avoid unnecessary moves. */
            {
                /*! @note Data that has been lost (invalidated) should never
                 *        have a valid checksum.
                 */
                fbe_xor_validate_data_if_enabled(srcblk, seed, option);

                switch (move_cmd)
                {
                    case FBE_XOR_MOVE_COMMAND_MEM_TO_MEM520:
                        dcsum = xorlib_calc_csum_and_cpy(srcblk->data_word, dstblk->data_word);
                        dcsum = xorlib_cook_csum(dcsum, 0);

                        /* Must allow known `invalidated' patterns.
                         */
                        if (srcblk->crc != dcsum)
                        {
                            /* A checksum error was detected on write data.
                             * Check if it a supported invalidate pattern or
                             * not.  If it isn't the xor status will be set to
                             * checksum error.
                             */
                            if ((xor_status = fbe_xor_handle_bad_crc_on_write(srcblk, seed, (1 << array_pos), option,
                                                                              mem_move_p->eboard_p, NULL, FBE_XOR_MAX_FRUS))
                                                                                                            != FBE_XOR_STATUS_NO_ERROR)
                            {
                                fbe_xor_set_status(&mem_move_p->status, xor_status);
                                continue_operation = FBE_FALSE;
                            }
                            else
                            {
                                dstblk->crc = srcblk->crc;
                            }
                        }
                        else
                        {
                            dstblk->crc = dcsum;
                        }

                        /* Append saved stamps */
                        dstblk->time_stamp = srcblk->time_stamp;
                        dstblk->write_stamp = srcblk->write_stamp;
                        dstblk->lba_stamp = srcblk->lba_stamp;

                        /* It doesn't make sense to have the set lba stamps flag set here.
                         */
                        if (XOR_COND(option & FBE_XOR_OPTION_GEN_LBA_STAMPS))
                        {
                            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                                  "unexpected option %d\n",
                                                  option);
                            return FBE_STATUS_GENERIC_FAILURE;
                        }
                        break;
                    case FBE_XOR_MOVE_COMMAND_MEM512_8_TO_MEM520:
                        dcsum = xorlib_calc_csum_and_cpy(srcblk->data_word, dstblk->data_word);
                        dcsum = xorlib_cook_csum(dcsum, 0);

                        /* Must allow known `invalidated' patterns.
                         */
                        if (srcblk->crc != dcsum)
                        {
                            /* A checksum error was detected on write data.
                             * Check if it a supported invalidate pattern or
                             * not.  If it isn't the xor status will be set to
                             * checksum error.
                             */
                            if ((xor_status = fbe_xor_handle_bad_crc_on_write(srcblk, seed, (1 << array_pos), option,
                                                                              mem_move_p->eboard_p, NULL, FBE_XOR_MAX_FRUS))
                                                                                                            != FBE_XOR_STATUS_NO_ERROR)
                            {
                                fbe_xor_set_status(&mem_move_p->status, xor_status);
                                continue_operation = FBE_FALSE;
                            }
                            else
                            {
                                dstblk->crc = srcblk->crc;
                            }
                        }
                        else
                        {
                            dstblk->crc = dcsum;
                        }

                        /* Append Programmed stamps */
                        dstblk->time_stamp = ts;
                        dstblk->write_stamp = ws;
                        dstblk->lba_stamp = ss;
                        break;
                    case FBE_XOR_MOVE_COMMAND_MEM_TO_MEM512:
                        xorlib_copy_data(srcblk->data_word, dstblk->data_word);
                        /* No stamps appended */ 

                        /* It doesn't make sense to have the set lba stamps flag set here.
                         */
                        if (XOR_COND(option & FBE_XOR_OPTION_GEN_LBA_STAMPS))
                        {
                            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                                  "unexpected option %d\n",
                                                  option);
                            return FBE_STATUS_GENERIC_FAILURE;
                        }
                        break;
                    case FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM512:
                        dcsum = xorlib_calc_csum_and_cpy(srcblk->data_word, dstblk->data_word);

                        if (srcblk->crc != xorlib_cook_csum(dcsum, 0))
                        {
                            /* An error has been detected. */
                            mem_move_p->status |= FBE_XOR_STATUS_CHECKSUM_ERROR;
                            continue_operation = FBE_FALSE;
                        }
                        /* No Stamps Appended */

                        /* It doesn't make sense to have the set lba stamps flag set here.
                         */
                        if (XOR_COND(option & FBE_XOR_OPTION_GEN_LBA_STAMPS))
                        {
                            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                                  "unexpected option %d\n",
                                                  option);
                            return FBE_STATUS_GENERIC_FAILURE;
                        }
                        break;
                    case FBE_XOR_MOVE_COMMAND_MEM512_TO_MEM520:
                        dcsum = xorlib_calc_csum_and_cpy(srcblk->data_word, dstblk->data_word);
                        dcsum = xorlib_cook_csum(dcsum, 0);

                        dstblk->crc = dcsum;
                        /* Append programmed stamps */
                        dstblk->time_stamp = ts;
                        dstblk->write_stamp = ws;
                        dstblk->lba_stamp = ss;
                        break;

                        /* A new case FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM520 has been added to handle the 
                         * situation of the new data having the checksum error.
                         */
                    case FBE_XOR_MOVE_COMMAND_MEM520_TO_MEM520:
                        dcsum = xorlib_calc_csum_and_cpy(srcblk->data_word, dstblk->data_word);
                        dcsum = xorlib_cook_csum(dcsum, 0);

                        /* Comparision for host data checksum with calculated new checksum.
                         */
                        if (srcblk->crc != dcsum)
                        {
                            /* A checksum error was detected on write data.
                             * Check if it a supported invalidate pattern or
                             * not.  If it isn't the xor status will be set to
                             * checksum error.
                             */
                            if ((xor_status = fbe_xor_handle_bad_crc_on_write(srcblk, seed, (1 << array_pos), option,
                                                                              mem_move_p->eboard_p, NULL, FBE_XOR_MAX_FRUS))
                                                                                                            != FBE_XOR_STATUS_NO_ERROR)
                            {
                                fbe_xor_set_status(&mem_move_p->status, xor_status);
                                continue_operation = FBE_FALSE;
                            }
                        }

                        /* Since the hostdata having the checksum error so we will not assign 
                         * the new calculated checksum.
                         */
                        dstblk->crc = srcblk->crc;

                        /* Since the hostdata having the checksum error the time stamp,write 
                         * stamp,shed stamp for the srcblk will be assigned to dstblk.
                         */
                        dstblk->time_stamp = srcblk->time_stamp;
                        dstblk->write_stamp = srcblk->write_stamp;
                        dstblk->lba_stamp = srcblk->lba_stamp;
                        break;
                    default:
                        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                              "unexpected move command %d\n", move_cmd);

                        return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Set the lba stamps if the flag is set.
                 */
                if (option & FBE_XOR_OPTION_GEN_LBA_STAMPS)
                {
                    dstblk->lba_stamp = xorlib_generate_lba_stamp(seed, mem_move_p->offset);
                }

                /*! @note Data that has been lost (invalidated) should never
                 *        have a valid checksum
                 */
                fbe_xor_validate_data_if_enabled(dstblk, seed, option);

            }
            /* Increment to point to the next sector.
             */
            status = FBE_XOR_SGLD_INC_N(src_sgd, 1, src_buf_size, sg_bytes);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
            status = FBE_XOR_SGLD_INC_N(dst_sgd, 1, dst_buf_size, sg_bytes);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            block++;
            seed++;
        }

        /* We may have additional sectors mapped in
         */
        status = FBE_XOR_SGLD_UNMAP(src_sgd);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
        status = FBE_XOR_SGLD_UNMAP(dst_sgd);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        array_pos++;
    }

    return FBE_STATUS_OK;
}  
/**************************************
 * end fbe_xor_mem_move()
 **************************************/

/****************************************************************
 * fbe_xor_zero_buffer             
 ****************************************************************
 *
 * @brief
 *   This function is called by xor_execute_processor_command
 *   to carry out the clear buffer command.  
 *
 * @param xor_status_p - ptr to XOR_STATUS block
 * @param vec_info_p - ptr to vectors that comprise this operation
 * @param data_disks - number of disks involved in operation
 * @param ts - programmed ts
 * @param ss - programmed ss
 * @param ws - programmed ws
 *
 * @return VALUES/ERRORS:
 *   none
 *
 *  @notes
 *    The XOR_VECTOR_LIST should have the following format:
 *
 *    [0] - sgl
 *    [1] - seed
 *    [2] - count
 *    [3] - offset
 *
 * @author
 *   10/01/99:  MPG -- Created.
 *
 ****************************************************************/
fbe_status_t fbe_xor_zero_buffer(fbe_xor_zero_buffer_t *zero_buffer_p)
{
    fbe_u32_t fru_cnt;
    fbe_u16_t csum;
    fbe_u32_t sg_count;
    fbe_status_t status;

    /* Append valid checksum and stamps to buffer.
     * Since the block is zeros, the checksum is constant for all blocks. 
     */
    csum = xorlib_cook_csum(0, 0);

    zero_buffer_p->status = FBE_XOR_STATUS_NO_ERROR;

    for (fru_cnt = 0; fru_cnt < zero_buffer_p->disks; fru_cnt++)
    {
        fbe_xor_sgl_descriptor_t sgd;
        fbe_lba_t seed;

        fbe_block_count_t blkcnt;
        fbe_block_count_t block;

        if(zero_buffer_p->fru[fru_cnt].offset > FBE_U32_MAX)
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_WARNING,
                                                  "offset crossing 32bit limit 0x%llx \n",
                                                  (unsigned long long)zero_buffer_p->fru[fru_cnt].offset);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Initialize the sgd.
         */
        FBE_XOR_SGLD_INIT_WITH_OFFSET( sgd, 
                                       zero_buffer_p->fru[fru_cnt].sg_p,
                                       (fbe_u32_t)zero_buffer_p->fru[fru_cnt].offset,
                                       FBE_BE_BYTES_PER_BLOCK );

        /* Verify that the Calling driver has work for the XOR driver, otherwise
           this may be an unnecessary call into the XOR driver and will effect
           system performance.
         */
        blkcnt = zero_buffer_p->fru[fru_cnt].count;
        if (XOR_COND(blkcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_WARNING,
                                  "block count is zero\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        seed = zero_buffer_p->fru[fru_cnt].seed;

        for (block = 0; block < blkcnt; block++)
        {
            fbe_sector_t *blkptr = NULL;

            blkptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd);

            /* Zero the data of the block.
             */
            fbe_zero_memory(blkptr->data_word, FBE_BYTES_PER_BLOCK);

            /* Put on the standard zero stamps.
             */
            blkptr->crc = csum;
            blkptr->time_stamp = FBE_SECTOR_INVALID_TSTAMP;
            blkptr->write_stamp = 0;
            /* Invalid seed means just use a 0 lba stamp for all blocks.
             */
            if (seed == FBE_LBA_INVALID)
            {
                blkptr->lba_stamp = 0;
            }
            else
            {
            blkptr->lba_stamp = xorlib_generate_lba_stamp(seed, zero_buffer_p->offset);
            seed++;
            }
            status = FBE_XOR_SGLD_INC_BE(sgd, sg_count);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
        }
        status = FBE_XOR_SGLD_UNMAP(sgd);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }
    return FBE_STATUS_OK;
}
/* end fbe_xor_zero_buffer() */

/*!**************************************************************
 * fbe_xor_lib_zero_buffer_raw_mirror()
 ****************************************************************
 * @brief
 *  Do a special zero for a raw mirror sector format.
 *  This zeros the sequence number and sets the magic.
 *
 * @param zero_buffer_p - Describes how to zero. 
 *
 * @return fbe_status-t
 *
 * @author
 *  6/12/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_lib_zero_buffer_raw_mirror(fbe_xor_zero_buffer_t *zero_buffer_p)
{
    fbe_u32_t fru_cnt;
    fbe_u16_t csum;
    fbe_u32_t sg_count;
    fbe_status_t status;
    zero_buffer_p->status = FBE_XOR_STATUS_NO_ERROR;

    for (fru_cnt = 0; fru_cnt < zero_buffer_p->disks; fru_cnt++)
    {
        fbe_xor_sgl_descriptor_t sgd;
        fbe_lba_t seed;

        fbe_block_count_t blkcnt;
        fbe_block_count_t block;

        if (zero_buffer_p->fru[fru_cnt].offset > FBE_U32_MAX)
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_WARNING,
                                  "offset crossing 32bit limit 0x%llx \n",
                                  (unsigned long long)zero_buffer_p->fru[fru_cnt].offset);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Initialize the sgd.
         */
        FBE_XOR_SGLD_INIT_WITH_OFFSET( sgd, 
                                       zero_buffer_p->fru[fru_cnt].sg_p,
                                       (fbe_u32_t)zero_buffer_p->fru[fru_cnt].offset,
                                       FBE_BE_BYTES_PER_BLOCK );

        /* Verify that the Calling driver has work for the XOR driver, otherwise
           this may be an unnecessary call into the XOR driver and will effect
           system performance.
         */
        blkcnt = zero_buffer_p->fru[fru_cnt].count;
        if (XOR_COND(blkcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_WARNING,
                                  "block count is zero\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        seed = zero_buffer_p->fru[fru_cnt].seed;

        for (block = 0; block < blkcnt; block++)
        {
            fbe_sector_t *blkptr = NULL;
            fbe_raw_mirror_sector_data_t *raw_blkptr = NULL;

            blkptr = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd);
            raw_blkptr = (fbe_raw_mirror_sector_data_t*)blkptr;
            /* Zero the data of the block.
             */
            fbe_zero_memory(blkptr->data_word, FBE_BYTES_PER_BLOCK);
            /* Sequence number was already set to 0, set magic.
             */
            raw_blkptr->magic_number = FBE_MAGIC_NUMBER_RAW_MIRROR_IO;
            csum = xorlib_cook_csum(xorlib_calc_csum(blkptr->data_word),0);

            /* Put on the standard zero stamps.
             */
            blkptr->crc = csum;
            blkptr->time_stamp = FBE_SECTOR_INVALID_TSTAMP;
            blkptr->write_stamp = 0;
            /* Invalid seed means just use a 0 lba stamp for all blocks.
             */
            if (seed == FBE_LBA_INVALID)
            {
                blkptr->lba_stamp = 0;
            }
            else
            {
                blkptr->lba_stamp = xorlib_generate_lba_stamp(seed, zero_buffer_p->offset);
                seed++;
            }
            status = FBE_XOR_SGLD_INC_BE(sgd, sg_count);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
        }
        status = FBE_XOR_SGLD_UNMAP(sgd);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_xor_lib_zero_buffer_raw_mirror()
 ******************************************/

/*!**************************************************************
 * fbe_xor_calculate_checksum()
 ****************************************************************
 * @brief
 *  Calculate the checksum and return it for the input sector.
 *
 * @param - Ptr for sector we passed in.
 *
 * @return fbe_u16_t crc.   
 *
 * @author
 *  11/4/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_u16_t fbe_xor_calculate_checksum(const fbe_u32_t * srcptr)
{

    fbe_u32_t raw_chksum;
    fbe_u16_t chksum;

    raw_chksum = xorlib_calc_csum((fbe_u32_t *)srcptr);
    chksum = xorlib_cook_csum(raw_chksum, 0 /* No seed */);

    return(chksum);
}
/******************************************
 * end fbe_xor_calculate_checksum()
 ******************************************/

/****************************************************************
 * fbe_xor_error_get_total_bitmask()
 ****************************************************************
 * @brief
 *  This function returns a mask with all the errors.
 *
 * @param eboard_p - eboard to generate pask from.
 *
 * @notes
 *  The assumption is that the cxts eboard has been filled in
 *  by the xor driver as part of an xor/csum operation.
 *
 * @return
 *  fbe_u16_t - mask of errors.
 *
 * @author
 *  2/10/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u16_t fbe_xor_lib_error_get_total_bitmask(fbe_xor_error_t * const eboard_p)
{
    fbe_u16_t total_bitmask;

    total_bitmask = eboard_p->c_crc_bitmap |
                    eboard_p->u_crc_bitmap |
                    eboard_p->c_ws_bitmap  |
                    eboard_p->u_ws_bitmap  |
                    eboard_p->c_ts_bitmap  |
                    eboard_p->u_ts_bitmap  |
                    eboard_p->c_ss_bitmap  |
                    eboard_p->u_ss_bitmap  |
                    eboard_p->c_coh_bitmap |
                    eboard_p->u_coh_bitmap |
                    eboard_p->c_n_poc_coh_bitmap |
                    eboard_p->u_n_poc_coh_bitmap |
                    eboard_p->c_poc_coh_bitmap   |
                    eboard_p->u_poc_coh_bitmap   |
                    eboard_p->c_coh_unk_bitmap  |
                    eboard_p->u_coh_unk_bitmap |
                    eboard_p->u_rm_magic_bitmap |
                    eboard_p->c_rm_magic_bitmap |
                    eboard_p->c_rm_seq_bitmap;

    return total_bitmask;
}
/*****************************************
 * end fbe_xor_error_get_total_bitmask()
 *****************************************/

/****************************************************************
 * fbe_xor_lib_error_get_crc_bitmask()
 ****************************************************************
 * @brief
 *  This function returns a mask with crc errors.
 *
 * @param eboard_p - eboard to generate pask from.
 *
 * @notes
 *  The assumption is that the cxts eboard has been filled in
 *  by the xor driver as part of an xor/csum operation.
 *
 * @return
 *  fbe_u16_t - mask of errors.
 *
 * @author
 *  5/10/2013 - Created. Rob Foley, Deanna Heng
 *
 ****************************************************************/
fbe_u16_t fbe_xor_lib_error_get_crc_bitmask(fbe_xor_error_t * const eboard_p)
{
    fbe_u16_t total_bitmask = 0;

    total_bitmask = eboard_p->c_crc_bitmap |
                    eboard_p->u_crc_bitmap; 


    return total_bitmask;
}
/*****************************************
 * end fbe_xor_lib_error_get_crc_bitmask()
 *****************************************/

/****************************************************************
 * fbe_xor_lib_error_get_wsts_bitmask()
 ****************************************************************
 * @brief
 *  This function returns a mask with write stamp/timestamp errors.
 *
 * @param eboard_p - eboard to generate pask from.
 *
 * @notes
 *  The assumption is that the cxts eboard has been filled in
 *  by the xor driver as part of an xor/csum operation.
 *
 * @return
 *  fbe_u16_t - mask of errors.
 *
 * @author
 *  5/10/2013 - Created. Rob Foley, Deanna Heng
 *
 ****************************************************************/
fbe_u16_t fbe_xor_lib_error_get_wsts_bitmask(fbe_xor_error_t * const eboard_p)
{
    fbe_u16_t total_bitmask = 0;

    total_bitmask = eboard_p->c_ws_bitmap  |
                    eboard_p->u_ws_bitmap  |
                    eboard_p->c_ts_bitmap  |
                    eboard_p->u_ts_bitmap  |
                    eboard_p->c_ss_bitmap  |
                    eboard_p->u_ss_bitmap;

    return total_bitmask;
}
/*****************************************
 * end fbe_xor_lib_error_get_wsts_bitmask()
 *****************************************/

/****************************************************************
 * fbe_xor_lib_error_get_coh_bitmask()
 ****************************************************************
 * @brief
 *  This function returns a mask with coherency errors
 *
 * @param eboard_p - eboard to generate pask from.
 *
 * @notes
 *  The assumption is that the cxts eboard has been filled in
 *  by the xor driver as part of an xor/csum operation.
 *
 * @return
 *  fbe_u16_t - mask of errors.
 *
 * @author
 *  5/10/2013 - Created. Rob Foley, Deanna Heng
 *
 ****************************************************************/
fbe_u16_t fbe_xor_lib_error_get_coh_bitmask(fbe_xor_error_t * const eboard_p)
{
    fbe_u16_t total_bitmask = 0;

    total_bitmask = eboard_p->c_coh_bitmap |
                    eboard_p->u_coh_bitmap |
                    eboard_p->c_n_poc_coh_bitmap |
                    eboard_p->u_n_poc_coh_bitmap |
                    eboard_p->c_poc_coh_bitmap   |
                    eboard_p->u_poc_coh_bitmap   |
                    eboard_p->c_coh_unk_bitmap  |
                    eboard_p->u_coh_unk_bitmap;

    return total_bitmask;
}
/*****************************************
 * end fbe_xor_lib_error_get_coh_bitmask()
 *****************************************/

fbe_status_t fbe_xor_check_lba_stamp_prefetch(fbe_sg_element_t *sg_ptr, fbe_block_count_t blocks)
{
    fbe_u16_t current_block_offset = 0; /* Offset into current fruts. */
    fbe_u16_t sg_offset = 0;          /* Offset into current sg.    */
    fbe_u16_t prefetch_block = 0;     /* Current block number being prefetched. */
    fbe_status_t status = FBE_STATUS_OK;

    /* Prefetch the first few blocks in the sg list.
     */
    while (prefetch_block < FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS &&
           prefetch_block < blocks)
    {
        if (sg_offset >= sg_ptr->count)
        {
            /* If we reached the end of this sg element, increment to next SG.
             */
            sg_ptr++;
            sg_offset = 0;
        }

        if(XOR_COND( (sg_ptr == NULL) ||
                      (sg_offset >= sg_ptr->count) ) )
        {
            /*Split trace to two lines*/
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                 "raid: sg_ptr 0x%p is NULL or sg_offset 0x%x >=\n",
                                 sg_ptr, sg_offset);
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                 "sg_ptr->count 0x%x;\n",
                                 sg_ptr->count);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Execute the prefetch.
         */
        FBE_XOR_PREFETCH_LBA_STAMP((fbe_sector_t*)(sg_ptr->address + sg_offset));

        /* Increment our offsets and prefetch block.
         */
        current_block_offset++;
        sg_offset += FBE_BE_BYTES_PER_BLOCK;
        prefetch_block++;
    } /* end while prefetch block < read fruts blocks. */

    /* Pause to let prefetch start.
     */
    csx_p_atomic_crude_pause();
    return status;
}
/******************************************
 * end fbe_xor_check_lba_stamp_prefetch() 
 ******************************************/

/*!**************************************************************
 * fbe_xor_convert_retryable_reason_to_unretryable()
 ****************************************************************
 * @brief
 *  Modify the invalidate reason for blocks found in this sg list.
 *  We search for a given invalidate reason and switch it
 *  to a new reason.
 *  We do nothing with blocks that are either not invalidated
 *  or are invalidated due to another reason.
 *
 * @param sg_p
 * @param total_blocks - Total blocks to check in the sg list.
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/10/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_xor_convert_retryable_reason_to_unretryable(fbe_sg_element_t *sg_p, 
                                                             fbe_block_count_t total_blocks)
{
    fbe_bool_t b_status;
    fbe_block_count_t blocks = total_blocks;
    fbe_block_count_t current_blocks;
    fbe_block_count_t current_sg_blocks;
    fbe_sector_t *sector_p = NULL;

    while (blocks > 0)
    {
        sector_p = (fbe_sector_t*)fbe_sg_element_address(sg_p);
        current_sg_blocks = fbe_sg_element_count(sg_p) / FBE_BE_BYTES_PER_BLOCK;
        for (current_blocks = 0; 
            ((current_blocks < current_sg_blocks) && (blocks > 0));
            current_blocks++)
        {
            xorlib_sector_invalid_reason_t  updated_reason;

            /* Check if the sector needs to be updated
             */
            b_status = xorlib_does_sector_need_to_be_marked_unretryable(sector_p,
                                                                        &updated_reason);

            /* If we are invalidated for a matching reason then 
             * we want to update this sector with a new reason. 
             */
            if (b_status == FBE_TRUE)
            {
                fbe_lba_t seed;
                xorlib_system_time_t    xorlib_systemtime;
                xorlib_sector_invalid_who_t who;
                xorlib_get_seed_from_invalidated_sector(sector_p, &seed);
                xorlib_get_who_from_invalidated_sector(sector_p, &who);

                fbe_xor_get_invalidation_time(&xorlib_systemtime);

                /* Switch the reason to invalidated.
                 */
                b_status = xorlib_fill_update_invalid_sector(sector_p, 
                                                             seed, 
                                                             updated_reason,
                                                             who,
                                                             &xorlib_systemtime);
            }
            sector_p++;
            blocks--;
        }
        sg_p++;
    }
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_xor_convert_retryable_reason_to_unretryable()
 *******************************************************/


/*****************************************
 * End of  fbe_xor_interface.c
 *****************************************/
