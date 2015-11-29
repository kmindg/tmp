#ifndef ADM_UNIT_API_H
#define ADM_UNIT_API_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2006
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  adm_unit_api.h
 ***************************************************************************
 *
 *  Description
 *
 *      This file contains the external interface to the unit-related
 *      interfaces of the Admin Interface "adm" module in the Flare driver.
 *
 *      This file is included by adm_api.h, the top-level interface for
 *      the ADM module.
 *
 *  History:
 *
 *      06/15/01 CJH    Created
 *      10/17/01 CJH    Add missing fields from pg 2B
 *      11/01/01 HEW    Added ifdefs for C++ compilation
 ***************************************************************************/

/************
 * Literals
 ************/


/************
 *  Types
 ************/

// Number of elements in read/write histograms
#define ADM_HIST_SIZE   10


/***************************
 *  Macro Definitions
 ***************************/


/********************************
 *  Function Prototypes
 ********************************/
#if defined(__cplusplus)
#define CALL_TYPE __stdcall
extern "C"
{
#else
#define CALL_TYPE
#endif


// returns the percent completion of a rebuild on this unit (0-100)
UINT_32 CALL_TYPE dyn_unit_pct_rebuilt(OPAQUE_PTR handle, UINT_32 lun);

// returns the percent completion of a bind on this unit (0-100)
// returns ADM_INVALID_PERCENTAGE if no bind is in progress
UINT_32 CALL_TYPE dyn_unit_pct_bound(OPAQUE_PTR handle, UINT_32 lun);

// returns the percent completion of an expansion on this unit (0-100)
// returns ADM_INVALID_PERCENTAGE if no expansion is in progress
UINT_32 CALL_TYPE dyn_unit_pct_expanded(OPAQUE_PTR handle, UINT_32 lun);

// returns the percent completion of zeroing on this unit (0-100)
UINT_32 CALL_TYPE dyn_unit_pct_zeroed(OPAQUE_PTR handle, UINT_32 lun);

// returns the physical capacity of the unit in blocks
LBA_T CALL_TYPE dyn_unit_physical_capacity(OPAQUE_PTR handle, UINT_32 lun);

// returns the write cache hit ratio of the unit (0-100)
UINT_32 CALL_TYPE dyn_unit_write_hit_ratio(OPAQUE_PTR handle, UINT_32 lun);

// returns the percentage of the cache pages owned by this unit that
// are dirty (0-100)
UINT_32 CALL_TYPE dyn_unit_pct_dirty_pages(OPAQUE_PTR handle, UINT_32 lun);

// returns the number of write requests that accessed only pages 
// already in the write cache
UINT_32 CALL_TYPE dyn_unit_write_hits(OPAQUE_PTR handle, UINT_32 lun);

// returns the bind alignment offset of this unit
UINT_32 CALL_TYPE dyn_unit_bind_alignment_offset(OPAQUE_PTR handle,
                                                 UINT_32 lun);

// returns the number of blocks of unusable user space for this unit
LBA_T CALL_TYPE dyn_unit_unusable_user_space(OPAQUE_PTR handle, UINT_32 lun);

// returns whether this SP owns the unit (in Flare's view)
// NOTE: this may differ from host-side's view
BOOL CALL_TYPE dyn_unit_is_current_owner(OPAQUE_PTR handle, UINT_32 lun);

// returns the unit's rebuild time
UINT_32 CALL_TYPE dyn_unit_rebuild_time(OPAQUE_PTR handle, UINT_32 lun);

// returns the write cache idle threshold (0-254)
UINT_32 CALL_TYPE dyn_unit_write_cache_idle_threshold(OPAQUE_PTR handle,
                                                      UINT_32 lun);

// returns the write cache idle delay sec (0-254)
UINT_32 CALL_TYPE dyn_unit_write_cache_idle_delay(OPAQUE_PTR handle,
                                                  UINT_32 lun);

// returns the write aside size (16-65,534)
UINT_32 CALL_TYPE dyn_unit_write_aside_size(OPAQUE_PTR handle, UINT_32 lun);

// returns the verify time
UINT_32 CALL_TYPE dyn_unit_verify_time(OPAQUE_PTR handle, UINT_32 lun);

// returns the sniff rate
UINT_32 CALL_TYPE dyn_unit_sniff_rate(OPAQUE_PTR handle, UINT_32 lun);



// returns the logical address offset of the LUN within the RAID group
// (The GLUT contains the physical address offset.)
LBA_T CALL_TYPE dyn_unit_logical_address_offset(OPAQUE_PTR handle,
                                                UINT_32 lun);

// returns TRUE if the unit is performing a background verify
BOOL CALL_TYPE dyn_unit_background_verify_in_progress(OPAQUE_PTR handle,
                                                      UINT_32 lun);

// returns TRUE if the unit is freshly bound
BOOL CALL_TYPE dyn_unit_is_freshly_bound(OPAQUE_PTR handle,
                                         UINT_32 lun);

// Returns the type of disk the unit is bound accross
UINT_32 CALL_TYPE dyn_unit_disk_type(OPAQUE_PTR handle,
                                           UINT_32 lun);

// Returns true is the unit is cache dirty and because of this, we are
// unable to assign
BOOL CALL_TYPE dyn_unit_cache_dirty_cant_assign(OPAQUE_PTR handle, UINT_32 lun);


// Returns state of the unit. It will give all the possible states a unit can have
//See Dimms 217865
UINT_32 CALL_TYPE dyn_unit_get_state(OPAQUE_PTR handle, UINT_32 lun);

/*
 * Unit Performance
 */

// returns the read cache hit ratio of the unit (0-100)
UINT_32 CALL_TYPE dyn_unit_read_hit_ratio(OPAQUE_PTR handle, UINT_32 lun);

// returns the time that the performance data was generated, in the form
// 0xhhmmssxx, where hh is hours (0-23), mm is minutes (0-59), ss is
// seconds (0-59), and xx is hundredths of a second
// NOTE: this value is identical to the global timestamp returned by
// dyn_timestamp(), but is included here for Poll All LUNs and Poll LUN,
// since the global value would not be available
UINT_32 CALL_TYPE dyn_unit_timestamp(OPAQUE_PTR handle, UINT_32 lun);

// returns the number of read requests that accessed only pages 
// already in the read or write cache
UINT_32 CALL_TYPE dyn_unit_read_cache_hits(OPAQUE_PTR handle, UINT_32 lun);

// returns the number of read requests satisfied from write cache
UINT_32 CALL_TYPE dyn_unit_read_cache_hits_from_write_cache(OPAQUE_PTR handle, UINT_32 lun);

// returns the number of read requests that accessed pages not
// already in the read or write cache
UINT_32 CALL_TYPE dyn_unit_read_cache_misses(OPAQUE_PTR handle, UINT_32 lun);

// returns the number of blocks that were prefetched into the read cache
UINT_32 CALL_TYPE dyn_unit_blocks_prefetched(OPAQUE_PTR handle, UINT_32 lun);

// returns the number of blocks that were prefetched into the read cache
// but never accessed
UINT_32 CALL_TYPE dyn_unit_unread_prefetched_blocks(OPAQUE_PTR handle, 
                                                    UINT_32 lun);

// returns the number of times a write had to flush a page to make room
// in the cache
UINT_32 CALL_TYPE dyn_unit_forced_flushes(OPAQUE_PTR handle, UINT_32 lun);

// returns the number of times an I/O crossed a stripe boundary
UINT_32 CALL_TYPE dyn_unit_stripe_crossings(OPAQUE_PTR handle, UINT_32 lun);

// returns the number fast write cache hits for the unit
UINT_32 CALL_TYPE dyn_unit_fast_write_hits(OPAQUE_PTR handle, UINT_32 lun);

// returns the sum of lengths of current request queues seen by each 
// user request to this LUN
UINT_64 CALL_TYPE dyn_unit_sum_q_length_arrival(OPAQUE_PTR handle,
                                                UINT_32 lun);

// returns the number of times a user request arrived while at least one 
// other request was being processed by this unit
UINT_32 CALL_TYPE dyn_unit_arrivals_to_nonzero_q(OPAQUE_PTR handle,
                                                 UINT_32 lun);

// returns the number of blocks read
UINT_32 CALL_TYPE dyn_unit_blocks_read(OPAQUE_PTR handle,
                                       UINT_32 lun);

// returns the number of blocks written
UINT_32 CALL_TYPE dyn_unit_blocks_written(OPAQUE_PTR handle,
                                          UINT_32 lun);

/* number of total reads from disk on this unit
 */
UINT_32 CALL_TYPE dyn_unit_disk_reads(OPAQUE_PTR handle, UINT_32 lun, UINT_32 index);

/* number of total writes to disk on this unit
 */
UINT_32 CALL_TYPE dyn_unit_disk_writes(OPAQUE_PTR handle, UINT_32 lun, UINT_32 index);

/* number of total blocks read from disk on this unit
 */
UINT_64 CALL_TYPE dyn_unit_disk_blocks_read(OPAQUE_PTR handle, UINT_32 lun, UINT_32 index);

/* number of total blocks written to disk on this unit
 */
UINT_64 CALL_TYPE dyn_unit_disk_blocks_written(OPAQUE_PTR handle, UINT_32 lun, UINT_32 index);

/* number of total mr3 writes on this unit
 */
UINT_32 CALL_TYPE dyn_unit_mr3_writes(OPAQUE_PTR handle, UINT_32 lun);

/* average read time for this unit
 */
UINT_64 CALL_TYPE dyn_unit_cum_read_time(OPAQUE_PTR handle, UINT_32 lun);

/* average write time for this unit
 */
UINT_64 CALL_TYPE dyn_unit_cum_write_time(OPAQUE_PTR handle, UINT_32 lun);



// returns the hist_index element of the read_histogram array
// Element n of the array contains the number of reads that were between 
// 2**n-1 and 2**n blocks.
UINT_32 CALL_TYPE dyn_read_histogram(OPAQUE_PTR handle, 
                                     UINT_32 lun,
                                     UINT_32 hist_index);

// returns the number of reads that were larger than 1023 (2**10) blocks
UINT_32 CALL_TYPE dyn_read_histogram_overflow(OPAQUE_PTR handle, UINT_32 lun);

// returns the hist_index element of the write_histogram array
// Element n of the array contains the number of writes that were 
// between 2**n-1 and 2**n blocks.
UINT_32 CALL_TYPE dyn_write_histogram(OPAQUE_PTR handle, 
                                      UINT_32 lun,
                                      UINT_32 hist_index);

// returns the number of writes that were larger than 1023 (2**10) blocks
UINT_32 CALL_TYPE dyn_write_histogram_overflow(OPAQUE_PTR handle, UINT_32 lun);

#if defined(__cplusplus)
}
#endif
#endif /* ADM_UNIT_API_H */

