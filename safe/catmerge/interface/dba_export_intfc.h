#ifndef DBA_EXPORT_INTFC_H
#define DBA_EXPORT_INTFC_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  dba_export_intfc.h
 ***************************************************************************
 *
 *      Description
 *
 *         This file contains the definitions for functions and macros
 *         used by the portion of the interface to the
 *         Database Access "dba" library in the Flare driver that is used
 *         outside of Flare.
 *
 *  History:
 *
 *      01/21/08 JED    Split from dba_exports.h
 *      10/02/01 LBV    Add fru_flag ODBS_QUEUED_SWAP
 *      10/17/01 CJH    move defines from dba_api.h
 *      11/01/01 HEW    Added ifdefs for C++ compilation
 *      04/17/02 HEW    Added CALL_TYPE to decl for dba_frusig_get_cache
 *      17-SEP-2009 DE  Removed redundant dba_fru_is_fru_flags_bit() definition
 *                      Ensured that dba_fru_is_np_flags_bit() and 
 *                      dba_fru_is_fru_flags_bit() return a BOOL type
 *
 ***************************************************************************/

#include "generics.h"
#include "k10defs.h"
#include "cm_config_exports.h"
#include "vp_exports.h"


#ifndef DBA_FORCE_INTERFACE_MACROS

// The second check below is a temporary patch to get the Admin lib to link,
// until a solution is available that will allow it to work with the macros
//
// 12/17/07 - JMM - removing the #if defined, so that RG_USE_DB_INTERFACE_FUNCTIONS
// will be defined at all times.  This allows the handlers to access DBA functions
// and fixes compile/link problems with existing components that are relying on the
// macros. 
//#if defined(LAB_DEBUG)  ||  defined(ADMIN_ENV)  ||  defined(NON_FLARE_ENV)
#define RG_USE_DB_INTERFACE_FUNCTIONS
//#endif

#endif // DBA_FORCE_INTERFACE_MACROS

// Macros for these are NOt defined yet
void dba_fru_resync_num_luns(UINT_32 fru);

/***************************
 *  Macro Declarations
 ***************************/

#define DBA_LU_DERIVE_DEFAULT_ASSIGN(m_handle, m_unit)  \
    (dba_unit_get_lu_attributes((m_handle), (m_unit)) & \
                                     LU_DEFAULT_ASSIGN_MASK)

    /* The macro above accepts the index of a glut structure as unit
     * and returns one of the default assign values
     * listed.  This indicates who is the default assigner of this
     * unit: the odd or even controller.
     * NOTE: for downward Flare-rev compatibility, beware of changing
     * these values!
     */


/* This macro returns TRUE if a unit is currently performing a background
 * verify.
 */
#define DBA_LU_BACKGROUND_VERIFY_NEEDED(m_handle, m_unit) \
    ((((ADM_DATA *)m_handle)->glut_tabl[m_unit].unit_type & \
                              (LU_RAID5_DISK_ARRAY | \
                               LU_RAID6_DISK_ARRAY | \
                               LU_FAST_RAID3_ARRAY | \
                               LU_DISK_MIRROR | \
                               LU_RAID10_DISK_ARRAY | \
                               LU_STRIPED_DISK | \
                               LU_DISK)) && \
     (((ADM_DATA *)m_handle)->glut_tabl[m_unit].flags & NEEDS_BV))

/* This macro returns TRUE if a unit is currently performing 
 * a  read only backgroundverify.
 */
#define DBA_LU_IS_READ_ONLY_BACKGROUND_VERIFY(m_handle, m_unit) \
    ((((ADM_DATA *)m_handle)->glut_tabl[m_unit].unit_type & \
                              (LU_RAID5_DISK_ARRAY | \
                               LU_RAID6_DISK_ARRAY | \
                               LU_FAST_RAID3_ARRAY | \
                               LU_DISK_MIRROR | \
                               LU_RAID10_DISK_ARRAY | \
                               LU_STRIPED_DISK | \
                               LU_DISK)) && \
     (((ADM_DATA *)m_handle)->glut_tabl[m_unit].flags & NEEDS_BV) && \
     (((ADM_DATA *)m_handle)->glut_tabl[m_unit].flags & READONLY_BV))

/*
 * This macro cancels a BG Verify on the specified unit
 */

#define DBA_LU_CANCEL_BG_VERIFY(m_unit) \
    dba_unit_unset_flags_bit( m_unit, (NEEDS_BV|READONLY_BV|RESTART_BV) )

/*
 * This macro triggers a BG Verify on the specified unit
 */

#define DBA_LU_TRIGGER_BG_VERIFY(m_unit) \
{                                                               \
    dba_unit_unset_flags_bit( m_unit, READONLY_BV );            \
    dba_unit_set_flags_bit( m_unit, (NEEDS_BV|RESTART_BV) );    \
}

/*
 * This macro triggers a Read-only BG Verify on the specified unit
 */

#define DBA_LU_TRIGGER_READONLY_BG_VERIFY(m_unit)    \
    dba_unit_set_flags_bit( m_unit, (NEEDS_BV|RESTART_BV|READONLY_BV) )

/* These Macros are used to determine the address offset and
 * array width of logical units.  It will check to see if an
 * expansion is occuring, and if so, will determine if the
 * request is above or below the expansion checkpoint.
 */

#define DBA_LU_ABOVE_EXP_CHKPT(_handle, _lun, _l_addr)  \
    (((_l_addr) >=  ((ADM_DATA *)(_handle))-> \
        raid_group_tabl[((ADM_DATA *)(_handle))-> \
        glut_tabl[_lun].group_id].logical_exp_chkpt) ? \
     TRUE : FALSE)

#define DBA_LU_ADDRESS_OFFSET(_handle, _unit, _addr)  \
    ((dba_unit_is_expanding((_handle), (_unit)) && \
      DBA_LU_ABOVE_EXP_CHKPT((_handle), (_unit), (_addr))) ? \
     ((ADM_DATA *)(_handle))->glut_tabl[_unit].pre_exp_addr_offset : \
     ((ADM_DATA *)(_handle))->glut_tabl[_unit].address_offset)

#define DBA_LU_ARRAY_WIDTH(_handle, _unit, _addr)  \
    ((dba_unit_is_expanding((_handle), (_unit)) && \
      DBA_LU_ABOVE_EXP_CHKPT((_handle), (_unit), (_addr))) ? \
     dba_unit_get_pre_exp_width(_handle, _unit) : \
     ((ADM_DATA *)(_handle))->glut_tabl[_unit].num_frus)

#define DBA_LU_ABOVE_EXP_CHKPT_PHYS(_handle, _lun, _p_addr)  \
    (((_p_addr) > ((ADM_DATA *)(_handle))-> \
        raid_group_tabl[((ADM_DATA *)(_handle))-> \
        glut_tabl[_lun].group_id].physical_exp_chkpt) ? \
     TRUE : FALSE)

#define DBA_LU_ADDRESS_OFFSET_PHYS(_handle, _unit, _addr)  \
    ((dba_unit_is_expanding((_handle), (_unit)) && \
      DBA_LU_ABOVE_EXP_CHKPT_PHYS(_handle, _unit, _addr)) ? \
         ((ADM_DATA *)(_handle))->glut_tabl[_unit].pre_exp_addr_offset : \
         ((ADM_DATA *)(_handle))->glut_tabl[_unit].address_offset)

#define DBA_LU_ARRAY_WIDTH_PHYS(_handle, _unit, _addr)  \
    ((dba_unit_is_expanding((_handle), (_unit)) && \
      DBA_LU_ABOVE_EXP_CHKPT_PHYS(_handle, _unit, _addr)) ? \
         dba_unit_get_pre_exp_width(_handle, _unit) : \
         ((ADM_DATA *)(_handle))->glut_tabl[_unit].num_frus)

/************************************************************
 * The following functions DO NOT have a matching macros.   *
 * since they are not invoked too often or too complex to   *
 * be converted to macros.                                  *
 * "RG_USE_DB_INTERFACE_FUNCTIONS" is not checked in the    *
 * following functions. Both checked and free built will    *
 * compile the following functions.                         *
 ************************************************************/

// returns bg verify checkpoint for the specified raid group
IMPORT LBA_T CALL_TYPE dba_raidg_get_bg_verify_chkpt( OPAQUE_PTR handle, UINT_32 group );

// returns sniff verify checkpoint for the specified raid group
IMPORT LBA_T CALL_TYPE dba_raidg_get_sniff_verify_chkpt( OPAQUE_PTR handle, UINT_32 group );

/* Returns the index (fru position) in the RAID group of the disk to select for rebuilding */
IMPORT UINT_32 dba_raidg_get_rebuild_index(OPAQUE_PTR handle, raid_group_t group);

/* Returns the position of a mirror disk within the array.
 * Used only by the Raid Driver.
 */
IMPORT UINT_32 CALL_TYPE dba_raidg_get_mirror_fru_pos(OPAQUE_PTR handle,
                                                      UINT_32 group,
                                                      INT_16 mirror_pos1,
                                                      UINT_32 fru_position);

IMPORT BOOL CALL_TYPE dba_raidg_is_state_flag_bit(OPAQUE_PTR handle,
                                                  UINT_32 group,
                                                  raid_group_flags_T state_flags);
//Check if the specific stage flag bit in set
IMPORT BOOL CALL_TYPE dba_raidg_is_state_flag_bit64(ULONG64 handle,
                                 UINT_32 group,
                                 raid_group_flags_T state_flags);
//64 Bit implemenation of dba_raidg_is_state_flag_bit


IMPORT BOOL CALL_TYPE dba_raidg_is_group_assigned(OPAQUE_PTR handle,
                                                  UINT_32 group_id);
/* returns TRUE if any units in group are assigned */

IMPORT BOOL CALL_TYPE dba_raidg_is_group_peer_assigned(OPAQUE_PTR handle,
                                                  UINT_32 group_id);
/* returns TRUE if any units in group are assigned on the peer*/

IMPORT UINT_32 CALL_TYPE dba_raidg_get_partition_index(OPAQUE_PTR handle,
                                                       UINT_32 group,
                                                       UINT_32 unit);
// get partition index in raid group of unit passed in

IMPORT UINT_32 CALL_TYPE dba_raidg_get_fru_pos(OPAQUE_PTR handle,
                                               UINT_32 group,
                                               UINT_32 fru);

// Returns TRUE if all units in the raid group are idle, or FALSE if any
// unit is not idle.
IMPORT BOOL dba_raidg_is_idle(OPAQUE_PTR handle, UINT_32 group);

// Returns the position of the fru within the raid group

IMPORT UINT_32 CALL_TYPE dba_raidg_get_hot_fru(OPAQUE_PTR handle,
                                               UINT_32 group,
                                               UINT_32 fru);
// returns the hot fru number in a group for fru

IMPORT UINT_32 CALL_TYPE dba_raidg_get_hot_fru_type(OPAQUE_PTR handle,
                                                    UINT_32 group,
                                                    UINT_32 fru);
// returns the hot fru type in a group for fru

IMPORT LBA_T dba_raidg_get_lun_zero_chkpt( OPAQUE_PTR handle, UINT_32 group );
// return LUN Zero checkpoint for specific RAID group

IMPORT dba_bgs_enable_flags_bitfield_t dba_raidg_get_bgs_enable_flags(OPAQUE_PTR handle,
                                     raid_group_t group,
                                     bgs_monitor_types_enum_t bgs_monitor_type);
// Get the RG-level BGS enable flags on a background service

IMPORT BOOL dba_raidg_is_bgs_enable_flags_bit(OPAQUE_PTR handle,
                                     raid_group_t group,
                                     bgs_monitor_types_enum_t bgs_monitor_type,
                                     dba_bgs_enable_flags_bitfield_t bgs_enable_flags_mask);
// Return TRUE if the queried BGS enable flags are set on RG "group" for a background service

IMPORT BOOL dba_raidg_is_bgs_enabled(OPAQUE_PTR handle,
                                     raid_group_t group,
                                     bgs_monitor_types_enum_t bgs_monitor_type);
// Return TRUE if a background service is enabled, both on the RG level and the SP level,
//  for a specified RG

IMPORT BOOL dba_raidg_is_rebuilding(OPAQUE_PTR handle, UINT_32 group);
// returns true if a rebuild could run on this RAID group

IMPORT BOOL dba_raidg_is_degraded(OPAQUE_PTR handle, UINT_32 group);
// returns true if the RAID group is in degraded state

IMPORT BOOL dba_raidg_is_rg_type_compatible(OPAQUE_PTR handle, UINT_32 group1, UINT_32 group2);
//  return TRUE if 2 RG types are compatible.

IMPORT dba_bgs_enable_flags_bitfield_t dba_fru_get_bgs_enable_flags(OPAQUE_PTR handle,
                                     disk_t fru, 
                                     bgs_monitor_types_enum_t bgs_monitor_type);
// Get the disk-level BGS enable flags on a background service

IMPORT BOOL dba_fru_is_bgs_enable_flags_bit(OPAQUE_PTR handle,
                                     disk_t fru,
                                     bgs_monitor_types_enum_t bgs_monitor_type,
                                     dba_bgs_enable_flags_bitfield_t bgs_enable_flags_mask);
// Return TRUE if the queried BGS enable flags are set on disk for a background service

BOOL dba_fru_is_bgs_enabled(OPAQUE_PTR handle,
                            disk_t fru,
                            bgs_monitor_types_enum_t bgs_monitor_type);
// Return TRUE if a background service is enabled, both on the disk level and the SP level,
// for a specified disk

IMPORT fru_partition_flags_T CALL_TYPE
    dba_fru_get_partition_flags(OPAQUE_PTR handle,
                                UINT_32 fru,
                                UINT_32 partition);
// returns the fru partition flags

IMPORT BOOL CALL_TYPE
    dba_fru_is_partition_flags_bit(OPAQUE_PTR handle,
                                   UINT_32 fru,
                                   UINT_32 partition,
                                   fru_partition_flags_T pflags);
// returns TRUE if any of the specified partition flag bits are set


IMPORT LBA_T CALL_TYPE dba_fru_get_partition_checkpoint(OPAQUE_PTR handle,
                                                        UINT_32 fru,
                                                        UINT_32 partition);
// returns the fru partition checkpoint
IMPORT LBA_T CALL_TYPE dba_fru_get_partition_checkpoint64(ULONG64 handle,
                                       UINT_32 fru,
                                       UINT_32 partition);


IMPORT BOOL CALL_TYPE dba_fru_get_keep_fru_dead_value(OPAQUE_PTR handle,
                                                     UINT_32 fru);
// get the value set only when the FRU has been deemed unstable

IMPORT UINT_32 CALL_TYPE dba_fru_get_time_of_first_fru_death(OPAQUE_PTR handle, 
                                                            UINT_32 fru, 
                                                            FRU_STABILITY_FAULT_TYPE fault_type);
// get  the time that this FRU first displayed unstable behavior

IMPORT UINT_32 CALL_TYPE dba_fru_get_num_deaths(OPAQUE_PTR handle, 
                                                UINT_32 fru,
                                                FRU_STABILITY_FAULT_TYPE fault_type);
// get the number of times that this FRU has displayed unstable behavior for the specified fault type.


IMPORT UINT_32 CALL_TYPE dba_fru_get_pbc_asserted_count(OPAQUE_PTR handle,
                                                        UINT_32 fru);
// get the number of times that this FRU has displayed unstable behavior

IMPORT UINT_32 CALL_TYPE dba_fru_get_interval_of_last_fru_death(OPAQUE_PTR handle, 
                                                                UINT_32 fru,
                                                                FRU_STABILITY_FAULT_TYPE fault_type);
// get the interval between unstable removes

IMPORT UINT_32 CALL_TYPE dba_fru_get_time_of_last_fru_death(OPAQUE_PTR handle, UINT_32 fru, FRU_STABILITY_FAULT_TYPE fault_type);

// get the time that this FRU last displayed unstable behavior

IMPORT UINT_32 CALL_TYPE dba_fru_get_drive_change_timeout_id(OPAQUE_PTR handle,
                                                        UINT_32 fru);
// get the timeout id that is set when processing an unstable remove

IMPORT BOOL CALL_TYPE dba_fru_is_probation(OPAQUE_PTR handle,
                                           UINT_32 fru);
// Check if the specified FRU is in probation or not.

IMPORT BOOL CALL_TYPE dba_fru_is_pd_init(OPAQUE_PTR handle,
                                         UINT_32 fru);
// Check if the specified FRU is in PD_INIT state or not.

IMPORT BOOL CALL_TYPE dba_fru_is_pd_stopping(OPAQUE_PTR handle,
                                               UINT_32 fru);
// Check if the specified FRU is pd stopping.

IMPORT BOOL CALL_TYPE dba_fru_is_pd_cancelling(OPAQUE_PTR handle,
                                               UINT_32 fru);
// Check if the specified FRU is in PD cancel.

IMPORT BOOL CALL_TYPE dba_fru_is_pd_powering_up(OPAQUE_PTR handle,
                                                UINT_32 fru);
// Check if the probational FRU is PD powering up or not.

IMPORT BOOL CALL_TYPE dba_fru_is_pd_active(OPAQUE_PTR handle,
                                                UINT_32 fru);
// Check if the specified FRU is PD_ACTIVE.


IMPORT PROBATIONAL_DRIVE_STATE CALL_TYPE dba_fru_get_pd_state(OPAQUE_PTR handle,
                                                              UINT_32 fru);
// Returns the probation state of the FRU.

IMPORT UINT_32 CALL_TYPE dba_fru_get_pd_sniff_count(OPAQUE_PTR handle,
                                                    UINT_32 fru);
//Returns the PD sniff count.


IMPORT void* CALL_TYPE dba_fru_get_pd_cm_element(OPAQUE_PTR handle,
                                                   UINT_32 fru);
//Returns the PD cm element.

IMPORT UINT_32 CALL_TYPE dba_fru_find_glut_index(OPAQUE_PTR handle,
                                                 UINT_32 frunum,
                                                 LBA_T phy_addr);
// Takes a fru number and physical address and searches for what unit
// the physical address falls in.

IMPORT UINT_32 CALL_TYPE dba_fru_get_group_id_index(OPAQUE_PTR handle,
                                                    UINT_32 fru,
                                                    UINT_32 group_id);
// Get index to group id array in fru table that matches passed in group id


IMPORT UINT_32 CALL_TYPE dba_fru_get_group_id_count(OPAQUE_PTR handle,
                                                    UINT_32 fru,
                                                    raid_attributes_T attr);
// return the number of raid group that with specified attr for a fru

IMPORT DRIVE_DEAD_REASON CALL_TYPE dba_fru_get_death_reason(OPAQUE_PTR handle,
                                               UINT_32 fru);                       
// returns the drive death reason to the caller

IMPORT UINT_32 CALL_TYPE dba_fru_get_dh_error_code(OPAQUE_PTR handle,
                                               UINT_32 fru);                      
// return the error code that caused DH to kill the drive

IMPORT void CALL_TYPE dba_fru_get_serial_number(OPAQUE_PTR handle,
                                               UINT_32 fru,TEXT *buffer,UINT_32 buffer_size);     
// Transfers the serial number of the of the drive into passed buffer 


IMPORT DRIVE_DEAD_REASON CALL_TYPE dba_fru_get_death_reason(OPAQUE_PTR handle,
                                               UINT_32 fru);
// Get death reason of the specified fru.

/* Get the number of valid LU's in user or system space.
 */
IMPORT UINT_32 CALL_TYPE
    dba_fru_get_num_luns_in_user_system_space(OPAQUE_PTR handle,
                                              UINT_32 fru,
                                              UINT_32 attribute);


IMPORT fru_host_state_T CALL_TYPE dba_fru_get_host_state(OPAQUE_PTR handle,
                                                         UINT_32 fru);
// returns fru host state
IMPORT fru_host_state_T CALL_TYPE dba_fru_get_host_state64(ULONG64 handle,
                                                         UINT_32 fru);
//64 Bit implementation of dba_fru_get_host_state

IMPORT BOOL CALL_TYPE dba_fru_is_rebuilding_complete(OPAQUE_PTR handle, UINT_32 fru);

IMPORT BOOL CALL_TYPE dba_fru_is_rebuilding_complete64(ULONG64 handle, UINT_32 fru);

IMPORT UINT_32 CALL_TYPE dba_fru_get_next_user_lun(OPAQUE_PTR handle,
                                                   const UINT_32 fru,
                                                   const UINT_32 unit);
// Returns the next USER lun in the fru partition table



IMPORT UINT_16E CALL_TYPE dba_fru_get_partition_index(OPAQUE_PTR handle,
                                                      UINT_32 fru,
                                                      UINT_32 unit);
// return the partition index of the unit on this fru.

IMPORT UINT_32 CALL_TYPE dba_fru_get_invalid_lun_count(OPAQUE_PTR handle,
                                                       UINT_32 fru);
// return the number of invalid lun partitons on this fru.


IMPORT BOOL CALL_TYPE dba_fru_is_fru_a_hotspare(UINT_32 fru);
// Returns True if fru is a hotspare, else returns False.

IMPORT UINT_32 CALL_TYPE dba_unit_get_hot_fru(OPAQUE_PTR handle,
                                              UINT_32 unit,
                                              UINT_32 index);
// get hot fru array in the unit

IMPORT BOOL CALL_TYPE dba_unit_is_rebuilding(OPAQUE_PTR handle, UINT_32 unit);
// returns true if a rebuild could run on this unit

IMPORT BOOL CALL_TYPE dba_unit_is_fru_present(OPAQUE_PTR handle,
                                              UINT_32 unit,
                                              UINT_32 fru);
// returns true if the given fru is a part the the unit in question

/* Returns the FED priority of the specified unit. */
IMPORT DBA_FED_UNIT_PRIORITY CALL_TYPE
    dba_unit_get_fed_priority(OPAQUE_PTR handle, UINT_32);

IMPORT BOOL CALL_TYPE dba_unit_any_bound_units(OPAQUE_PTR handle);
// To go through the glut table to see if there are any bound units.


IMPORT UINT_32 CALL_TYPE dba_unit_get_num_rebuilds_needed(OPAQUE_PTR handle,
                                                          UINT_32 unit);
// Returns the number of frus that are marked as needing a rebuild

IMPORT UINT_32 CALL_TYPE dba_unit_get_stripe_size(OPAQUE_PTR handle,
                                                  UINT_32 unit);
// Calculates the stripe size for this unit

IMPORT UINT_16E CALL_TYPE dba_unit_get_fru_partition_index(OPAQUE_PTR handle,
                                                           UINT_32 unit);
// get the partition index of the unit for this FRU.
// Same as CALL_TYPE dba_unit_get_fru_position() followed by
// CALL_TYPE dba_unit_get_partition_index()

IMPORT UINT_32 CALL_TYPE dba_unit_get_fru_position(OPAQUE_PTR handle,
                                                   UINT_32 unit,
                                                   UINT_32 fru);
// returns the position of the fru in the fru array of the unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_num_degraded_frus(OPAQUE_PTR handle,
                                                        UINT_32 unit);
// get the number of degraded disks in a unit

//  Get the checkpoints to adjust when LUN capacity needs to be changed, and their 
//  associated data 
BOOL dba_unit_get_all_checkpoints_to_adjust
(
    OPAQUE_PTR                          handle, 
    lu_t                                unit, 
    void*                               buffer_p,
    UINT_32                             buffer_size
);

//  Populates a table with the new checkpoints needed after a capacity change completes 
BOOL dba_unit_populate_new_checkpoint_values
(
    OPAQUE_PTR                      handle, 
    lu_t                            unit, 
    void*                           buffer_p
); 

//  Determines if the given checkpoint falls within the given LUN 
BOOL dba_unit_is_checkpoint_within_lun
(
    OPAQUE_PTR                          handle, 
    lu_t                                unit, 
    LBA_T                               checkpoint
); 

// Get corona end of unit after BGS RG Defrag in Release 31; used by BGS RG Expansion/Defrag service
IMPORT LBA_T dba_unit_exp_def_get_corona_end_after_defrag_r31(OPAQUE_PTR adm_handle, UINT_32 lun);

IMPORT dba_bgs_enable_flags_bitfield_t dba_unit_get_bgs_enable_flags(OPAQUE_PTR handle,
                                     lu_t unit,
                                     bgs_monitor_types_enum_t bgs_monitor_type);
// Get the LU-level BGS enable flags on a background service

IMPORT BOOL dba_unit_is_bgs_enable_flags_bit(OPAQUE_PTR handle,
                                     lu_t unit,
                                     bgs_monitor_types_enum_t bgs_monitor_type,
                                     dba_bgs_enable_flags_bitfield_t bgs_enable_flags_mask);
// Return TRUE if the queried BGS enable flags are set on LU "unit" for a background service

IMPORT BOOL dba_unit_is_bgs_enabled(OPAQUE_PTR handle,
                                     lu_t unit,
                                     bgs_monitor_types_enum_t bgs_monitor_type);
// Return TRUE if a background service is enabled, on both the LU level and the SP level,
//  for a specified unit. The RG-level enable for the Raid Group which the LU is in is also
//  considered if this is a bound unit.

//calculates new capacity based on passed capacity
IMPORT LBA_T CALL_TYPE dba_calculate_new_internal_capacity(OPAQUE_PTR handle,
														   LBA_T capacity_in, 
														   UINT_32 unit);

IMPORT BOOL dba_unit_is_unit_state_broken(OPAQUE_PTR handle, UINT_32 unit, BOOL fixlun_flag);
// return TRUE if the unit state is broken

IMPORT UINT_32 CALL_TYPE dba_fru_get_hs_unit(OPAQUE_PTR handle,
                                             const UINT_32 fru);

IMPORT BOOL CALL_TYPE dba_fru_is_a_swapped_in_spare( OPAQUE_PTR handle,
                                             UINT_32 fru,
                                             BOOL *invalid_group_p,
                                             BOOL *invalid_lun_p,
                                             DBA_SWAP_RQ_TYPE swap_rq_type);
IMPORT BOOL CALL_TYPE dba_fru_is_spared(OPAQUE_PTR handle, UINT_32 fru,
                                        DBA_SWAP_RQ_TYPE swap_rq_type);

IMPORT BOOL CALL_TYPE dba_fru_is_spared_in_group(OPAQUE_PTR handle,
                                                 UINT_32 fru,
                                                 UINT_32 group,
                                                 DBA_SWAP_RQ_TYPE req_type);

IMPORT UINT_32 CALL_TYPE dba_fru_get_real_fru(OPAQUE_PTR handle, UINT_32 fru,
                                              DBA_SWAP_RQ_TYPE spare_req_type);
// get real fru

IMPORT UINT_32 CALL_TYPE dba_fru_get_bus(OPAQUE_PTR handle, UINT_32 fru, DBA_SWAP_RQ_TYPE req_type);

IMPORT UINT_32 CALL_TYPE dba_fru_get_device_id(OPAQUE_PTR handle, UINT_32 fru, DBA_SWAP_RQ_TYPE req_type);
// get the device id field in the fru

IMPORT BOOL dba_fru_is_hot_sparing( OPAQUE_PTR handle, UINT_32 fru);

IMPORT BOOL dba_fru_is_proactive_sparing( OPAQUE_PTR handle, UINT_32 fru);
// returns TRUE if group is a user group and frru is spared

IMPORT BOOL dba_fru_is_partition_fully_rebuilt(OPAQUE_PTR adm_handle, UINT_32 fru,
                                        UINT_32 partition);

IMPORT BOOL dba_fru_needs_rebuild(OPAQUE_PTR handle, UINT_32 fru);
// returns true if this fru needs to be rebuilt

IMPORT BOOL dba_fru_is_needs_rebuild_marked_or_in_progress(OPAQUE_PTR handle, UINT_32 fru);
// returns true if this fru is marked NR or in the process of being marked NR

IMPORT BOOL dba_fru_is_db_rebuilding(OPAQUE_PTR handle, UINT_32 fru);
// returns true if a database rebuild could run on this fru

IMPORT BOOL dba_fru_is_rebuilding(OPAQUE_PTR handle, UINT_32 fru);
// returns true if a rebuild could run on this fru

IMPORT BOOL dba_fru_is_rebuilding_raidg(OPAQUE_PTR handle, UINT_32 fru, UINT_32 group);
// returns true if a rebuild could run on this RAID group on this fru

IMPORT BOOL dba_fru_needs_eqz_or_paco(OPAQUE_PTR handle, UINT_32 hs_fru);
// returns TRUE if the FRU has more LUNs to eqz/paco.

IMPORT BOOL dba_fru_is_swap_in_progress(OPAQUE_PTR handle, disk_t fru);
// returns true if disk is in process of being swapped

IMPORT disk_t  dba_fru_get_swapping_with_disk(OPAQUE_PTR handle, disk_t fru);

IMPORT BOOL dba_fru_sanity_check_proactive_candidate(OPAQUE_PTR handle, UINT_32 fru, UINT_32* error);
// return TRUE if disk can be proactively spared, FALSE otherwise. reason for failure is set in error variable.

IMPORT BOOL dba_fru_can_rg_allow_proactive_sparing(OPAQUE_PTR handle, UINT_32 fru);
//  return TRUE if atleast one unit on disk is ENABLED or PEER ENABLED

IMPORT BOOL dba_fru_is_proactive_sparing_active_in_rg(OPAQUE_PTR handle, UINT_32 fru, UINT_32* error);
//  return TRUE if proactive sparing is active in RG of disk.

IMPORT  BOOL dba_fru_can_ps_swap_in(OPAQUE_PTR handle, UINT_32 fru, UINT_32* error);
//  return TRUE if PS can swap in

IMPORT  BOOL dba_fru_get_available_ps_with_same_rg_type(OPAQUE_PTR handle, disk_t fru, UINT_32* num_spares_with_same_raid_type);
//  return TRUE if 1 or more than 1 PS are available to swap in.

IMPORT BOOL dba_fru_get_total_ps_with_same_rg_type(OPAQUE_PTR handle, disk_t fru, UINT_32* total_num_of_spares);
//  return TRUE if only 1 spare is bound in the system.

IMPORT BOOL dba_fru_check_stable(OPAQUE_PTR handle, disk_t disk);
//  return TRUE if no unit on disk is unbinding or shutting down.

IMPORT K10_WWID CALL_TYPE dba_unit_get_lu_wwn(OPAQUE_PTR handle,
                                              UINT_32 unit);
// gets the world wide name for the unit

IMPORT void CALL_TYPE dba_unit_get_lu_name(OPAQUE_PTR handle,
                                           UINT_32 unit,
                                           K10_LOGICAL_ID *lu_name);
IMPORT UINT_32 dba_unit_wwn_to_lun(K10_WWID wwid);

IMPORT LBA_T dba_unit_get_corona_end(OPAQUE_PTR adm_handle, UINT_32 lun);

/*
 * SYSCONFIG_DB functions
 */
IMPORT BOOL CALL_TYPE dba_sysconfig_get_chrb_enabler_installed(OPAQUE_PTR handle);

IMPORT REVISION_NUMBER CALL_TYPE dba_sysconfig_get_rev_num(OPAQUE_PTR handle);
/* Flare revision number (obsolete) */

IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_system_id(OPAQUE_PTR handle,
                                                     UINT_32 buffer_size,
                                                     TEXT *buffer);
/* System serial number.  The get function returns the actual length */

/* Product Part Number */
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_product_pn(OPAQUE_PTR handle, 
                                                    UINT_32 buffer_size, 
                                                    TEXT *buffer);

/* Product Revision */
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_product_rev(OPAQUE_PTR handle, 
                                                    UINT_32 buffer_size, 
                                                    TEXT *buffer);
/* Peer CM Revision */
IMPORT UINT_32 dba_sysconfig_get_peer_cm_revision(OPAQUE_PTR handle);

/* Local CM Revision */
IMPORT UINT_32 dba_sysconfig_get_local_cm_revision(OPAQUE_PTR handle);



/* Memory configuration */
IMPORT UINT_32 CALL_TYPE
    dba_sysconfig_get_write_cache_size(OPAQUE_PTR handle);
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_read_cache_size(OPAQUE_PTR handle,
                                                           UINT_32 sp);

/* Cache configuration */
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_cache_page_size(OPAQUE_PTR handle);
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_hi_watermark(OPAQUE_PTR handle);
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_lo_watermark(OPAQUE_PTR handle);

/* indicates mirrored vs non-mirrored cache */
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_cache_config(OPAQUE_PTR handle);

/* Option configuration */
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_system_type(OPAQUE_PTR handle);
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_config_options(OPAQUE_PTR handle);
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_misc_flags(OPAQUE_PTR handle,
                                                      UINT_32 sp);
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_addressing_model(OPAQUE_PTR handle);

/* Miscellaneous values */
IMPORT UINT_32 CALL_TYPE
    dba_sysconfig_get_unsol_resend_period(OPAQUE_PTR handle,
                                          UINT_32 sp);
IMPORT BBU_TIME CALL_TYPE dba_sysconfig_get_bbu_timeout(OPAQUE_PTR handle);
IMPORT LCC_REV_TIME CALL_TYPE dba_sysconfig_get_lcc_timeout(OPAQUE_PTR handle);
IMPORT UINT_32 CALL_TYPE
    dba_sysconfig_get_host_interface_flags(OPAQUE_PTR handle);
IMPORT BOOL dba_sysconfig_is_host_interface_flags_bit(OPAQUE_PTR handle,
                                                      UINT_32 bit);
IMPORT UINT_32 CALL_TYPE
    dba_sysconfig_get_flare_characteristics_flags(OPAQUE_PTR handle);
IMPORT BOOL CALL_TYPE dba_sysconfig_is_flare_characteristics_flags_bit(
                                                      OPAQUE_PTR handle,
                                                      UINT_32 bit);
IMPORT UINT_8E CALL_TYPE
    dba_sysconfig_get_FE_fairness_algorithm(OPAQUE_PTR handle);
IMPORT UINT_8E CALL_TYPE
    dba_sysconfig_get_SFE_fairness_algorithm(OPAQUE_PTR handle);
IMPORT UINT_16E CALL_TYPE
    dba_sysconfig_get_raid_optimization_type(OPAQUE_PTR handle);
IMPORT UINT_32 CALL_TYPE
    dba_sysconfig_get_flare_performance_flags(OPAQUE_PTR handle);
IMPORT BOOL dba_sysconfig_is_flare_performance_flags_bit(OPAQUE_PTR handle,
                                                         UINT_32 bit);

/* Empty Data Indicator (EDI) bits. */
IMPORT BITS_32 CALL_TYPE dba_sysconfig_get_edi_bits(OPAQUE_PTR handle);
IMPORT BOOL dba_sysconfig_is_edi_bit(OPAQUE_PTR handle, UINT_32 bit);

/* SAFE revision number */
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_safe_rev_no(OPAQUE_PTR handle);

/* automode configuration */
IMPORT UINT_32 CALL_TYPE
    dba_sysconfig_get_automode_configuration(OPAQUE_PTR handle);

/* L2 cache 'reserved' size */
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_L2_cache_size(OPAQUE_PTR handle);

/* get conversion commit check disabled */
IMPORT BOOL CALL_TYPE dba_sysconfig_get_conversion_commit_check_disabled(OPAQUE_PTR handle);

/* drive spin down sysconfig functions */
IMPORT UINT_64 CALL_TYPE dba_sysconfig_get_global_idle_time_for_standby(OPAQUE_PTR handle);

IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_drive_health_check_time(OPAQUE_PTR handle);

IMPORT BOOL CALL_TYPE dba_sysconfig_get_system_power_saving_state(OPAQUE_PTR handle);

IMPORT BOOL CALL_TYPE dba_sysconfig_get_power_saving_stats_logging(OPAQUE_PTR handle);

IMPORT BOOL dba_sysconfig_get_bpf_feature_switch(OPAQUE_PTR handle);

/* The following 3 functions are used by admin to determine
 * if certain features are supported.
 */
IMPORT BOOL CALL_TYPE dba_is_proactive_sparing_supported( const LU_TYPE unit_type );
IMPORT BOOL CALL_TYPE dba_is_rg_defrag_supported( const LU_TYPE unit_type );
IMPORT BOOL CALL_TYPE dba_is_rg_expansion_supported( const LU_TYPE unit_type );

/* The following 2 functions are used by bgs for getting/checking 
 * bgs specific master controller flags 
 */
IMPORT UINT_32 CALL_TYPE dba_sysconfig_get_mc_flags(OPAQUE_PTR handle);
IMPORT BOOL CALL_TYPE dba_sysconfig_is_bgs_mc_flags_bit(OPAQUE_PTR handle, UINT_32 flags_bits);

/* get be/fe port info */
//VOID dba_sysconfig_get_port_info(OPAQUE_PTR handle, UINT_32 port, SYSCONFIG_PORT_INFO *new_info);
VOID dba_sysconfig_get_port_info(OPAQUE_PTR handle, UINT_32 port, OPAQUE_PTR new_info);

/* Return the SP-level BGS enable flags for a background service */
dba_bgs_enable_flags_bitfield_t dba_sysconfig_get_bgs_enable_flags(OPAQUE_PTR handle,
                                     bgs_monitor_types_enum_t bgs_monitor_type);

/* Return TRUE if the queried SP-level BGS enable flags are set on a background service */
IMPORT BOOL dba_sysconfig_is_bgs_enable_flags_bit(OPAQUE_PTR handle,
                                     bgs_monitor_types_enum_t bgs_monitor_type,
                                     dba_bgs_enable_flags_bitfield_t bgs_enable_flags_mask);

/* Return TRUE if the queried background service is enabled at the SP-level */
IMPORT BOOL dba_sysconfig_is_bgs_enabled(OPAQUE_PTR handle,
                              bgs_monitor_types_enum_t bgs_monitor_type);

/* Return TRUE if probation is disabled */
IMPORT BOOL dba_sysconfig_get_cm_disable_probation_flag(OPAQUE_PTR handle);

/* Return TRUE if proactive sparing is disabled */
IMPORT BOOL dba_sysconfig_get_disable_proactive_sparing(OPAQUE_PTR handle);

/* Return TRUE if fast sniff is enabled */
IMPORT BOOL dba_sysconfig_get_enable_fast_sniff(OPAQUE_PTR handle);

/* Return fast sniff time */
IMPORT UINT_32 dba_sysconfig_get_fast_sniff_time(OPAQUE_PTR handle);

IMPORT OPAQUE_64 CALL_TYPE dba_sysconfig_get_shared_expected_memory_info(OPAQUE_PTR handle);

IMPORT void     CALL_TYPE dba_raidg_get_fru_serial_num(OPAQUE_PTR handle, UINT_32 group, UINT_32 fru, UINT_8 *serial_number, UINT_32 buffer_size);
IMPORT UINT_32  CALL_TYPE dba_fru_get_pup_blocked_fru_element_blocked_type(OPAQUE_PTR handle, UINT_32 fru);
IMPORT void     CALL_TYPE dba_fru_get_pup_blocked_fru_element_fru_sn(OPAQUE_PTR handle, UINT_32 fru, UINT_8 *fru_sn, UINT_32 buffer_size);


IMPORT BOOL CALL_TYPE dba_fru_is_missing_disk64(ULONG64 handle,
                                                            UINT_32 fru);

IMPORT UINT_32 CALL_TYPE dba_fru_get_drive_type(OPAQUE_PTR handle,
                                            UINT_32 fru);


IMPORT UINT_32 CALL_TYPE dba_fru_get_enclosure_index(OPAQUE_PTR handle, UINT_32 fru);

IMPORT BOOL CALL_TYPE dba_fru_is_pup_blocked_fru_element_blocked_type(OPAQUE_PTR handle, 
                                                                      UINT_32 fru, 
                                                                      UINT_32 fru_pup_blocked_type_bit);


/*********************************************************
 * The following dba functions have a match dba macro.   *
 * If you modify any of the following functions, make    *
 * sure the corresponding macro is modified also.        *
 * The dba macros are defined in either dba_export_api.h *
 * or dba_api.h.                                         *
 * When "RG_USE_DB_INTERFACE_FUNCTIONS" is defined then  *
 * the dba functions will be compiled (usually during    *
 * the checked built). And RG_USE_DB_INTERFACE_FUNCTIONS *
 * is defined in dba_export_api.h                        *
 *********************************************************/

#ifdef RG_USE_DB_INTERFACE_FUNCTIONS

/********************************
 *  Fru Functions Declarations  *
 ********************************/

//
// Hot spare related
//


IMPORT LBA_T CALL_TYPE
    dba_fru_get_part_chkpt_by_unit_and_fru_pos(OPAQUE_PTR handle,
                                               const UINT_32 unit,
                                               const UINT_32 fru_pos);


//
// Other FRU related parameters
//

IMPORT REVISION_NUMBER CALL_TYPE dba_fru_get_rev_num(OPAQUE_PTR handle,
                                                     UINT_32 fru);
//get revision number


IMPORT fru_state_T CALL_TYPE dba_fru_get_state(OPAQUE_PTR handle, UINT_32 fru);
// returns the state of the fru

IMPORT UINT_32 CALL_TYPE dba_fru_get_enclosure(OPAQUE_PTR handle, UINT_32 fru);
// returns the enclosure number the fru is in

IMPORT BITS_32 CALL_TYPE dba_fru_get_slot_mask(OPAQUE_PTR handle, UINT_32 fru);
// returns the slot mask field

IMPORT LBA_T CALL_TYPE dba_fru_get_capacity(OPAQUE_PTR handle, UINT_32 fru);
// get the capacity field

IMPORT fru_fru_flags_T CALL_TYPE dba_fru_get_fru_flags(OPAQUE_PTR handle,
                                                       UINT_32 fru);
// returns fru fflags (fru specific flags);

IMPORT BOOL CALL_TYPE dba_fru_is_fru_flags_bit(OPAQUE_PTR handle,
                                               UINT_32 fru,
                                               fru_fru_flags_T fflags_bit);
// returns TRUE if specified bits are set

IMPORT UINT_32 CALL_TYPE dba_fru_get_user_group_id(OPAQUE_PTR handle,
                                                   UINT_32 fru);
// gets the raid group id that's designated as the 'user'
// group on this fru to whatever raid group it should be.
// The user raid group will be kept in fru.group_id[0]

IMPORT UINT_32 CALL_TYPE dba_fru_get_group_id(OPAQUE_PTR handle,
                                              UINT_32 fru,
                                              UINT_32 index);
// gets the value of fru.group_id[index]


IMPORT UINT_32 CALL_TYPE dba_fru_get_num_luns(OPAQUE_PTR handle, UINT_32 fru);
// get the num_luns field on the fru, this is the total number of partitions
// that are allocated across all raid groups


IMPORT LBA_T CALL_TYPE dba_fru_get_zeroed_mark(OPAQUE_PTR handle, UINT_32 fru);
// get the zero mark on the fru

IMPORT void *dba_fru_get_cm_element(OPAQUE_PTR handle, UINT_32 fru);
// get the cm_element involved in fru powerup.

IMPORT BOOL dba_fru_is_it_safe_to_powerup_fru(OPAQUE_PTR handle, UINT_32 fru);
// get if it is safe to request a fru powerup or not.


IMPORT UINT_32 CALL_TYPE dba_fru_get_partition_lun(OPAQUE_PTR handle,
                                                   UINT_32 fru,
                                                   UINT_32 partition);
// get the lun for a specified partition on this fru


IMPORT UINT_32 CALL_TYPE dba_fru_get_partition_group(OPAQUE_PTR handle,
                                                     UINT_32 fru,
                                                     UINT_32 partition);

// get the group for a specified partition on a fru


#if 0
IMPORT TIMESTAMP CALL_TYPE dba_fru_get_partition_timestamp(OPAQUE_PTR handle,
                                                           UINT_32 fru,
                                                           UINT_32 partition);
#endif
// get the timestamp for a specified partition on this fru


IMPORT fru_partition_flags_T CALL_TYPE
    dba_fru_get_partition_flags(OPAQUE_PTR handle,
                                UINT_32 fru,
                                UINT_32 partition);
// get all partition flags on a specified fru/partition


IMPORT BOOL CALL_TYPE
    dba_fru_is_partition_flags_bit(OPAQUE_PTR handle,
                                   UINT_32 fru,
                                   UINT_32 partition,
                                   fru_partition_flags_T pflags);
// return TRUE if specified bits are set

IMPORT LBA_T CALL_TYPE dba_fru_get_partition_checkpoint(OPAQUE_PTR handle,
                                                        UINT_32 fru,
                                                        UINT_32 partition);
// get the rebuild/eqz checkpoint for a specified fru/partition
IMPORT LBA_T CALL_TYPE dba_fru_get_partition_checkpoint64(ULONG64 handle,
                                       UINT_32 fru,
                                       UINT_32 partition);

IMPORT UINT_32 CALL_TYPE dba_fru_get_fru_deads_in_progress(OPAQUE_PTR handle,
                                                           UINT_32 fru);
// get the fru deads in progress on the fru.

IMPORT BOOL CALL_TYPE dba_fru_get_keep_fru_dead_value(OPAQUE_PTR handle,
                                                     UINT_32 fru);

IMPORT UINT_32 CALL_TYPE dba_fru_get_time_of_first_fru_death(OPAQUE_PTR handle, 
                                                            UINT_32 fru, 
                                                            FRU_STABILITY_FAULT_TYPE fault_type);
// get  the time that this FRU first displayed unstable behavior

IMPORT UINT_32 CALL_TYPE dba_fru_get_pbc_asserted_count(OPAQUE_PTR handle,
                                                        UINT_32 fru);
// get the number of times that this FRU has displayed unstable behavior

IMPORT UINT_32 CALL_TYPE dba_fru_get_interval_of_last_fru_death(OPAQUE_PTR handle, 
                                                                UINT_32 fru,
                                                                FRU_STABILITY_FAULT_TYPE fault_type);
// get the interval between unstable removes

IMPORT UINT_32 CALL_TYPE dba_fru_get_time_of_last_fru_death(OPAQUE_PTR handle, UINT_32 fru, FRU_STABILITY_FAULT_TYPE fault_type);

// get the time that this FRU last displayed unstable behavior

IMPORT UINT_32 CALL_TYPE dba_fru_get_drive_change_timeout_id(OPAQUE_PTR handle,
                                                        UINT_32 fru);
// get the timeout id that is set when processing an unstable remove

IMPORT UINT_32 CALL_TYPE dba_fru_get_max_speed(OPAQUE_PTR handle,
                                               UINT_32 fru);
// get the max speed capability of the drive

IMPORT UINT_32 dba_fru_get_pro_sparing_request_count(OPAQUE_PTR handle,
                                                            UINT_32 fru);
// get the number of failed attempts by the DH to proactively spare this FRU


IMPORT BITS_32 CALL_TYPE dba_fru_is_pd_flags_bit(OPAQUE_PTR handle,
                                               UINT_32 fru,
                                               PD_FLAGS pd_flag_bit);
// Get the setting of the specified PD flag.

IMPORT PD_REASON CALL_TYPE dba_fru_get_pd_reason(OPAQUE_PTR handle, UINT_32 fru);
// get the drive probation reason.
 
IMPORT UINT_32 CALL_TYPE dba_fru_get_pbc_pd_timeout(OPAQUE_PTR handle, UINT_32 fru);
// get the timeout id for probational drive.

IMPORT fru_state_T CALL_TYPE dba_fru_get_peer_disk_state(OPAQUE_PTR handle, UINT_32 fru);
// returns the state of the fru as reported by PEER

IMPORT DRIVE_DEAD_REASON CALL_TYPE dba_fru_get_peer_disk_death_reason(OPAQUE_PTR handle, UINT_32 fru);
// returns the state of the fru as reported by PEER

/*********************************
 *  UNIT Functions Declarations  *
 *********************************/


IMPORT BOOL CALL_TYPE dba_unit_is_type(OPAQUE_PTR handle,
                                       UINT_32 unit,
                                       LU_TYPE type);
// return TRUE if unit type is 'type'

//
// Expansion and Geometry Related
//

IMPORT UINT_32 CALL_TYPE dba_unit_get_num_frus(OPAQUE_PTR handle,
                                               UINT_32 unit);
// get units regular width



//
// Other Unit related parameters
//

IMPORT REVISION_NUMBER CALL_TYPE dba_unit_get_rev_num(OPAQUE_PTR handle,
                                                      UINT_32 unit);
// get the revision number

IMPORT  LU_STATE CALL_TYPE dba_unit_get_state(OPAQUE_PTR handle, UINT_32 unit);
// get the state of this unit

IMPORT BOOL CALL_TYPE dba_unit_is_state(OPAQUE_PTR handle,
                                        UINT_32 unit,
                                        LU_STATE state);
// returns TRUE if the state of the unit is exactly 'state'

IMPORT BOOL CALL_TYPE dba_unit_is_bound(OPAQUE_PTR handle, UINT_32 unit);
// returns TRUE if unit is not in the unbound state.


IMPORT BOOL CALL_TYPE dba_unit_is_peer_assigned(OPAQUE_PTR handle,
                                                UINT_32 unit);
// returns TRUE if unit is in one of the peer assigned states

IMPORT BOOL CALL_TYPE dba_unit_is_peer_shutdown(OPAQUE_PTR handle,
                                                UINT_32 unit);
// returns TRUE if unit is in one of the shutdown states


IMPORT UINT_32 CALL_TYPE dba_unit_get_fru(OPAQUE_PTR handle,
                                          UINT_32 unit,
                                          UINT_32 index);
// get the fru array in the unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_dirty_word(OPAQUE_PTR handle,
                                                 UINT_32 unit);
// gets the dirty_word from a unit

IMPORT LU_STATUS CALL_TYPE dba_unit_get_status(OPAQUE_PTR handle,
                                               UINT_32 unit);
// returns the 'status' of a unit

IMPORT BOOL CALL_TYPE dba_unit_is_status_bit(OPAQUE_PTR handle,
                                             UINT_32 unit,
                                             LU_STATUS status);
// returns TRUE if all of the bits in 'status' are set

IMPORT LBA_T CALL_TYPE dba_unit_get_capacity(OPAQUE_PTR handle, UINT_32 unit);
// return the capacity of a unit

IMPORT LBA_T CALL_TYPE dba_unit_get_metadata_size(OPAQUE_PTR handle, UINT_32 unit);
// return the metadata_size of a unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_rebuild_time(OPAQUE_PTR handle,
                                                   UINT_32 unit);
// get the unit rebuild time in seconds

IMPORT UINT_32 CALL_TYPE dba_unit_get_request_count(OPAQUE_PTR handle,
                                                    UINT_32 unit);
// return the unit's request count

IMPORT TIMESTAMP CALL_TYPE dba_unit_get_timestamp(OPAQUE_PTR handle,
                                                  UINT_32 unit);
// return the timestamp on a unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_dirty_io_count(OPAQUE_PTR handle,
                                                     UINT_32 unit);
// return the unit's dirty_io_count(outstanding writes)

IMPORT UINT_32 CALL_TYPE dba_unit_get_chkpt_verify_time(OPAQUE_PTR handle,
                                                        UINT_32 unit);
// returns verify time in seconds

IMPORT LBA_T CALL_TYPE dba_unit_get_chkpt_verify_chkpt(OPAQUE_PTR handle,
                                                       UINT_32 unit);
// get the unit's verify chkpt

// convert verify priority to time
IMPORT UINT_32 CALL_TYPE dba_unit_verify_priority_to_time(UINT_32 verify_priority);

// convert verify time to priority
IMPORT UINT_32 CALL_TYPE dba_unit_verify_time_to_priority(UINT_32 verify_time);

// return unit's verify priority
IMPORT UINT_32 CALL_TYPE dba_unit_get_verify_priority(OPAQUE_PTR handle, 
                                                      UINT_32 unit);

IMPORT UINT_32 CALL_TYPE dba_unit_get_assign_word(OPAQUE_PTR handle,
                                                  UINT_32 unit);
// get the unit's assign_word

IMPORT BITS_32 CALL_TYPE dba_unit_get_lu_attributes(OPAQUE_PTR handle,
                                                    UINT_32 unit);
// return a bit mask of the units assign permissions

IMPORT BOOL CALL_TYPE
    dba_unit_is_lu_attributes_bit(OPAQUE_PTR handle,
                                  UINT_32 unit,
                                  BITS_32 lu_attributes_bit);

// Get the suggested hlu for this unit (for use with Celerra luns)
IMPORT UINT_8E CALL_TYPE dba_unit_get_suggested_hlu(OPAQUE_PTR handle,
                                                    UINT_32 unit);

// Get the lun type for this unit (to distinguish Celerra luns from Flare luns)
IMPORT LU_LUN_TYPE CALL_TYPE dba_unit_get_lun_type(OPAQUE_PTR handle,
                                                    UINT_32 unit);

// Returns if sniffing is enabled or not. Takes into account if user
// control of sniffing is enabled or not. (The dba calls for lu_attributes
// do NOT take this into account, and return what is actually stored in the
// DB.
IMPORT BOOL CALL_TYPE dba_unit_is_sniff_verify_enabled(OPAQUE_PTR handle,
                                                       UINT_32 m_unit);
// return TRUE if the specified bit is set.

IMPORT UINT_64 CALL_TYPE dba_unit_get_flags(OPAQUE_PTR handle,
                                               UINT_32 unit);
// return unit flags

IMPORT BOOL CALL_TYPE dba_unit_is_flags_bit(OPAQUE_PTR handle,
                                            UINT_32 unit,
                                            UINT_64 flags_bits);
// return TRUE if all of the bits specified by flags_bits are set.

IMPORT BOOL CALL_TYPE dba_unit_is_np_flags_bit(OPAQUE_PTR handle,
                                               UINT_32 unit,
                                               LU_NP_FLAGS_T flags_bits);

IMPORT BOOL CALL_TYPE dba_fru_is_np_flags_bit(OPAQUE_PTR handle,
                                               UINT_32 fru,
                                               FRU_NP_FLAGS_T flags_bits);

IMPORT UINT_32 CALL_TYPE dba_unit_get_session_id(OPAQUE_PTR handle,
                                                 UINT_32 unit);
// return units cache session id


IMPORT UINT_32 CALL_TYPE dba_unit_get_vp_sniff_time(OPAQUE_PTR handle,
                                                    UINT_32 unit);
// get units sniff time (100 ms)

IMPORT UINT_16E CALL_TYPE dba_unit_get_partition_index(OPAQUE_PTR handle,
                                                       UINT_32 unit);
// get the partition index of the unit on the fru



IMPORT UINT_32 CALL_TYPE dba_unit_get_rg_next_lun(OPAQUE_PTR handle,
                                                  UINT_32 unit);
// get the lun after 'unit' in the raid group

IMPORT UINT_32 CALL_TYPE dba_unit_get_L2_stage(OPAQUE_PTR handle,
                                               UINT_32 unit);
// returns glut that holds the l2 for this hi5 lu

IMPORT LBA_T CALL_TYPE dba_unit_get_address_offset(OPAQUE_PTR handle,
                                                   UINT_32 unit);
// returns the address offset field on the LU


// gets the user defined name for the unit

IMPORT UINT_16 CALL_TYPE dba_unit_get_nsec(OPAQUE_PTR handle, UINT_32 unit);
// gets the number of sectors

IMPORT UINT_16 CALL_TYPE dba_unit_get_nhead(OPAQUE_PTR handle, UINT_32 unit);
// gets the number of heads

IMPORT UINT_32 CALL_TYPE dba_unit_get_ncyl(OPAQUE_PTR handle, UINT_32 unit);
// gets the number of cylinders


IMPORT UINT_16 CALL_TYPE dba_unit_get_bind_offset(OPAQUE_PTR handle,
                                                  UINT_32 unit);
// gets the bind offset

IMPORT LBA_T CALL_TYPE dba_unit_get_external_capacity(OPAQUE_PTR handle,
                                                      UINT_32 unit);
// gets the external capacity

IMPORT UINT_16 CALL_TYPE dba_unit_get_aligned_sector(OPAQUE_PTR handle,
                                                     UINT_32 unit);
// gets the aligned sector

IMPORT void CALL_TYPE dba_unit_get_cache_params(OPAQUE_PTR handle,
                                                UINT_32 unit,
                                                UNIT_CACHE_PARAMS *ucache_p);

//fills in a DBA_UNIT_CACHE_PARAMS block passed in from caller

IMPORT LBA_T CALL_TYPE dba_unit_get_zero_chkpt(OPAQUE_PTR handle, UINT_32 unit);
//Get the zero checkpoint for the specified unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_throttle_rate(OPAQUE_PTR handle, UINT_32 unit);
//Get the zero throttle rate for the specified unit

IMPORT LBA_T CALL_TYPE dba_unit_get_bd_transition_lba(OPAQUE_PTR handle, UINT_32 unit);
//Get the bind driver user data lba for the specified unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_bd_transition_data(OPAQUE_PTR handle, UINT_32 unit);
 //Get the bind driver user data for the specified unit.

IMPORT UINT_32 dba_unit_get_bgzero_percent(OPAQUE_PTR handle, UINT_32 unit);
// get the background zero percent

IMPORT UINT_32 CALL_TYPE dba_unit_get_priority_lru_length(OPAQUE_PTR handle,
                                                          UINT_32 unit);
// Get the priority_lru_length for the passed in unit

IMPORT BITS_8E CALL_TYPE dba_unit_get_cache_config_flags(OPAQUE_PTR handle,
                                                         UINT_32 unit);
// Get bits in the cache config flags for the passed in unit

IMPORT BOOL CALL_TYPE
    dba_unit_is_cache_config_flags_bit(OPAQUE_PTR handle,
                                       UINT_32 unit,
                                       BITS_8E config_flags);
// Return TRUE if all of the bits specified by "config_flags" are set.

IMPORT UINT_8E CALL_TYPE
    dba_unit_get_read_retention_priority(OPAQUE_PTR handle, UINT_32 unit);
// Get the read_retention_priority for the passed in unit

IMPORT UINT_8E CALL_TYPE dba_unit_get_cache_idle_threshold(OPAQUE_PTR handle,
                                                           UINT_32 unit);

// Get the read cache be req size
IMPORT UINT_8E CALL_TYPE dba_unit_get_cache_be_req_size(OPAQUE_PTR handle,
                                                           UINT_32 unit);

// Get the cache idle threshold for the passed in unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_cache_write_aside(OPAQUE_PTR handle,
                                                        UINT_32 unit);
// Get the cache write aside field for the passed in unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_cache_dirty_word(OPAQUE_PTR handle,
                                                       UINT_32 unit);
// Get the cache dirty word for the passed in unit

IMPORT UINT_8E CALL_TYPE dba_unit_get_cache_idle_delay(OPAQUE_PTR handle,
                                                          UINT_32 unit);
// Get the cache_idle_delay for the passed in unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_prefetch_idle_count(OPAQUE_PTR handle,
                                                          UINT_32 unit);
// Get the prefetch idle count for the passed in unit

IMPORT UINT_32 CALL_TYPE dba_unit_get_L2_cache_dirty_word(OPAQUE_PTR handle,
                                                          UINT_32 unit); // Get the L2 cache dirty word for the passed in unit

IMPORT UINT_32 CALL_TYPE dba_unit_R10_mirror_count(OPAQUE_PTR handle,
                                                   UINT_32 unit);
// returns the number of mirrored pairs

IMPORT LBA_T CALL_TYPE dba_unit_get_total_element_count(OPAQUE_PTR handle,
                                                        UINT_32 unit);
// Get the total_element_count field in glut

IMPORT UINT_64 CALL_TYPE dba_unit_get_clean_status(OPAQUE_PTR handle, UINT_32 lu);
// Returns if the write status bit in flag field is set.


IMPORT UINT_32 CALL_TYPE dba_unit_get_disk_io_count(OPAQUE_PTR handle,
                                                    UINT_32 unit);
// gets the disk_io_count for the unit

IMPORT BOOL CALL_TYPE dba_unit_is_lun_in_peer_lu_state(OPAQUE_PTR handle,
                                                       UINT_32 unit);
// returns TRUE if the unit is in a peer LU state.

/***************************************
 *  RAID GROUP Function Declarations
 ***************************************/

IMPORT UINT_32 CALL_TYPE dba_raidg_get_expansion_rate(OPAQUE_PTR handle,
                                                      UINT_32 raidg);
// get the expansion rate

IMPORT REVISION_NUMBER CALL_TYPE dba_raidg_get_rev_num(OPAQUE_PTR handle,
                                                       UINT_32 group);
// get revision number

IMPORT raid_group_flags_T CALL_TYPE
    dba_raidg_get_state_flags(OPAQUE_PTR handle, UINT_32 group);
// get the groups state_flags

IMPORT TIMESTAMP CALL_TYPE
    dba_raidg_get_partition_timestamp(OPAQUE_PTR handle,
                                      UINT_32 group,
                                      UINT_32 partition);
// get the group/partition timestamp


IMPORT LBA_T CALL_TYPE dba_raidg_get_address_offset(OPAQUE_PTR handle,
                                                    UINT_32 group);
// set the physical address that this raid group starts at.

IMPORT LBA_T CALL_TYPE dba_raidg_get_ending_address(OPAQUE_PTR handle,
                                                    UINT_32 group);
// this is the last physical address used by this group

IMPORT vp_flags_T CALL_TYPE dba_raidg_get_vp_flags(OPAQUE_PTR handle,
                                                   UINT_32 group);
// get current state of all vp flags on the raid group

IMPORT BOOL CALL_TYPE dba_raidg_is_vp_flags_bit(OPAQUE_PTR handle,
                                                UINT_32 group,
                                                vp_flags_T vp_flags);
// returns true if specified flags are set

IMPORT UINT_32 CALL_TYPE dba_raidg_get_vp_lun(OPAQUE_PTR handle,
                                              UINT_32 group);
// get the vp_lun field in the raidg

IMPORT raid_status_flags_T CALL_TYPE
    dba_raidg_get_status_flags(OPAQUE_PTR handle,
                               UINT_32 group);
// get current state of all status flags on the raid group

IMPORT BOOL CALL_TYPE
    dba_raidg_is_status_flags_bit(OPAQUE_PTR handle,
                                  UINT_32 group,
                                  raid_status_flags_T status_flags);
// returns true if specified status flags are set

IMPORT BOOL CALL_TYPE dba_raidg_above_exp_chkpt_logical(OPAQUE_PTR handle,
                                                        UINT_32 unit,
                                                        LBA_T start_lba);
// return TRUE/FALSE if the address is above logical expansion chkpt

IMPORT BOOL CALL_TYPE dba_raidg_above_exp_chkpt_phys(OPAQUE_PTR handle,
                                                     UINT_32 unit,
                                                     LBA_T start_lba);
// return TRUE/FALSE if the address is above physical expansion chkpt

IMPORT LBA_T CALL_TYPE dba_raidg_get_logical_exp_chkpt(OPAQUE_PTR handle,
                                                       UINT_32 group);
//get logical expansion checkpoint

IMPORT LBA_T CALL_TYPE dba_raidg_get_physical_exp_chkpt(OPAQUE_PTR handle,
                                                        UINT_32 group);
//get physical expansion checkpoint

IMPORT UINT_32 dba_raidg_get_num_exp_chkpt_disks(OPAQUE_PTR handle, 
                                                 UINT_32 group);
//get number of expansion checkpoint database disks for given RAID Group

IMPORT BOOL dba_raidg_get_exp_chkpt_disk_list(OPAQUE_PTR in_handle, 
                                              UINT_32 in_group, 
                                              disk_t* out_exp_chkpt_disk_list_p);
//get expansion checkpoint database disks list for given RAID group

IMPORT UINT_32 CALL_TYPE dba_raidg_get_num_luns(OPAQUE_PTR handle,
                                                UINT_32 group);

// get number of frus in the unit, this is for physical configuration,
// not I/O path which should use a function from the geometry section

IMPORT BITS_32 CALL_TYPE dba_raidg_get_rg_attributes(OPAQUE_PTR handle,
                                                     UINT_32 group);
/* Are the specified rg_attributes bit(s) set. */

IMPORT UINT_32 dba_raidg_get_element_size(OPAQUE_PTR handle, UINT_32 group);
// get the element size of the group

IMPORT BOOL dba_raidg_is_num_of_disks_valid(UINT_32 unit_type, UINT_32 num_frus);
/*returns TRUE if RG has valid number of frus.*/

IMPORT BOOL CALL_TYPE dba_fru_is_hs_fru_equalizing(OPAQUE_PTR handle,
                                                   UINT_32 fru);
// returns TRUE is a hot spare fru is being equalized on a lun



IMPORT UINT_32 CALL_TYPE dba_fru_get_hs_fru_num(OPAQUE_PTR handle,
                                                UINT_32 hs_fru,
                                                UINT_32 position);
// returns the real fru number is a fru is hotspared

IMPORT BOOL CALL_TYPE dba_unit_is_shutdown(OPAQUE_PTR handle, UINT_32 unit);
// returns TRUE if the unit is shutdown

IMPORT BOOL CALL_TYPE dba_raidg_is_shutdown(OPAQUE_PTR handle, UINT_32 group);
// returns TRUE is a raid group is shutdown

IMPORT BOOL CALL_TYPE dba_raidg_is_group_bound(OPAQUE_PTR handle,
                                               UINT_32 group);
// returns TRUE is a raid group is bound

IMPORT BOOL CALL_TYPE dba_unit_is_parity(OPAQUE_PTR handle, UINT_32 unit);
// returns TRUE if a unit uses parity based data protection

IMPORT BOOL CALL_TYPE dba_unit_is_raw(OPAQUE_PTR handle, UINT_32 unit);
// returns TRUE if a unit is a 'raw'?? unit


IMPORT UINT_32 CALL_TYPE dba_unit_get_post_exp_width(OPAQUE_PTR handle,
                                                     UINT_32 unit);
// returns num_frus, the new width including expansion drives

IMPORT LBA_T CALL_TYPE dba_unit_get_post_exp_offset(OPAQUE_PTR handle,
                                                    UINT_32 unit);
// returns addr_offset, the address offset where the unit starts after
// expansion finishes

IMPORT LBA_T CALL_TYPE dba_unit_get_pre_exp_offset(OPAQUE_PTR handle,
                                                   UINT_32 unit);
// returns pre_exp_addr_offset, the address offset where the unit started
// before expansion

IMPORT LBA_T CALL_TYPE dba_unit_get_adjusted_addr_offset(OPAQUE_PTR handle,
                                                         UINT_32 unit,
                                                         LBA_T lba);
// returns the correct address offset, taking expansion state and LBA into
// account

IMPORT LBA_T CALL_TYPE
    dba_unit_get_adjusted_addr_offset_phys(OPAQUE_PTR handle,
                                           UINT_32 unit,
                                           LBA_T lba);
// returns the correct physical address offset, taking expansion state and
// LBA into account

IMPORT UINT_32 CALL_TYPE dba_unit_get_width(OPAQUE_PTR handle,
                                            UINT_32 unit,
                                            LBA_T lba);
// returns the correct width, taking expansion state and LBA into account

IMPORT UINT_32 CALL_TYPE dba_unit_get_sectors_per_element(OPAQUE_PTR handle,
                                                          UINT_32 unit);
// returns the number of blocks in one data element on one drive

IMPORT LU_TYPE CALL_TYPE dba_unit_get_type(OPAQUE_PTR handle, UINT_32 unit);
// returns the RAID type of the unit

IMPORT LBA_T CALL_TYPE
    dba_unit_get_elements_per_parity_stripe(OPAQUE_PTR handle, UINT_32 unit);
// returns the number of data elements on one disk that are
// covered by one parity element

IMPORT LBA_T CALL_TYPE dba_unit_get_exp_chkpt(OPAQUE_PTR handle, UINT_32 unit);
// returns the expansion checkpoint for a unit

IMPORT LBA_T CALL_TYPE dba_unit_get_exp_chkpt_phys(OPAQUE_PTR handle,
                                                   UINT_32 unit);
// returns the physical expansion checkpoint

IMPORT BOOL CALL_TYPE dba_unit_is_expanding(OPAQUE_PTR handle, UINT_32 unit);
// returns TRUE if the unit is expanding

IMPORT BOOL CALL_TYPE dba_unit_is_bandwidth(OPAQUE_PTR handle, UINT_32 unit);
// returns TRUE if the unit is a bandwidth R5 (a R3)

IMPORT UINT_32 CALL_TYPE dba_raidg_get_width(OPAQUE_PTR handle,
                                             UINT_32 group,
                                             UINT_32 is_mirror);
// returns the number of frus in the group

IMPORT UINT_32 CALL_TYPE dba_raidg_get_pre_exp_width(OPAQUE_PTR handle,
                                                     UINT_32 group);
// returns the number of pre-expansion frus in the group

IMPORT UINT_32 CALL_TYPE dba_raidg_get_mirror_fru_num(OPAQUE_PTR handle,
                                                      UINT_32 group,
                                                      INT_16 mirror_pos1,
                                                      UINT_32 fru_position);
// returns the fru number of a disk in a mirror, the mirrors aren't
// directly represented by GLUT entries, so this is necessary

IMPORT DRIVE_PHYS_LOCATION CALL_TYPE dba_raidg_get_phys_location(OPAQUE_PTR handle,
                                                                 UINT_32 group,
                                                                 UINT_32 fru_position);

IMPORT BOOL CALL_TYPE dba_raidg_is_mirrored(OPAQUE_PTR handle, UINT_32 group);
// returns TRUE if the group is a mirror

IMPORT LBA_T CALL_TYPE dba_unit_get_bv_chkpt(OPAQUE_PTR handle, UINT_32 unit);
// returns the background verify checkpoint for the unit

IMPORT BOOL CALL_TYPE dba_unit_am_i_default_owner(OPAQUE_PTR handle,
                                                  UINT_32 lun);
// returns true if SP calling the function is the default owner

IMPORT UINT_32 CALL_TYPE dba_raidg_get_lun(OPAQUE_PTR handle,
                                           UINT_32 group,
                                           UINT_32 index);
// get the lun at a specific index in the lun list

IMPORT LU_TYPE CALL_TYPE dba_raidg_get_unit_type(OPAQUE_PTR handle, UINT_32 raidg);
// get the unit_type from the raid group

/* drive spin down raidg get functions */
IMPORT UINT_64 CALL_TYPE dba_raidg_get_idle_time_for_standby(OPAQUE_PTR handle,
                                                   UINT_32 group);

IMPORT UINT_64  CALL_TYPE dba_raidg_get_latency_time_for_becoming_active (OPAQUE_PTR handle,
                                                               UINT_32 group);


/***************************************
 *  FRU Signature Function Declarations
 ***************************************/

IMPORT void CALL_TYPE dba_frusig_get_cache(OPAQUE_PTR handle,
                                 UINT_32 fru,
                                 FRU_SIGNATURE *pfru_sig);
// returns FRU signature from cache


IMPORT UINT_32 CALL_TYPE dba_frusig_get_wwn_seed(FRU_SIGNATURE * pfru_sig);
// Gets the WWN Seed from the supplied FRU_SIGNATURE structure
// since these are kept only on disk (or cached from disk), they are more
// structures and are not indexed by array indices.

IMPORT UINT_32 CALL_TYPE dba_frusig_get_fru (FRU_SIGNATURE * pfru_sig);
// Get the fru (slot) number from the fru signature

IMPORT LBA_T CALL_TYPE dba_frusig_get_zeroed_mark(FRU_SIGNATURE * pfru_sig);
// get the zeroed mark from the fru signature

IMPORT UINT_32 CALL_TYPE
    dba_frusig_get_zeroed_stamp(FRU_SIGNATURE * pfru_sig);
// get the zeroed stamp from the fru signature

IMPORT UINT_32 CALL_TYPE dba_frusig_get_partition_lun(FRU_SIGNATURE *pfru_sig,
                                                      UINT_32 partition);
// get the lun for a specified partition on this fru signature

IMPORT TIMESTAMP CALL_TYPE
    dba_frusig_get_partition_timestamp(FRU_SIGNATURE *pfru_sig,
                                       UINT_32 partition);
// get the timestamp for a specified partition on this fru

IMPORT UINT_32 CALL_TYPE
    dba_frusig_get_partition_group_id(FRU_SIGNATURE *pfru_sig,
                                      UINT_32 partition);
// get the group_id for a specified partition on this fru signature

#else

/***********************************************************************
 * RG_USE_DB_INTERFACE_FUNCTIONS is not defined => to compile macros   *
 ***********************************************************************/

#define dba_raidg_above_exp_chkpt_logical(m_handle, m_unit, m_start_lba)\
    (DBA_LU_ABOVE_EXP_CHKPT(m_handle, m_unit, m_start_lba))

#define dba_raidg_above_exp_chkpt_phys(m_handle, m_unit, m_start_lba)\
    (DBA_LU_ABOVE_EXP_CHKPT_PHYS(m_handle, m_unit, m_start_lba))

/* GLUT interface function to check for shutdown.
 */

#define dba_unit_is_shutdown(m_handle, m_unit)\
    (((((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_SHUTDOWN_FAIL) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_SHUTDOWN_TRESPASS) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_SHUTDOWN_INTERNAL_ASSIGN) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_SHUTDOWN_UNBIND))? TRUE : FALSE)
// returns TRUE if the unit is shutdown

#define dba_unit_is_peer_shutdown(m_handle, m_unit)\
    (((((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_SHUTDOWN_FAIL) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_SHUTDOWN_TRESPASS) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_SHUTDOWN_INTERNAL_ASSIGN) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_SHUTDOWN_UNBIND))? TRUE : FALSE)
// returns TRUE if the unit is shutdown

/* GLUT interface function to check for peer LU state.
 */
#define dba_unit_is_lun_in_peer_lu_state(m_handle, m_unit)\
    (((((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_BIND) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_ASSIGN) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_ENABLED) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_DEGRADED) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_SHUTDOWN_FAIL) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_SHUTDOWN_TRESPASS) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_SHUTDOWN_UNBIND) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_SHUTDOWN_INTERNAL_ASSIGN) || \
      (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].state == LU_PEER_UNBIND))? TRUE : FALSE)
// returns TRUE if the unit is in a peer LU state

#define dba_raidg_is_shutdown(m_handle, m_group)\
    ((((ADM_DATA *)(m_handle))-> \
        raid_group_tabl[m_group].status_flags & RG_SHUTDOWN) ? \
        TRUE : FALSE)

/* GLUT interface function to check for shutdown.
 */

#define dba_raidg_is_group_bound(m_handle, m_group)\
    ((((ADM_DATA *)(m_handle))-> \
        raid_group_tabl[m_group].lun_list[0] != INVALID_UNIT) ? TRUE : FALSE)

#define dba_raidg_get_state_flags(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].state_flags)

/* At the top raid group level, the array width is returned.
 * If it is a mirror, RG_MIRROR_ARRAY_WIDTH(2) is always returned.
 * m_group passed in is always the highest group number.
 */

/*
#define dba_raidg_get_width(m_group, m_mirror)\
     (m_mirror ? RG_MIRROR_ARRAY_WIDTH\
               : raid_group_tabl[m_group].num_disks)
*/

#define dba_raidg_get_width(m_handle, m_group, m_mirror)\
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].num_disks)
        
/* Returns the minimum pre-expansion width.
 */
#define dba_raidg_get_pre_exp_width(m_handle, m_group)                           \
    ((dba_raidg_get_state_flags(m_handle, group) & RG_EXPANDING) ?           \
        ((ADM_DATA *)(m_handle))->raid_group_tabl[group].pre_exp_num_disks : \
        ((ADM_DATA *)(m_handle))->raid_group_tabl[group].num_disks )

/* Returns TRUE if group is a parity unit.
 */
#define dba_unit_is_parity(m_handle, m_unit) \
    ((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].unit_type & \
        LU_PARITY_ARRAY) ? TRUE : FALSE)

/* Returns TRUE if group is a raw unit.
 */
#define dba_unit_is_raw(m_handle, m_unit) \
    ((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].unit_type & \
        LU_RAW_ARRAY) ? TRUE : FALSE)

#define dba_raidg_get_num_luns(m_handle, m_group) \
    (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].num_luns)



#define dba_raidg_get_partition_timestamp(m_handle, m_group, partition) \
        (((ADM_DATA *)(m_handle))-> \
            raid_group_tabl[m_group].timestamp[partition])

#define dba_raidg_get_address_offset(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].address_offset)

#define dba_raidg_get_ending_address(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].ending_address)

#define dba_raidg_is_vp_flags_bit(m_handle, m_group, rg_vp_flags) \
        ((((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].vp_flags & \
            rg_vp_flags) ? TRUE : FALSE)

#define dba_raidg_get_vp_lun(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].vp_lun)

#define dba_raidg_get_vp_flags(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].vp_flags)

#define dba_raidg_get_status_flags(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].status_flags)

#define dba_raidg_is_status_flags_bit(m_handle, m_group, rg_status_flags) \
        ((((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].status_flags & \
            rg_status_flags) ? TRUE : FALSE)

#define dba_raidg_get_logical_exp_chkpt(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].logical_exp_chkpt)

#define dba_raidg_get_physical_exp_chkpt(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))-> \
            raid_group_tabl[m_group].physical_exp_chkpt)

#define dba_raidg_get_expansion_rate(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].exp_rate)

#define dba_raidg_get_rev_num(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].rev_num)

/* Returns TRUE if raid group is a r1 or r10. This macro will not
 * work as a hot spare mirror test, but only for regular raid groups.
 * For mirror raid groups that are not carry any hot spare right now,
 * it will always return FALSE.
 */
#define dba_raidg_is_mirrored(m_handle, m_group) \
    (((m_group < RAID_GROUP_COUNT) && \
     (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].lun_list[0] != \
          INVALID_UNIT)) ? \
     ((((ADM_DATA *)(m_handle))-> \
          glut_tabl[((ADM_DATA *)(m_handle))-> \
              raid_group_tabl[m_group].lun_list[0]].unit_type) & \
              LU_MIRRORED_ARRAY)\
     : FALSE)

#define dba_raidg_get_rg_attributes(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].rg_attributes)

#define dba_unit_R10_mirror_count(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].num_frus / 2)

#define dba_unit_get_post_exp_width(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].num_frus)

#define dba_unit_get_post_exp_offset(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].address_offset)

#define dba_unit_get_pre_exp_offset(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].pre_exp_addr_offset)

#define dba_unit_get_adjusted_addr_offset(m_handle, m_unit, m_lba)\
    (((dba_unit_is_expanding((m_handle), (m_unit)) && \
             dba_raidg_above_exp_chkpt_logical((m_handle), \
                                               (m_unit), \
                                               (m_lba))) ? \
            ((ADM_DATA *)(m_handle))->glut_tabl[m_unit]. \
                pre_exp_addr_offset : \
            ((ADM_DATA *)(m_handle))->glut_tabl[m_unit].address_offset))


#define dba_unit_get_adjusted_addr_offset_phys(m_handle, m_unit, m_lba)\
    (((dba_unit_is_expanding((m_handle), (m_unit)) && \
             dba_raidg_above_exp_chkpt_phys((m_handle), \
                                            (m_unit), \
                                            (m_lba))) ? \
            ((ADM_DATA *)(m_handle))->glut_tabl[m_unit]. \
                pre_exp_addr_offset : \
            ((ADM_DATA *)(m_handle))->glut_tabl[m_unit].address_offset))


#define dba_unit_get_width(m_handle, m_unit,m_lba)\
    (DBA_LU_ARRAY_WIDTH(m_handle, m_unit, m_lba))

#define dba_unit_get_sectors_per_element(m_handle, m_unit)\
      ( m_unit == GLUT_SAFE_LU_NUMBER ?  \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].sectors_per_element) :  \
        dba_raidg_get_element_size(m_handle, dba_unit_get_group_id(m_handle, m_unit)))
    
#define dba_unit_get_zero_chkpt(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].bg_zero_checkpoint)

#define dba_unit_get_throttle_rate(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].bg_zero_throttle_rate)

#define dba_unit_get_bd_transition_lba(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].bd_transition_lba)

#define dba_unit_get_bd_transition_data(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].bd_transition_data)

#define dba_unit_get_bgzero_percent(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].bg_zero_percent)

#define dba_unit_get_type(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].unit_type)

#define dba_unit_get_elements_per_parity_stripe(m_handle, m_unit)\
    (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].elements_per_parity_stripe)

#define dba_unit_get_exp_chkpt(m_handle, m_unit)\
    (DBA_LU_GET_EXP_CHKPT(m_unit))

#define dba_unit_get_exp_chkpt_phys(m_handle, m_unit)\
    (DBA_LU_GET_EXP_CHKPT_PHYS(m_unit))

#define dba_unit_is_expanding(m_handle, m_unit)\
	  ((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].pre_exp_addr_offset != LU_DEFAULT_EXPAND_FIELD) ? \
	                                                             TRUE : FALSE)

#define dba_fru_get_hs_fru_num(m_handle, m_hs_fru, m_fru_position) \
    ((m_fru_position == 0) ? m_hs_fru : \
        ((ADM_DATA *)(m_handle))->fru_tabl[m_hs_fru].hot_fru)


/***************************************************
 * raid_is_bandwidth
 * returns boolean that indicates if this unit
 * is a bandwidth unit.
 ***************************************************/

#define dba_unit_is_bandwidth(m_handle, m_group) \
    (((m_group < RAID_GROUP_COUNT) && \
     (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].lun_list[0] != \
         INVALID_UNIT) && \
     (((ADM_DATA *)m_handle)->raid_group_tabl[group].lun_list[0] < \
         GLUT_COUNT)) \
     ? ((((ADM_DATA *)(m_handle))-> \
           glut_tabl[((ADM_DATA *)(m_handle))-> \
               raid_group_tabl[m_group].lun_list[0]].unit_type) & \
           LU_FAST_RAID3_ARRAY)\
     : FALSE)


#define dba_raidg_get_phys_location(m_handle, m_group, m_fru_position) \
    (((ADM_DATA *)m_handle)-> \
                raid_group_tabl[m_group].rg_disk_set[m_fru_position].phys_location)

/* The mirror group doesn't have entry in glut table.
 * This function looks up the entry of according highest level raid
 * group to find the right fru number. It doesn't work for hot spares.
 * The fru is listed as primary1, primary2,..., secondary1, secondary2...
 * so that the index of primary disk is mirror position, and the index
 * of secondary is mirror position + total no. of primary disks.
 */


#define dba_raidg_get_mirror_fru_num(m_handle, m_group, \
                                     m_mirror_pos1, m_fru_pos)\
    ((((ADM_DATA *)(m_handle))->  \
            glut_tabl[((ADM_DATA *)(m_handle))-> \
                    raid_group_tabl[m_group].lun_list[0]].unit_type == \
                        LU_DISK_MIRROR) \
    ? (((ADM_DATA *)(m_handle))-> \
            raid_group_tabl[m_group].disk_set[m_fru_pos]) : \
    ((((m_fru_pos) == 0)\
         ? (((ADM_DATA *)(m_handle))-> \
                    raid_group_tabl[m_group].disk_set[m_mirror_pos1])\
         : (((ADM_DATA *)(m_handle))-> \
                    raid_group_tabl[m_group].disk_set[m_mirror_pos1 + \
                            ((ADM_DATA *)(m_handle))-> \
                                raid_group_tabl[m_group].num_disks / 2]))))

#define dba_unit_get_bv_chkpt(m_handle, m_unit)\
    (DBA_LU_BACKGROUND_VERIFY_NEEDED(m_handle, m_unit) \
        ? ((ADM_DATA *)(m_handle))->glut_tabl[m_unit].chkpt_verify_checkpoint\
        : INVALID_BV_CHKPT)

/* The following macro gives the glut_index for a unit,
 * given its group id and partition_index in the Raid group.
 */
#define dba_raidg_get_lun(m_handle, _group, _partition_index) \
        (((ADM_DATA *)(m_handle))-> \
            raid_group_tabl[_group].lun_list[_partition_index])

#define dba_raidg_get_unit_type(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].unit_type)

/* drive spin down macros */
#define dba_raidg_get_idle_time_for_standby(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].idle_time_for_standby)

#define dba_raidg_get_latency_time_for_becoming_active(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].latency_time_for_becoming_active)

#define dba_raidg_get_element_size(m_handle, m_group) \
        (((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].element_size)


/*********************/
/* FRU Table Macros  */
/*********************/

#define dba_fru_get_state(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].state)

#define dba_fru_get_enclosure(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].enclosure)

#define dba_fru_get_slot_mask(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].slot_mask)

//#define dba_fru_get_bus(m_handle, m_fru) \
    //    (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].bus)

//#define dba_fru_get_device_id(m_handle, m_fru) \
       // (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].deviceID)

#define dba_fru_get_capacity(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].capacity)

#define dba_fru_get_fru_flags(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_flags)

#define dba_fru_is_fru_flags_bit(m_handle, m_fru, m_fflags_bit) \
        ((((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_flags & \
            m_fflags_bit) == 0) ? FALSE : TRUE

#define dba_fru_get_user_group_id(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].group_id[0])

#define dba_fru_get_group_id(m_handle, m_fru, m_index) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].group_id[m_index])

#define dba_fru_get_user_group_id(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].group_id[0])

#define dba_fru_get_num_luns(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].num_luns)

#define dba_fru_get_zeroed_mark(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].zeroed_mark)

#define dba_fru_get_cm_element(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].cm_element)

#define dba_fru_get_partition_lun(m_handle, m_fru, m_partition) \
    (((ADM_DATA *)(m_handle))-> \
        fru_tabl[m_fru].partition_table[m_partition].lun)


#define dba_fru_get_partition_group(m_handle, m_fru, m_partition) \
        ((((ADM_DATA *)(m_handle))-> \
            fru_tabl[m_fru].partition_table[m_partition].lun != \
                INVALID_UNIT) \
            ? ((ADM_DATA *)(m_handle))-> \
                  glut_tabl[((ADM_DATA *)(m_handle))-> \
                  fru_tabl[m_fru].partition_table[m_partition].lun].group_id \
            : INVALID_RAID_GROUP)

#define dba_fru_get_rev_num(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].rev_num)

#define dba_fru_get_fru_deads_in_progress(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_deads_in_progress)

#define dba_fru_get_part_chkpt_by_unit_and_fru_pos(m_handle, \
                                                   m_unit, m_fru_pos) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].fru_info[m_fru_pos].checkpoint)

#define dba_fru_get_max_speed(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].max_speed)

#define dba_fru_is_probation(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_state != PD_IDLE)
// Check if the specified FRU is in probation or not.

#define dba_fru_is_pd_init(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_state == PD_INIT)
// Check if the specified FRU is in PD_INIT state or not.

#define dba_fru_is_pd_stopping(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_state ==\
                                                    PD_STOPPING)
// Check if the probational FRU is PD stopping or not.

#define dba_fru_is_pd_cancelling(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_state ==\
                                                    PD_CANCEL)
// Check if the probational FRU is PD cancelling or not.

#define dba_fru_is_pd_powering_up(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_state ==\
                                                    PD_POWERUP)
// Check if the probational FRU is PD powering up or not.

#define dba_fru_is_pd_active(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_state ==\
                                                    PD_ACTIVE)
// Check if the specified FRU is PD_ACTIVE.

#define dba_fru_get_pd_state(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_state)
// Returns the probation state of the FRU.

#define dba_fru_get_pd_sniff_count(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_sniff_count)
//Returns the PD sniff count.

#define dba_fru_get_pd_cm_element(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_cm_element)
//Returns the PD cm element.

#define dba_fru_get_pro_sparing_request_count(m_handle, \
                                                        m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].proactive_spare_request_from_DH)

#define dba_fru_is_pd_flags_bit(m_handle, m_fru, m_pd_flags_bit) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_flags & \
                                                        m_pd_flags_bit)
// Get the setting of the specified PD flag.

#define dba_fru_get_death_reason(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].disk_obit.death_reason)  
// Get death reason of the specified fru.

#define dba_fru_get_pd_reason(m_handle, m_fru)  \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pd_reason)
// Get the reason why the drive is put in probation.

#define dba_fru_get_pbc_pd_timeout(m_handle, m_fru)  \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].fru_pd.pbc_timeout_id)
// Get the timeout id to check if the drive is back from probation.

#define dba_fru_get_dh_error_code(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].disk_obit.dh_error_code)  
// Get dh error code that caused DH to kill the drive.

#define dba_fru_get_serial_number(m_handle, m_fru, m_buffer, m_buffer_size) \
        strncpy(m_buffer,((ADM_DATA *)(m_handle))->fru_tabl[m_fru].disk_obit.serial_num,m_buffer_size)  
// Get dh error code that caused DH to kill the drive.

#define dba_fru_get_peer_disk_state(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].peer_disk_state)
// Get the disk state reported by PEER

#define dba_fru_get_peer_disk_death_reason(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].disk_obit.peer_death_reason)
// Get the disk death reason reported by PEER

#define dba_fru_is_peer_disk_state_ready(m_handle, m_fru) \
        (((ADM_DATA *)(m_handle))->fru_tabl[m_fru].peer_disk_state == PFSM_STATE_READY)
// check if peer reported disk state as ready

/*********************
 * Glut Table Macros *
 *********************/

#define dba_unit_get_state(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].state)

#define dba_unit_is_state(m_handle, m_unit, m_state) \
        ((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].state == m_state) ? \
        TRUE : FALSE)

#define dba_unit_get_disk_io_count(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].disk_io_count)

#define dba_unit_get_dirty_word(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].dirty_word)

#define dba_unit_get_status(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].status)

#define dba_unit_is_status_bit(m_handle, m_unit, m_status) \
        ((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].status & m_status) ? \
        TRUE : FALSE)

#define dba_unit_get_capacity(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].capacity )

#define dba_unit_get_metadata_size(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].metadata_size )

#define dba_unit_get_rebuild_time(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].rebuild_time)

#define dba_unit_get_request_count(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].request_count)

#define dba_unit_get_timestamp(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].timestamp)

#define dba_unit_get_dirty_io_count(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].dirty_io_count)

#define dba_unit_get_chkpt_verify_time(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].chkpt_verify_time)

#define dba_unit_get_chkpt_verify_chkpt(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].chkpt_verify_checkpoint)
                
#define dba_unit_verify_priority_to_time(m_verify_priority) \
        (m_verify_priority * 3600)

#define dba_unit_verify_time_to_priority(m_verify_time) \
        (m_verify_time / 3600)

#define dba_unit_get_verify_priority(m_handle, m_unit) \
        (dba_unit_verify_time_to_priority(((ADM_DATA *)m_handle)->glut_tabl[m_unit].chkpt_verify_time))

#define dba_unit_get_assign_word(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].assign_word)

#define dba_unit_get_lu_attributes(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].lu_attributes)

#define dba_unit_is_lu_attributes_bit(m_handle, m_unit, m_lu_attributes_bit)\
        ((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].lu_attributes & \
            m_lu_attributes_bit) ? TRUE : FALSE)

#define dba_unit_get_suggested_hlu(m_handle, m_unit) \
        ((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].lu_attributes & \
            LU_ATTRIBUTES_VCX_MASK) >> LU_ATTRIBUTES_VCX_SHIFT)

#define dba_unit_get_lun_type(m_handle, m_unit) \
        ((!dba_unit_is_bound(m_handle, m_unit)) ?                  \
            LU_LUN_TYPE_NON_FLARE :                                \
            ((dba_unit_get_suggested_hlu(m_handle, m_unit) != 0) ? \
            LU_LUN_TYPE_CELERRA : LU_LUN_TYPE_FLARE))

#define dba_unit_is_sniff_verify_enabled(m_handle, m_unit)          \
        (dba_sysconfig_is_flare_characteristics_flags_bit(m_handle, \
         SYSCONFIG_USER_SNIFF_CONTROL_ENABLED) ?                    \
          dba_unit_is_flags_bit(m_handle,                           \
                                m_unit,                             \
                                VP_SNIFFING_ENABLED)                \
          : TRUE)

#define dba_unit_get_flags(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].flags)

#define dba_unit_is_flags_bit(m_handle, m_unit, m_flags_bits) \
        (((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].flags & \
            m_flags_bits) == m_flags_bits) ? TRUE : FALSE)

#define dba_unit_is_np_flags_bit(m_handle, m_unit, m_np_flags_bits) \
        (((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].np_flags & \
        m_np_flags_bits) == m_np_flags_bits) ? TRUE : FALSE)

#define dba_fru_is_np_flags_bit(m_handle, m_fru, m_np_flags_bits) \
        ((((ADM_DATA *)m_handle)->fru_tabl[m_fru].np_flags & m_np_flags_bits) == 0) ? FALSE : TRUE;

#define dba_unit_get_session_id(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].lun_SessionID)


/* The sniff rate is now always a #define, return the correct #define depending on the
 * type of unit.
 */
#define dba_unit_get_vp_sniff_time(m_handle, m_unit)                         \
( (dba_unit_get_type(m_handle, m_unit) == LU_HOT_SPARE) ?                    \
            VP_HOT_SPARE_SNIFF_RATE :                                        \
              (dba_unit_is_lu_attributes_bit(m_handle, m_unit, LU_GENERAL) ? \
               VP_DEFAULT_SNIFF_TIME :                                       \
                 ((ADM_DATA *)(m_handle))->glut_tabl[m_unit].vp_sniff_time))

#define dba_unit_get_partition_index(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].partition_index)

#define dba_unit_get_rg_next_lun(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].rg_next_lun)

#define dba_unit_get_L2_stage(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].L2_stage )

#define dba_unit_get_nhead(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].nhead)

#define dba_unit_get_ncyl(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].ncyl)

#define dba_unit_get_nsec(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].nsec)

#define dba_unit_get_bind_offset(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].bind_offset)

#define dba_unit_get_external_capacity(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].external_capacity)

#define dba_unit_get_aligned_sector(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].aligned_sector)


#define dba_unit_get_cache_params(m_handle, m_unit, m_p_ucache_p) \
{ \
        memset(m_p_ucache_p, 0, sizeof (UNIT_CACHE_PARAMS)); \
        memcpy(m_p_ucache_p, \
               &((ADM_DATA *)m_handle)->glut_tabl[m_unit].unit_cache_params, \
               sizeof (UNIT_CACHE_PARAMS)); \
}

#define dba_unit_is_type(m_handle, m_unit, m_lu_type) \
        ((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].unit_type == \
            m_lu_type) ? TRUE : FALSE)

#define dba_unit_get_num_frus(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].num_frus)

#define dba_unit_get_address_offset(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].address_offset)

#define dba_unit_get_fru(m_handle, m_unit, m_index) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].fru_info[m_index].fru)

#define dba_unit_is_peer_assigned(m_handle, m_unit) \
   (((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].state == \
         LU_PEER_ENABLED) || \
     (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].state == \
         LU_PEER_DEGRADED)) ? TRUE : FALSE)

#define dba_unit_is_bound(m_handle, m_unit) \
   (((((ADM_DATA *)(m_handle))->glut_tabl[m_unit].state != LU_UNBOUND) && \
     (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].state != LU_BIND) && \
     (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].state != \
         LU_PEER_BIND)) ? TRUE : FALSE)

#define dba_unit_get_rev_num(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].rev_num)

#define dba_unit_get_priority_lru_length(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.priority_lru_length)

#define dba_unit_get_cache_idle_threshold(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.cache_idle_threshold)

#define dba_unit_get_cache_be_req_size(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.read_be_req_size)

#define dba_unit_is_cache_config_flags_bit(m_handle, m_unit, m_config_flags) \
        ((((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.cache_config_flags & \
                m_config_flags) \
            ? TRUE : FALSE)

#define dba_unit_get_cache_config_flags(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.cache_config_flags)

#define dba_unit_get_cache_write_aside(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.cache_write_aside)

#define dba_unit_get_cache_dirty_word(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.cache_dirty_word)

#define dba_unit_get_cache_idle_delay(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.cache_idle_delay)

#define dba_unit_get_prefetch_idle_count(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.prefetch_idle_count)

#define dba_unit_get_L2_cache_dirty_word(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))-> \
            glut_tabl[m_unit].unit_cache_params.L2_cache_dirty_word)

#define dba_unit_get_total_element_count(m_handle, m_unit) \
        (((ADM_DATA *)(m_handle))->glut_tabl[m_unit].total_element_count)

#define dba_unit_am_i_default_owner(m_handle, m_unit) \
        ((((AM_I_ODD_CONTROLLER) && \
           (DBA_LU_DERIVE_DEFAULT_ASSIGN(m_handle, m_unit) == \
            LU_DEFAULT_ASSIGN_ODD_SP)) || \
           ((!AM_I_ODD_CONTROLLER) &&  \
            (DBA_LU_DERIVE_DEFAULT_ASSIGN(m_handle, m_unit) == \
             LU_DEFAULT_ASSIGN_EVEN_SP))) \
                                          ? TRUE : FALSE)

#define dba_unit_get_clean_status(m_handle, m_unit) \
    ((((ADM_DATA *)m_handle)->glut_tabl[m_unit].flags & \
        LU_UNIT_WRITTEN) ? TRUE : FALSE)


#define dba_frusig_get_cache(m_handle, m_fru, m_pfru_sig) \
        memcpy((m_pfru_sig), \
               &((ADM_DATA *)m_handle)->fru_sig_cache[(m_fru)].fru_sig, \
               sizeof(FRU_SIGNATURE))

#define dba_frusig_get_wwn_seed(m_pfru_sig) \
        ((m_pfru_sig)->wwn_seed)

#define dba_frusig_get_fru(m_pfru_sig) \
        ((m_pfru_sig)->fru)

#define dba_frusig_get_zeroed_mark(m_pfru_sig) \
        ((m_pfru_sig)->zeroed_mark)

#define dba_frusig_get_zeroed_stamp(m_pfru_sig) \
        ((m_pfru_sig)->zeroed_stamp)

#define dba_frusig_get_partition_lun(m_pfru_sig, partition) \
        ((m_pfru_sig)->partition_table[partition].lun)

#define dba_frusig_get_partition_timestamp(m_pfru_sig, partition) \
        ((m_pfru_sig)->partition_table[partition].timestamp)

#define dba_frusig_get_partition_group_id(m_pfru_sig, partition) \
        ((m_pfru_sig)->partition_table[partition].group_id)

#endif /* DBA_USE_DB_INTERFACE_FUNCTIONS */

/* The following macros do NOT have an associated function.
*/

#define dba_raidg_is_type(m_handle, m_group, m_unit_type) \
    ((((ADM_DATA *)(m_handle))->raid_group_tabl[m_group].unit_type == (m_unit_type)) ? TRUE : FALSE)

#define dba_raidg_get_exp_lun_internal_assign_failure_count(m_handle, m_group)   \
    (((ADM_DATA *)(m_handle))-> \
     raid_group_tabl[m_group].exp_lun_internal_assign_failure_count)

#define dba_sysconfig_get_platform_type(m_handle)    \
        (((ADM_DATA *)m_handle)->sys_config.flare_characteristics_flags & \
            SYSCONFIG_PLATFORM_TYPE_MASK)

UINT_32 dba_unit_get_pre_exp_offset64(ULONG64 m_handle,UINT_32 m_unit);
BOOL dba_unit_is_expanding64(ULONG64 _handle,UINT_32 _lun);
LBA_T DBA_LU_GET_EXP_CHKPT64(ULONG64 m_handle, UINT_32 _lun);
LBA_T dba_unit_get_capacity64(ULONG64 m_handle,UINT_32 m_unit);
BOOL dba_unit_is_state64(ULONG64 m_handle,UINT_32 m_unit,LU_STATE m_state);
UINT_32 dba_unit_get_num_frus64(ULONG64 m_handle,UINT_32 m_unit);
UINT_32 dba_unit_get_pre_exp_width64(ULONG64 m_handle,UINT_32 m_unit);
BOOL dba_unit_is_bound64(ULONG64 m_handle,UINT_32 m_unit);
UINT_32 dba_raidg_get_first_lun64(ULONG64 m_handle,UINT_32 m_group);
UINT_32 dba_raidg_get_num_luns64(ULONG64 m_handle,UINT_32 m_group);
raid_group_flags_T dba_raidg_get_state_flags64(ULONG64 handle,UINT_32 m_group);
LBA_T dba_raidg_get_physical_exp_chkpt64(ULONG64 handle, UINT_32 group);
BOOL dba_unit_is_lu_attributes_bit64(ULONG64 handle,UINT_32 m_unit,BITS_32 m_lu_attributes_bit);
LU_TYPE dba_unit_get_type64(ULONG64 m_handle,UINT_32 m_unit);
BOOL  dba_unit_is_type64(ULONG64 m_handle,UINT_32 m_unit,LU_TYPE m_lu_type);
UINT_32 dba_raidg_get_lun64(ULONG64 m_handle,UINT_32 _group,UINT_32 _partition_index);
BOOL dba_fru_is_a_swapped_in_spare64( ULONG64 handle, UINT_32 fru, BOOL *invalid_group_p, BOOL *invalid_lun_p, DBA_SWAP_RQ_TYPE spare_rq_type);
BOOL dba_fru_is_fru_flags_bit64(ULONG64 handle, UINT_32 fru, fru_fru_flags_T fflags_bit);
BOOL dba_unit_is_flags_bit64(ULONG64 handle, UINT_32 unit, UINT_64 flags_bits);
BOOL dba_unit_get_flags_bit64(ULONG64 handle, UINT_32 unit);
LBA_T dba_unit_get_exp_chkpt_phys64(ULONG64 handle, UINT_32 unit);
BOOL dba_fru_needs_eqz_or_paco64(ULONG64 handle, UINT_32 hs_fru);
UINT_32 dba_fru_get_group_id64(ULONG64 handle, UINT_32 fru, UINT_32 index);
fru_state_T dba_fru_get_state64(ULONG64 handle, UINT_32 fru);
BOOL dba_fru_is_state64(ULONG64 handle, UINT_32 fru, fru_state_T state);
BOOL dba_fru_is_db_rebuilding64(ULONG64 handle, UINT_32 fru);
UINT_32 dba_raidg_get_num_disks64(ULONG64 handle, UINT_32 group);
UINT_32 dba_raidg_get_fru_pos64 (ULONG64 handle, UINT_32 group, UINT_32 fru);
BOOL dba_unit_is_fru_present64(ULONG64 handle, UINT_32 unit, UINT_32 fru);
UINT_32 dba_fru_get_hot_fru_type64(ULONG64 handle, UINT_32 fru);
UINT_32 dba_unit_get_fru_info_flags64(ULONG64 handle, UINT_32 unit, UINT_32 fru_pos);
UINT_32 dba_fru_get_real_fru64(ULONG64 handle, UINT_32 fru, DBA_SWAP_RQ_TYPE req_type);
UINT_32 dba_fru_get_user_group_id64(ULONG64 handle, UINT_32 fru);
BITS_32 dba_unit_get_lu_attributes64(ULONG64 m_handle,UINT_32 m_unit);
UINT_32 dba_unit_get_rg_next_lun64(ULONG64 m_handle,UINT_32 m_unit);
LBA_T dba_unit_get_address_offset64(ULONG64 m_handle,UINT_32 m_unit);
BOOL dba_unit_is_flags_bit64(ULONG64 m_handle,UINT_32 m_unit, UINT_64 m_flags_bits);
UINT_32 dba_unit_get_fru64(ULONG64 m_handle,UINT_32 m_unit,UINT_32 m_index);
UINT_32 dba_fru_get_partition_lun64(ULONG64 handle, UINT_32 fru, UINT_32 partition);
LBA_T dba_fru_get_capacity64(ULONG64 handle, UINT_32 fru);
LBA_T dba_unit_get_corona_end64(ULONG64 adm_handle, UINT_32 lun);
LU_STATE dba_unit_get_state64(ULONG64 handle, UINT_32 unit);
BOOL dba_unit_is_state64(ULONG64 handle, UINT_32 unit, LU_STATE state);
BOOL dba_fru_is_rebuilding_raidg64(ULONG64 handle, UINT_32 fru, UINT_32 group);
BOOL dba_fru_is_rebuilding64(ULONG64 handle, UINT_32 fru);
BOOL dba_fru_needs_rebuild64(ULONG64 handle, UINT_32 fru);
LBA_T dba_raidg_get_sniff_verify_chkpt64( ULONG64 m_handle, UINT_32 rg );
LBA_T dba_raidg_get_bg_verify_chkpt64( ULONG64 m_handle, UINT_32 rg );
UINT_32 bgs_verify_percent_complete( UINT_32 lun, BOOL sv);

IMPORT BOOL CALL_TYPE dba_fru_is_state(OPAQUE_PTR handle,
                                       UINT_32 fru,
                                       fru_state_T state);
// returns TRUE is state of fru equals state passed in


IMPORT BOOL CALL_TYPE dba_unit_is_assigned(OPAQUE_PTR handle, UINT_32 unit);
// returns TRUE if unit is in one of the assigned states

IMPORT UINT_32 CALL_TYPE dba_unit_get_group_id(OPAQUE_PTR handle,
                                               UINT_32 unit);
// returns the group_id of the unit

/* Get the rg_attributes value. */

IMPORT BOOL CALL_TYPE
    dba_raidg_is_rg_attributes_bit(OPAQUE_PTR handle,
                                   UINT_32 group,
                                   BITS_32 rg_attributes_bits);
//get num luns

IMPORT UINT_32 CALL_TYPE dba_raidg_get_num_disks(OPAQUE_PTR handle,
                                                 UINT_32 group);

IMPORT UINT_32 CALL_TYPE dba_raidg_get_fru_num(OPAQUE_PTR handle,
                                               UINT_32 group,
                                               UINT_32 fru_position);
// returns the fru number in a group at fru_position

IMPORT UINT_32 CALL_TYPE dba_raidg_get_first_lun(OPAQUE_PTR handle,
                                                 UINT_32 group);
// get the first lun bound on the group

IMPORT UINT_32 CALL_TYPE dba_unit_get_pre_exp_width(OPAQUE_PTR handle,
                                                    UINT_32 unit);
// returns pre_exp_num_disks,
// the width of the array before it started expanding

IMPORT UINT_32 dba_raidg_get_num_exp_disks( OPAQUE_PTR handle, UINT_32 group);

IMPORT UINT_32 CALL_TYPE dba_unit_get_rebuild_priority(OPAQUE_PTR handle, UINT_32 unit);
// get the unit rebuild priority


IMPORT UINT_32 CALL_TYPE dba_unit_rebuild_priority_to_time(UINT_32 rebuild_priority);
// get the unit rebuild time from the priority

IMPORT UINT_32 dba_unit_rebuild_time_to_priority(UINT_32 rebuild_time); 
// get the unit rebuild priority from the time (should be exact time)

IMPORT UINT_32 dba_unit_rebuild_time_to_priority_rounded(UINT_32 rebuild_time, UINT_32 *ptr_rebuild_time);
// for debug only:  get rounded (down) rebuild priority

#endif
