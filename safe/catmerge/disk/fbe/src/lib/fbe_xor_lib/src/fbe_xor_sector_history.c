/***************************************************************************
 * Copyright (C) EMC Corporation 2005 - 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file xor_sector_history.c
 ***************************************************************************
 *
 * @brief
 *  This file contains code to help save copies of sectors for 
 *  debugging purposes.
 *
 * TABLE OF CONTENTS
 *  xor_sector_history_lock
 *  xor_sector_history_unlock
 *  fbe_xor_sector_history_save
 *  fbe_xor_sector_history_trace
 *  fbe_xor_sector_history_trace_vector
 *  fbe_xor_sector_history_init
 *  xor_sh_fill_record
 *  xor_sh_trace_sector()
 *  xor_sh_trace
 *  xor_sh_get_oldest_index
 *  xor_sh_get_record
 *  xor_sh_calculate_fru_number
 *  xor_sh_find_match
 *  xor_sh_records_equal
 *  xor_sh_get_pos_redundancy_group
 *  
 * @author
 *  06/30/2005 - Created Rob Foley.
 *
 ***************************************************************************/

/*************************
 **   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_sector_trace_interface.h"
#include "fbe_xor_sector_history.h"
#include "fbe_xor_private.h"
#include "fbe/fbe_library_interface.h"

/****************************
 *  static VARIABLES
 ****************************/

/****************************
 *  static FUNCTIONS
 ****************************/

static fbe_status_t fbe_xor_sh_fill_record( fbe_xor_sector_history_record_t *const info_p,
                                    const fbe_lba_t lba, 
                                    const fbe_u32_t pos_bitmask, 
                                    const fbe_u32_t num_diff_bits_in_crc,
                                    const fbe_object_id_t raid_group_object_id,
                                    const fbe_lba_t raid_group_offset, 
                                    const fbe_sector_t * const sect_p );
void fbe_xor_sh_trace( const fbe_xor_sector_history_record_t * const info_p,
                       const char *const error_string_p,
                       const fbe_sector_trace_error_level_t error_level, 
                       const fbe_sector_trace_error_flags_t error_flag);
static fbe_status_t fbe_xor_sh_calculate_drive_object_id(const fbe_xor_sector_history_record_t * const rec_p,
                                                         fbe_object_id_t *drive_object_id_p);
static fbe_u32_t fbe_xor_sh_get_pos_redundancy_group( fbe_u32_t pos_bitmask );

/****************************************
 * MACROS
 ****************************************/


/****************************************
 * METHODS
 ****************************************/

/************************************************************
 *  fbe_xor_report_error()
 ************************************************************
 *
 * @brief   Report that an error was found.
 *
 * @param   error_string_p - Error string to display in trace.
 * @param   error_level - Levels of severity or debug of the event being reported.
 * @param   error_flag - Type of tracing error of the event being reported.
 *                   
 * @return fbe_status_t
 *
 * @author
 *  11/14/2013  Ron Proulx  - Created.
 *
 ************************************************************/
fbe_status_t fbe_xor_report_error(const char * const error_string_p,
                                  const fbe_sector_trace_error_level_t error_level,
                                  const fbe_sector_trace_error_flags_t error_flag )
{
    /* Only bother if the tracing is enabled!!
     */
    if (!fbe_sector_trace_is_enabled(error_level, error_flag))
    {
        return FBE_STATUS_OK;
    }

    if (!fbe_sector_trace_can_trace(1)) {
        /* Not time to trace this.
         */
        return FBE_STATUS_OK;
    }
    /* Log the detailed error description string.
     */
    fbe_sector_trace_entry(error_level, error_flag,
                           FBE_SECTOR_TRACE_INITIAL_PARAMS,
                           "xor: %s Event Detected - Error Level: %d Error Flags: 0x%x \n",
                           FBE_SECTOR_TRACE_LOCATION_BUFFERS,
                           error_string_p, error_level, error_flag); 

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_xor_report_error()
 ******************************************/

/************************************************************
 *  fbe_xor_trace_sector()
 ************************************************************
 *
 * @brief   Save a copy of the sector data in the xor trace
 *          buffer.
 *
 * @param lba - Lba of block.
 * @param pos_bitmask - 1 << position in redundancy group.
 * @param num_diff_bits_in_crc [I] - The CRC computed from the data 
 *                            differs from the CRC in the 
 *                            metadata by this many bits.
 * @param raid_group_object_id - The object identifier of the raid group that
 *                               encountered the error
 * @param raid_group_offset - The raid group offset
 * @param sect_p - pointer to sector to save. We assume it
 *                  is already mapped in.
 * @param error_string_p - Error string to display in trace.
 * @param error_level - Levels of severity or debug of the event being reported.
 * @param error_flag - Type of tracing error of the event being reported.
 *                   
 *
 * @notes
 *  It is recommended that any trace levels with this tracing 
 *  are NOT be enabled by default in free builds, since  
 *  this mode of tracing can get very verbose.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/14/2013  Ron Proulx  - Created.
 *
 ************************************************************/
fbe_status_t fbe_xor_trace_sector(const fbe_lba_t lba, 
                               const fbe_u32_t pos_bitmask,
                               const fbe_u32_t num_diff_bits_in_crc, 
                               const fbe_object_id_t raid_group_object_id,
                               const fbe_lba_t raid_group_offset, 
                               const fbe_sector_t * const sect_p,
                               const char * const error_string_p,
                               const fbe_sector_trace_error_level_t error_level,
                               const fbe_sector_trace_error_flags_t error_flag )
{
    fbe_status_t status;
    fbe_xor_sector_history_record_t info;

    /* Only bother if the tracing is enabled!!
     */
    if ( !(fbe_sector_trace_is_enabled(error_level, error_flag)) ||
          (pos_bitmask == FBE_XOR_INV_DRV_POS)    )
    {
        return FBE_STATUS_OK;
    }

    /* Fill out the record we just retrieved with the information passed in.
     */
    status = fbe_xor_sh_fill_record( &info, lba, pos_bitmask, num_diff_bits_in_crc,
                                     raid_group_object_id, raid_group_offset, sect_p );
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* If tracing is enabled then we display the block
     * to the appropriate ktrace buffer.
     */
    fbe_xor_sh_trace( &info, /* Information to save. */
                  error_string_p, /* Error string to use. */
                  error_level, /* trace level */ 
                  error_flag /* error type */);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_xor_trace_sector()
 ******************************************/

/************************************************************
 *          fbe_xor_trace_sector_complete()
 ************************************************************
 *
 * @brief   Tell the sector trace service that we are done
 *          processing this error.
 * 
 * @param   error_level - The error level of this trace
 * @param   error_flag - Type of tracing error of the event being reported.
 *                   
 * @return fbe_status_t
 *
 * @author
 *  04/10/2014  Ron Proulx  - Created.
 *
 ************************************************************/
fbe_status_t fbe_xor_trace_sector_complete(const fbe_sector_trace_error_level_t error_level,
                                           const fbe_sector_trace_error_flags_t error_flag)
{
    /* Only bother if the tracing is enabled!!
     */
    if (!fbe_sector_trace_is_enabled(error_level, error_flag)) {
        return FBE_STATUS_OK;
    }

    /* Tell the sector trace service that we are done tracing.
     */
    fbe_sector_trace_check_stop_on_error(error_flag);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_xor_trace_sector_complete()
 ******************************************/

/************************************************************
 *  fbe_xor_report_error_trace_sector()
 ************************************************************
 *
 * @brief   Report the error and aave a copy of this sector to
 *          the ktrace.
 *
 * @param lba - Lba of block.
 * @param pos_bitmask - 1 << position in redundancy group.
 * @param num_diff_bits_in_crc [I] - The CRC computed from the data 
 *                            differs from the CRC in the 
 *                            metadata by this many bits.
 * @param raid_group_object_id - The object identifier of the raid group that
 *                               encountered the error
 * @param raid_group_offset - The raid group offset
 * @param sect_p - pointer to sector to save. We assume it
 *                  is already mapped in.
 * @param error_string_p - Error string to display in trace.
 * @param error_level - Levels of severity or debug of the event being reported.
 * @param error_flag - Type of tracing error of the event being reported.
 *                   
 *
 * @notes
 *  It is recommended that any trace levels with this tracing 
 *  are NOT be enabled by default in free builds, since  
 *  this mode of tracing can get very verbose.
 *
 * @return fbe_status_t
 *
 * History:
 *  08/22/05:  Rob Foley -- Created.
 *  04/16/08:  RDP -- Updated to use new tracing facility.
 *  03/22/10:  Omer Miranda -- Updated to use the new sector tracing facility.
 *  11/14/2013  Ron Proulx  - Renamed and updated.
 *
 ************************************************************/
fbe_status_t fbe_xor_report_error_trace_sector( const fbe_lba_t lba, 
                               const fbe_u32_t pos_bitmask,
                               const fbe_u32_t num_diff_bits_in_crc, 
                               const fbe_object_id_t raid_group_object_id,
                               const fbe_lba_t raid_group_offset, 
                               const fbe_sector_t * const sect_p,
                               const char * const error_string_p,
                               const fbe_sector_trace_error_level_t error_level,
                               const fbe_sector_trace_error_flags_t error_flag )
{
    fbe_status_t status;

    /* Only bother if the tracing is enabled!!
     */
    if ( !(fbe_sector_trace_is_enabled(error_level, error_flag)) ||
          (pos_bitmask == FBE_XOR_INV_DRV_POS)    )
    {
        return FBE_STATUS_OK;
    }

    /* Report the error.
     */
    status = fbe_xor_report_error(error_string_p, error_level, error_flag);
    if (status != FBE_STATUS_OK)
    {
        /* Return the status
         */
        return status;
    }

    /* Trace the sector
     */
    status = fbe_xor_trace_sector(lba, pos_bitmask, num_diff_bits_in_crc, 
                                  raid_group_object_id, raid_group_offset, sect_p,
                                  error_string_p, error_level, error_flag);

    /* Notify the sector trace service that we are done processing this error.
     */
    status = fbe_xor_trace_sector_complete(error_level, error_flag);

    return status;
}
/******************************************
 * end fbe_xor_report_error_trace_sector()
 ******************************************/

/************************************************************
 *  fbe_xor_sector_history_trace_vector()
 ************************************************************
 *
 * @brief
 *  Save a copy of this sector to the ktrace.
 *

 * @param lba - Lba of block.
 * @param sector_p - vector of sectors to trace. We assume it
 *                    is already mapped in.
 * @param pos_bitmask - vector of positions
 *                       1 << position in redundancy group.
 * @param ppos - Parity position for this array.
 * @param raid_group_object_id - The object identifier of the raid group that
 *                               encountered the error
 * @param raid_group_offset - The raid group offset
 * @param error_string_p - Error string to display in trace.
 * @param error_level - trace error level. 
 * @param error_flag - trace error type.
 *
 * @notes
 *  It is recommended that any trace levels with this tracing 
 *  should NOT be enabled by default in free builds, since  
 *  this mode of tracing can get very verbose and could
 *  wrap the standard ktrace buffer.
 *
 * @return fbe_status_t
 *
 *
 * History:
 *  08/22/05:  Rob Foley -- Created.
 *  04/16/08:  RDP -- Updated to use new tracing facility.
 *  03/22/10:  Omer Miranda -- Updated to use the new sector tracing facility.
 *
 ************************************************************/
fbe_status_t fbe_xor_sector_history_trace_vector( const fbe_lba_t lba, 
                                      const fbe_sector_t * const sector_p[],
                                      const fbe_u16_t pos_bitmask[],
                                      const fbe_u16_t width,
                                      const fbe_u16_t ppos,
                                      const fbe_object_id_t raid_group_object_id,
                                      const fbe_lba_t raid_group_offset, 
                                      char *error_string_p,
                                      const fbe_sector_trace_error_level_t error_level,
                                      const fbe_sector_trace_error_flags_t error_flag)
{
    fbe_status_t status;
    char *display_err_string_p;
    fbe_u32_t array_position;
    
    /* If the correct tracing level is not enabled, just return
     * since there is nothing for us to do.
     */
    if (!fbe_sector_trace_is_enabled(error_level, error_flag))
    {
        return FBE_STATUS_OK;
    }

    /* Report the error.
     */
    status = fbe_xor_report_error(error_string_p, error_level, error_flag);
    if (status != FBE_STATUS_OK)
    {
        /* Return the status
         */
        return status;
    }

    /* Loop over the entire width, tracing all.
     */
    for ( array_position = 0; array_position < width; array_position++ )
    {
		char parity_error_string[FBE_XOR_SECTOR_HISTORY_ERROR_MAX_CHARS];

        display_err_string_p = error_string_p;
                
        if ( ppos != FBE_XOR_INV_DRV_POS && ppos == array_position )
        {
            sprintf((char*) parity_error_string, "parity %s", display_err_string_p);

            /* For parity display a bit more with the error string.
             */
            display_err_string_p = (char *)parity_error_string;
            
        }
        status = fbe_xor_trace_sector( lba,  /* Lba */
                                  pos_bitmask[array_position], /* Bitmask of position in group. */
                                  0, /* The number of crc bits in error isn't known */
                                  raid_group_object_id,
                                  raid_group_offset, 
                                  sector_p[array_position], /* Sector to save. */
                                  display_err_string_p, /* Error type. */ 
                                  error_level, /* Tracing level to use */
                                  error_flag /* Tracing error type */);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }
    
    /* Notify the sector trace service that we are done processing this error.
     */
    status = fbe_xor_trace_sector_complete(error_level, error_flag);

    return status;
}
/****************************************
 * end fbe_xor_sector_history_trace_vector()
 ****************************************/


/************************************************************
 *  fbe_xor_sh_fill_record()
 ************************************************************
 *
 * @brief
 *  Save information into the info_p sector history record.
 *

 * @param info_p - information record to fill out with info.
 * @param lba - Lba of block.
 * @param pos_bitmask - 1 << position in redundancy group.
 * @param num_diff_bits_in_crc [I] - Number of bits that the computed CRC
 *                        differs from that read from the metadata.
 * @param raid_group_object_id - The object identifier of the raid group that
 *                               encountered the error.
 * @param raid_group_offset - the offset for this raid group
 * @param sect_p - pointer to sector to save. We assume it
 *                 is already mapped in.
 *
 * @return fbe_status_t
 *
 * History:
 *  06/30/05:  Rob Foley -- Created.
 *
 ************************************************************/

static fbe_status_t fbe_xor_sh_fill_record( fbe_xor_sector_history_record_t *const info_p,
                               const fbe_lba_t lba, 
                               const fbe_u32_t pos_bitmask, 
                               const fbe_u32_t num_diff_bits_in_crc,
                               const fbe_object_id_t raid_group_object_id,
                               const fbe_lba_t raid_group_offset, 
                               const fbe_sector_t * const sect_p )
{
    fbe_u32_t position_in_redundancy_group = fbe_xor_sh_get_pos_redundancy_group( pos_bitmask );
    fbe_status_t status;
    
    /* Fill the info ptr here now that we have a ptr.
     */
    memcpy( &info_p->sector, sect_p, sizeof(fbe_sector_t) );
    
    /* Set the time stamp on the information ptr.
     */
    info_p->time_stamp = fbe_get_time();
    info_p->last_time_stamp = fbe_get_time();
    
    /* Save the set of fields that needs no translation.
     */
    info_p->lba = lba;
    info_p->position = position_in_redundancy_group;
    info_p->raid_group_object_id = raid_group_object_id;
    info_p->pba = fbe_xor_convert_lba_to_pba(lba, raid_group_offset);
    info_p->num_diff_bits_in_crc = num_diff_bits_in_crc;

    
    /* Hit count is set to zero since this is a new sector and
     * we haven't seen another hit on this block.
     */
    info_p->hit_count = 0;
    
    /*! @todo Currently there is no way to get the vd object id, but
     *        position should be sufficient.
     */
     status = fbe_xor_sh_calculate_drive_object_id( info_p, &info_p->drive_object_id);
     if (status != FBE_STATUS_OK) 
     {
         return status;
     }

#if 0
    /* Try a few basic sanity tests of the lba information.
     * If the unit is valid, make sure the lba is at least the address 
     * offset of the unit.
     */
    if (XOR_COND((unit != RAID_IORB_INVALID_UNIT) &&
                 (lba  < dba_unit_get_adjusted_addr_offset_phys(adm_handle, unit, 0))))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "(unit 0x%x != 0x%x) && "
                              "(lba 0xll < unit relative address 0x%ll)\n",
                              unit,
                              RAID_IORB_INVALID_UNIT,
                              dba_unit_get_adjusted_addr_offset_phys(adm_handle, unit, 0));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the lba does not go beyond the fru capacity.
     * If the drive is dead then capacity is 0, so we
     * can't verify the lba.  Only Raid 6 will go through this code,
     * since only RAID 6 will check the crc of rebuilt data.
     */
    if (XOR_COND((lba >= dba_fru_get_capacity(adm_handle, info_p->fru_number)) &&
                 (dba_fru_get_capacity( adm_handle, info_p->fru_number ) != 0 )))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: %s (lba 0x%ll > fru capacity 0x%ll) && "
                              "(fru capacity != 0)\n",
                              lba,
                              dba_fru_get_capacity(adm_handle, info_p->fru_number));

        return FBE_STATUS_GENERIC_FAILURE;
    }


#endif

    return FBE_STATUS_OK;
}
/****************************************
 * end fbe_xor_sh_fill_record()
 ****************************************/

/************************************************************
 *  fbe_xor_sh_trace()
 ************************************************************
 *
 * @brief
 *  Trace a copy of this sector and some other useful
 *  information related to sector trace to the ktrace buffer.
 *

 * @param info_p - pointer to sector information to trace.
 * @param error_string_p - The type of error to display.
 * @param tracing_level - If this tracing level is enabled
 *                         then we will display the block.
 *                         Defined in flare_debug_tracing.h
 * @param error_flag - Type of tracing error.
 *
 * @return
 *  none
 *
 * History:
 *  08/11/05:  Rob Foley -- Created.
 *  04/16/08:  RDP -- Updated to use new error tracing facility.
 *  03/22/10:  Omer Miranda -- Updated to use the sector tracing facility.
 ************************************************************/

void fbe_xor_sh_trace( const fbe_xor_sector_history_record_t * const info_p,
                   const char *const error_string_p,
                   const fbe_sector_trace_error_level_t error_level,
                   const fbe_sector_trace_error_flags_t error_flag)
{
    const fbe_sector_t * const sector_p = &info_p->sector;

    if (!fbe_sector_trace_can_trace(1 + FBE_SECTOR_TRACE_ENTRIES_PER_SECTOR)) {
        /* Not time to trace this.
         */
        return;
    }
    /* Log the detailed error description string, the trace identifier,
     * the unit, lba, position and fru.
     */
    fbe_sector_trace_entry(error_level,error_flag,
                           FBE_SECTOR_TRACE_PARAMS,
                           "xor: rg object id: 0x%x position: %d pba(w/o ofst): 0x%llx  pba: 0x%llx\n",
                           FBE_SECTOR_TRACE_LOCATION_BUFFERS, 
                           info_p->raid_group_object_id, info_p->position, info_p->lba, info_p->pba);
    
    /* Finally, display the sector.
     */
    fbe_xor_sector_trace_sector(error_level, error_flag, sector_p);
    
    return;
}
/****************************************
 * end fbe_xor_sh_trace()
 ****************************************/

/************************************************************
 *  fbe_xor_sh_calculate_drive_object_id()
 ************************************************************
 *
 * @brief
 *  Determine the drive (virtual drive) object id for a given
 *  sector history record.
 *
 * @param rec_p - pointer to record to calculate drive object id for
 * @param drive_object_id_p - Address of drive object id to populate
 *
 * ASSUMPTIONS:
 *  Note that this function assumes that unit, redundancy_group and
 *  position are calculated already in info_p.
 *
 * @return fbe_status_t
 *  
 *
 * History:
 *  08/18/05:  Rob Foley -- Created.
 *
 ************************************************************/
static fbe_status_t fbe_xor_sh_calculate_drive_object_id(const fbe_xor_sector_history_record_t * const rec_p,
                                                         fbe_object_id_t *drive_object_id_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       drive_position_relative_to_group;
    fbe_object_id_t drive_object_id = FBE_OBJECT_ID_INVALID;
    

    /*! @todo Currently we don't generate a valid virtual drive object id.
     */
    drive_position_relative_to_group = rec_p->position;
    *drive_object_id_p = drive_object_id;

    /* Always return the execution status.
     */
    return(status);
}
/***********************************************
 * end of fbe_xor_sh_calculate_drive_object_id() 
 ***********************************************/

/************************************************************
 *  fbe_xor_sh_get_pos_redundancy_group()
 ************************************************************
 *
 * @brief
 *  Convert a bitmap into a position in a redundancy group.
 *
 * @param pos_bitmap - Bitmap with one bit set indicating
 *                    position in the array.
 *
 * @return
 *  fbe_u32_t - the position in the redundancy group.
 *
 * History:
 *  08/23/05:  Rob Foley -- Created.
 *
 ************************************************************/

static fbe_u32_t fbe_xor_sh_get_pos_redundancy_group( fbe_u32_t pos_bitmask )
{
    
    fbe_u32_t position_in_redundancy_group = 0;

    /* position needs to be converted from a bitmask
     * to a bit position.  Just calculate which bit number is set.
     */
    while ( pos_bitmask > 1 ) 
    {
        pos_bitmask >>= 1;
        position_in_redundancy_group++;
    }
    return position_in_redundancy_group;
}
/****************************************
 * end fbe_xor_sh_get_pos_redundancy_group()
 ****************************************/

/*!***************************************************************************
 *          fbe_xor_sector_history_generate_sector_trace_info()
 *****************************************************************************
 *
 * @brief   Determines (based on priority) the sector trace error level and 
 *          sector trace error flag.
 *
 * @param   err_type        [I] - The unique XOR error identifier
 * @param   uncorrectable   [I] - FBE_TRUE if the error was uncorrectable
 *                                Used to determine trace level
 * @param   sector_trace_level_p - Address of sector trace level to update
 * @param   sector_trace_flags_p - Address of sector trace flags to update
 * @param   error_string_p  [I/O] - Address to set return string value to.
 *
 * @note    Only those eboard errors that are valid are currently handled.
 *          If different error types will be assumed add them at the correct
 *          priority below.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * History:
 *  04/23/2008:  RDP -- Created.
 *  10/29/2008:  RDP -- Updated to correct issue with RAID-6 errors
 *****************************************************************************/

fbe_status_t fbe_xor_sector_history_generate_sector_trace_info(const fbe_xor_error_type_t err_type,
                                                      const fbe_bool_t uncorrectable,
                                                      fbe_sector_trace_error_level_t *const sector_trace_level_p,
                                                      fbe_sector_trace_error_flags_t *const sector_trace_flags_p,
                                                      const char **error_string_p)
{
    /*! @note The following are the default values.
     */
    fbe_sector_trace_error_level_t  trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR;
    fbe_sector_trace_error_flags_t  trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_NONE;

    /* Simply use the error type to determine the 
     * trace type.
     */
    switch(err_type)
    {
        case FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_RND_MEDIA_ERR:
            trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING;
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "Media Error";
            break;

        case FBE_XOR_ERR_TYPE_CRC:
        case FBE_XOR_ERR_TYPE_KLOND_CRC:
        case FBE_XOR_ERR_TYPE_DH_CRC:
        case FBE_XOR_ERR_TYPE_CORRUPT_CRC:
        case FBE_XOR_ERR_TYPE_COPY_CRC:
        case FBE_XOR_ERR_TYPE_PVD_METADATA:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_UCRC;
            *error_string_p = "CRC Error";
            break;
        
        case FBE_XOR_ERR_TYPE_RAID_CRC:
        case FBE_XOR_ERR_TYPE_INVALIDATED:
            /* RAID has corrupted the CRC due to a previous error!
             * Therefore DO NOT report this error!
             */
            trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_INFO;
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_RINV;
            *error_string_p = "Invalidated CRC";
            break;

        case FBE_XOR_ERR_TYPE_WS:
        case FBE_XOR_ERR_TYPE_BOGUS_WS:
            trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING;
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_WS;
            *error_string_p = "Write Stamp Error";
            break;

        case FBE_XOR_ERR_TYPE_TS:
        case FBE_XOR_ERR_TYPE_BOGUS_TS:
            trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING;
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_TS;
            *error_string_p = "Time Stamp Error";
            break;

        case FBE_XOR_ERR_TYPE_SS:
        case FBE_XOR_ERR_TYPE_BOGUS_SS:
            trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING;
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_SS;
            *error_string_p = "Shed Stamp Error";
            break;

        case FBE_XOR_ERR_TYPE_1NS:
            trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING;
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "1NS Error";
            break;

        case FBE_XOR_ERR_TYPE_1S:
            trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING;
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "1S Error";
            break;

        case FBE_XOR_ERR_TYPE_1R:
            trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING;
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "1R Error";
            break;

        case FBE_XOR_ERR_TYPE_1D:
            trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING;
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_RCRC;
            *error_string_p = "1D Error";
            break;

        case FBE_XOR_ERR_TYPE_1COD:
            /* All coherency errors are error level.
             */
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
            *error_string_p = "1COD Error";
            break;

        case FBE_XOR_ERR_TYPE_1COP:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
            *error_string_p = "1COP Error";
            break;

        case FBE_XOR_ERR_TYPE_1POC:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
            *error_string_p = "1POC Error";
            break;

        case FBE_XOR_ERR_TYPE_COH:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
            *error_string_p = "Coherency";
            break;

        case FBE_XOR_ERR_TYPE_CORRUPT_DATA:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_UCRC;
            *error_string_p = "Corrupt Data";
            break;

        case FBE_XOR_ERR_TYPE_N_POC_COH:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_NPOC;
            *error_string_p = "Multiple POC";
            break;

        case FBE_XOR_ERR_TYPE_POC_COH:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_POC;
            *error_string_p = "POC COH Error";
            break;

        case FBE_XOR_ERR_TYPE_COH_UNKNOWN:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_UCOH;
            *error_string_p = "Unknown coherency";
            break;

        case FBE_XOR_ERR_TYPE_RB_FAILED:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_RAID;
            *error_string_p = "Rebuild Failed";
            break;

        case FBE_XOR_ERR_TYPE_LBA_STAMP:
            trace_flags = FBE_SECTOR_TRACE_ERROR_FLAG_LBA;
            *error_string_p = "LBA Stamp Error";
            break;

        default:
            /* For all other value use the default trace id.
             */
            *error_string_p = "Generic Error";
            break;
     
    } /* end switch on err_type */

    /* If the uncorrectable flag is set, change the level
     * to error.
     */
    if (uncorrectable)
    {
        trace_level = FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR;
    }

    /* Now update the values requested
     */
    *sector_trace_level_p = trace_level;
    *sector_trace_flags_p = trace_flags;

    /* Status is always success
     */
    return(FBE_STATUS_OK);
}
/********************************************************
 * end fbe_xor_sector_history_generate_sector_trace_info()
 ********************************************************/

/***************************************************************************
 * END xor_sector_history.c
 ***************************************************************************/
