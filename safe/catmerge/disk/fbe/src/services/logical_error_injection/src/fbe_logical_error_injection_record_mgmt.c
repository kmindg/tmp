/*******************************************************************
 * Copyright (C) EMC Corporation 1999 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/
 
/*!******************************************************************
 *  @file fbe_logical_error_injection_record_mgmt.c
 *******************************************************************
 *
 * @brief
 *    This file contains functions which are used to insert
 *    errors into I/Os.
 *    In general, these functions help to automate testing.
 *
 * I N S T R U C T I O N S:
 *
 *    Create a single line in the rg_error_sim_table for each
 *    range on a fru you want to insert errors for.
 *
 *    The errors are all relative to an array width.
 *    So if there is an entry to insert errors on 
 *    position three, then all arrays running I/O through the RG Driver
 *    the RG Driver will experience errors on position three.
 *
 *    To enable error insertion, first uncomment the
 *    RG_ERROR_SIM_ENABLED #define in rg_local.h
 *
 *    Then, to turn on error insertion set the rg_error_sim.enabled
 *    flag to FBE_TRUE.
 *
 * @author
 *  1/4/2010 - Created from rderr.  Rob Foley.
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "xorlib_api.h"
#include "fbe_raid_library_proto.h"
#include "fbe_logical_error_injection_private.h"
#include "fbe_logical_error_injection_proto.h"


/*****************************************************
 *  Global Variables
 *****************************************************/

/*****************************************************
 *  static FUNCTIONS DECLARED FOR VISIBILITY
 *****************************************************/

static fbe_status_t fbe_logical_error_injection_fruts_inject(fbe_raid_fruts_t * fruts_p, fbe_u32_t idx, fbe_logical_error_injection_record_t * rec_p);
static void fbe_logical_error_injection_fruts_inject_media_error(fbe_raid_fruts_t * const fruts_p, fbe_logical_error_injection_record_t * const rec_p);

static fbe_bool_t fbe_logical_error_injection_inject_media_err_write(fbe_raid_fruts_t *const fruts_p,
                                                                     fbe_logical_error_injection_record_t *const rec_p, 
                                                                     fbe_lba_t *bad_lba_p, 
                                                                     fbe_block_count_t bad_blocks);
static void fbe_logical_error_injection_inject_media_err_read(fbe_raid_fruts_t *const fruts_p,
                                                              fbe_logical_error_injection_record_t *const rec_p, 
                                                              fbe_lba_t *bad_lba_p, 
                                                              fbe_block_count_t bad_blocks);
static fbe_status_t fbe_logical_error_injection_corrupt_data(fbe_sector_t * sector_p, fbe_xor_error_type_t type);
static fbe_u16_t fbe_logical_error_injection_convert_bitmap_phys2log(fbe_raid_siots_t* const siots_p,
                                                                     fbe_u16_t bitmap_phys);
static fbe_bool_t fbe_logical_error_injection_match_err_region(const fbe_xor_error_region_t*  const reg_xor_p,
                                   const fbe_xor_error_regions_t* const regs_tbl_p);
static void fbe_logical_error_injection_print_record(const fbe_logical_error_injection_record_t* const rec_p);
static fbe_bool_t fbe_logical_error_injection_is_table_for_r6_only(void);
static void fbe_logical_error_injection_setup_err_adj(void);
fbe_bool_t fbe_logical_error_injection_table_validate(void);
static fbe_bool_t fbe_logical_error_injection_table_validate_flags(void);
static fbe_bool_t fbe_logical_error_injection_table_validate_all(const fbe_logical_error_injection_record_t* const rec_p);
fbe_status_t fbe_logical_error_injection_get_bad_lba( fbe_u64_t request_id,
                           fbe_lba_t *const lba_p,
                           fbe_block_count_t *const blocks_p);

static fbe_bool_t fbe_logical_error_injection_is_lba_metadata_lba(
                                                   fbe_lba_t start_lba,
                                                   fbe_block_count_t block_count,
                                                   fbe_raid_geometry_t *raid_geometry_p);

/* RAID 6 specific local functions.
 */
fbe_s32_t fbe_logical_error_injection_bitmap_to_position(const fbe_u16_t bitmap);
static fbe_bool_t fbe_logical_error_injection_table_validate_r6(const fbe_logical_error_injection_record_t* const rec_p);
static fbe_bool_t fbe_logical_error_injection_rec_pos_match_r6(fbe_raid_fruts_t* const fruts_p,
                                                            fbe_logical_error_injection_record_t* const rec_p);
static fbe_status_t fbe_logical_error_injection_inject_for_r6(fbe_sector_t* const sect_p, 
                                                      const fbe_logical_error_injection_record_t* const rec_p);
static fbe_bool_t fbe_logical_error_injection_rec_pos_match_all_types(fbe_raid_fruts_t*      const fruts_p,
                                       fbe_logical_error_injection_record_t* const rec_p);
static fbe_status_t fbe_logical_error_injection_inject_for_parity_unit(fbe_sector_t* const sect_p, 
                                                               const fbe_logical_error_injection_record_t* const rec_p);
fbe_bool_t fbe_logical_error_injection_fruts_span_err_tbl_max_lba(fbe_raid_fruts_t* const fruts_head_p,
                                                                  fbe_bool_t b_chain);
/******************************************************
 * This means that one in 5 times we will inject a
 * media error if the error type is set to RNDMED.
 * This is arbitrary and can be changed to any
 * number depending on how we want to test.
 ******************************************************/
static fbe_u32_t FBE_LOGICAL_ERROR_INJECTION_RANDOM_MEDIA_SEED = 5;

/*****************************************
 * The below two functions return FBE_TRUE
 * if the table is of the given type.
 * These determine how the table is
 * by looking at the error sim flags,
 * which were read from the xml file.
 *****************************************/
fbe_bool_t fbe_logical_error_injection_is_table_correctable(void)
{
    fbe_logical_error_injection_table_flags_t table_flags;
    fbe_logical_error_injection_get_table_flags(&table_flags);
    return ((table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_CORRECTABLE_TABLE) != 0);
}
fbe_bool_t fbe_logical_error_injection_is_table_uncorrectable(void)
{
    fbe_logical_error_injection_table_flags_t table_flags;
    fbe_logical_error_injection_get_table_flags(&table_flags);
    return ((table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_UNCORRECTABLE_TABLE) != 0);
}

/****************************************************************
 *  fbe_logical_error_injection_corrupt_data()
 ****************************************************************
 *  @brief
 *    Corrupt a sector's data, but leave the checksum as valid.
 *
 * @param sector_p - ptr to sector to corrupt
 * @param type - sort of error to inject TS,WS, etc.
 *
 * @return fbe_status_t
 *    
 *
 *  @author
 *    02/14/00 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_corrupt_data(fbe_sector_t * sector_p, fbe_xor_error_type_t type)
{
    fbe_status_t status;
    fbe_u16_t    cooked_csum;

    /* Ask XOR to invalidate this block for us.
     * Note that this creates a sector with an invalid checksum.
     *
     * The contents of the sector come out to have a checksum
     * of 0x5EED, so by putting a 0x5EED on the sector below,
     * we are intentionally creating a good checksum.
     *
     * Thus we corrupted the data, but left the checksum "good".
     */
    status = fbe_xor_lib_fill_invalid_sector( sector_p /* Block ptr */,
                                              0 /* seed */,
                                              XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA /* reason */,
                                              XORLIB_SECTOR_INVALID_WHO_ERROR_INJECTION /* who */ );
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Now calculated the proper checksum and update the sector crc.
     */
    cooked_csum = xorlib_cook_csum(xorlib_calc_csum(sector_p->data_word),0);
    sector_p->crc = cooked_csum;

    return FBE_STATUS_OK;
}                               /* fbe_logical_error_injection_corrupt_data() */

/****************************************************************
 *  fbe_logical_error_injection_overlap()
 ****************************************************************
 *  @brief
 *    Determine if there is an overlap between the two extents.
 *
 * @param lba - beginning of first extent
 * @param count - end of first extent
 * @param elba - beginning of second extent
 * @param ecount - end of second extent
 *
 *  @return
 *    fbe_bool_t - FBE_TRUE if overlap, FBE_FALSE otherwise.
 *
 *  @author
 *    02/14/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_logical_error_injection_overlap(fbe_lba_t lba,
                                               fbe_block_count_t count,
                                               fbe_lba_t elba,
                                               fbe_block_count_t ecount)
{
    fbe_lba_t erange_end = elba + ecount - 1;
    fbe_lba_t range_end = lba + count - 1;

    if (!((erange_end < lba) ||
          (range_end < elba)))
    {
        /***********************************************************************
         * If there is NO overlap, then lba is either beyond the end of the 
         * beyond the end of the erange
         * OR
         * range_end is before elba
         *                            -------- 
         *                           |        |
         *                           |        |
         *                           |        |
         *                           |        | <----range_end less than elba
         *                 elba----> |********|
         *                           |********|
         *                           |********|
         *                           |********|
         *          erange_end ----> |********|
         *                           |        | <----lba greater than
         *                           |        |      erange_end
         *                           |        |
         *                           |________|
         *
         *
         ***********************************************************************/
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}
/****************************
 * end fbe_logical_error_injection_overlap()
 ****************************/
/*!**************************************************************
 * fbe_logical_error_injection_validate_record_type()
 ****************************************************************
 * @brief
 *  Determine if this error type is one that lei will inject.
 *
 * @param err_type - Validate this err type.
 *
 * @return FBE_STATUS_OK if valid.
 *         FBE_STATUS_GENERIC_FAILURE otherwise.
 *
 * @author
 *  8/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_validate_record_type(fbe_xor_error_type_t err_type)
{
    switch (err_type)
    {
        case FBE_XOR_ERR_TYPE_CRC:
        case FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC:
        case FBE_XOR_ERR_TYPE_MULTI_BIT_CRC:
        case FBE_XOR_ERR_TYPE_MULTI_BIT_WITH_LBA_STAMP:
        case FBE_XOR_ERR_TYPE_KLOND_CRC:
        case FBE_XOR_ERR_TYPE_DH_CRC:
        case FBE_XOR_ERR_TYPE_INVALIDATED:
        case FBE_XOR_ERR_TYPE_RAID_CRC:
        case FBE_XOR_ERR_TYPE_CORRUPT_CRC:
        case FBE_XOR_ERR_TYPE_RND_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM:
        case FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM:
        case FBE_XOR_ERR_TYPE_WS:
        case FBE_XOR_ERR_TYPE_TS:
        case FBE_XOR_ERR_TYPE_BOGUS_WS:
        case FBE_XOR_ERR_TYPE_BOGUS_TS:
        case FBE_XOR_ERR_TYPE_LBA_STAMP:
        case FBE_XOR_ERR_TYPE_SS:
        case FBE_XOR_ERR_TYPE_BOGUS_SS:
        case FBE_XOR_ERR_TYPE_1NS:
        case FBE_XOR_ERR_TYPE_1S:
        case FBE_XOR_ERR_TYPE_1R:
        case FBE_XOR_ERR_TYPE_1D:
        case FBE_XOR_ERR_TYPE_1COD:
        case FBE_XOR_ERR_TYPE_1COP:
        case FBE_XOR_ERR_TYPE_1POC:
        case FBE_XOR_ERR_TYPE_COH:
        case FBE_XOR_ERR_TYPE_DELAY_DOWN:
        case FBE_XOR_ERR_TYPE_DELAY_UP:          
        case FBE_XOR_ERR_TYPE_TIMEOUT_ERR:  
        case FBE_XOR_ERR_TYPE_SILENT_DROP:    
        case FBE_XOR_ERR_TYPE_INCOMPLETE_WRITE:  
        case FBE_XOR_ERR_TYPE_KEY_ERROR:
        case FBE_XOR_ERR_TYPE_KEY_NOT_FOUND:
        case FBE_XOR_ERR_TYPE_ENCRYPTION_NOT_ENABLED:
            return FBE_STATUS_OK;
            break;

        case FBE_XOR_ERR_TYPE_NONE:
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    };
}
/******************************************
 * end fbe_logical_error_injection_validate_record_type()
 ******************************************/
/****************************************************************
 *  fbe_logical_error_injection_corrupt_sector()
 ****************************************************************
 * @brief
 *    Corrupt the given sector with the given error type.
 *
 * @param sector_p - ptr to sector to corrupt
 * @param rec_p - pointer to error injuection record
 * @param pos - array position of this sector
 * @param seed - seed (lba) for this sector.
 * @param raid_type - raid type (if applicable) for object being injected
 * @param b_is_proactive_sparing - FBE_TRUE - Proactive sparing
 * @param start_lba - Starting lba of transfer
 * @param xfer_count - Blocks to transfer
 * @param width - width of array
 * @param parity_bitmask - Bitmask of parity positions
 * @param b_is_parity_position - FBE_TRUE - This is a parity position
 *
 *  @return
 *    fbe_status_t
 *
 *  @author
 *    02/14/00 - Created. Rob Foley
 *    06/27/00 - For good measure, let's corrupt data whenever
 *               we inject an error, by calling fbe_logical_error_injection_corrupt_data
 *               TS errors will (still) not cause crc errors.
 *               
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_corrupt_sector(fbe_sector_t * sector_p,
                                                        const fbe_logical_error_injection_record_t * const rec_p,
                                                        fbe_u16_t pos, 
                                                        fbe_lba_t seed,
                                                        fbe_raid_group_type_t raid_type,
                                                        fbe_bool_t b_is_proactive_sparing,
                                                        fbe_lba_t start_lba,
                                                        fbe_block_count_t xfer_count,
                                                        fbe_u32_t width,
                                                        fbe_raid_position_bitmask_t parity_bitmask,
                                                        fbe_bool_t b_is_parity_position)

{
    fbe_status_t status;
    const fbe_xor_error_type_t error_type = rec_p->err_type;
    const fbe_u16_t       array_width = rec_p->width;
    fbe_u16_t mask = 0;           /* Mask of valid write stamps. */

    /* Special handling for hot-spares.
     */
    if ((raid_type == FBE_RAID_GROUP_TYPE_SPARE) && 
        (b_is_proactive_sparing == FBE_FALSE)       )  
    {
        /* Hotspares sometimes inject errors (like during equalizes)
         * But they can only inject errors that they can detect.
         * Stamp errors should only be injected by layers that
         * can detect them so that they can be correctly
         * injected to their proper position.
         */

        switch (error_type)
        {
            case FBE_XOR_ERR_TYPE_CRC:
            case FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC:
            case FBE_XOR_ERR_TYPE_MULTI_BIT_WITH_LBA_STAMP:
            case FBE_XOR_ERR_TYPE_KLOND_CRC:
            case FBE_XOR_ERR_TYPE_DH_CRC:
            case FBE_XOR_ERR_TYPE_INVALIDATED:
            case FBE_XOR_ERR_TYPE_RAID_CRC:
            case FBE_XOR_ERR_TYPE_CORRUPT_CRC:
            case FBE_XOR_ERR_TYPE_RND_MEDIA_ERR:
            case FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR:
            case FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR:
            case FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM:
            case FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM:
                break;

            default:
                return FBE_STATUS_OK;
        }

    }

    if (b_is_parity_position)
    {
        /* For parity drives we need to know the correct
         * write stamp bits, so we will corrupt
         * the appropriate bits.
         */
        fbe_u32_t i;

        /* Create array mask.  */
        for (mask = 0, i = 0; i < array_width; i++)
        {
            mask |= (1 << i);
        }
        /* Clear parity bit
         */
        mask &= ~(1 << pos);
    }
    {
        fbe_sector_t *ref_sect_p = NULL;
        fbe_raw_mirror_sector_data_t *reference_p;

        ref_sect_p = (fbe_sector_t *) sector_p;
        
        /* Corrupt the metadata differently depending upon
         * the actual error we intend to inject.
         */
        switch (error_type)
        {
            case FBE_XOR_ERR_TYPE_CRC:
            case FBE_XOR_ERR_TYPE_MULTI_BIT_CRC:
                /* Corrupt the sector by clearing it out.
                 * Also corrupt the crc by making it zero.
                 */
                fbe_zero_memory((void *) ref_sect_p, sizeof(fbe_sector_t));
                ref_sect_p->crc = 0;
                break;

            case FBE_XOR_ERR_TYPE_MULTI_BIT_WITH_LBA_STAMP:
                /* Corrupt the sector by clearing it out.
                 * Also corrupt the crc by making it zero. 
                 * Also make lba stamp invalid for this block. 
                 */
                fbe_zero_memory((void *) ref_sect_p, sizeof(fbe_sector_t));
                ref_sect_p->crc = 0;
                ref_sect_p->lba_stamp = (seed != 0xBAD) ? 0xBAD : 0xBAD0;
                break;


            case FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC:
                /* Corrupt the sector by clearing it out. Also, corrupt CRC 
                 * which differs only in one-bit. It simulates the single bit 
                 * CRC error.
                 */
                fbe_zero_memory((void *) ref_sect_p, sizeof(fbe_sector_t));
                ref_sect_p->crc = fbe_xor_lib_calculate_checksum(ref_sect_p->data_word);
                ref_sect_p->crc = (ref_sect_p->crc & 0x0001) ? (ref_sect_p->crc & 0xFFFE) : (ref_sect_p->crc | 0x0001);
                break;

            case FBE_XOR_ERR_TYPE_KLOND_CRC:
                /* Klondike CRC error.
                 *
                 * Simulates what happens when Kondike invalidates
                 * a block during its own media error handling.
                 *
                 * Klondike pattern is a background of zeros,
                 * and first and last 32 bit words are 0xFFFFFFFF.
                 */
                fbe_zero_memory((void *) ref_sect_p, sizeof(fbe_sector_t));
                *((fbe_u32_t*)ref_sect_p->data_word) = 0xFFFFFFFF;

                /* The shed stamp and write stamp are
                 * the last 32 bits in the block.
                 */
                ref_sect_p->lba_stamp = 0xFFFF;
                ref_sect_p->write_stamp = 0xFFFF;
                break;

            case FBE_XOR_ERR_TYPE_DH_CRC:
                /* CRC error, just invert the checksum.
                 *
                 * Simulates a block the DH invalidated due
                 * to remap handling.
                 */
                status = fbe_xor_lib_fill_invalid_sector(ref_sect_p,
                                                         0 /* seed */,
                                                         XORLIB_SECTOR_INVALID_REASON_DH_INVALIDATED /* reason */,
                                                         XORLIB_SECTOR_INVALID_WHO_ERROR_INJECTION /* who */);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }
                break;

            case FBE_XOR_ERR_TYPE_INVALIDATED:
                /* Inject a `invalidated' sector error.
                 *
                 * This simulates what would happen if the raid/xor invalidated
                 * a block that was not recoverable at the raid level.
                 * We don't inject for Raid 6 since it treats raid invalidated 
                 * errors as "expected", and injecting one would cause a 
                 * coherency error.
                 */
                if (raid_type != FBE_RAID_GROUP_TYPE_RAID6)
                {
                    status = fbe_xor_lib_fill_invalid_sector(ref_sect_p,
                                                             seed /* seed */,
                                                             XORLIB_SECTOR_INVALID_REASON_DATA_LOST /* reason */,
                                                             XORLIB_SECTOR_INVALID_WHO_ERROR_INJECTION /* who */);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }
                }
                break;

            case FBE_XOR_ERR_TYPE_RAID_CRC:
                /* Inject a "raid" invalidated crc.
                 *
                 * This simulates what would happen if the
                 * raid/xor invalidated a block that was
                 * not recoverable at the raid level.
                 * We don't inject for Raid 6 since it 
                 * treats raid invalidated errors as "expected",
                 * and injecting one would cause a coherency error.
                 */
                if (raid_type != FBE_RAID_GROUP_TYPE_RAID6)
                {
                    status = fbe_xor_lib_fill_invalid_sector(ref_sect_p,
                                                             seed /* seed */,
                                                             XORLIB_SECTOR_INVALID_REASON_VERIFY /* reason */,
                                                             XORLIB_SECTOR_INVALID_WHO_ERROR_INJECTION /* who */);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }
                }
                break;

            case FBE_XOR_ERR_TYPE_CORRUPT_CRC:
                /*! @note Inject a corrupt crc style.
                 *
                 * This simulates what would have happened if a sector read from
                 * disk had a bad checksum value.  We use the invalidation library
                 * since it only modifies the crc metadata field and it generates
                 * a recognizable pattern. We don't inject for Raid 6 since it 
                 * treats raid invalidated errors as "expected" and injecting one 
                 * would cause a coherency error.
                 */
                if (raid_type != FBE_RAID_GROUP_TYPE_RAID6)
                {
                    status = fbe_xor_lib_fill_invalid_sector(ref_sect_p,
                                                             0 /* seed */,
                                                             XORLIB_SECTOR_INVALID_REASON_RAID_CORRUPT_CRC /* reason */,
                                                             XORLIB_SECTOR_INVALID_WHO_ERROR_INJECTION /* who */);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }
                }
                break;

            case FBE_XOR_ERR_TYPE_WS:
                /* Write stamp error.
                 */
                if (b_is_parity_position)
                {
                    /* Toggle any set write stamps */
                    ref_sect_p->write_stamp = ref_sect_p->write_stamp ^ mask;

                    /* Make sure any write stamps are not beyond width.
                     */
                    ref_sect_p->write_stamp &= ((1 << width) - 1);

                    /* Make sure write stamps not set for parity, and that
                     * all time stamps is not set, otherwise we'll get
                     * a bogus stamp error.
                     */
                    ref_sect_p->write_stamp &= ~parity_bitmask;
                    ref_sect_p->time_stamp &= ~0x8000;
                }
                else
                {
                    /* Corrupt by just toggling write stamp.  */
                    ref_sect_p->write_stamp = ref_sect_p->write_stamp
                        ? ref_sect_p->write_stamp & ~(1 << pos)
                        : (1 << pos);
                    ref_sect_p->time_stamp = 0x7FFF;
                }
                break;

            case FBE_XOR_ERR_TYPE_TS:
                /* Corrupt time stamp by
                 * first trying to put 0x0BAD as ts
                 * alternatively use 0x1234 as ts
                 */
                ref_sect_p->time_stamp = 0x1BAD;
                if (b_is_parity_position == FBE_FALSE)
                {
                    /* Mark write stamp to match the time stamp.
                     * Otherwise this will be a bogus time stamp error.
                     */
                    ref_sect_p->write_stamp = 0;
                }
                
                break;

            case FBE_XOR_ERR_TYPE_BOGUS_WS:
                /* Create an Invalid write stamp; i.e. a write stamp that is "corrupt"
                 */
                if (b_is_parity_position)
                {
                    /* Corrupt parity ws by setting parity ws bit
                     */
                    ref_sect_p->write_stamp = ref_sect_p->write_stamp | (1 << pos);
                }
                else
                {
                    /* Corrupt by just toggling position beyond array.
                     */
                    ref_sect_p->write_stamp = (pos == 0) ? 2 : 1;
                }
                break;

            case FBE_XOR_ERR_TYPE_BOGUS_TS:
                /* Create an Invalid time stamp; i.e. a time stamp that is "corrupt"
                 */
                if (b_is_parity_position)
                {
                    /* Corrupt parity ts by setting all ts AND invalid ts.
                     */
                    ref_sect_p->time_stamp = 0xFBAD;
                }
                else
                {
                    /* Corrupt by setting all ts bit on data sector
                     */
                    ref_sect_p->time_stamp = 0x8BAD;
                }
                break;

            case FBE_XOR_ERR_TYPE_LBA_STAMP:
            case FBE_XOR_ERR_TYPE_SS:
            case FBE_XOR_ERR_TYPE_BOGUS_SS:
                /* Create a corrupted ss.
                 * LBA stamps will be handled the same since they are essentially
                 * the same error.
                 */
                if (b_is_parity_position)
                {
                    if (error_type == FBE_XOR_ERR_TYPE_BOGUS_SS)
                    {
                        /* Create a corrupt ss by setting this position's bit and other bits.
                         */
                        ref_sect_p->lba_stamp = 0xFF | (1 << pos);
                    }
                    else
                    {
                        /* Create a corrupt ss by setting this position's bit.
                         */
                        ref_sect_p->lba_stamp = (1 << pos);
                    }
                }
                else
                {
                    /* Data positions get their LBA stamp set to 0x8BAD.
                     */
                    if (((fbe_u16_t) seed) != 0x8BAD)
                    {
                        ref_sect_p->lba_stamp = 0x8BAD;
                    }
                    else
                    {
                        /* If the address calculates to the same lba stamp
                         * as our corrupted value, use a different one.
                         */
                        ref_sect_p->lba_stamp = 0xFBAD;
                    }
                }
                break;

            case FBE_XOR_ERR_TYPE_1NS:
            case FBE_XOR_ERR_TYPE_1S:
            case FBE_XOR_ERR_TYPE_1R:
            case FBE_XOR_ERR_TYPE_1D:
            case FBE_XOR_ERR_TYPE_1COD:
            case FBE_XOR_ERR_TYPE_1COP:
            case FBE_XOR_ERR_TYPE_1POC:
                status = fbe_logical_error_injection_inject_for_r6(ref_sect_p, rec_p);
                if(LEI_COND(status != FBE_STATUS_OK))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: logical error injection: Bad status from fbe_logical_error_injection_inject_for_r6"
                        "rec_p %p ref_sect_p %p start_lba 0x%llx xfer_count 0x%llx\n",
                        __FUNCTION__, rec_p, ref_sect_p,
		        (unsigned long long)start_lba,
		        (unsigned long long)xfer_count);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                break;

            case FBE_XOR_ERR_TYPE_COH:
                status = fbe_logical_error_injection_inject_for_parity_unit(ref_sect_p, rec_p);
                if(LEI_COND(status != FBE_STATUS_OK))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: rec_p %p ref_sect_p %p start_lba 0x%llx xfer_count 0x%llx\n",
                        __FUNCTION__, rec_p, ref_sect_p,
		        (unsigned long long)start_lba,
		        (unsigned long long)xfer_count);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                break;

            case FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM:
                /* Raw Mirror bad magic number error.
                 *
                 * Simulates what happens when a mirror in a raw mirror has an invalid magic number.
                 */
                reference_p = (fbe_raw_mirror_sector_data_t*)sector_p;
                reference_p->magic_number = FBE_LBA_INVALID;

                /* Recalculate CRC for this data field.
                 */
                ref_sect_p->crc = fbe_xor_lib_calculate_checksum((fbe_u32_t*)sector_p);

                break;

            case FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM:
                /* Raw Mirror bad sequence number error.
                 *
                 * Simulates what happens when a mirror in a raw mirror has an invalid sequence number.
                 * Decrement the current sequence number to cause a mismatch.
                 */        
                reference_p = (fbe_raw_mirror_sector_data_t*)sector_p;
                reference_p->sequence_number--;

                /* Recalculate CRC for this data field.
                 */
                ref_sect_p->crc = fbe_xor_lib_calculate_checksum((fbe_u32_t*)sector_p);

                break;

            case FBE_XOR_ERR_TYPE_RND_MEDIA_ERR:
            case FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR:
            case FBE_XOR_ERR_TYPE_NONE:
            case FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR:
            default:
                break;
        }
    }
    
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_logical_error_injection_corrupt_sector()
 **************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_inject_media_err_read()
 ****************************************************************
 * @brief
 *  Injects a media error on the read phase of a read and remap.
 *  
 * @param fruts_p - Fruts on which we are injecting an error.
 * @param rec_p - Error we are injecting.
 * @param bad_lba_p - Ptr to the bad lba to inject.
 * @param bad_blocks - The length of the bad region.
 *
 * @return - None.
 *
 * @author
 *  12/9/2008 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_logical_error_injection_inject_media_err_read(fbe_raid_fruts_t *const fruts_p,
                                                              fbe_logical_error_injection_record_t *const rec_p, 
                                                              fbe_lba_t *bad_lba_p, 
                                                              fbe_block_count_t bad_blocks)
{
    fbe_lba_t last_error_lba;
    fbe_bool_t b_last_error_match;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    fbe_logical_error_injection_object_t *object_p = NULL;

    /* Fetch the object ptr.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);

    last_error_lba = object_p->last_media_error_lba[fruts_p->position];

    /* FBE_TRUE if the last error we hit was in our range.
     */
    b_last_error_match = (last_error_lba >= *bad_lba_p &&
                          last_error_lba < *bad_lba_p + bad_blocks);

    if (b_last_error_match)
    {
        /* The last block we took an error on is where we need to inject an error.
         */
        *bad_lba_p = last_error_lba;
    }
    else
    {
        /* We didn't find a match with the last remap error.
         * Simply set the last remap error so that write verify 
         * will inject an error on the next lba indicated by the error record.
         */
        object_p->last_media_error_lba[fruts_p->position] = *bad_lba_p;
        object_p->first_media_error_lba[fruts_p->position] = *bad_lba_p;
    }

    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "logic err inject:inject_media_err obj_id:%d " 
                                              "pos:0x%x Lba:0x%llX Blks:0x%llx badlba:0x%llX\n",
                                              object_id, fruts_p->position,
					      (unsigned long long)fruts_p->lba,
					      (unsigned long long)fruts_p->blocks,
					      (unsigned long long)(*bad_lba_p));

#if 0
    fbe_logical_error_injection_add_log(*bad_lba_p, rec_p->err_type, fruts_p->position,
                         fruts_p->opcode, RG_GET_UNIT(siots_p));
#endif


    return;
}
/**************************************
 * end fbe_logical_error_injection_inject_media_err_read()
 **************************************/

/*!**************************************************************
 * fbe_logical_error_injection_inject_media_err_write()
 ****************************************************************
 * @brief
 *  Injects a media error on the write phase of a read remap request.
 *  
 * @param fruts_p - Fruts injecting for.
 * @param rec_p - Error we are injecting.
 * @param bad_lba_p - Ptr to the bad lba to inject.
 * @param bad_blocks - The length of the bad region.
 *
 * @return FBE_TRUE if there is an error to inject.
 *         FBE_FALSE if there is no error to inject. 
 *
 * @author
 *  12/9/2008 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_bool_t 
fbe_logical_error_injection_inject_media_err_write(fbe_raid_fruts_t *const fruts_p,
                                                   fbe_logical_error_injection_record_t *const rec_p, 
                                                   fbe_lba_t *bad_lba_p, 
                                                   fbe_block_count_t bad_blocks)
{
    fbe_bool_t b_inject_error = FBE_FALSE;
    fbe_lba_t last_error_lba;
    fbe_bool_t b_last_error_match;
    fbe_bool_t b_next_error_match;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    fbe_logical_error_injection_object_t *object_p = NULL;
    /* Fetch the object ptr.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);

    last_error_lba = object_p->last_media_error_lba[fruts_p->position];

    /* FBE_TRUE if the last error we hit was in our range.
     */
    b_last_error_match = (last_error_lba >= *bad_lba_p &&
                          last_error_lba < *bad_lba_p + bad_blocks);
    /* True if the next error is also in our fruts range.
     */
    b_next_error_match = (last_error_lba + 1 >= *bad_lba_p &&
                          last_error_lba + 1 < *bad_lba_p + bad_blocks);

    if (b_last_error_match)
    {
        /* If our last remap lba is within the range of our 
         * request, then set the last remap lba (and bad lba) to the
         * next block that needs to be remapped.  
         */
        object_p->last_media_error_lba[fruts_p->position]++;
        fbe_logical_error_injection_object_inc_num_wr_remaps(object_p);

        if (b_next_error_match)
        {
            /* The next error is also within our range. 
             * Set the transfer count accordingly. 
             *  
             * |******************************| /|\
             * |******************************|  | Range of the current
             * |******************************|  | error being injected.
             * |*prior remap error************| \|/
             * |**Next remap error************| 
             * |******************************| 
             * |******************************| 
             *  
             */
            *bad_lba_p = object_p->last_media_error_lba[fruts_p->position];
            b_inject_error = FBE_TRUE;

            /*Split trace into two lines*/
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                      FBE_TRACE_MESSAGE_ID_INFO,
                                                      "logical error injection: inject_media_error op:0x%x object_id:%d " 
                                                      "pos:0x%x>\n",
                                                      fruts_p->opcode, object_id, fruts_p->position);
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                      FBE_TRACE_MESSAGE_ID_INFO,
                                                      "Lba:0x%llX Blks:0x%llx bad lba:0x%llX last bad lba:0x%llX<\n",
                                                      (unsigned long long)fruts_p->lba, (unsigned long long)fruts_p->blocks,
                                                      (unsigned long long)*bad_lba_p, (unsigned long long)object_p->last_media_error_lba[fruts_p->position]);
  
  
#if 0   
            fbe_logical_error_injection_add_log(fru_info_p->last_media_error_lba, 
                                 rec_p->err_type, fruts_p->position,
                                 fruts_p->opcode, RG_GET_UNIT(siots_p));
#endif


        }
        else
        {
            /* The current error is in our range, but 
             * the next error is not in our range. 
             */
            if ((*bad_lba_p + bad_blocks + 1) > (fruts_p->lba + fruts_p->blocks))
            {
                /* The next bad lba (end + 1), which is the next media error to inject, is
                 * beyond our fruts range.
                 * This means that the write verify was successful.
                 * Don't inject an error.
                 *
                 * |******************************| /|\
                 * |******************************|  | Range of the current
                 * |******************************|  | error being injected.
                 * |*prior remap error************| \|/
                 * | Next remap error             |
                 */
            }
            else
            {
                /* The next bad lba is beyond our fruts range. 
                 * This means that the write verify was successful.
                 * Don't inject an error.
                 * However, the error range ends within the fruts. This means that there
                 * really is no "next" media error to inject.  Set the last remap media
                 * error to invalid.
                 *
                 *  /|\  |********************| /|\ 
                 *   |   |********************|  |  Range of
                 * fruts |********************|  |  the current
                 * range |********************|  |  error being
                 *   |   |*prior remap error**| \|/ injected.
                 *   |   | Next remap error   |
                 *   |   |********************|
                 *   |   |********************|
                 *  \|/  |********************|
                 */
                object_p->last_media_error_lba[fruts_p->position] = FBE_LBA_INVALID;
            }
            if (((rec_p->lba + rec_p->blocks) == (*bad_lba_p + bad_blocks)) &&
                (object_p->first_media_error_lba[fruts_p->position] == rec_p->lba) &&
                (rec_p->err_mode == FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED))
            {
                /* The end of the record matches the end of this transfer.
                 * The start of the record matches the start of where we injected 
                 * errors. 
                 * And the type of record is of type inject until remapped, then disable 
                 * it. 
                 */
                rec_p->err_type = FBE_XOR_ERR_TYPE_NONE;
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                          FBE_TRACE_MESSAGE_ID_INFO,
                                                          "LEI:err op:0x%x obj_id:0x%x " 
                                                          "pos:0x%x,Lba:0x%llX Blks:0x%llx bad:0x%llX last bad:0x%llX\n",
                                                          fruts_p->opcode, object_id, fruts_p->position, 
                                                          fruts_p->lba, fruts_p->blocks,
                                                          *bad_lba_p, object_p->last_media_error_lba[fruts_p->position]);
            }
            /* No error to inject.
             */
            b_inject_error = FBE_FALSE;

        }    /* end else next error not in our range. */

    }    /* end the last error matched our error to inject. */
    else
    {
        /* No error to inject.
         */
        b_inject_error = FBE_FALSE;
    }
    return b_inject_error;
}
/**************************************
 * end fbe_logical_error_injection_inject_media_err_write()
 **************************************/

/*!**************************************************************
 * fbe_logical_error_injection_fruts_inject_media_error
 ****************************************************************
 * @brief
 *  Inject a media error for the given error record and fruts.
 *  
 * @param fruts_p - Active fruts to inject for.
 * @param rec_p - Error record to inject for.
 *
 * @return - None.
 *
 * @author
 *  12/5/2008 - Created. Rob Foley
 *
 ****************************************************************/
static void fbe_logical_error_injection_fruts_inject_media_error(fbe_raid_fruts_t *const fruts_p, fbe_logical_error_injection_record_t *const rec_p)
{
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_object_id_t object_id;
    fbe_packet_t * packet;
    fbe_payload_ex_t * payload;
    fbe_logical_error_injection_object_t *object_p = NULL;

    packet = fbe_raid_fruts_get_packet(fruts_p);
    payload = fbe_transport_get_payload_ex(packet);

    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    block_operation_p = fbe_raid_fruts_to_block_operation(fruts_p);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    /* Fetch the object ptr.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);
    /*! Enable below in order to turn on tracing in this function.  Useful for debugging. 
     */
    //service_p->base_service.trace_level = FBE_TRACE_LEVEL_DEBUG_LOW;

    /* Just return if this record is not of the media error type.
     */
    if ( rec_p->err_type != FBE_XOR_ERR_TYPE_RND_MEDIA_ERR &&
         rec_p->err_type != FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR && 
         rec_p->err_type != FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR )
    {
        return;
    }
    /* We should only inject hard errors on certain opcodes.
     * Writes will only have soft media errors.
     */
    if ((fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)                    ||
        (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)                   ||
        (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)            ||
        (fbe_raid_geometry_is_raid0(raid_geometry_p)                               &&
         ((fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY)       ||
          (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY)    )   )     )
    {
        fbe_xor_error_type_t eErrType = rec_p->err_type;
        fbe_lba_t bad_lba;
        fbe_block_count_t bad_blocks; 

        /* Determine the current range of error we are injecting.
         */
        fbe_logical_error_injection_get_bad_lba( (fbe_u64_t)(csx_ptrhld_t)fruts_p,
                                                 &bad_lba,
                                                 &bad_blocks );

        /* We handle the lba of the error differently on read remap. 
         * This is so that we will appear to make progress (or not) 
         * while remapping. 
         */
        if ( (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY ) ||             
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY) )            
        {
            if (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
            {
                fbe_logical_error_injection_inject_media_err_read(fruts_p, rec_p, &bad_lba, bad_blocks);
            } /* fruts is a read. */
            else if (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)
            {
                /* If an error should not be injected on this write/write-verify, then return.
                 */
                if (!fbe_logical_error_injection_inject_media_err_write(fruts_p,
                                                                        rec_p, 
                                                                        &bad_lba, 
                                                                        bad_blocks))
                {
                    return;
                }
            } /* Fruts is write verify or write*/
        } /* iots is a read remap. */

        /* Make sure the lba we are injecting the error on is within the range of the
         * fruts.  It should be greater than or equal to the start of the fruts range 
         * and less than the end of the fruts range. 
         */
        if ((bad_lba < fruts_p->lba) ||
            (bad_lba >= fruts_p->lba + fruts_p->blocks))
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                      "%s line %d", __FUNCTION__, __LINE__);
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                      "bad lba: 0x%llx not in range of fruts_lba 0x%llx fruts_bl: 0x%llx",
                                                      (unsigned long long)bad_lba, (unsigned long long)fruts_p->lba, (unsigned long long)fruts_p->blocks);
            return;
        }
        /* Set the status up so that we remember to tell
         * the DH to inject media errors.
         */
        if ( eErrType == FBE_XOR_ERR_TYPE_RND_MEDIA_ERR )
        {
            /* Randomly choose a hard or soft media error.
             * The DH will also randomly choose where to
             * put this error.
             */
            eErrType = (fbe_random() % FBE_LOGICAL_ERROR_INJECTION_RANDOM_MEDIA_SEED) ? 
                       FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR : FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR;

            if ( eErrType == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR )
            {
                fbe_payload_block_set_status(block_operation_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
                fbe_payload_ex_set_media_error_lba(payload, bad_lba);

                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: Inject ME obj: 0x%x alg: 0x%x pos: %d lba/bad: %llx/%llx blocks: 0x%llx opcode: %d\n",
                    object_id, siots_p->algorithm, fruts_p->position, (unsigned long long)fruts_p->lba, (unsigned long long)bad_lba, (unsigned long long)fruts_p->blocks, fruts_p->opcode);
            }
            else
            {
                if(LEI_COND( eErrType != FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR ))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: logical error injection: eErrType 0x%x != FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR\n",
                        __FUNCTION__, eErrType);
                    return;
                }
                fbe_payload_ex_set_media_error_lba(payload, bad_lba);
                fbe_payload_block_set_status(block_operation_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED);
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: Inject SME obj: 0x%x alg: %d pos: %d lba/bad: %llx/%llx blocks: 0x%llx opcode: %d\n",
                    object_id, siots_p->algorithm, fruts_p->position, (unsigned long long)fruts_p->lba, (unsigned long long)bad_lba, (unsigned long long)fruts_p->blocks, fruts_p->opcode);
            }
        }
        else if ( eErrType == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR )
        {
            /* If we are not injecting random media errors,
             * then we must inject errors precisely and not
             * attempt to randomize at all.
             */
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
            fbe_payload_ex_set_media_error_lba(payload, bad_lba);
            fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: Inject ME obj: 0x%x alg: 0x%x pos: %d lba/bad: %llx/%llx blocks: 0x%llx opcode: %d\n",
                    object_id, siots_p->algorithm, fruts_p->position, (unsigned long long)fruts_p->lba, (unsigned long long)bad_lba, (unsigned long long)fruts_p->blocks, fruts_p->opcode);
        }
        else
        {
            if ( eErrType != FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR )
            {
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                          "%s eErrType %d not expected line %d\n",
                                                          __FUNCTION__, eErrType,__LINE__);
                return;
            }
            fbe_payload_ex_set_media_error_lba(payload, bad_lba);
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED);
            fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: Inject SME obj: 0x%x alg: %d pos: %d lba/bad: %llx/%llx blocks: 0x%llx opcode: %d\n",
                    object_id, siots_p->algorithm, fruts_p->position, (unsigned long long)fruts_p->lba,  (unsigned long long)bad_lba, (unsigned long long)fruts_p->blocks, fruts_p->opcode);
        }
        /* Since the error is injected, increment the read error counts */
        if (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
        {
            fbe_logical_error_injection_object_inc_num_rd_media_errors(object_p);
        } /* fruts is a read. */

        fbe_raid_fruts_set_flag(fruts_p, FBE_RAID_FRUTS_FLAG_ERROR_INJECTED);
    } /* end if READ/READ_REMAP/VERIFY_BLOCKS/(RAW && VERIFY */
    return;
}
/**************************************
 * end fbe_logical_error_injection_fruts_inject_media_error()
 **************************************/

/****************************************************************
 *  fbe_logical_error_injection_fruts_inject()
 ****************************************************************
 *  @brief
 *    Corrupt all overlap between the fruts and the
 *    input error record with the error(s) indicated in the
 *    error record.
 *
 * @param fruts_p - ptr to fruts to corrupt
 * @param idx - Index into error table.
 * @param rec_p - ptr to current error record
 *
 *  @return fbe_status_t
 *    
 *
 *  @author
 *    02/14/00 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_fruts_inject(fbe_raid_fruts_t * fruts_p, fbe_u32_t idx, fbe_logical_error_injection_record_t * rec_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t parity_pos = FBE_FALSE;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);  
    fbe_u64_t phys_addr_local = 0;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    fbe_logical_error_injection_object_t *object_p = NULL;
    fbe_raid_position_bitmask_t parity_bitmask = fbe_raid_siots_parity_bitmask(siots_p);
    fbe_raid_group_type_t raid_type;
    fbe_bool_t b_is_proactive_sparing;

    /* Fetch the object ptr.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);
    if (object_p == NULL)
    {
        /* We are not injecting for this object, so not sure why we are here.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
             "%s object_id: 0x%x was not found"
             "while inject 0x%x on algorithm %d fruts pos: %d lba: %llx blocks: 0x%llx opcode: %d\n",
             __FUNCTION__, object_id, rec_p->err_type, siots_p->algorithm, 
             fruts_p->position, (unsigned long long)fruts_p->lba,
	    (unsigned long long)fruts_p->blocks, fruts_p->opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Zero opcode is not supported.
     */
    if (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO )
    {
        /* These ops do not have buffers associated with them, just return.
         */
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: logical error injection: fruts_p->opcode 0x%x == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO"
            "object_id:%d pos:0x%x, Lba:0x%llX Blks:0x%llx \n",
            __FUNCTION__, fruts_p->opcode, object_id,
            fruts_p->position, (unsigned long long)fruts_p->lba,
	    (unsigned long long)fruts_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Take care of Incomplete Write errors
     */
    if (rec_p->err_type == FBE_XOR_ERR_TYPE_INCOMPLETE_WRITE)
    {
        if ((fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE ) || 
            (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY))
        {

            /* Error was injected before data is written to disk by decresing block count.
             * Now reset the block count and set the status. 
             */
            fbe_block_count_t blk_cnt = 0;
            fbe_payload_ex_t* payload_p = fbe_transport_get_payload_ex(&fruts_p->io_packet);
            fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

            fbe_payload_block_get_block_count(block_operation_p, &blk_cnt);
            if (blk_cnt >= 1)
            {
                blk_cnt++;
                fbe_payload_block_set_block_count(block_operation_p, blk_cnt);
                fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);

                /* Increment counts */
                fbe_logical_error_injection_lock();
                fbe_logical_error_injection_object_lock(object_p);
                fbe_logical_error_injection_object_inc_num_errors(object_p);
                fbe_logical_error_injection_object_unlock(object_p);
                fbe_logical_error_injection_inc_num_errors_injected();
                fbe_logical_error_injection_unlock();
            }
        }
        return FBE_STATUS_OK;
    }

    /* We don't allow corruption for write and write verify.
     */
    if ((fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)        ||
        (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)    )
    {
        /* We don't corrupt crc for write operations.
         */
        if ((rec_p->err_type != FBE_XOR_ERR_TYPE_RND_MEDIA_ERR)  &&
            (rec_p->err_type != FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR) &&
            (rec_p->err_type != FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR)    )
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, FBE_TRACE_MESSAGE_ID_INFO,
                                          "LEI: fruts inject skip write: obj: 0x%x pos: %d op: %d lba: 0x%llx blks: 0x%llx \n",
                                          object_id, fruts_p->position, fruts_p->opcode,
                                          (unsigned long long)fruts_p->lba, (unsigned long long)fruts_p->blocks);
            return FBE_STATUS_OK;
        }

    }

    /* Always inject media errors.
     */
    fbe_logical_error_injection_fruts_inject_media_error(fruts_p, rec_p);

    /* Determine locals required for corrupt sector.
     */
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    b_is_proactive_sparing = fbe_raid_geometry_is_sparing_type(raid_geometry_p);

    /* Also set the callback so that raid can call us back to validate that the correct 
     * action was taken for this error. 
     */
    fbe_raid_siots_set_error_validation_callback(siots_p, fbe_logical_error_injection_correction_validate);

    /* If we are here, then we really decided to inject an error.
     * Mark the SIOTS as having an error injected.
     * This flag allows us to know errors have been injected,
     * so that we do not flood the ulog with messages.
     */
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_ERROR_INJECTED);

    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_object_lock(object_p);
    fbe_logical_error_injection_object_inc_num_errors(object_p);
    fbe_logical_error_injection_object_unlock(object_p);
    fbe_logical_error_injection_inc_num_errors_injected();
    fbe_logical_error_injection_unlock();
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, object_p->object_id,
                                              "LEI: inject fruts rec[%d/0x%llx] err_type: 0x%x pos: %d lba:0x%llx blks:0x%llx op: %d\n",
                                              idx, rec_p->lba, rec_p->err_type, 
                                              fruts_p->position, fruts_p->lba, fruts_p->blocks, fruts_p->opcode);
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, object_p->object_id,
                                              "LEI: inject fruts rec[%d] pos: %d err cnt: %d limit: %d mode: %d\n",
                                              idx, fruts_p->position, rec_p->err_count, rec_p->err_limit, rec_p->err_mode);

    /* We always inject errors into the sectors,
     * even for hard errors.
     */
    {
        fbe_u32_t block_offset;   /* Block offset into fruts sg list */
        fbe_block_count_t modify_blocks = rec_p->blocks;  /* Total blocks in fruts list to modify */
        fbe_sg_element_with_offset_t sg_desc;
        fbe_lba_t table_lba;
        fbe_lba_t seed;
        fbe_sg_element_t *sg_p = NULL;
        table_lba = fbe_logical_error_injection_get_table_lba(siots_p, fruts_p->lba, fruts_p->blocks);
        
        {
            /* Based upon the amount of overlap, we may need to decrease the number
             * of blocks we are modifying.
             */            
            fbe_lba_t end_fruts = table_lba + fruts_p->blocks - 1;   /* End of fruts range. */
            fbe_lba_t end_rec = rec_p->lba + rec_p->blocks - 1;    /* End of record range. */

            /* If record ends before this fruts,
             * decrease range to end at this fruts.
             */
            modify_blocks -= (end_rec > end_fruts) ? (fbe_u32_t)(end_rec - end_fruts) : 0;

            /* If fruts begins after the record start,
             * then decrease blocks to begin at this fruts.
             */
            modify_blocks -= (table_lba > rec_p->lba) ? (fbe_u32_t)(table_lba - rec_p->lba): 0;
            if(LEI_COND((modify_blocks == 0) || (modify_blocks > fruts_p->blocks)) )
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: logical error injection: modify_blocks 0x%llx == 0 Or"
                    "modify_blocks 0x%llx > fruts_p->blocks 0x%llx"
                    "object_id:%d pos:0x%x, Lba:0x%llX Blks:0x%llx \n",
                    __FUNCTION__, (unsigned long long)modify_blocks,
		    (unsigned long long)modify_blocks,
                    (unsigned long long)fruts_p->blocks,
		    object_id, fruts_p->position, 
                    (unsigned long long)fruts_p->lba,
		    (unsigned long long)fruts_p->blocks);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        /* The area being modified begins after the start of the fruts.
         * We have an offset into the area to be modified.
         */
        block_offset = (rec_p->lba <= table_lba) ? 0 : (fbe_u32_t)(rec_p->lba - table_lba);

        /* Init our sg_descriptor to point at head of fruts SG.
         */
        fbe_raid_fruts_get_sg_ptr(fruts_p, &sg_p);
        fbe_sg_element_with_offset_init(&sg_desc,  
                                        block_offset * FBE_BE_BYTES_PER_BLOCK, /* offset */
                                        sg_p,
                                        NULL /* default increment */);
        seed = fruts_p->lba + block_offset;

        /* Initially:
         *  sg_desc.offset is the byte offset into the entire sg "list",
         *    so offset does not respect sg element boundaries.
         *  sg_desc.sg_element is pointing at the head of the sg list.
         *
         * Call macro to sync up sg_element ptr and sg_desc offset.
         */
        fbe_raid_adjust_sg_desc(&sg_desc);

        /* Check whether the injection is to a parity position. Because this 
         * is common throughout modify_block, do it outside of the loop. 
         */
        if (fbe_logical_error_injection_is_table_for_all_types() == FBE_TRUE)
        {
            /* For tables common to all RAID types, pos_bitmap is physical.
             * Check directly. 
             */
            if (!(fbe_raid_geometry_is_mirror_type(fbe_raid_siots_get_raid_geometry(siots_p))) &&
                 (rec_p->pos_bitmap == (1 << fbe_raid_siots_get_row_parity_pos(siots_p)))         )
            {
                parity_pos = FBE_TRUE;
            }
            else if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) && 
                     rec_p->pos_bitmap == (1 << fbe_raid_siots_dp_pos(siots_p)))
            {
                parity_pos = FBE_TRUE;
            }
        }
        else
        {
            /* Table 7 and above are logical positions. Convert first.
             */
            const fbe_u16_t bitmap_phys = 
                fbe_logical_error_injection_convert_bitmap_log2phys(siots_p, rec_p->pos_bitmap);

            if ((rec_p->pos_bitmap & RG_R6_MASK_LOG_ROW) != 0 &&
                bitmap_phys == (1 << fbe_raid_siots_get_row_parity_pos(siots_p)))
            {
                parity_pos = FBE_TRUE;
            }
            else if ((rec_p->pos_bitmap & RG_R6_MASK_LOG_DIAG) != 0 &&
                     bitmap_phys == (1 << fbe_raid_siots_dp_pos(siots_p)))
            {
                parity_pos = FBE_TRUE;
            }
        }

        while (modify_blocks > 0)
        {
            fbe_sector_t * sector_p;

            /* Mark the fruts so that we can tell later an error was injected.
             */
            fbe_raid_fruts_set_flag(fruts_p, FBE_RAID_FRUTS_FLAG_ERROR_INJECTED);

            if(LEI_COND(sg_desc.offset >= sg_desc.sg_element->count) )
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: logical error injection: sg_desc.offset 0x%x >= sg_desc.sg_element->count 0x%x \n",
                    __FUNCTION__, sg_desc.offset, sg_desc.sg_element->count);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Map in the memory.
             */
            sector_p = (fbe_sector_t *) FBE_LOGICAL_ERROR_SGL_MAP_MEMORY(sg_desc.sg_element, 
                                                                         sg_desc.offset, 
                                                                         FBE_BE_BYTES_PER_BLOCK);

            /* Corrupt the sector.
             */
            status = fbe_logical_error_injection_corrupt_sector(sector_p,
                                                                rec_p,
                                                                fruts_p->position,
                                                                seed,
                                                                raid_type,
                                                                b_is_proactive_sparing,
                                                                siots_p->start_lba,
                                                                siots_p->xfer_count,
                                                                width,
                                                                parity_bitmask,
                                                                parity_pos);
            if(LEI_COND(status != FBE_STATUS_OK))
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: logical error injection: Bad status from fbe_logical_error_injection_corrupt_sector"
                    "siots_p %p sector_p %p rec_p %p fruts_p->position 0x%x parity_pos 0x%x seed 0x%llx\n",
                    __FUNCTION__, siots_p, sector_p, rec_p, fruts_p->position,
		    parity_pos, (unsigned long long)seed);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            /* Done with the memory, if we had mapped in a virtual address
             * then unmap it now.
             */
            FBE_LOGICAL_ERROR_SGL_GET_PHYSICAL_ADDRESS(sg_desc.sg_element, phys_addr_local);
            if(phys_addr_local != 0)
            {
                FBE_LOGICAL_ERROR_SGL_UNMAP_MEMORY(sg_desc.sg_element, FBE_BE_BYTES_PER_BLOCK);
            }
            modify_blocks--;
            seed++;

            /* Increment our sg descriptor.
             */
            if(LEI_COND((sg_desc.sg_element->count % FBE_BE_BYTES_PER_BLOCK) != 0) )
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: logical error injection: sg_desc.sg_element->count 0x%x %% FBE_BE_BYTES_PER_BLOCK) != 0\n",
                    __FUNCTION__, sg_desc.sg_element->count);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (modify_blocks)
            {
                if ((sg_desc.offset + FBE_BE_BYTES_PER_BLOCK) == sg_desc.sg_element->count)
                {
                    /* At end of an sg, goto next.
                     */
                    sg_desc.sg_element++,
                        sg_desc.offset = 0;

                    /* After incrementing the SG element ptr
                     * make sure we didn't fall off into the weeds.
                     */
                    if(LEI_COND( (sg_desc.sg_element->count == 0) ||
                                  (sg_desc.sg_element->address == NULL) ) )
                    {
                        fbe_logical_error_injection_service_trace(
                            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: logical error injection: sg_desc.sg_element->count 0x%x == 0 Or"
                            "sg_desc.sg_element->address 0x%p == NULL\n",
                            __FUNCTION__, sg_desc.sg_element->count, sg_desc.sg_element->address);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                else
                {
                    /* Blocks remaining in the current
                     * scatter gather element.
                     */
                    sg_desc.offset += FBE_BE_BYTES_PER_BLOCK;
                }
            } /* end if modify_blocks */
        } /* end while modify_blocks > 0 */
    }
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_logical_error_injection_fruts_inject()
 *************************************************/

/*****************************************************************************
 *          fbe_logical_error_injection_get_lba_adjustment()
 ***************************************************************************** 
 * 
 * @brief   Locate the object that is being injected to and populate the
 *          adjustment value.  Return if the adjustment is non-zero or not.
 * 
 * @param   siots_p - fbe_raid_siots_t pointer
 * @param   lba_adjustment_p - Pointer to adjustment value
 *
 * @return  bool - FBE_TRUE - The adjustment value is non-zero
 *                 FBE_FALSE - The adjustment value is zero (i.e. there is
 *
 * @author
 *  05/03/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_logical_error_injection_get_lba_adjustment(fbe_raid_siots_t* const siots_p,
                                                                 fbe_lba_t *lba_adjustment_p)
{
    fbe_raid_geometry_t                    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t                         object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    fbe_logical_error_injection_object_t   *object_p = NULL;
    fbe_bool_t                              b_enabled = FBE_FALSE;

    /* Initialize the imput value to 0 (i.e. no adjustment)
     */
    *lba_adjustment_p = 0;

    /* Get the object pointer.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);
    if (object_p == NULL)
    {
        /*! @note Since there is no synchronization, there will always be a
         *        window where error injection is disabling but the object
         *        has been removed.  Simply generate a warning.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                                  "%s: obj: 0x%x was not found\n", __FUNCTION__, object_id);
        return FBE_FALSE;
    }

    /* If this object is not enabled we should not be here.
     */
    b_enabled = fbe_logical_error_injection_object_is_enabled(object_p);
    if (b_enabled == FBE_FALSE)
    {
        /*! @note Since there is no synchronization, there will always be a
         *        window where error injection is disabling but the object
         *        has been disabled.  Simply generate a warning.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                                  "%s: obj: 0x%x not enabled\n", __FUNCTION__, object_id);
        return FBE_FALSE;
    }

    /* If the there is an adjustment display some debug information and
     * populate the input and return True.
     */
    if (object_p->injection_lba_adjustment != 0)
    {
        /* Trace some debug info.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                                  "LEI: get adjust: obj: 0x%x siots_p: %p adjust: 0x%llx\n", 
                                                  object_id, siots_p, (unsigned long long)object_p->injection_lba_adjustment);

        /* Populate the input and return True.
         */
        *lba_adjustment_p = object_p->injection_lba_adjustment;
        return FBE_TRUE;
    }

    /* Return False (i.e. adjustement is 0).
     */
    return FBE_FALSE;
}
/**************************************************************************
 * end fbe_logical_error_injection_get_lba_adjustment()
 **************************************************************************/

/**************************************************************************
 * fbe_logical_error_injection_adjust_start_lba()
 **************************************************************************
 * @brief
 * Locate the object that is being injected to return the adjustment value.
 * 
 * @param siots_p - fbe_raid_siots_t pointer
 * @param start_lba_p - Pointer to start lba to adjust
 * @param b_adjust_up - FBE_TRUE - Adjust up, FBE_FALSE - Adjust down
 *
 * @return  status
 *  void
 *
 * @author
 *  05/02/2012  Ron Proulx  - Created.
 *
 *************************************************************************/
void fbe_logical_error_injection_adjust_start_lba(fbe_raid_siots_t* const siots_p,
                                                  fbe_lba_t *start_lba_p,
                                                  fbe_bool_t b_adjust_up)
{
    fbe_lba_t   adjustment_value = 0;

    /* Check if there is a valid adjustment.
     */
    if (fbe_logical_error_injection_get_lba_adjustment(siots_p, &adjustment_value) == FBE_TRUE)
    {
        /* Update the start_lba
         */
        if (b_adjust_up == FBE_TRUE)
        {

            *start_lba_p += adjustment_value;
        }
        else
        {
            *start_lba_p -= adjustment_value;
        }
    }

    return;
}
/**************************************************************************
 * end fbe_logical_error_injection_adjust_start_lba()
 **************************************************************************/

/****************************************************************
 *  fbe_logical_error_injection_get_table_lba()
 ****************************************************************
 * @brief
 *  Get the LBAs normalized to the current table.
 *  In other words, we'll make the input lba fit within our 
 *  current error table by performing a mod with the maximum
 *  lba of the table.
 * 
 * @param siots_p - Pointer to siots (NULL is possible)
 * @param start_lba - Start lba to normalize.
 * @param blocks - Block count.
 *
 * @return
 *  Normalized start lba.
 *
 * @author
 *  08/15/06 - Created. Rob Foley
 *
 ****************************************************************/

fbe_lba_t fbe_logical_error_injection_get_table_lba(fbe_raid_siots_t *siots_p, fbe_lba_t orig_start_lba, fbe_block_count_t blocks)
{
    fbe_lba_t   start_lba = orig_start_lba;
    fbe_lba_t   end_lba;
    fbe_lba_t   table_start_lba;
    fbe_lba_t   max_table_lba;

    /* If the siots is valid, adjust if required.
     */
    if (siots_p != NULL)
    {
        fbe_logical_error_injection_adjust_start_lba(siots_p, &start_lba,
                                                     FBE_FALSE /* Adjust down*/);
    }

    /* Now iniatialize local values.
     */
    end_lba = start_lba + blocks - 1;
    table_start_lba = start_lba;
    fbe_logical_error_injection_get_max_lba(&max_table_lba);
    
    /* If any part of the request is within the table don't adjust.
     */
    if ( end_lba > max_table_lba &&
         start_lba < max_table_lba )
    {
        /* In this spanning case we will not normalize the lbas,
         * since it would cause a discontinuous set of lbas with
         * end < start.
         */
        ;
    }
    else
    {
        /* We're greater than the max, so let's normalize to max_lba.
         */
        table_start_lba %= max_table_lba;
    }
    return table_start_lba;
}
/*************************************************
 * end fbe_logical_error_injection_get_table_lba()
 *************************************************/

/*********************************************************
 * RG_SIOTS_ALLOW_INJECTION
 * Determine if this SIOTS should allow error injection.
 *********************************************************/

/*!***************************************************************************
 *          fbe_logical_error_injection_allow_inject_to_spare()
 *****************************************************************************
 *
 * @brief   Due to the fact that sparing object invalidated a chunk at a time,
 *          when we inject errors during a copy operation we can no longer
 *          inject additional errors to that range.  We use both the parent
 *          siots to determine if we have invalidated the range.
 *
 * @param   siots_p - Pointer to request to check parent siots  
 * @param   fruts_p - Pointer to fruts to determine if we can inject on or not         
 *
 * @return  bool - FBE_TRUE - Allow error injection for this position
 *                 FBE_FALSE - Cannot allow injection for this position
 *
 * @author
 *  05/09/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_logical_error_injection_b_inject_copy_read = FBE_TRUE;
static fbe_bool_t fbe_logical_error_injection_allow_inject_to_spare(fbe_raid_siots_t * const siots_p,
                                                                    fbe_raid_fruts_t* const fruts_p)
{
    fbe_raid_siots_t       *parent_siots_p = NULL;
    fbe_xor_error_t        *eboard_p = NULL;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u16_t               valid_bitmask = 0;
    fbe_u16_t               hard_media_error_bitmask = 0;
    fbe_u16_t               copy_crc_bitmask = 0;
    fbe_u16_t               total_error_bitmask = 0;

    /*! @note If we have already injected do not inject again since the spare object
     *        does not check/retry the checksums and does not go into verify.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_ERROR_INJECTED))
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "LEI: allow spare: skip obj: 0x%x siots_p: %p already flags: 0x%x alg: 0x%x lba: 0x%llx blks: 0x%llx\n", 
                                  fbe_raid_geometry_get_object_id(raid_geometry_p), siots_p, siots_p->flags, 
                                  siots_p->algorithm, siots_p->start_lba, siots_p->xfer_count);
        return FBE_FALSE;
    }

    /* If this is copy operation don't inject on writes.
     */
    if ((siots_p->algorithm == MIRROR_COPY) ||
        (siots_p->algorithm == MIRROR_PACO)    )
    {
        /* If this is a write don't inject.
         */
        if (fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "LEI: allow spare: skip obj: 0x%x siots_p: %p op: %d alg: 0x%x lba: 0x%llx blks: 0x%llx\n", 
                                  fbe_raid_geometry_get_object_id(raid_geometry_p), siots_p, fruts_p->opcode, 
                                  siots_p->algorithm, siots_p->start_lba, siots_p->xfer_count);
            return FBE_FALSE;
        }

        /* Only inject on ever other read for a copy operation.
         */
        if (fbe_logical_error_injection_b_inject_copy_read == FBE_FALSE)
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "LEI: allow spare: skip obj: 0x%x siots_p: %p toggle: %d alg: 0x%x lba: 0x%llx blks: 0x%llx\n", 
                                  fbe_raid_geometry_get_object_id(raid_geometry_p), siots_p, fbe_logical_error_injection_b_inject_copy_read, 
                                  siots_p->algorithm, siots_p->start_lba, siots_p->xfer_count);
            fbe_logical_error_injection_b_inject_copy_read = FBE_TRUE;
            return FBE_FALSE;
        }
        else
        {
            /* Else toggle so that we inject on every other copy operation.
             */
            fbe_logical_error_injection_b_inject_copy_read = FBE_FALSE;
        }
    }

    /* If there is no parent allow the injection.
     */
    parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    if (parent_siots_p == NULL)
    {
        /* Trace and debug and return True.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "LEI: allow spare: inject obj: 0x%x siots_p:%p pos:%d alg: 0x%x lba:0x%llx blks:0x%llx\n", 
                                  fbe_raid_geometry_get_object_id(raid_geometry_p), siots_p, fruts_p->position,
                                  siots_p->algorithm, siots_p->start_lba, siots_p->xfer_count);
        return FBE_TRUE;
    }

    /* If both positions have encountered errors in the original siots we 
     * cannot inject since we will get a validation failure.
     */
    eboard_p = fbe_raid_siots_get_eboard(parent_siots_p);
    valid_bitmask = fbe_logical_error_injection_get_valid_bitmask(parent_siots_p);
    hard_media_error_bitmask = eboard_p->hard_media_err_bitmap;
    total_error_bitmask = fbe_xor_lib_error_get_total_bitmask(eboard_p);
    if (((hard_media_error_bitmask & valid_bitmask) == valid_bitmask) ||
        ((total_error_bitmask & valid_bitmask) == valid_bitmask)         )
    {
        /* Trace a message and skip the injection.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "LEI: allow spare: skip obj: 0x%x siots_p: %p media: 0x%x error: 0x%x alg: 0x%x lba: 0x%llx blks: 0x%llx\n", 
                                  fbe_raid_geometry_get_object_id(raid_geometry_p), siots_p, hard_media_error_bitmask, total_error_bitmask,
                                  siots_p->algorithm, siots_p->start_lba, siots_p->xfer_count);
        return FBE_FALSE;
    }

    /* If the parent has encountered a `crc_copy_bitmap' error we cannot inject
     * since much more than a region has been invalidated.
     */
    copy_crc_bitmask = eboard_p->crc_copy_bitmap;
    if ((copy_crc_bitmask & valid_bitmask) != 0)
    {
        /* Trace a message and skip the injection.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO, 
                                  "LEI: allow spare: skip obj: 0x%x siots_p: %p copy: 0x%x total: 0x%x alg: 0x%x lba: 0x%llx blks: 0x%llx\n", 
                                  fbe_raid_geometry_get_object_id(raid_geometry_p), siots_p, copy_crc_bitmask, total_error_bitmask,
                                  siots_p->algorithm, siots_p->start_lba, siots_p->xfer_count);
        return FBE_FALSE;
    }

    /* If we are here we will allow the injection.
     */
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                              "LEI: allow spare: inject obj: 0x%x siots_p:%p pos: %d alg: 0x%x lba:0x%llx blks:0x%llx\n", 
                              fbe_raid_geometry_get_object_id(raid_geometry_p), siots_p, fruts_p->position, 
                              siots_p->algorithm, siots_p->start_lba, siots_p->xfer_count);

    /* Allow the injection.
     */
    return FBE_TRUE;
}
/*****************************************************************
 * end fbe_logical_error_injection_allow_inject_to_spare()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_logical_error_injection_allow_inject_check_downstream()
 *****************************************************************************
 *
 * @brief   Based on the error table `correctiveness' etc, determine if one or
 *          more downstream objects has generated any of the following:
 *              o Downstream object invalidated a range of blocks:
 *                  + We cannot inject to other positions if it will break the
 *                    `correctiveness' of the table.
 *              o Downstream object injected on a overlapping range:
 *                  + We cannot inject to other positions if it will break the
 *                    `correctiveness' of the table.
 *
 * @param   siots_p - Pointer to request to check parent siots  
 * @param   fruts_p - Pointer to fruts to determine if we can inject on or not         
 *
 * @return  bool - FBE_TRUE - Allow error injection for this position
 *                 FBE_FALSE - Cannot allow injection for this position
 *
 * @author
 *  05/08/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_logical_error_injection_allow_inject_check_downstream(fbe_raid_siots_t * const siots_p,
                                                                            fbe_raid_fruts_t* const fruts_p)
{
    fbe_raid_siots_t                       *parent_siots_p = NULL;
    fbe_raid_geometry_t                    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_object_id_t                         object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    fbe_logical_error_injection_object_t   *object_p;
    fbe_bool_t                              b_correctable_table = FBE_TRUE;
    fbe_u16_t                               object_enabled_bitmask = 0;
    fbe_u16_t                               downstream_enabled_bitmask = 0;
    fbe_u32_t                               dead_pos_cnt = 0;
    fbe_u32_t                               max_dead_pos_cnt = 0;
    fbe_u32_t                               downstream_enabled_cnt = 0;
    fbe_xor_error_t                        *eboard_p = NULL;
    fbe_u16_t                               total_error_bitmask = 0;

    /* If there is no parent siots then we will allow injection.
     */
    parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    if (parent_siots_p == NULL)
    {
        return FBE_TRUE;
    }

    /* First get the object assocaited with this request.
     * If there is no object associated with this request something is wrong.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);
    if (object_p == NULL)
    {
        /* Print an error message and return False.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: check downstream: siots_p: %p start: 0x%llx blks: 0x%llx object not found.\n",
                    siots_p, (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count);
        return FBE_FALSE;
    }

    /* If all the positions are enabled then we are not injecting downstream.
     */
    object_enabled_bitmask = fbe_logical_error_injection_get_valid_bitmask(siots_p);
    downstream_enabled_bitmask = (fbe_u16_t)object_p->edge_hook_bitmask;
    if ((downstream_enabled_bitmask == FBE_U16_MAX)            ||
        (downstream_enabled_bitmask == object_enabled_bitmask)    )
    {
        /*  Allow injection (typical case).
         */
        return FBE_TRUE;
    }

    /*! @note If the table allows uncorrectable errors then we allow the injection.
     */
    b_correctable_table = fbe_logical_error_injection_is_table_correctable();
    if (b_correctable_table == FBE_FALSE)
    {
        /* Allow the injection.
         */
        return FBE_TRUE;
    }

    /*! @note If the table is correctable and this is a verify (i.e. has a parent)
     *        the downstream object may return an error status
     */
    downstream_enabled_bitmask = ~downstream_enabled_bitmask & object_enabled_bitmask;
    downstream_enabled_cnt = fbe_logical_error_injection_get_bit_count(downstream_enabled_bitmask);
    fbe_raid_geometry_get_max_dead_positions(raid_geometry_p, &max_dead_pos_cnt);
    dead_pos_cnt = fbe_raid_siots_dead_pos_count(siots_p);
    if ((downstream_enabled_cnt + dead_pos_cnt) >= max_dead_pos_cnt)
    {
        /* Trace and message and don't allow injection.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: check downstream: skip siots_p: %p start: 0x%llx blks: 0x%llx pos: %d down: %d dead: %d max: %d\n",
                    siots_p, (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count, fruts_p->position,
                    downstream_enabled_cnt, dead_pos_cnt, max_dead_pos_cnt);
        return FBE_FALSE;
    }

    /* Trace a debug message if we made it here.
     */
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: check downstream: siots_p: %p start: 0x%llx blks: 0x%llx pos: %d down: %d dead: %d max: %d\n",
                    siots_p, (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count, fruts_p->position,
                    downstream_enabled_cnt, dead_pos_cnt, max_dead_pos_cnt);

    /*! @note If we have already injection we cannot allow another.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_ERROR_INJECTED))
    {
        /* Trace and message and don't allow injection.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: check downstream: siots_p: %p start: 0x%llx blks: 0x%llx pos: %d already injected.\n",
                    siots_p, (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count, fruts_p->position);
        return FBE_FALSE;
    }

    /* If the original request detected errors to any disabled position 
     * and the table is correctable we cannot allow injection to any other
     * position.
     */
    eboard_p = fbe_raid_siots_get_eboard(parent_siots_p);
    total_error_bitmask = fbe_xor_lib_error_get_total_bitmask(eboard_p);
    if ((downstream_enabled_bitmask & total_error_bitmask) != 0)
    {
        /* Print a trace and don't allow error injection.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                    "LEI: check downstream: siots_p: %p start: 0x%llx blks: 0x%llx pos: %d error mask: 0x%x overlap.\n",
                    siots_p, (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count, fruts_p->position, total_error_bitmask);
        return FBE_FALSE;
    }

    /* If we are here allow the injection.
     */
    return FBE_TRUE;
}
/*****************************************************************
 * end fbe_logical_error_injection_allow_inject_check_downstream()
 *****************************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_allow_siots_injection()
 ****************************************************************
 * @brief
 *  Determine if we can inject an error for this siots.
 *
 * @param  -               
 *
 * @return -   
 *
 * @author
 *  1/4/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_bool_t fbe_logical_error_injection_allow_siots_injection(fbe_raid_siots_t * const siots_p,
                                                                    fbe_raid_fruts_t* const fruts_p,
                                                                    fbe_bool_t b_chain)
{
    /* By default we enable injection.
     * below we may decide not to based on algorithm or group type.
     */
    fbe_bool_t              b_allow_injection = FBE_TRUE;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_group_type_t   raid_type;

    /* Disable error injection for striped mirrors since error injection
     * must be performed at the mirror level.
     */
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Some group types should never inject errors.
         * Striped mirrors, since the mirror below injects errors.
         */
        b_allow_injection = FBE_FALSE;
    }
    else if (fbe_raid_geometry_is_sparing_type(raid_geometry_p) == FBE_TRUE)
    {
        /* For spare raid groups don't allow injection to degraded I/O unless
         * it is a copy operation.
         */
        b_allow_injection = fbe_logical_error_injection_allow_inject_to_spare(siots_p, fruts_p);
    }
    else
    {

        /* Check whether the maximum range of the fruts chain spans the 
         * error table max_lba. If it does, we disable error injection. 
         */
        b_allow_injection = !fbe_logical_error_injection_fruts_span_err_tbl_max_lba(fruts_p, b_chain);

        /* Check if there are error that are inject from below.
         */
        if (b_allow_injection == FBE_TRUE)
        {
            /* Check if we should skip the injection because we have injected
             * an error on the downstream object (and it would break the
             * `correctiveness').
             */
            b_allow_injection = fbe_logical_error_injection_allow_inject_check_downstream(siots_p, fruts_p);
        }
    }

    return b_allow_injection;
}
/*********************************************************
 * end fbe_logical_error_injection_allow_siots_injection()
 *********************************************************/

/****************************************************************
 *  fbe_logical_error_injection_fruts_inject_all()
 ****************************************************************
 *  @brief
 *    Search through all error records for overlap with the
 *    input fruts.  If overlap is found, then
 *    the fruts is corrupted with the errors for the given record.
 *
 * @param fruts_p - ptr to fruts to look for range overlap with.
 *
 *  @return
 *    fbe_status_t
 *
 *  @author
 *    02/14/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_fruts_inject_all( fbe_raid_fruts_t * const fruts_p )
{
    fbe_u32_t err_index;
    fbe_bool_t    all_raid_types;  /* FBE_TRUE if a table is for all RAID types. */
    fbe_raid_siots_t* const siots_p  = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t b_enabled;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_object_id_t object_id;

    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    block_operation_p = fbe_raid_fruts_to_block_operation(fruts_p);


    if (block_operation_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* If our status is not good already then we should not be injecting errors on 
         * this request, since in some cases we set the status to good if we are injecting 
         * soft media errors. 
         */
        return status;
    }

    if ( fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ &&
         //fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_REMAP &&
         //fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_BLOCKS &&
         fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY &&
         fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY &&
         fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY &&
         fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
    {
        /* We do not want to insert errors on this.
         */
        return status;
    }

    /* Return if Rderr is not enabled or we are instructed 
     * not to inject error. 
     */
    fbe_logical_error_injection_get_enable(&b_enabled);
    if (b_enabled == FBE_FALSE)
    {
        return status;
    }

    /* Only inject errors if it is allowed.
     */
    if (fbe_logical_error_injection_allow_siots_injection(siots_p, fruts_p, FBE_TRUE) == FBE_FALSE)
    {
        return status;
    }    

    /* Check the table types. 
     */
    all_raid_types = fbe_logical_error_injection_is_table_for_all_types();
    
    /* Loop through all our error records 
     * searching for a match.
     */
    for (err_index = 0; err_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; err_index++)
    {
        /* If this fruts` position matches our record's position(s)
         * and this is the right opcode, then inject an error.
         * Mirrors will ignore the position bitmap and insert an error
         * based on the table entry. Odd entries will be position one,
         * and even will be zero.
         */
        fbe_logical_error_injection_record_t* rec_p = NULL;
        fbe_bool_t is_match = FBE_FALSE;
        fbe_lba_t record_end_lba;
        fbe_lba_t table_lba;
        fbe_bool_t fruts_metadata_lba_b;
        fbe_bool_t err_rec_metadata_lba_b;

        fbe_logical_error_injection_get_error_info(&rec_p, err_index);

        /* We have inject this error before */
        if (rec_p->err_type == FBE_XOR_ERR_TYPE_SILENT_DROP) {
            continue;
        }

        /* Check if the record is valid for the opcode
         */
        if (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
        {
            if (rec_p->err_type != FBE_XOR_ERR_TYPE_INCOMPLETE_WRITE)
            {
                continue;
            }
        }

        /* If the opcode was specified and it doesn't match skip the injection.
         */
        if (rec_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            if (fruts_p->opcode != rec_p->opcode)
            {
                continue;
            }
        }
        object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

        if ((rec_p->object_id != FBE_OBJECT_ID_INVALID) &&
            (object_id != rec_p->object_id)) {
            is_match = FBE_FALSE;
        }
        else if (all_raid_types == FBE_TRUE)
        {
            /* Handle error tables that are common to all RAID types. 
             */
            is_match = fbe_logical_error_injection_rec_pos_match_all_types(fruts_p, rec_p);
        }
        else if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
        {
            /* Handle error tables that are specific to RAID 6. 
             */
            is_match = fbe_logical_error_injection_rec_pos_match_r6(fruts_p, rec_p);
        }

        /* Skip this error record if no match is found.
         */
        if (!is_match)
        {
            continue;
        }
       
        /* Coherency errors should only be injected to VERIFY process. 
         * crc_det is only meaningful to these error types. 
         */
        if (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY && 
            opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY && 
            rec_p->crc_det == 0              &&
            (rec_p->err_type == FBE_XOR_ERR_TYPE_1NS || 
             rec_p->err_type == FBE_XOR_ERR_TYPE_1S  ||
             rec_p->err_type == FBE_XOR_ERR_TYPE_1R  ||
             rec_p->err_type == FBE_XOR_ERR_TYPE_1D  ||
             rec_p->err_type == FBE_XOR_ERR_TYPE_COH   ) )
        {
            continue;
        }

        /* Try to detect an overlap in the record's range,
         * and the current fruts' range.
         */

        /* Determine if fruts is for a metadata I/O.
         */
        fruts_metadata_lba_b = fbe_logical_error_injection_is_lba_metadata_lba(block_operation_p->lba,
                                                                               block_operation_p->block_count, 
                                                                               raid_geometry_p);

        if (fruts_metadata_lba_b)
        {
            /* Determine if error record is also for a metadata I/O.
             */
            err_rec_metadata_lba_b = fbe_logical_error_injection_is_lba_metadata_lba(rec_p->lba,
                                                                                     rec_p->blocks, 
                                                                                     raid_geometry_p);
    
            /* If both are for metadata I/Os, log a message indicating we are working in the metadata region.
             */
            if (err_rec_metadata_lba_b)
            {
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                                          "LEI: lba is metadata lba"
                                                          "fruts_p %p lba 0x%llx status: 0x%x\n",
                                                          fruts_p,
							  (unsigned long long)block_operation_p->lba,
							  status);
            }
            else
            {
                /* If fruts is for a metadata I/O, but this error record is not for metadata I/O,
                 * continue to the next record.  We do not want to check for overlap because the
                 * table lba could be normalized to the max lba in the table.  This could allow
                 * us to unexpectedly proceed with error injection for this metadata I/O.
                 */
                continue;
            }
        }
        
        /* Get the normalized table start lba. 
         */
        table_lba = fbe_logical_error_injection_get_table_lba(siots_p, fruts_p->lba, fruts_p->blocks );

        /* Get the end lba.
         */
        record_end_lba = rec_p->lba;   
        
        /* Check for overlap.
         */     
        if (!fbe_logical_error_injection_overlap(table_lba, fruts_p->blocks, record_end_lba, rec_p->blocks)) 
        {
            continue;
        }

        /* found a region to inject errors into.
         */
        switch (rec_p->err_mode)
        {
            case FBE_LOGICAL_ERROR_INJECTION_MODE_TRANS_RND:
                /* Randomize between transitory and always.
                 */
                if (rec_p->err_limit == 0)
                {
                    rec_p->err_limit++;
                }

                /* Most of the errors in the error table have the err_limit set to
                 * 0x2 so that 50% of the time we inject a transitory error, and
                 * 50% of the time the error is injected as always.
                 */
                if ((fbe_random() % rec_p->err_limit) != 0)
                {
                    /* Random insert of error.
                     */
                    rec_p->err_count++;
                    status = fbe_logical_error_injection_fruts_inject(fruts_p, err_index, rec_p);
                    break;
                }

                /* Fall through to Transitory.
                 */

            case FBE_LOGICAL_ERROR_INJECTION_MODE_TRANS:
                /* Do not inject error if it is being retried.
                 * Some verifies use single region mode retries, need to detect that too.
                 */
                if (   fbe_raid_common_is_flag_set(&fruts_p->common, FBE_RAID_COMMON_FLAG_REQ_RETRIED)
                    || fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
                {
                     /* This is a retry, we should not inject any errors
                      * as we want it to succeed.
                      */
                    break;
                }
                else
                { 
                    status = fbe_logical_error_injection_fruts_inject( fruts_p, err_index, rec_p);
                    rec_p->err_count++;
                }
                break;
                                
            case FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP:
                /* While in skip mode mode only inject an error after we
                 * have reached the skip limit.
                 */
                if (rec_p->skip_count++ >= 
                    rec_p->skip_limit)
                {
                    rec_p->skip_count = 0;
                    rec_p->err_mode = FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT;
                }
                break;
            case FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT:
                /* Insert an error and then wait until we reach the skip count
                 * to inject again.
                 */
                rec_p->err_count++;
                status = fbe_logical_error_injection_fruts_inject(fruts_p, err_index, rec_p);
                if (rec_p->skip_count++ >= 
                    rec_p->skip_limit)
                {
                    /* Hit limit, so reset count and inject next time.
                     */
                    rec_p->skip_count = 0;
                    rec_p->err_mode = FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP;
                }
                break;

            case FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT:
                if (rec_p->err_count >= 
                    rec_p->err_limit)
                {
                    /* Already hit limit, don't inject.
                     */
                    break;
                }
                else
                {
                    /* errors remaining for this record.
                     */
                    rec_p->err_count++;
                    status = fbe_logical_error_injection_fruts_inject(fruts_p, err_index, rec_p);
                }
                break;

            case FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED:
            case FBE_LOGICAL_ERROR_INJECTION_MODE_ALWAYS:
                /* Counting of errors is turned off.
                 */
                rec_p->err_count++;
                status = fbe_logical_error_injection_fruts_inject(fruts_p, err_index, rec_p);
                break;

            case FBE_LOGICAL_ERROR_INJECTION_MODE_RANDOM:
                /* Inject errors on a random basis.
                 * We use the err_limit field as the
                 * mod for the random number.
                 */
                {
                    if (rec_p->err_limit == 0)
                    {
                        rec_p->err_limit++;
                    }
                    if ((fbe_random() % rec_p->err_limit) == 0)
                    {
                        /* Random insert of error.
                         */
                        rec_p->err_count++;
                        status = fbe_logical_error_injection_fruts_inject(fruts_p, err_index, rec_p);
                    }
                }
                break;
            default:
                break;
        } /* switch() */
        if(LEI_COND(status != FBE_STATUS_OK))
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                      "LEI: Bad status from fbe_logical_error_injection_fruts_inject"
                                                      "rec_p %p fruts_p %p status: 0x%x\n",
                                                      rec_p, fruts_p, status);
        }
    } /* for() */
    return status;
}
/*****************************************
 * end fbe_logical_error_injection_fruts_inject_all()
 *****************************************/


/****************************************************************
 *  fbe_logical_error_injection_is_lba_metadata_lba()
 ****************************************************************
 *  @brief
 *    Determine whether the lba to inject error is a metadata lba
 *    or not
 * 
 *  @param  start_lba - starting lba 
 *  @param  block_count - number of blocks in I/O request
 *  @param  raid_geometry_p - pointer to the geometry for the raid group
 *
 *  @return
 *    BOOL 
 *
 *  @author
 *    06/21/10 - Created. Ashwin
 *
 ****************************************************************/

static fbe_bool_t fbe_logical_error_injection_is_lba_metadata_lba(
                                                   fbe_lba_t start_lba,
                                                   fbe_block_count_t block_count,
                                                   fbe_raid_geometry_t *raid_geometry_p)
{
    fbe_lba_t                      actual_lba;
    fbe_lba_t                      metadata_start_lba; 
    fbe_u32_t                      width;
    fbe_u16_t                      data_disks;
    fbe_raid_group_type_t          raid_type;

    /* Find out whether the lba is a metadata start lba*/
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_start_lba);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    fbe_raid_type_get_data_disks(raid_type, width, &data_disks);
    actual_lba = (start_lba + block_count) * data_disks;
    if(actual_lba >= metadata_start_lba)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;

}/*****************************************
 * end fbe_logical_error_injection_is_lba_metadata_lba()
 *****************************************/


/****************************************************************
 *  fbe_logical_error_injection_get_bad_lba()
 ****************************************************************
 *  @brief
 *    Determine the lba within this fruts where we are allowed
 *    to inject a HARD MEDIA error.
 *

 *    request_id - this is a fruts ptr.
 *    lba_p - The bad lba to return.
 *    blocks_p - The range of blocks in this I/O that are bad.
 *
 *  @return
 *    fbe_status_t
 *
 *  @author
 *    09/20/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_get_bad_lba( fbe_u64_t request_id,
                                              fbe_lba_t *const lba_p,
                                              fbe_block_count_t *const blocks_p )
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           err_index;
    fbe_raid_fruts_t   *fruts_p = (fbe_raid_fruts_t*)(csx_ptrhld_t)request_id;
    fbe_raid_siots_t   *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_lba_t           start_lba;
    fbe_lba_t           lba_to_inject;
    fbe_lba_t           adjustment_value = 0;
    fbe_block_count_t   blocks = fruts_p->blocks;
    fbe_logical_error_injection_record_t* rec_p = NULL;
    fbe_lba_t           max_table_lba;

    /* Initialize locals.
     */
    fbe_logical_error_injection_get_max_lba(&max_table_lba);
    fbe_logical_error_injection_get_lba_adjustment(siots_p, &adjustment_value);

    /* Validate the adjustment value.
     */
    if (fruts_p->lba < adjustment_value)
    {
        /* Something is wrong.  Trace and error and return error.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                                  "lei: get bad lba: siots_p: %p fruts lba: 0x%llx is less than adjust: 0x%llx \n",
                                                  siots_p, (unsigned long long)fruts_p->lba, (unsigned long long)adjustment_value);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    start_lba = fruts_p->lba - adjustment_value;
    lba_to_inject = start_lba; 

    /* Loop through all our error records 
     * searching for a match.
     */
    for (err_index = 0; err_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; err_index++)
    {
        fbe_logical_error_injection_get_error_info(&rec_p, err_index);

        /* If this fruts` position matches our record's position(s)
         * and this is the right opcode, then inject an error.
         */
        if ((rec_p->pos_bitmap & (1 << fruts_p->position))             &&
            ((rec_p->err_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR) ||
             (rec_p->err_type == FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR)))
        {
            /* Try to detect an overlap in the record's range,
             * and the current fruts' range.
             */
            fbe_lba_t table_lba = fbe_logical_error_injection_get_table_lba(siots_p, fruts_p->lba, fruts_p->blocks );
            if (fbe_logical_error_injection_overlap( table_lba,
                                                     fruts_p->blocks,
                                                     rec_p->lba,
                                                     rec_p->blocks )  )
            {
                /* found a region to inject errors into.
                 */
                if ( table_lba > rec_p->lba )
                {
                    /* Just inject at the first lba of this transfer.
                     * Since we have a match, and error_address starts before
                     * us, we are guaranteed that lba is within the error range.
                     */
                    lba_to_inject = start_lba;

                    /* The error ends at the minimum of either the
                     * total blocks in the transfer or the end of the injection. 
                     */
                    blocks = ((rec_p->lba + rec_p->blocks) - table_lba);
                    blocks = FBE_MIN(blocks, fruts_p->blocks);
                }
                else
                {
                    /* In this case the error address is somewhere after the
                     * start of the transfer, so just inject at the
                     * first error address.
                     * Start with the beginning of the error table entry.
                     */
                    fbe_lba_t error_address = rec_p->lba;

                    /* Next get the offset of this lba beyond the normal table range.
                     */
                    error_address += (start_lba / max_table_lba) * max_table_lba;

                    /* Make sure the error address is within the range of the transfer.
                     */
                    if(LEI_COND(  error_address < start_lba || 
                                   error_address > start_lba + fruts_p->blocks - 1 ) )
                    {
                        fbe_logical_error_injection_service_trace(
                            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: logical error injection: error_address 0x%llx < fruts_p->lba 0x%llx Or"
                            "error_address  > fruts_p->lba + fruts_p->blocks 0x%llx - 1\n",
                            __FUNCTION__, error_address, start_lba, fruts_p->blocks );
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                    
                    lba_to_inject = error_address;
                    
                    /* The error ends at the minimum of either the
                     * total blocks in the transfer or the end of the injection. 
                     */
                    if (error_address + rec_p->blocks >=
                        start_lba + fruts_p->blocks)
                    {
                        /* We should have the number of blocks from the start of 
                         * injection until the end of the fruts. 
                         */
                        blocks = (start_lba + fruts_p->blocks) - lba_to_inject;
                    }
                    else
                    {
                        /* Otherwise injection ends at the end of the error record.
                         */
                        blocks = (error_address + rec_p->blocks) - lba_to_inject;
                    }
                } /* end else error is after the transfer.*/

                /* Save the return values.
                 */
                lba_to_inject += adjustment_value;
                *lba_p = lba_to_inject;
                if (blocks_p != NULL)
                {
                    *blocks_p = blocks;
                }
                return status;
            } /* found overlap between this fruts and this error record. */

        }/* Match found */

    } /* For all error records. */

    /* Else just use the fruts start lba.
     */
    lba_to_inject += adjustment_value;
    *lba_p = lba_to_inject;
    if (blocks_p != NULL)
    {
        *blocks_p = blocks;
    }
    return status;
}
/**********************************************
 * end fbe_logical_error_injection_get_bad_lba()
 **********************************************/

/****************************************************************
 * fbe_logical_error_injection_is_table_for_all_types()
 ****************************************************************
 * @brief
 *  This function checks whether a given table is for all RAID types.
 *
 * @param None.
 *
 * @return
 *  FBE_TRUE:  The given table is     for all RAID types.
 *  FBE_FALSE: The given table is not for all RAID types.
 *
 * @author
 *  5/31/06 - Created. SL
 *
 ****************************************************************/
fbe_bool_t fbe_logical_error_injection_is_table_for_all_types(void)
{
    fbe_logical_error_injection_table_flags_t table_flags;
    fbe_logical_error_injection_get_table_flags(&table_flags);
    return ((table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_ALL_RAID_TYPES) != 0);
}
/******************************
 * end fbe_logical_error_injection_is_table_for_all_types()
*****************************/

/****************************************************************
 * fbe_logical_error_injection_is_table_for_r6_only()
 ****************************************************************
 * @brief
 *  This function checks whether a given table is for RAID 6 only.
 *
 * @param None.
 *
 * @return
 *  FBE_TRUE:  The given table is     for RAID 6 only.
 *  FBE_FALSE: The given table is not for RAID 6 only.
 *
 * @author
 *  5/31/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_is_table_for_r6_only(void)
{
    fbe_logical_error_injection_table_flags_t table_flags;
    fbe_logical_error_injection_get_table_flags(&table_flags);
    return ((table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_RAID6_ONLY) != 0);
}
/******************************
 * end fbe_logical_error_injection_is_table_for_r6_only()
*****************************/

/****************************************************************
 * fbe_logical_error_injection_setup_err_adj()
 ****************************************************************
 * @brief
 *  This function sets up all err_adj for the currect error table. 
 *
 * @param None.
 *
 * @return
 *  None.
 *
 * @author
 *  5/31/06 - Created. SL
 *
 ****************************************************************/
static void fbe_logical_error_injection_setup_err_adj(void)
{
    fbe_s32_t idx_first = 0;

    /* Loop over the error table to set up all err_adj.
     */
    while (idx_first < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS)
    {
        fbe_logical_error_injection_record_t *rec_p = NULL;
        fbe_u16_t err_adj;
        fbe_s32_t idx_last = idx_first;
        fbe_s32_t idx;

        fbe_logical_error_injection_get_error_info(&rec_p, idx_first);
        err_adj = rec_p->pos_bitmap;
        /* FBE_XOR_ERR_TYPE_NONE marks the end of the error table. 
         */
        if (rec_p->err_type == FBE_XOR_ERR_TYPE_NONE)
        {
            break;
        }

        /* Loop over error records from idx_first + 1 to MAX_ERR_SIM_RECS
         * searching for matching error records. The resulted range of 
         * marching error records is [idx_first, idx_last]. 
         */
        for (idx = idx_first + 1; idx < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; ++idx)
        {
            fbe_logical_error_injection_record_t* rec_tmp_p = NULL;

            fbe_logical_error_injection_get_error_info(&rec_tmp_p, idx);

            if ((rec_tmp_p->err_type != FBE_XOR_ERR_TYPE_NONE) && 
                (rec_p->lba == rec_tmp_p->lba) &&
                (rec_p->blocks == rec_tmp_p->blocks))
            {
                /* Find a match. Save its pos_bitmap and move the last index.
                 */
                err_adj |= rec_tmp_p->pos_bitmap;
                idx_last = idx;
            }
            else
            {
                /* Stop when find the first error record that does not match.
                 */
                break;
            }
        } /* end of for (idx...) */

        /* Error records ranging from idx_first to idx_last overlap.
         * Set up their err_adj, which is the OR of all pos_bitmap.
         */
        for (idx = idx_first; idx <= idx_last; ++idx)
        {
            fbe_logical_error_injection_record_t* rec_tmp_p = NULL;
            fbe_logical_error_injection_get_error_info(&rec_tmp_p, idx);
            rec_tmp_p->err_adj = err_adj;
        }

        /* Increment the index to the beginning of the next record.
         */
        idx_first = idx_last + 1;
    } /* end of while (idx_first) */

    return;
}
/******************************
 * end fbe_logical_error_injection_setup_err_adj()
*****************************/

/****************************************************************
 * fbe_logical_error_injection_table_validate()
 ****************************************************************
 * @brief
 *  This function validates a given error table. The function works 
 *  whether the table has been randomized by fbe_logical_error_injection_table_randomize()
 *  or not. All parameters are checked as thoroughly as possible. 
 *
 * @param None. 
 *
 * @return
 *  FBE_STATUS_OK:  The error table is   valid.
 *  FBE_STATUS_GENERIC_FAILURE: The error table is invalid.
 *
 * @author
 *  4/7/06 - Created. SL
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_table_validate(void)
{
    const fbe_bool_t all_raid_types = fbe_logical_error_injection_is_table_for_all_types();
    fbe_status_t status = FBE_STATUS_OK;
    fbe_s32_t idx;
    fbe_lba_t max_lba = 0;
    fbe_lba_t max_table_lba;
    fbe_lba_t round_blocks = FBE_RAID_DEFAULT_CHUNK_SIZE;
    fbe_logical_error_injection_get_max_lba(&max_table_lba);
    
    /* Validate the flags of this error table.
     */
    if (fbe_logical_error_injection_table_validate_flags() == FBE_FALSE)
    {
        /* The table flags are invalid. 
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    /* Loop over all records of the error table.
     */
    for (idx = 0; idx < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; ++idx)
    {
        fbe_logical_error_injection_record_t *rec_p = NULL;

        fbe_logical_error_injection_get_error_info(&rec_p, idx);
        /* FBE_XOR_ERR_TYPE_NONE marks the end of the error table. 
         */
        if (rec_p->err_type == FBE_XOR_ERR_TYPE_NONE)
        {
            break;
        }

        if (all_raid_types)
        {
            /* Validate tables that are common to all RAID types. 
             */
            if (fbe_logical_error_injection_table_validate_all(rec_p) == FBE_FALSE)
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
        }
        else
        {
            /* Validate tables that are specific to RAID 6.
             */
            if (fbe_logical_error_injection_table_validate_r6(rec_p) == FBE_FALSE)
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
        }
        if (rec_p->lba + rec_p->blocks > max_lba)
        {
            /* Yes, we intentionally leave a gap in between the new and old table.
             */
            max_lba = rec_p->lba + rec_p->blocks;
        }
    } /* for (idx...) */
    
    /* 1) We leave a gap of at least 100 blocks, so that one table is not 
     *    right against the next wrap around table. 
     * 2) For ATA drives, DH driver extends the error invalidation to 
     *    FBE_LOGICAL_ERROR_INJECTION_RECORD_ROUND_BLOCKS block section. To make the wrap around 
     *    handling easier, max_lba must be a modulo of FBE_LOGICAL_ERROR_INJECTION_RECORD_ROUND_BLOCKS.
     */
    max_table_lba = max_lba + 100;

    if((max_table_lba % round_blocks) != 0)
    {
        max_table_lba += round_blocks - (max_table_lba % round_blocks);
    }
    fbe_logical_error_injection_set_max_lba(max_table_lba);
    return status;
}
/******************************
 * end fbe_logical_error_injection_table_validate()
*****************************/

/****************************************************************
 * fbe_logical_error_injection_table_validate_flags()
 ****************************************************************
 * @brief
 *  This function validates the table flags of an error table. 
 *
 * @param None.
 *
 * @return
 *  FBE_TRUE:  The flags are   valid.
 *  FBE_FALSE: The flags are invalid.
 *
 * @author
 *  5/31/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_table_validate_flags(void)
{
    fbe_bool_t is_valid = FBE_TRUE;
    fbe_logical_error_injection_table_flags_t table_flags;
    fbe_logical_error_injection_get_table_flags(&table_flags);

    /* The table must be either correctable or uncorrectable.
     */
    if ((table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_CORRECTABLE_TABLE)   != 0 &&
        (table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_UNCORRECTABLE_TABLE) == 0    )
    {
        /* Correctable errors.
         */
    }
    else if ((table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_UNCORRECTABLE_TABLE) != 0 &&
             (table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_CORRECTABLE_TABLE)   == 0    )
    {
        /* UNCorrectable errors.
         */
    }
    else
    {
        /* The table_flags are invalid. 
         */
        is_valid = FBE_FALSE;
    }

    /* Error table must be for either all RAID types, or RAID 6 only.
     */
    if ((table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_ALL_RAID_TYPES) != 0 && 
        (table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_RAID6_ONLY)     == 0)
    {
        /* This error table is for all RAID types.
         */
    }
    else if ((table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_ALL_RAID_TYPES) == 0 && 
             (table_flags & FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_RAID6_ONLY)     != 0)
    {
        /* This error table is for RAID 6 only.
         */
    }
    else
    {
        /* The table_flags are invalid. 
         */
        is_valid = FBE_FALSE;
    }

    if (is_valid == FBE_FALSE)
    {
        /* Print out the invalid error record and the reason for debugging.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s flags 0x%x invalid \n", 
                                                  __FUNCTION__, table_flags);
    }

    return is_valid;
}
/******************************
 * end fbe_logical_error_injection_table_validate_flags()
*****************************/

/****************************************************************
 * fbe_logical_error_injection_table_validate_all()
 ****************************************************************
 * @brief
 *  This function validates a given error record. The function works 
 *  whether the record has been randomized by fbe_logical_error_injection_table_randomize()
 *  or not. All parameters are checked as thoroughly as possible. 
 *
 * @param rec_p - pointer to an error record
 *
 * @return
 *  FBE_TRUE:  The error record is   valid.
 *  FBE_FALSE: The error record is invalid.
 *
 * @author
 *  5/31/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_table_validate_all(const fbe_logical_error_injection_record_t* const rec_p)
{
    const fbe_u16_t pos_bitmap = rec_p->pos_bitmap;
    const fbe_u16_t err_adj    = rec_p->err_adj;
    fbe_bool_t is_valid;

    if (pos_bitmap == RG_R6_ERR_PARM_RND && 
        err_adj    == RG_R6_ERR_PARM_RND)
    {
        is_valid = FBE_TRUE;
    }
    else if (pos_bitmap == RG_R6_ERR_PARM_UNUSED || 
             err_adj    == RG_R6_ERR_PARM_UNUSED)
    {
        is_valid = FBE_FALSE;
    }
    else if ((pos_bitmap &  err_adj) != 0 && 
             (pos_bitmap & ~err_adj) == 0)
    {
        is_valid = FBE_TRUE;
    }
    else
    {
        is_valid = FBE_FALSE;
    }

    if (is_valid == FBE_FALSE)
    {
        /* Print out the invalid error record and the reason for debugging.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s record invalid.  pos_bitmap: 0x%x err_adj: 0x%x", 
                                                  __FUNCTION__, pos_bitmap, err_adj);
        fbe_logical_error_injection_print_record(rec_p);
    }

    return is_valid;
}
/******************************
 * end fbe_logical_error_injection_table_validate_all()
*****************************/

/****************************************************************
 * fbe_logical_error_injection_table_validate_r6()
 ****************************************************************
 * @brief
 *  This function validates a given error table. The function works 
 *  whether the table has been randomized by fbe_logical_error_injection_table_randomize()
 *  or not. All parameters are checked as thoroughly as possible. 
 *
 * @param rec_p - pointer to an error record
 *
 * @return
 *  FBE_TRUE:  The error table is   valid.
 *  FBE_FALSE: The error table is invalid.
 *
 * @author
 *  5/31/06 - Created. SL
 *
 ****************************************************************/
fbe_bool_t fbe_logical_error_injection_table_validate_r6(const fbe_logical_error_injection_record_t* const rec_p)
{
    fbe_bool_t b_coh;             /* Whether coherency error can be injected.  */
    fbe_bool_t b_pos;             /* Whether pos_bitmap and err_adj are valid. */
    fbe_bool_t b_width;           /* Whether width                   is valid. */
    fbe_bool_t b_start;           /* Whether start_bit               is valid. */
    fbe_bool_t b_num;             /* Whether num_bits                is valid. */
    fbe_bool_t b_both;            /* Whether start_bit and num_bits are valid. */
    fbe_bool_t b_adj;             /* Whether bit_adj                 is valid. */
    fbe_bool_t b_crc;             /* Whether crc_det                 is valid. */
    fbe_s32_t sym_size;        /* "symbol" size = [16, 32*8] bits */
    fbe_bool_t is_valid = FBE_TRUE;   /* By default, the error record is valid. */
    fbe_bool_t b_poc_injection;

    fbe_logical_error_injection_get_poc_injection(&b_poc_injection);

    /* This function validates RAID 6 specific features. 
     */
    if(LEI_COND(fbe_logical_error_injection_is_table_for_r6_only() != FBE_TRUE) )
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: logical error injection: rec_p %p, tabel is not for R6\n",
            __FUNCTION__, rec_p);
        return FBE_FALSE;
    }

    /* The error type must be valid. 
     */
    if(LEI_COND(rec_p->err_type == FBE_XOR_ERR_TYPE_NONE) )
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: logical error injection: rec_p->err_type 0x%x == FBE_XOR_ERR_TYPE_NONE\n",
            __FUNCTION__, rec_p->err_type);
        return FBE_FALSE;
    }

    /* Because it is time consuming to randomly create errors 
     * that are not detectable by the checksum, the following 
     * restrictions apply if crc_det == RG_R6_ERR_PARM_RND/NO.
     * Recall the checksum is 16bits.
     */
    if (rec_p->crc_det == RG_R6_ERR_PARM_YES    || 
        rec_p->crc_det == RG_R6_ERR_PARM_UNUSED ||
        (rec_p->bit_adj == RG_R6_ERR_PARM_YES && 
         !(rec_p->num_bits % FBE_U32_NUM_BITS)))
    {
        /* If these conditions are met, do nothing and fall through. 
         */
        b_coh = FBE_TRUE;
    }
    else
    {
        b_coh = FBE_FALSE;
    }

    /* Validate the following parameters of an error record:
     * pos_bitmap: the logical position bitmap of a strip,
     * err_adj:    the error adjacency position bitmap.
     * If there are N errors within one strip, err_adj is equal to 
     * the OR of all N corresponding pos_bitmap's.
     */
    switch (rec_p->err_type)
    {
        case FBE_XOR_ERR_TYPE_1NS:
        case FBE_XOR_ERR_TYPE_1COD:
            b_pos = rg_r6_pos_bitmap_err_adj_chk(rec_p->pos_bitmap,
                                                 rec_p->err_adj,
                                                 (fbe_u16_t)RG_R6_MASK_LOG_DATA,
                                                 RG_R6_MASK_LOG_PAR);
            break;

        case FBE_XOR_ERR_TYPE_1S:
            b_pos = rg_r6_pos_bitmap_err_adj_chk(rec_p->pos_bitmap,
                                                 rec_p->err_adj,
                                                 (fbe_u16_t)RG_R6_MASK_LOG_S,
                                                 RG_R6_MASK_LOG_PAR);
            break;

        case FBE_XOR_ERR_TYPE_1R:
            /* For row parity error, the logical position must be 
             * RG_R6_MASK_LOG_ROW.
             */
            b_pos = 
                rec_p->pos_bitmap == RG_R6_MASK_LOG_ROW    && 
                (rec_p->pos_bitmap &  rec_p->err_adj) != 0 && 
                (rec_p->pos_bitmap & ~rec_p->err_adj) == 0;
            break;

        case FBE_XOR_ERR_TYPE_1D:
            /* For diagonal parity error, the logical position must be 
             * RG_R6_MASK_LOG_DIAG.
             */
            b_pos = 
                rec_p->pos_bitmap == RG_R6_MASK_LOG_DIAG   && 
                (rec_p->pos_bitmap &  rec_p->err_adj) != 0 && 
                (rec_p->pos_bitmap & ~rec_p->err_adj) == 0;
            break;

        case FBE_XOR_ERR_TYPE_1COP:
            b_pos = rg_r6_pos_bitmap_err_adj_chk(rec_p->pos_bitmap,
                                                 rec_p->err_adj,
                                                 RG_R6_MASK_LOG_PAR,
                                                 (fbe_u16_t)RG_R6_MASK_LOG_DATA);
            break;

        case FBE_XOR_ERR_TYPE_1POC:
            /* If poc_injection is equal to FBE_TRUE, POC error injection 
             * is enabled. In this case, pos_bitmap and err_adj are 
             * validated normally.
             * If poc_injection is equal to FBE_FALSE, POC error injection 
             * is disabled. In ths case, pos_bitmap should be zero. 
             */
            if (b_poc_injection)
            {
                b_pos = rg_r6_pos_bitmap_err_adj_chk(rec_p->pos_bitmap,
                                                     rec_p->err_adj,
                                                     RG_R6_MASK_LOG_PAR,
                                                     (fbe_u16_t)RG_R6_MASK_LOG_DATA);
            }
            else
            {
                b_pos = rec_p->pos_bitmap == 0;
            }
            break;

        default:
            fbe_logical_error_injection_table_validate_all(rec_p);
            return FBE_TRUE;
            break;
    }

    /* Validate the following parameters of an error record: 
     * width:    the symbol index within a block. 
     * crc_det:  whether error(s) injected are CRC detectable.
     * sym_size: the symbol size. It is equal to 16 or 32*8 bits.
     */
    switch (rec_p->err_type)
    {
        case FBE_XOR_ERR_TYPE_1NS:
        case FBE_XOR_ERR_TYPE_1R:
        case FBE_XOR_ERR_TYPE_1D:
            b_width = 
                rec_p->width == RG_R6_ERR_PARM_RND || 
                rec_p->width <= (RG_R6_SYM_PER_SECTOR - 1);
            b_crc = 
                rec_p->crc_det == RG_R6_ERR_PARM_RND || 
                rec_p->crc_det == RG_R6_ERR_PARM_YES || 
                rec_p->crc_det == RG_R6_ERR_PARM_NO;
            sym_size = RG_R6_SYM_SIZE_BITS;
            break;

        case FBE_XOR_ERR_TYPE_1S:
            /* The S data symbol is located at a specific location 
             * on a given drive. The sum of its logical position 
             * index and its symbol position index must be equal to 
             * the number of symbols per data sector. If randomization 
             * is selected, both parameters must be random. 
             */
            if (rec_p->pos_bitmap == RG_R6_ERR_PARM_RND && 
                rec_p->width      == RG_R6_ERR_PARM_RND)
            {
                b_width = FBE_TRUE;
            }
            else if (rec_p->pos_bitmap != RG_R6_ERR_PARM_RND         && 
                     rec_p->pos_bitmap != RG_R6_MASK_LOG_NS          && 
                     rec_p->width      <= (RG_R6_SYM_PER_SECTOR - 1) && 
                     (fbe_logical_error_injection_bitmap_to_position(rec_p->pos_bitmap) + rec_p->width) 
                     == RG_R6_SYM_PER_SECTOR)
            {
                b_width = FBE_TRUE;
            }
            else
            {
                /* All other combinations are invalid.
                 */
                b_width = FBE_FALSE;
            }

            b_crc = 
                rec_p->crc_det == RG_R6_ERR_PARM_RND || 
                rec_p->crc_det == RG_R6_ERR_PARM_YES || 
                rec_p->crc_det == RG_R6_ERR_PARM_NO;
            sym_size = RG_R6_SYM_SIZE_BITS;
            break;

        case FBE_XOR_ERR_TYPE_1COD:
        case FBE_XOR_ERR_TYPE_1COP:
        case FBE_XOR_ERR_TYPE_1POC:
            b_width = rec_p->width   == RG_R6_ERR_PARM_UNUSED;
            b_crc   = rec_p->crc_det == RG_R6_ERR_PARM_UNUSED;
            sym_size = FBE_U16_NUM_BITS;
            break;
           
        /* Defensive programming as any bad error types would have been 
         * caught by the default case of the previous switch block. 
         */
        default:
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: logical error injection: error in validating parameters err_type 0x%x\n",
                __FUNCTION__, rec_p->err_type);
            return FBE_FALSE;
    }

    /* Validate the following parameters of an error record: 
     * start_bit: the start bit where an error will be injected.
     * num_bits:  the number of bits to be corrupted. 
     *            start_bit + num_bits must be less than or equal
     *            to the size of a symbol. 
     * bit_adj:   whether num_bits are adjacent.
     */
    b_start = rec_p->start_bit == RG_R6_ERR_PARM_RND || 
        rec_p->start_bit <= (sym_size - 1);

    b_num   = rec_p->num_bits  == RG_R6_ERR_PARM_RND || 
        (rec_p->num_bits > 0 && rec_p->num_bits <= sym_size);

    b_both  = rec_p->start_bit == RG_R6_ERR_PARM_RND || 
        rec_p->num_bits == RG_R6_ERR_PARM_RND || 
        (rec_p->start_bit + rec_p->num_bits) <= sym_size;

    b_adj = 
        rec_p->bit_adj == RG_R6_ERR_PARM_RND || 
        rec_p->bit_adj == RG_R6_ERR_PARM_YES || 
        rec_p->bit_adj == RG_R6_ERR_PARM_NO;

    /* If any of these conditions is false, this error record is invalid.
     */
    if (!b_coh || !b_pos || !b_width || !b_start || !b_num || !b_both || 
        !b_adj || !b_crc)
    {
        /* Print out the invalid error record and the reason for debugging.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "fbe_logical_error_injection_table_validate_r6: record invalid!\n");
        fbe_logical_error_injection_print_record(rec_p);

        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "Invalid parameter(s):\n\t");
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s", !b_coh   ? "coh err can't be injected, " : "");
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s", !b_pos   ? "pos_bitmap and/or err_adj, " : "");
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s", !b_width ? "width, "                     : "");
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s", !b_start ? "start_bit, "                 : "");
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s", !b_num   ? "num_bits, "                  : "");
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s", !b_both  ? "start_bit and num_bits, "    : "");
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s", !b_crc   ? "crc_det, "                   : "");
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s", !b_adj   ? "bit_adj, "                   : "");

        is_valid = FBE_FALSE;
    }

    return is_valid;
}
/******************************
 * end fbe_logical_error_injection_table_validate_r6()
*****************************/

/****************************************************************
 * rg_err_printf_err_record()
 ****************************************************************
 * @brief
 *  This function outputs an error record.
 *

 * @param rec_p - pointer to an error record
 *
 * @return
 *  None.
 *
 * @author
 *  5/31/06 - Created. SL
 *
 ****************************************************************/
static void fbe_logical_error_injection_print_record(const fbe_logical_error_injection_record_t* const rec_p)
{
    
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                              "\tpos_bitmap = %x, width = %x, opcode = %x, lba = %llx\n",
                                              rec_p->pos_bitmap, rec_p->width, rec_p->opcode, (unsigned long long)rec_p->lba);
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                              "\tblocks = %llx, err_type = %x, err_mode = %x, err_count = %x\n",
                                              (unsigned long long)rec_p->blocks, rec_p->err_type, rec_p->err_mode, rec_p->err_count);
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                              "\terr_limit = %x, skip_count = %x, skip_limit = %x, err_adj = %x\n",
                                              rec_p->err_limit, rec_p->skip_count, rec_p->skip_limit, rec_p->err_adj);
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                              "\tstart_bit = %x, num_bits = %x, bit_adj = %x, bit_adj = %x\n",
                                              rec_p->start_bit, rec_p->num_bits, rec_p->bit_adj, rec_p->bit_adj);

    return;
}
/******************************
 * end fbe_logical_error_injection_print_record()
*****************************/

/****************************************************************
 * fbe_logical_error_injection_table_randomize()
 ****************************************************************
 * @brief
 *  This function randomizes an error table. 
 *
 *  Note by the time this function is called, the error record
 *  should have been validated by rg_err_rec_validate().
 *
 * @param None.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/7/06 - Created. SL
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_table_randomize(void)
{
    fbe_status_t status;
    fbe_s32_t idx;

    /* This function can only randomize RAID 6 error records.
     */
    if (fbe_logical_error_injection_is_table_for_r6_only() == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }

    /* Loop over all records of the error table.
     */
    for (idx = 0; idx < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; ++idx)
    {
        fbe_logical_error_injection_record_t *rec_p = NULL;
        fbe_s32_t sym_size; /* symbol size in bits */

        fbe_logical_error_injection_get_error_info(&rec_p, idx);

        /* FBE_XOR_ERR_TYPE_NONE marks the end of the error table. 
         */
        if (rec_p->err_type == FBE_XOR_ERR_TYPE_NONE)
        {
            break;
        }

        /* Randomize the logical position bitmap of a strip. 
         * Error tables are randomized right after they are read in from 
         * files, independent of the array widths of the LUNs at the time. 
         * If a bitmap represents a position that is not physically present,
         * the corresponding error record will be skipped by Rderr. This
         * is no difference from how the other error tables are used. 
         */
        switch (rec_p->err_type)
        {
            case FBE_XOR_ERR_TYPE_1NS:
            case FBE_XOR_ERR_TYPE_1COD:
                if (rec_p->pos_bitmap == RG_R6_ERR_PARM_RND)
                {
                    rec_p->pos_bitmap = fbe_random() % RG_R6_DRV_PHYS_MAX;
                    rec_p->pos_bitmap = 1 << rec_p->pos_bitmap;
                }
                break;

            case FBE_XOR_ERR_TYPE_1S:
                if (rec_p->pos_bitmap == RG_R6_ERR_PARM_RND)
                {
                    /* First column has no S data. 
                     */
                    rec_p->pos_bitmap = fbe_random() % (RG_R6_DRV_PHYS_MAX - 1);
                    rec_p->pos_bitmap = 1 << (rec_p->pos_bitmap + 1);
                }
                break;

            case FBE_XOR_ERR_TYPE_1R:
            case FBE_XOR_ERR_TYPE_1D:
                break;

            case FBE_XOR_ERR_TYPE_1COP:
            case FBE_XOR_ERR_TYPE_1POC:
                if (rec_p->pos_bitmap == RG_R6_ERR_PARM_RND)
                {
                    rec_p->pos_bitmap = RG_R6_POS_LOG_ROW + fbe_random() % 2;
                    rec_p->pos_bitmap = 1 << rec_p->pos_bitmap;
                }
                break;

            /* Defensive programming as all parameters have been validated 
             * by the time this function is called. 
             */
            default:
                return FBE_STATUS_OK;
                break;
        } /* end of switch (rec_p->err_type) */
  
        switch (rec_p->err_type)
        {
            case FBE_XOR_ERR_TYPE_1NS:
            case FBE_XOR_ERR_TYPE_1R:
            case FBE_XOR_ERR_TYPE_1D:
                if (rec_p->width == RG_R6_ERR_PARM_RND)
                {
                    rec_p->width = fbe_random() % RG_R6_SYM_PER_SECTOR;
                }
                sym_size = RG_R6_SYM_SIZE_BITS;
                break;

            case FBE_XOR_ERR_TYPE_1S:
                if (rec_p->width == RG_R6_ERR_PARM_RND)
                {
                    /* The S data symbol is located at a specific location 
                     * on a given drive. The sum of its logical position 
                     * index and its symbol position index must be equal to 
                     * the number of symbols per data sector. 
                     */
                    rec_p->width = RG_R6_SYM_PER_SECTOR 
                        - fbe_logical_error_injection_bitmap_to_position(rec_p->pos_bitmap);
                }
                sym_size = RG_R6_SYM_SIZE_BITS;
                break;

            case FBE_XOR_ERR_TYPE_1COD:
            case FBE_XOR_ERR_TYPE_1COP:
            case FBE_XOR_ERR_TYPE_1POC:
                sym_size = FBE_U16_NUM_BITS;
                break;

            /* Defensive programming as all parameters have been validated 
             * by the time this function is called. 
             */
            default:
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                          "%s unexpected type %d",
                                                          __FUNCTION__, rec_p->err_type);
                return FBE_STATUS_GENERIC_FAILURE;
                break;
        } /* switch (rec_p->err_type) */

        if (rec_p->start_bit == RG_R6_ERR_PARM_RND && 
            rec_p->num_bits  == RG_R6_ERR_PARM_RND)
        {
            /* The valid range is [0, sym_size - 1].
             */
            rec_p->start_bit = fbe_random() % sym_size;

            /* The valid range is [1, sym_size - start_bit], but the % range
             * is [0, sym_size - start_bit - 1], so we need to add 1 after %.
             */
            rec_p->num_bits = fbe_random() % (sym_size - rec_p->start_bit) + 1;
        }
        else if (rec_p->start_bit == RG_R6_ERR_PARM_RND && 
                 rec_p->num_bits  != RG_R6_ERR_PARM_RND)
        {
            /* The valid range is [0, sym_size - num_bits], but the % range 
             * is [0, sym_size - num_bits - 1], so we need to add 1 to %.
             */
            rec_p->start_bit = fbe_random() % (sym_size - rec_p->num_bits + 1);
        }
        else if (rec_p->start_bit != RG_R6_ERR_PARM_RND && 
                 rec_p->num_bits  == RG_R6_ERR_PARM_RND)
        {
            /* The valid range is [1, sym_size - start_bit], but the % range
             * is [0, sym_size - start_bit - 1], so we need to add 1 after %.
             */
            rec_p->num_bits = fbe_random() % (sym_size - rec_p->start_bit) + 1;
        }

        if (rec_p->bit_adj == RG_R6_ERR_PARM_RND)
        {
            /* The valid values are [0/1] = [FBE_FALSE/FBE_TRUE].
             */
            rec_p->bit_adj = fbe_random() % 2;
        } 

        if (rec_p->crc_det == RG_R6_ERR_PARM_RND)
        {
            /* The valid values are [0/1] = [FBE_FALSE/FBE_TRUE].
             */
            rec_p->crc_det = fbe_random() % 2;
        }
    } /* for() */

    /* Loop over the error table to set up err_adj for error records which 
     * pos_bitmap is randomly generated.
     */
    fbe_logical_error_injection_setup_err_adj();

    /* Make sure the table has been randomized correctly. 
     */
    status = fbe_logical_error_injection_table_validate();

    return status;
}
/******************************
 * end fbe_logical_error_injection_table_randomize()
*****************************/

/****************************************************************
 * fbe_logical_error_injection_rec_pos_match_r6()
 ****************************************************************
 * @brief
 *  This function checks whether fruts->position matches the 
 *  pos_bitmap of the error record of interest. 
 *
 *  Note by the time this function is called, the error record
 *  should have been validated by fbe_logical_error_injection_table_validate().
 *

 * @param fruts_p - pointer to fruts to inject error
 * @param rec_p - pointer to an error record
 *
 * @return
 *  FBE_TRUE:  pos_bitmap matches        fruts->position.
 *  FBE_FALSE: pos_bitmap does not match fruts->position.
 *
 * @author
 *  4/7/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_rec_pos_match_r6(fbe_raid_fruts_t*      const fruts_p,
                                       fbe_logical_error_injection_record_t* const rec_p)
{
    fbe_raid_siots_t* const siots_p = (fbe_raid_siots_t* const)fbe_raid_fruts_get_siots(fruts_p);
    const fbe_u16_t type = rec_p->err_type;
    const fbe_u16_t pos  = rec_p->pos_bitmap;
    fbe_bool_t is_match = FBE_FALSE;

    /* The order is row, diagonal, and then data position. Data position 
     * need to be checked last because FBE_RAID_EXTENT_POSITION() assumes that 
     * its input is a data position. 
     */
    if (fruts_p->position == fbe_raid_siots_get_row_parity_pos(siots_p))
    {
        /* This is a row parity position. The 3 possible matches are:
         * 1) Error type is row parity.
         * 2) Error type is checksum of parity and the position matches.
         * 3) Error type is parity of checksum and the position matches.
         */
        if ((type == FBE_XOR_ERR_TYPE_1R                               ) ||
            (type == FBE_XOR_ERR_TYPE_1COP && pos == RG_R6_MASK_LOG_ROW) ||
            (type == FBE_XOR_ERR_TYPE_1POC && pos == RG_R6_MASK_LOG_ROW))
        {
            is_match = FBE_TRUE;
        }
    }
    else if (fruts_p->position == fbe_raid_siots_dp_pos(siots_p))
    {
        /* This is a diagonal parity position. The 3 possible matches are:
         * 1) Error type is diagonal parity.
         * 2) Error type is checksum of parity and the position matches.
         * 3) Error type is parity of checksum and the position matches.
         */
        if ((type == FBE_XOR_ERR_TYPE_1D                                ) ||
            (type == FBE_XOR_ERR_TYPE_1COP && pos == RG_R6_MASK_LOG_DIAG) ||
            (type == FBE_XOR_ERR_TYPE_1POC && pos == RG_R6_MASK_LOG_DIAG))
        {
            is_match = FBE_TRUE;
        }
    }
    else
    {
        /* This is a data position. Because the Non-S data check is simpler,
         * it is done first. 
         * Only calculate the logical position, pos_logical, when we need it.
         */
        fbe_u32_t pos_logical;
        fbe_status_t status = FBE_STATUS_OK;
        status = 
            FBE_RAID_EXTENT_POSITION(fruts_p->position, 
                               siots_p->parity_pos, 
                               fbe_raid_siots_get_width(siots_p), 
                               fbe_raid_siots_parity_num_positions(siots_p),
                               &pos_logical);
        if(LEI_COND(status != FBE_STATUS_OK))
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: logical error injection: status 0x%x != FBE_STATUS_OK\n",
                __FUNCTION__, status);
            return is_match;
        }
        if(LEI_COND(pos_logical >= RG_R6_DRV_PHYS_MAX))
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: logical error injection: pos_logical 0x%x >= RG_R6_DRV_PHYS_MAX\n",
                __FUNCTION__, pos_logical);
            return is_match;
        }

        if (pos_logical == RG_R6_POS_LOG_NS)
        {
            /* The 2 possible matches are:
             * 1) Error type is Non-S       data and the position is 0. 
             * 2) Error type is checksum of data and the position is 0.
             */
            fbe_bool_t is_pos = pos & RG_R6_MASK_LOG_NS;
            if (is_pos && (type == FBE_XOR_ERR_TYPE_1NS || 
                           type == FBE_XOR_ERR_TYPE_1COD))
            {
                is_match = FBE_TRUE;
            }
        }
        else if (pos_logical > RG_R6_POS_LOG_NS && 
                 pos_logical < (fbe_raid_siots_get_width(siots_p) - 
                                fbe_raid_siots_parity_num_positions(siots_p)))
        {
            /* The 2 possible matches are:
             * 1) Error type is Non-S       data and the position is non-0.
             * 2) Error type is     S       data and the position is non-0. 
             * 3) Error type is checksum of data and the position is non-0.
             */
            fbe_bool_t is_pos = pos & (1 << pos_logical);
            if (is_pos && (type == FBE_XOR_ERR_TYPE_1NS || 
                           type == FBE_XOR_ERR_TYPE_1S  || 
                           type == FBE_XOR_ERR_TYPE_1COD))
            {
                is_match = FBE_TRUE;
            }
        }
    }

    return is_match;
}
/****************************************************
 * end fbe_logical_error_injection_rec_pos_match_r6()
*****************************************************/

/****************************************************************
 * fbe_logical_error_injection_inject_for_r6()
 ****************************************************************
 * @brief
 *  This function injects error to this fruts position. 
 *  Note by the time this function is called, the error record
 *  should have been randomized if necessary and then validated
 *  as well as verified to match the sector type.  Currently this
 *  routine ONLY supports R6.  It could be modified in the future 
 *  to support other RAID types but checks on the `width' etc are
 *  needed. 
 *
 *  The minimum unit is a type of fbe_u32_t.
 *   3322 2222 2222 1111 1111 11            
 *   1098 7654 3210 9876 5432 1098 7654 3210
 *  |----|----|----|----|----|----|----|----|
 *
 *  Example No.1:
 *   xxxx xxx
 *  |----|----|----|----|----|----|----|----|
 *  bits_first   = 25;
 *  width_first  = 7;
 *  words_middle = 0
 *  width_last   = 0;
 *
 *  Example No.2:
 *   xxxx xxx
 *  |----|----|----|----|----|----|----|----|
 *   xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
 *  |----|----|----|----|----|----|----|----|
 *   xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
 *  |----|----|----|----|----|----|----|----|
 *                               x xxxx xxxx
 *  |----|----|----|----|----|----|----|----|
 *  bits_first   = 25;
 *  width_first  = 7;
 *  words_middle = 2;
 *  width_last   = 9;
 *
 *  Example No.3:
 *                               x xxxx xxxx
 *  |----|----|----|----|----|----|----|----|
 *  bits_first   = 0;
 *  width_first  = 9;
 *  words_middle = 0;
 *  width_last   = 0;
 *

 * @param sect_p - pointer to a data or parity sector
 * @param rec_p - pointer to an error record
 *
 * @return
 *  fbe_status_t.
 *
 * @author
 *  4/7/06 - Created. SL
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_inject_for_r6(fbe_sector_t* const sect_p, 
                          const fbe_logical_error_injection_record_t* const rec_p)
{
    const fbe_u16_t type      = rec_p->err_type;
    const fbe_u16_t start_bit = rec_p->start_bit;
    const fbe_u16_t num_bits  = rec_p->num_bits;
    fbe_u16_t  crc_orig;     /* original checksum before injection */
    fbe_status_t status       = FBE_STATUS_OK;
    fbe_lba_t       invalidated_lba;

    /* Save the original checksum for comparison later.
     */
    crc_orig = sect_p->crc;

    if (type == FBE_XOR_ERR_TYPE_1NS || type == FBE_XOR_ERR_TYPE_1S || 
        type == FBE_XOR_ERR_TYPE_1R  || type == FBE_XOR_ERR_TYPE_1D    )
    {
        fbe_u16_t  crc_cooked;   /* cooked   checksum after  injection */
        fbe_s32_t    word_first;   /* word offset of the first injection */
        fbe_s32_t    bits_first;   /* bit  offset of the first injection */
        fbe_s32_t    width_first;  /* width       of the first injection */
        fbe_s32_t    words_middle; /* number of words to be injected     */
        fbe_s32_t    width_last;   /* width       of the last  injection */
        fbe_u32_t* data32_p;     /* word to be injected */
        fbe_u32_t  mask32;       /* mask to be injected */
        fbe_s32_t    idx;
        fbe_bool_t b_invalidated;
        
        /* If the block was previously invalidated, then
         * handle it differently since it already has a bad csum.
         */
        b_invalidated = xorlib_get_seed_from_invalidated_sector(sect_p, 
                                                                &invalidated_lba);
        if (b_invalidated)
        {
            fbe_u32_t   test_word;

            /* Get the test word from the sector
             */
            xorlib_get_test_word_from_invalidated_sector(sect_p, &test_word);

            /* Now check if we need to togle
             */
            if (rec_p->crc_det == RG_R6_ERR_PARM_NO)
            {
                /* Leave the crc alone, it should continue to be a bad crc.
                 * Since the code above may or may not have injected any errors,
                 * Just inject a coherency error by flipping the same bits in each
                 * half of this word.  The effect is a coherency error, since
                 * when we rotate and fold the raw crc the crc is the same.
                 */
                if (test_word == 0xBAD0BAD0)
                {
                    /* Toggle the test word */
                    xorlib_set_test_word_in_invalidated_sector(sect_p, 0xBAD1BAD1);
                }
                else
                {
                    /* Toggle the test word */
                    xorlib_set_test_word_in_invalidated_sector(sect_p, 0xBAD0BAD0);
                }

                /* Assert that we will not catch this error.
                 */
                if(LEI_COND(fbe_xor_lib_calculate_checksum(sect_p->data_word) != FBE_SECTOR_DEFAULT_CHECKSUM))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: logical error injection: Checksum calculated for data_word 0x%p is different that with 0\n",
                        __FUNCTION__, sect_p->data_word);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            else
            {
                /* Corrupt the invalid block in such a way that we'll detect this error.
                 * Also assert that we'll catch this error.
                 */
                if(LEI_COND( test_word == 0xBAD0 ))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: logical error injection: inv_p->test_word 0x%x == 0xBAD0\n",
                        __FUNCTION__, test_word);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                /* Blast the test word to a special value*/
                xorlib_set_test_word_in_invalidated_sector(sect_p, 0xBAD0);
                if(LEI_COND(fbe_xor_lib_calculate_checksum(sect_p->data_word) == FBE_SECTOR_DEFAULT_CHECKSUM))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: logical error injection: Checksum calculated for data_word 0x%p is equal to that with 0\n",
                        __FUNCTION__, sect_p->data_word);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            return status;
        } /* End block already invalid. */
        
        /* Inject error to the first partial word.
         */
        word_first = start_bit / FBE_U32_NUM_BITS;
        bits_first = start_bit % FBE_U32_NUM_BITS;
        data32_p = &sect_p->data_word
            [rec_p->width * RG_R6_SYM_SIZE_LONGS + word_first];

        if (bits_first == 0)
        {
            /* Cases covered: 
             *                                      xxxx 
             *  |----|----|----|----|----|----|----|----|
             *
             *   xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
             *  |----|----|----|----|----|----|----|----|
             */
            width_first = (num_bits < FBE_U32_NUM_BITS) ? num_bits : 0;
        }
        else
        {
            /* Cases covered: 
             *                       xxxx xxxx
             *  |----|----|----|----|----|----|----|----|
             *
             *   xxxx xxxx xxxx
             *  |----|----|----|----|----|----|----|----|
             */
            width_first = FBE_MIN(num_bits, FBE_U32_NUM_BITS - bits_first);
        }
        if(LEI_COND(width_first > num_bits))
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: logical error injection: width_first 0x%x > num_bits 0x%x \n",
                __FUNCTION__, width_first, num_bits);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (width_first != 0)
        {
            mask32   = (1 << width_first) - 1;
            mask32 <<= bits_first;
            RG_R6_ERR_INJECT_RANGE(32, data32_p, rec_p->bit_adj, mask32);
            ++data32_p;
        }

        /* Inject error to the middle words_middle words.
         */
        words_middle = (num_bits - width_first) / FBE_U32_NUM_BITS;
        for (idx = 0; idx < words_middle; ++idx)
        {
            rg_r6_err_inject_uint32(data32_p, rec_p->bit_adj);
            ++data32_p;
        }

        /* Inject error to the last partial word.
         */
        width_last = (num_bits - width_first) % FBE_U32_NUM_BITS;
        if (width_last != 0)
        {
            mask32 = (1 << width_last) - 1;
            RG_R6_ERR_INJECT_RANGE(32, data32_p, rec_p->bit_adj, mask32);
            ++data32_p;
        }
        
        /* Call an XOR function to recompute the checksum. 
         * seed of xorlib_cook_csum(data_ptr, seed) is no longer used. 
         */
        crc_cooked = fbe_xor_lib_calculate_checksum(sect_p->data_word);

        if (rec_p->crc_det == RG_R6_ERR_PARM_NO)
        {
            sect_p->crc = crc_cooked;
        }
        else if (rec_p->crc_det == RG_R6_ERR_PARM_YES && 
                 crc_cooked == crc_orig)
        {
            /* It is possible the recomputed checksum does not change due to
             * the randomness of the injection. In this case we flip the 
             * first bit so that it is guaranteed to be a checksum error. 
             */
            data32_p = &sect_p->data_word
                [rec_p->width * RG_R6_SYM_SIZE_LONGS + word_first];
            mask32 = 1 << bits_first;
            RG_R6_ERR_INJECT_RANGE(32, data32_p, FBE_TRUE, mask32);

            /* Just to be absolutely sure. 
             */
            if(LEI_COND(fbe_xor_lib_calculate_checksum(sect_p->data_word) == crc_orig) )
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: logical error injection: Checksum calculated for data_word 0x%p is different than crc_orig 0x%x\n",
                    __FUNCTION__, sect_p->data_word, crc_orig);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    else
    {
        fbe_u16_t* data16_p;
        fbe_u16_t  mask16;
        fbe_u16_t  value_orig; /* original value before injection */
        fbe_u16_t  crc_cooked;   /* cooked   checksum after  injection */
        /* Corrupt different meta data depending on the error type.
         */
        data16_p = rec_p->err_type == FBE_XOR_ERR_TYPE_1POC ?
            &sect_p->lba_stamp : &sect_p->crc;

        /* Save the original value for comparison later.
         */
        value_orig = *data16_p;

        mask16   = (1 << num_bits) - 1;
        mask16 <<= start_bit;
        RG_R6_ERR_INJECT_RANGE(16, data16_p, rec_p->bit_adj, mask16);
    
        if (*data16_p == value_orig)
        {
            /* It is possible the generated value does not change due to
             * the randomness of the injection. In this case we flip the 
             * first bit so that the new value is different. 
             */
            mask16 = 1 << start_bit;
            RG_R6_ERR_INJECT_RANGE(16, data16_p, FBE_TRUE, mask16);
        }

        /* Just to be absolutely sure. 
         */
        if(LEI_COND(*data16_p == value_orig))
        {
            fbe_logical_error_injection_service_trace(
                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: logical error injection: *data16_p 0x%x == value_orig 0x%x \n",
                __FUNCTION__, *data16_p, value_orig);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Call an XOR function to recompute the checksum. 
         * seed of xorlib_cook_csum(data_ptr, seed) is no longer used. 
         */
        crc_cooked = fbe_xor_lib_calculate_checksum(sect_p->data_word);

        if (rec_p->err_type != FBE_XOR_ERR_TYPE_1POC &&
            crc_cooked == sect_p->crc)
        {
            /* The crc is still not invalid.
             * Our base requirement is to create an invalid crc,
             * so do that now.
             * In some cases we have multiple error records,
             * which in some rare cases will cancel each other out.
             * Thus, we just make sure the crc is bad on the sector.
             */
            sect_p->crc = (crc_cooked == 0x0BAD) ? 0x1BAD : 0xBAD;
        }
    }
    return status;
}
/**************************************************
 * end fbe_logical_error_injection_inject_for_r6()
 **************************************************/

/****************************************************************
 * ffbe_logical_error_injection_rec_pos_match_all_types()
 ****************************************************************
 * @brief
 *  This function checks whether fruts->position matches the 
 *  pos_bitmap of the error record of interest. 
 *
 *  Note by the time this function is called, the error record
 *  should have been validated by fbe_logical_error_injection_table_validate().
 *

 * @param fruts_p - pointer to fruts to inject error
 * @param rec_p - pointer to an error record
 *
 * @return
 *  FBE_TRUE:  pos_bitmap matches        fruts->position.
 *  FBE_FALSE: pos_bitmap does not match fruts->position.
 *
 * @author
 *  08/10/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_rec_pos_match_all_types(fbe_raid_fruts_t *const fruts_p,
                                                                      fbe_logical_error_injection_record_t *const rec_p)
{
    fbe_bool_t              is_match = FBE_FALSE;
    fbe_raid_siots_t       *const siots_p = (fbe_raid_siots_t* const)fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Check for a `sparing' type*/
    if (fbe_raid_geometry_is_sparing_type(raid_geometry_p))
    {
        fbe_object_id_t         object_id = FBE_OBJECT_ID_INVALID;
        fbe_logical_error_injection_object_t *object_p = NULL;
        fbe_logical_error_injection_object_t *upstream_object_p = NULL;
        fbe_u32_t               enabled_bitmask;
        fbe_u32_t               upstream_not_enabled_bitmask = 0;
        fbe_payload_block_operation_t *block_operation_p = fbe_raid_fruts_to_block_operation(fruts_p);
        fbe_u32_t               width;
        fbe_u32_t               position;

        /* Get our object
         */
        fbe_raid_geometry_get_width(raid_geometry_p, &width);
        object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
        object_p = fbe_logical_error_injection_find_object(object_id);
        enabled_bitmask = object_p->edge_hook_bitmask;

        /* First we must match out injection bitmask.
         */
        if ((rec_p->pos_bitmap & enabled_bitmask) == 0)
        {
            return is_match;
        }
        /* If this is a metadata request only inject for (1) position
         */
        if (fbe_logical_error_injection_is_lba_metadata_lba(block_operation_p->lba,
                                                            block_operation_p->block_count, 
                                                            raid_geometry_p))
        {
            /* If this is a metadata operation only inject to (1) position.
             * We select the first enabled position.
             */
            for (position = 0; position < width; position++)
            {
                if ((1 << position) & enabled_bitmask)
                {
                    enabled_bitmask = (1 << position);
                    break;
                }
            }
            if ((enabled_bitmask & rec_p->pos_bitmap) != 0)
            {
                is_match = FBE_TRUE;
            }
        }
        else
        {
            /* Get the object id and determine the position that we allow injection 
             * for.  We ignore the fruts position and only allow injection if the
             * error injection record is for the `allowed' position.
             */
            upstream_object_p = fbe_logical_error_injection_get_upstream_object(fruts_p);
            if (upstream_object_p == NULL)
            {
                /* Background operations will not have an upstream object.  Simply use the
                 * fruts.
                 */
                
                /* If no upstream was found simply use the record and fruts position.
                 */
                if ((rec_p->pos_bitmap & (1 << fruts_p->position)) != 0)
                {
                    is_match = FBE_TRUE;
                    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                      "LEI: pos match: %d obj: 0x%x rec mask: 0x%x no upstream fruts_p: %p pos: %d alg: 0x%x lba: 0x%llx blks: 0x%llx\n", 
                                      is_match, fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      rec_p->pos_bitmap, fruts_p, fruts_p->position, siots_p->algorithm,
                                      (unsigned long long)fruts_p->lba, (unsigned long long)fruts_p->blocks);
                }
            }
            else
            {
                /* Else use the not of the upstream object bitmask.
                 */
                upstream_not_enabled_bitmask = ~upstream_object_p->edge_hook_bitmask;
    
                /* Now use the `not enabled' mask to determine if we should inject
                 * or not.
                 */
                if ((upstream_not_enabled_bitmask & rec_p->pos_bitmap) != 0)
                {
                    is_match = FBE_TRUE;
                }
            }
        }

        /* If there is a match trace some information.
         */
        if (is_match)
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                      "LEI: pos match: %d obj: 0x%x rec mask: 0x%x mask: 0x%x fruts_p: %p pos: %d op: %d lba: 0x%llx blks: 0x%llx\n", 
                                      is_match, fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      rec_p->pos_bitmap, upstream_not_enabled_bitmask,
                                      fruts_p, fruts_p->position, fruts_p->opcode,
                                      (unsigned long long)fruts_p->lba, (unsigned long long)fruts_p->blocks);
        }
    }
    else
    {
        fbe_object_id_t object_id;
        object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

        /* Else use the fruts position*/
        if ((rec_p->pos_bitmap & (1 << fruts_p->position)) != 0)
        {
            is_match = FBE_TRUE;
        }

        if ((rec_p->object_id != FBE_OBJECT_ID_INVALID) &&
            (object_id != rec_p->object_id)) {
            is_match = FBE_FALSE;
        }
    }

    /* Return if there was a match or not.
     */
    return is_match;
}
/***********************************************************
 * end fbe_logical_error_injection_rec_pos_match_all_types()
************************************************************/

/****************************************************************
 * fbe_logical_error_injection_inject_for_parity_unit()
 ****************************************************************
 * @brief
 *  This function injects error to this fruts position. 
 *  Note by the time this function is called, the error record
 *  should have been randomized if necessary and then validated
 *  as well as verified to match the sector type. 
 *

 * @param sect_p - pointer to a data or parity sector
 * @param rec_p - pointer to an error record
 *
 * @return
 *  fbe_status_t.
 *
 * @author
 *  04/24/2008 - RDP Copied from fbe_logical_error_injection_inject_for_r6 and updated
 *               to support R5 and mirror types.
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_inject_for_parity_unit(fbe_sector_t* const sect_p, 
                                   const fbe_logical_error_injection_record_t* const rec_p)
{
    const fbe_u16_t type        = rec_p->err_type;
    const fbe_u16_t start_word  = rec_p->start_bit;
    const fbe_u16_t num_words   = rec_p->num_bits;
    fbe_u16_t  crc_orig;         /* original checksum before injection */
    fbe_status_t status         = FBE_STATUS_OK;
    fbe_lba_t       invalidated_lba;

    /* Save the original checksum for comparison later.
     */
    crc_orig = sect_p->crc;

    if ( (type == FBE_XOR_ERR_TYPE_COH) &&
         (rec_p->crc_det == 0)         )
    {
        fbe_u16_t  crc_cooked;   /* cooked   checksum after  injection */
        fbe_u32_t* data32_p;     /* word to be injected */
        fbe_s32_t    idx;
        fbe_bool_t b_invalidated;
        
        /* If the block was previously invalidated, then
         * handle it differently since it already has a bad csum.
         */
        b_invalidated = xorlib_get_seed_from_invalidated_sector(sect_p, 
                                                                &invalidated_lba);
        if (b_invalidated)
        {
            fbe_u32_t   test_word;

            /* Get the test word from the sector
             */
            xorlib_get_test_word_from_invalidated_sector(sect_p, &test_word);
            
            /* Toggle the test word
             */
            if (rec_p->crc_det == RG_R6_ERR_PARM_NO)
            {
                /* Leave the crc alone, it should continue to be a bad crc.
                 * Since the code above may or may not have injected any errors,
                 * Just inject a coherency error by flipping the same bits in each
                 * half of this word.  The effect is a coherency error, since
                 * when we rotate and fold the raw crc the crc is the same.
                 */
                if (test_word == 0xBAD0BAD0)
                {
                    /* Toggle the test word */
                    xorlib_set_test_word_in_invalidated_sector(sect_p, 0xBAD1BAD1);
                }
                else
                {
                    /* Toggle the test word */
                    xorlib_set_test_word_in_invalidated_sector(sect_p, 0xBAD0BAD0);
                }

                /* Assert that we will not catch this error.
                 */
                if(LEI_COND(fbe_xor_lib_calculate_checksum(sect_p->data_word) != FBE_SECTOR_DEFAULT_CHECKSUM))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: logical error injection: Checksum calculated for data_word 0x%p is different that with 0\n",
                        __FUNCTION__, sect_p->data_word);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            else
            {
                /* Corrupt the invalid block in such a way that we'll detect this error.
                 * Also assert that we'll catch this error.
                 */
                if(LEI_COND( test_word == 0xBAD0 ))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: logical error injection: inv_p->test_word 0x%x == 0xBAD0 \n",
                        __FUNCTION__, test_word);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Blast the test word to a special value*/
                xorlib_set_test_word_in_invalidated_sector(sect_p, 0xBAD0);

                if(LEI_COND(fbe_xor_lib_calculate_checksum(sect_p->data_word) == FBE_SECTOR_DEFAULT_CHECKSUM))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: logical error injection: Checksum calculated for data_word 0x%p is equal to that with 0\n",
                        __FUNCTION__, sect_p->data_word);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            return status;
        } /* End block already invalid. */
        
        /* For non R6 types simply inject the bad word
         */
        if ((start_word + num_words) > FBE_WORDS_PER_BLOCK)
        {
            /* Attempted to go beyond block.  Simply return.
             */
            return status;
        }

        /* For each affected word corrupt it.
         */
        data32_p = &sect_p->data_word[start_word];
        for (idx = start_word; idx < (start_word + num_words); idx++)
        {
            fbe_u32_t next_word_value;
            /* Toggle the word value between 0xBAD0BAD0 and 0xBAD1BAD1
             */
            next_word_value = (*data32_p == 0xBAD0BAD0) ? 0xBAD1BAD1 : 0xBAD0BAD0;
            *data32_p++ = next_word_value; 
        }
        
        /* Call an XOR function to recompute the checksum. 
         * seed of xorlib_cook_csum(data_ptr, seed) is no longer used. 
         */
        crc_cooked = fbe_xor_lib_calculate_checksum(sect_p->data_word);
            
        /* Update checksum to be correct for new data!
         */
        sect_p->crc = crc_cooked;
    }
    return status;
}
/******************************
 * end fbe_logical_error_injection_inject_for_parity_unit()
 *****************************/

/****************************************************************
 * fbe_logical_error_injection_convert_bitmap_log2phys()
 ****************************************************************
 * @brief
 *  This function converts a logical bitmap to a physical one.
 * 

 * @param siots_p - pointer to a fbe_raid_siots_t struct
 * @param bitmap_log - logical bitmap to be converted.
 *
 * @return
 *  physical bitmap. 
 *
 * @author
 *  05/03/06 - Created. SL
 *
 ****************************************************************/
fbe_u16_t fbe_logical_error_injection_convert_bitmap_log2phys(fbe_raid_siots_t* const siots_p,
                                                              fbe_u16_t bitmap_log)
{
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_u16_t bitmap_phys = 0;
    fbe_s32_t   idx;
    fbe_s32_t   width;

    if ( fbe_logical_error_injection_is_table_for_all_types() )
    {
        return bitmap_log;
    }
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* fbe_xor_error_regions_t do not support mirror and non-redundant units. 
     * The long term goal is to use this validation code for all types. 
     */
    if(LEI_COND( (fbe_raid_geometry_is_mirror_type(raid_geometry_p) != FBE_FALSE) ||
                  (fbe_raid_geometry_is_raid0(raid_geometry_p) != FBE_FALSE) ) )
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: logical error injection: raid_geometry_p %p is either mirror type or Raid0 \n",
            __FUNCTION__, raid_geometry_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (bitmap_log == 0)
    {
        /* No need to loop over each bit if the bitmap is equal to zero.
         */
        return bitmap_phys;
    }

    /* Check all data positions one bit at a time. 
     */
    width = fbe_raid_siots_get_width(siots_p) - fbe_raid_siots_parity_num_positions(siots_p);
    for (idx = 0; idx < width; ++idx)
    {
        if ((bitmap_log & (1 << idx)) != 0)
        {
            bitmap_phys |= 1 << siots_p->geo.position[idx];
        } 
    } /* end of for (idx...) */

    /* Check the row parity position.
     */
    if ((bitmap_log & RG_R6_MASK_LOG_ROW) != 0)
    {
        bitmap_phys |= 1 << fbe_raid_siots_get_row_parity_pos(siots_p);
    }

    /* Check the diagonal parity position if RAID 6.
     */
    if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) && 
        (bitmap_log & RG_R6_MASK_LOG_DIAG) != 0)
    {
        bitmap_phys |= 1 << fbe_raid_siots_dp_pos(siots_p);
    }

    return bitmap_phys;
}
/****************************************
 * end fbe_logical_error_injection_convert_bitmap_log2phys()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_convert_bitmap_phys2log()
 ****************************************************************
 * @brief
 *  This function converts a physical bitmap to a logical one.
 * 

 * @param siots_p - pointer to a fbe_raid_siots_t struct
 * @param bitmap_phys - physical bitmap to be converted.
 *
 * @return
 *  logical bitmap. 
 *
 * @author
 *  5/2/06 - Created. SL
 *
 ****************************************************************/
static fbe_u16_t fbe_logical_error_injection_convert_bitmap_phys2log(fbe_raid_siots_t* const siots_p,
                                            fbe_u16_t bitmap_phys)
{
    fbe_u16_t bitmap_log = 0;
    fbe_s32_t idx;

    if (bitmap_phys == 0)
    {
        /* No need to loop over each bit if the bitmap is equal to zero.
         */
        return bitmap_log;
    }

    /* Loop over the physical bitmap one bit at a time.
     */
    for (idx = 0; idx < FBE_RAID_MAX_DISK_ARRAY_WIDTH; ++idx)
    {
        if ((bitmap_phys & (1 << idx)) != 0)
        {
            /* Check parity positions first because FBE_RAID_EXTENT_POSITION() 
             * expects a data position.
             */
            if (idx == fbe_raid_siots_get_row_parity_pos(siots_p))
            {
                /* For R5, this is the     parity position.
                 * For R6, this is the row parity position.
                 */
                bitmap_log |= RG_R6_MASK_LOG_ROW;
            }
            else if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) && 
                     idx == fbe_raid_siots_dp_pos(siots_p))
            {
                /* For R6, this is the diagonal parity position.
                 */
                bitmap_log |= RG_R6_MASK_LOG_DIAG;
            }
            else
            {
               fbe_status_t status = FBE_STATUS_OK;
               /* This is a data position. 
                */
                fbe_u32_t pos_logical;
                status = 
                    FBE_RAID_EXTENT_POSITION(idx, 
                                       siots_p->parity_pos, 
                                       fbe_raid_siots_get_width(siots_p), 
                                       fbe_raid_siots_parity_num_positions(siots_p),
                                       &pos_logical);
                if(LEI_COND(status != FBE_STATUS_OK))
                {
                    fbe_logical_error_injection_service_trace(
                        FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: logical error injection: status 0x%x != FBE_STATUS_OK\n",
                        __FUNCTION__, status);
                    return bitmap_log;
                }
                bitmap_log |= 1 << pos_logical;
            } /* end of if (idx...) */

        } /* end of if (bitmap_phys...)*/

    } /* end of for (idx...) */

    return bitmap_log;
}
/****************************************
 * end fbe_logical_error_injection_convert_bitmap_phys2log()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_match_err_region()
 ****************************************************************
 * @brief
 *  This function searches for matches between an fbe_xor_error_region_t from 
 *  XOR driver, and an fbe_xor_error_regions_t from the current error table. 
 * 

 * @param reg_xor_p - pointer to an fbe_xor_error_region_t  from XOR driver.
 * @param regs_tbl_p - pointer to an fbe_xor_error_regions_t from error table. 
 *
 * @return
 *  FBE_TRUE  - If a match of error regions is     found. 
 *  FBE_FALSE - If a match of error regions is not found.
 *
 * @author
 *  05/03/06 - Created. SL
 *
 ****************************************************************/
static fbe_bool_t fbe_logical_error_injection_match_err_region(const fbe_xor_error_region_t*  const reg_xor_p,
                                   const fbe_xor_error_regions_t* const regs_tbl_p)
{
    const fbe_bool_t b_c_xor = !(reg_xor_p->error & FBE_XOR_ERR_TYPE_UNCORRECTABLE);
    const fbe_u16_t      bitmap_from_xor = reg_xor_p->pos_bitmask;
    const fbe_xor_error_type_t error_from_xor = reg_xor_p->error;
    fbe_s32_t idx;
    fbe_bool_t b_match = FBE_FALSE;

    /* Loop over all error regions from error table's fbe_xor_error_regions_t.
     */
    for (idx = 0; idx < regs_tbl_p->next_region_index; ++idx)
    {
        const fbe_xor_error_region_t* const tbl_p = &regs_tbl_p->regions_array[idx];
        const fbe_bool_t b_c_tbl = !(tbl_p->error & FBE_XOR_ERR_TYPE_UNCORRECTABLE);
        const fbe_u16_t      bitmap_from_tbl = tbl_p->pos_bitmask;
        const fbe_xor_error_type_t error_from_tbl = tbl_p->error;

        if (b_c_xor == b_c_tbl && 
            (bitmap_from_xor & bitmap_from_tbl) != 0 && 
            error_from_xor == error_from_tbl)
        {
            /* We found a match if the following three conditions are met.
             * 1) Error types are both correctable or both uncorrectable.
             * 2) Error bitmaps have overlaps. 
             * 3) Error types match. 
             */
            b_match = FBE_TRUE;
            break;
        }
    } /* end of for (idx...) */

    return b_match;
}
/****************************************
 * end fbe_logical_error_injection_match_err_region()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_rec_evaluate()
 ****************************************************************
 *
 * @brief
 *  This function evaluates and determines whether an error record 
 *  is correctable, based on err_adj bitmask and the LUN status.
 * 

 * @param siots_p - pointer to a fbe_raid_siots_t struct
 * @param idx_rec - index of the error record to evaluate
 * @param err_adj_phys - Adjusted physical err_adj. 
 *
 * @return
 *  FBE_TRUE  - If the error is   correctable.
 *  FBE_FALSE - If the error is uncorrectable.
 *
 * ASSUMPTION:
 *  Each error record has a range, which controls the number of sectors 
 *  to inject error. When the ranges of error records overlap, multiple 
 *  errors are injected to the same strip. 
 *  If two error records overlap, they must overlap completely. 
 *  In the following examples, only Case1 is allowed.
 *
 *                    Case1     Case2     Case3
 *  LBA ----------->  |  |      |         |
 *                    |  |      |  |      |  |
 *                    R1 R2     R1 R2     R1 R2
 *                    |  |      |  |      |  |
 *  LBA + blocks -->  |  |      |            |
 *                     OK        NO        NO 
 *
 * @author
 *  05/12/06 - Created. SL
 *
 ****************************************************************/
fbe_bool_t fbe_logical_error_injection_rec_evaluate(fbe_raid_siots_t* const siots_p,
                                                    fbe_s32_t idx_rec,
                                                    fbe_u16_t err_adj_phys)
{
    fbe_logical_error_injection_record_t* rec_p = NULL;
    fbe_bool_t b_correctable = FBE_TRUE;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u16_t deg_bitmask;

    fbe_logical_error_injection_get_error_info(&rec_p, idx_rec);

    /*! All errors rae uncorrectable on Raid 0.
     */
    if (fbe_raid_geometry_is_raid0(raid_geometry_p))
    {
        b_correctable = FBE_FALSE;
        return b_correctable;
    }

    /* Remove degraded positions from the bitmap since we cannot inject 
     * error(s) to the degraded position(s).
     */
    fbe_raid_siots_get_degraded_bitmask((fbe_raid_siots_t* const)siots_p, &deg_bitmask);
    err_adj_phys &= ~deg_bitmask;

    if (err_adj_phys == 0)
    {
        /* If pos_bitmap happens to be a degraded postion, no error can be 
         * injected. In this case, the error is considered correctable.
         */
    }
    else
    {
        /* The error was injected. Determine whether it is correctable
         * based on the LUN status. 
         */
        fbe_u32_t dead_pos_cnt = fbe_raid_siots_dead_pos_count(siots_p);
        fbe_u32_t num_err_injected = fbe_raid_get_bitcount(err_adj_phys);
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_u32_t num_errs = dead_pos_cnt + num_err_injected;
        fbe_u32_t width;

        fbe_raid_geometry_get_width(raid_geometry_p, &width);

        if ( (num_errs <= 2) && 
             (fbe_raid_geometry_is_raid6(raid_geometry_p) ||
              (fbe_raid_geometry_is_mirror_type(raid_geometry_p) && (width > 2))) )
        {
            /* Up to two errors are only correctable if the LUN is a RAID 6 or 
             */
        }
        else if (num_errs <= 1)
        {
            /* One error is correctable for everyone else.
             */
        }
        else
        {
            /* This number of errors is correctable.
             */
            b_correctable = FBE_FALSE;
        }
    }

    return b_correctable;
}
/****************************************
 * end fbe_logical_error_injection_rec_evaluate()
 ****************************************/

/**********************************************************************
 * fbe_logical_error_injection_bitmap_to_position
 **********************************************************************
 * 
 * @brief
 *  This function converts a bitmap to a position.
 *

 * @param bitmap - bitmap to convert.
 *
 * @return
 *  position. 
 *
 * @author
 *  07/13/06 - Created. SL
 *  
 *********************************************************************/
fbe_s32_t fbe_logical_error_injection_bitmap_to_position(const fbe_u16_t bitmap)
{
    fbe_u32_t bit_inspect = (1 << 0); /* start with bit 0*/
    fbe_s32_t  position = 0;

    /* The bitmap must be non-zero.
     */
    if(LEI_COND(bitmap == 0))
    {
        fbe_logical_error_injection_service_trace(
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: logical error injection: bitmap 0x%x == 0\n",
            __FUNCTION__, bitmap);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (position = 0; position < 16; ++position)
    {
        if ((bitmap & bit_inspect) != 0)
        {
            break;
        }
        else
        {
            bit_inspect <<= 1;
        }
    }

    return position;
}
/****************************************
 * end fbe_logical_error_injection_bitmap_to_position()
 ****************************************/


/****************************************************************
 * fbe_logical_error_injection_is_coherency_err_type()
 ****************************************************************
 * @brief
 *  This function determines if the input error record is a
 *  coherency error.
 *
 * @param rec_p - Error record to check.
 *
 * @return
 *  FBE_TRUE  - If error rec is     a coherency error.
 *  FBE_FALSE - If error rec is not a coherency error.
 *
 * @author
 *  07/24/06 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t fbe_logical_error_injection_is_coherency_err_type( const fbe_logical_error_injection_record_t* const rec_p )
{
    fbe_bool_t b_return = FBE_FALSE;
    
    if (rec_p->err_type == FBE_XOR_ERR_TYPE_1POC ||
        rec_p->err_type == FBE_XOR_ERR_TYPE_INVALIDATED ||
        rec_p->err_type == FBE_XOR_ERR_TYPE_RAID_CRC ||
        rec_p->err_type == FBE_XOR_ERR_TYPE_CORRUPT_CRC)
    {
        /* For this error type, don't check the crc_det.
         */
        b_return = FBE_TRUE;
    }
    else if ( rec_p->crc_det == FBE_FALSE )
    {
        /* If record is crc detectable then it might be
         * a coherency error.
         * Next, check if error type is a coh type.
         */
        switch (rec_p->err_type)
        {
            case FBE_XOR_ERR_TYPE_1NS:
            case FBE_XOR_ERR_TYPE_1S:
            case FBE_XOR_ERR_TYPE_1R:
            case FBE_XOR_ERR_TYPE_1D:
            case FBE_XOR_ERR_TYPE_COH:
                /* Definitely a coherency error.
                 */
                b_return = FBE_TRUE;
                break;
            
            default:
                /* Not coherency, take default of FBE_FALSE.
                 */
                break;
        } /* end switch rec_p->error_type */
            
    } /* end if rec_p->crc_det == FBE_TRUE */
    return b_return;
}
/****************************************
 * end fbe_logical_error_injection_is_coherency_err_type()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_coherency_adjacent()
 ****************************************************************
 * @brief
 *  This function determines if there is a coherency error
 *  adjacent to this error in the table.. 
 * 

 * @param siots_p - Pointer to a fbe_raid_siots_t struct.
 * @param rec_p - Record ptr to start searching from.
 * @param width - Width of this LUN.
 * @param deg_bitmask - Current degraded bitmask.
 *
 * @return
 *  FBE_TRUE  - If an adjacent coherency error found.
 *  FBE_FALSE -    No adjacent coherency error found.
 *
 * @author
 *  07/24/06 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t fbe_logical_error_injection_coherency_adjacent(fbe_raid_siots_t* const siots_p,
                                                          fbe_logical_error_injection_record_t* const rec_p,
                                                          fbe_u16_t width,
                                                          fbe_u16_t deg_bitmask )
{
    fbe_u16_t error_mask;
    fbe_u16_t cur_bitmap_phys;

    cur_bitmap_phys = 
        fbe_logical_error_injection_convert_bitmap_log2phys(siots_p, rec_p->pos_bitmap);
    
    /* Get a mask for all array positions.
     */
    error_mask = (1 << width) - 1;

    /* We do not search for errors on the current position since we're
     * only interested in "adjacent" errors.
     * Degraded positions are not interesting since we didn't inject
     * an error on a degraded position.
     */
    error_mask &= ~(deg_bitmask | cur_bitmap_phys);

    /* Now perform the search.
     */
    return fbe_logical_error_injection_has_adjacent_type( siots_p,
                                     rec_p,
                                     fbe_logical_error_injection_is_coherency_err_type,
                                     error_mask );
}
/****************************************
 * end fbe_logical_error_injection_coherency_adjacent()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_has_adjacent_type()
 ****************************************************************
 * @brief
 *  This function searches for an adjacent error of the
 *  given error type, starting with the input record. 
 * 

 * @param siots_p - Pointer to a fbe_raid_siots_t struct.
 * @param rec_p - Record ptr to start from.
 * @param error_type - Error type to look for.
 * @param pos_bitmask - Position bitmask to look for.
 *
 * @return
 *  FBE_TRUE  - If an adjacent coherency error found.
 *  FBE_FALSE -    No adjacent coherency error found.
 *
 * @author
 *  07/24/06 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t fbe_logical_error_injection_has_adjacent_type( fbe_raid_siots_t* const siots_p,
                                                          fbe_logical_error_injection_record_t* const rec_p,
                                                          fbe_bool_t is_match_fn(const fbe_logical_error_injection_record_t *const),
                                                          fbe_u16_t pos_bitmask )
{
    fbe_bool_t b_found_type = FBE_FALSE;
    fbe_bool_t b_found_range = FBE_FALSE;
    fbe_logical_error_injection_record_t *start_rec_p = NULL;
    fbe_logical_error_injection_record_t *last_rec_p = NULL;

    /* We arbitrarily start our search 4 below the current record,
     * or the start of the table (if we're within 4 of the start).
     * 4 is an arbitrary number.
     */
    const fbe_logical_error_injection_record_t* current_rec_p = rec_p - 4;

    fbe_logical_error_injection_get_error_info(&start_rec_p, 0);
    fbe_logical_error_injection_get_error_info(&last_rec_p, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS-1);
    if ( current_rec_p < start_rec_p)
    {
        /* If we're close to the start, then we will start from the top.
         */
        current_rec_p = start_rec_p;
    }

    /* From this point onwards in the table,
     * find all records with a match of lba and pos_bitmask that are
     * not beyond the end of the table.
     */
    while ( current_rec_p < last_rec_p )
    {
        if ( current_rec_p->lba == rec_p->lba)
        {
            fbe_u16_t cur_bitmap_phys;

            cur_bitmap_phys = 
                fbe_logical_error_injection_convert_bitmap_log2phys(siots_p, current_rec_p->pos_bitmap);
            
            /* Found one of these lbas in the given range.
             */
            b_found_range = FBE_TRUE;
            
            if ( (cur_bitmap_phys & pos_bitmask) != 0 && 
                 is_match_fn(current_rec_p) )
            {
                /* We found an error type and position match.
                 */
                b_found_type = FBE_TRUE;
                break;
            }
        }
        else if ( b_found_range )
        {
            /* Found a match previously, but we're now
             * beyond the lbas we're interested in.
             * Just break now so we don't loop over the rest of the table.
             */
            break;
        }
        /* Advance to the next record.
         */
        current_rec_p++;
    } /* End while not beyond end of table. */
    
    return b_found_type;
}
/****************************************
 * end fbe_logical_error_injection_has_adjacent_type()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_remove_poc()
 ****************************************************************
 * @brief
 *  This function removes all POC errors from the current error table 
 *  if poc_injection is equal to FBE_FALSE.
 *
 * @param None.
 *
 * @return fbe_status_t
 * 
 * @author
 *  08/23/06 - Created. SL
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_remove_poc(void)
{
    fbe_s32_t idx;
    fbe_bool_t b_poc_injection;

    fbe_logical_error_injection_get_poc_injection(&b_poc_injection);

    /* If poc_injection is equal to FBE_TRUE, POC errors are to be injected. 
     * Do nothing to the error table. 
     */
    if (b_poc_injection == FBE_TRUE)
    {
        return FBE_STATUS_OK;
    }

    /* Loop over all error records of the current error table.
     */
    for (idx = 0; idx < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; ++idx)
    {
        fbe_logical_error_injection_record_t * rec_p = NULL;

        fbe_logical_error_injection_get_error_info(&rec_p, idx);
        /* FBE_XOR_ERR_TYPE_NONE marks the end of the error table. 
         */
        if (rec_p->err_type == FBE_XOR_ERR_TYPE_NONE)
        {
            break;
        }

        /* When pos_bitmap is equal to 0, the error injection code will 
         * not find a match for this error record. Thus the error is 
         * never injected. 
         */
        if (rec_p->err_type == FBE_XOR_ERR_TYPE_1POC)
        {
            rec_p->pos_bitmap = 0;
        }
    }

    /* Loop over the error table to recalculate all err_adj.
     */
    fbe_logical_error_injection_setup_err_adj();

    return FBE_STATUS_OK;
}
/****************************************
 * end fbe_logical_error_injection_remove_poc()
 ****************************************/

/****************************************************************
 * fbe_logical_error_injection_fruts_span_err_tbl_max_lba()
 ****************************************************************
 * @brief
 *  This function determined whether the maximum range of the fruts
 *  chain straddles the error table max_lba. 
 *

 * @param fruts_head_p - Head of the fruts chain to examine. 
 * @param b_chain - Whether to examine the entire fruts chain. 
 * @param max_lba - From the global service.
 *
 * @return
 *  FBE_TRUE  - Fruts range          spans max_lba. 
 *  FBE_FALSE - Fruts range does not span  max_lba.
 *
 * @author
 *  09/14/06 - Created. SL
 *
 ****************************************************************/
fbe_bool_t fbe_logical_error_injection_fruts_span_err_tbl_max_lba(fbe_raid_fruts_t* const fruts_head_p,
                                                                  fbe_bool_t b_chain)
{
    fbe_raid_siots_t   *siots_p = fbe_raid_fruts_get_siots(fruts_head_p);
    fbe_lba_t           adjustment_value = 0;
    fbe_lba_t           range_start = FBE_LBA_INVALID;
    fbe_lba_t           range_end = 0;
    fbe_bool_t          b_span = FBE_FALSE; /* By default, range not span max_lba. */
    fbe_bool_t          b_enabled;
    fbe_lba_t           max_table_lba;
    fbe_lba_t           start_lba;
    fbe_raid_fruts_t   *fruts_p = NULL;

    /* If error table is not enabled, range does not span max_lba.
     */
    fbe_logical_error_injection_get_enable(&b_enabled);
    if (b_enabled == FBE_FALSE)
    {
        return b_span;
    }

    /* Initialize variables.
     */
    fbe_logical_error_injection_get_max_lba(&max_table_lba);
    fbe_logical_error_injection_get_lba_adjustment(siots_p, &adjustment_value);

    /* If there is a chain process it.
     */
    if (b_chain)
    {
        /* Find the maximum range of the entire fruts chain. 
         */
        for (fruts_p  = fruts_head_p; 
             fruts_p != NULL; 
             fruts_p = fbe_raid_fruts_get_next(fruts_p))
        {
            /* Get the start lba including adjustment.
             */
            if (fruts_p->lba < adjustment_value)
            {
                /* Something is wrong.  Trace and error and return `spans'.
                 */
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                          FBE_TRACE_MESSAGE_ID_INFO, 
                                                          "lei: check span: siots_p: %p fruts lba: 0x%llx is less than adjust: 0x%llx \n",
                                                          siots_p, (unsigned long long)fruts_p->lba, (unsigned long long)adjustment_value);
                return FBE_TRUE;
            }

            /* Subtract the adjustment.
             */
            start_lba = fruts_p->lba - adjustment_value;

            /* Update range as required.
             */
            if (start_lba < range_start)
            {
                range_start = start_lba;
            }

            if ((start_lba + fruts_p->blocks - 1) > range_end)
            {
                range_end = start_lba + fruts_p->blocks - 1;
            }
        }
    }
    else
    {
        /* Only the current fruts need to be examined.
         */ 
        fruts_p = fruts_head_p;
        if (fruts_p->lba < adjustment_value)
        {
            /* Something is wrong.  Trace and error and return `spans'.
             */
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                                      "LEI: check span: siots_p: %p fruts lba: 0x%llx is less than adjust: 0x%llx \n",
                                                      siots_p, (unsigned long long)fruts_p->lba, (unsigned long long)adjustment_value);
            return FBE_TRUE;
        }

        /* Subtract the adjustment.
         */
        start_lba = fruts_p->lba - adjustment_value;

        range_start = start_lba;
        range_end = start_lba + fruts_head_p->blocks - 1;
    }

    /* Adjust the start and end addresses before making the comparison.
     * This is how the error injection code works. 
     */
    range_start = range_start % max_table_lba;
    range_end   = range_end % max_table_lba;
    if (range_start > range_end)
    {
        b_span = FBE_TRUE;
    }

    return b_span;
}
/*************************************************************
 * end fbe_logical_error_injection_fruts_span_err_tbl_max_lba()
 *************************************************************/

/*!**************************************************************
 * @fn fbe_logical_error_injection_init_log(void)
 ****************************************************************
 * @brief
 *  Initialize the log of media errors.
 *
 * @param  - None.          
 *
 * @return - None.
 *
 * @author
 *  12/8/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_logical_error_injection_init_log(void)
{
    fbe_logical_error_injection_service_t * service_p = fbe_get_logical_error_injection_service();
    if (service_p->log_p == NULL)
    {
        /* Simply malloc the log and initialize it to zeros.
         */
        service_p->log_p = fbe_memory_native_allocate(sizeof(fbe_logical_error_injection_log_t));

        if (service_p->log_p == NULL)
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                      "%s unable to allocate log",
                                                      __FUNCTION__);
        }
        else
        {
            fbe_zero_memory(service_p->log_p,sizeof(fbe_logical_error_injection_log_t));
        }
    } /* end if error sim log not allocated. */
    return;
}
/******************************************
 * end fbe_logical_error_injection_init_log()
 ******************************************/

/*!**************************************************************
 * @fn fbe_logical_error_injection_add_log(void)
 ****************************************************************
 * @brief
 *  Add to the log of media errors.
 *
 * @param  - None.          
 *
 * @return - None.
 *
 * @author
 *  12/8/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_logical_error_injection_add_log(fbe_lba_t lba,
                                         fbe_u32_t err_type,
                                         fbe_u32_t position,
                                         fbe_u32_t opcode,
                                         fbe_u32_t unit)
{
    fbe_logical_error_injection_service_t * service_p = fbe_get_logical_error_injection_service();
    if (service_p->log_p != NULL)
    {
        fbe_logical_error_injection_log_record_t *rec_p;
        rec_p = &service_p->log_p->log[service_p->log_p->index];

        /* Copy values into record.
         */
        rec_p->lba = lba;
        rec_p->err_type = err_type;
        rec_p->position = position;
        rec_p->opcode = opcode;
        rec_p->unit = unit;

        /* Increment the index.  If necessary wrap around to zero.
         */
        service_p->log_p->index++;
        if (service_p->log_p->index >= FBE_LOGICAL_ERROR_INJECTION_MAX_LOG_RECORDS)
        {
            service_p->log_p->index = 0;
        }
    } /* end if error sim log not allocated. */
    return;
}
/******************************************
 * end fbe_logical_error_injection_add_log()
 ******************************************/

/*!***************************************************************************
 *          fbe_logical_error_injection_get_downstream_object()
 ***************************************************************************** 
 * 
 * @brief   For the position requested, get the downstream logical injection
 *          object (if it exist).
 *
 * @param   siots_p - Pointer to request to get downstream object for.
 * @param   position - Position in raid group to get object for.
 *
 * @return  object_p - Either NULL or valid downstream error injection object.
 *
 * @author
 *  05/08/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_logical_error_injection_object_t *fbe_logical_error_injection_get_downstream_object(fbe_raid_siots_t *const siots_p,
                                                                                        fbe_u32_t position)
{
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_edge_t       *block_edge_p = NULL;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_object_id_t         object_id = FBE_OBJECT_ID_INVALID;
    fbe_logical_error_injection_object_t *object_p = NULL;

    /* Validate position.
     */
    if (position >= width)
    {
        /* Trace an error and return NULL.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s siots_p: %p width: %d pos: %d invalid. \n",
                                                  __FUNCTION__, siots_p, width, position);
        return NULL;
    }

    /* Get the server id for this position.
     */
    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, position);
    if ((block_edge_p == NULL)                                       ||
        (block_edge_p->base_edge.server_id == 0)                     ||
        (block_edge_p->base_edge.server_id == FBE_OBJECT_ID_INVALID)    )
    {
        /* Trace an error and return NULL.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s siots_p: %p width: %d pos: %d no downstream.\n",
                                                  __FUNCTION__, siots_p, width, position);
        return NULL;
    }

    /* Check if there is an error injection object associated with this position.
     */
    object_id = block_edge_p->base_edge.server_id;
    object_p = fbe_logical_error_injection_find_object(object_id);

    /* Return the object.
     */
    return object_p;
}
/*********************************************************
 * end fbe_logical_error_injection_get_downstream_object()
 *********************************************************/

/*!***************************************************************************
 *          fbe_logical_error_injection_get_upstream_object()
 ***************************************************************************** 
 * 
 * @brief   Get the upstream logical error injeciton object (if it exist).
 *
 * @param   fruts_p - Pointer to fruts to locate object from
 *
 * @return  object_p - Either NULL or valid upstream error injection object.
 *
 * @author
 *  08/10/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_logical_error_injection_object_t *fbe_logical_error_injection_get_upstream_object(fbe_raid_fruts_t *const fruts_p)
{
    fbe_raid_siots_t       *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_iots_t        *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_object_id_t         object_id = FBE_OBJECT_ID_INVALID;
    fbe_logical_error_injection_object_t *object_p = NULL;
    fbe_packet_t           *packet_p  = NULL;
    fbe_packet_t           *master_packet_p  = NULL;

    /* Get the packet associated with the siots.  This was the packet received
     * from upsteam.
     */
    packet_p = fbe_raid_iots_get_packet(iots_p);

    /* Get the master packet of the packet received from upstream.
     * This originated from the upstream object.
     */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    if (master_packet_p == NULL)
    {
        return NULL;
    }

    /* Get the upstream object id from the master packet.
     */
    fbe_transport_get_object_id(master_packet_p, &object_id);

    /* Now find the object id using the siots
     */
    object_p = fbe_logical_error_injection_find_object(object_id);

    /* Return the upstream object
     */
    return object_p;
}
/*********************************************************
 * end fbe_logical_error_injection_get_upstream_object()
 *********************************************************/


/***************************************************
 * end of fbe_logical_error_injection_record_mgmt.c
 ***************************************************/
