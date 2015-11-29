#ifndef ADM_FRU_API_H
#define ADM_FRU_API_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  adm_fru_api.h
 ***************************************************************************
 *
 *  Description
 *
 *      This file contains the external interface to the FRU-related
 *      interfaces of the Admin Interface "adm" module in the Flare driver.
 *
 *      This file is included by adm_api.h, the top-level interface for
 *      the ADM module.
 *
 *  History:
 *
 *      06/15/01 CJH    Created
 *      11/01/01 HEW    Added ifdefs for C++ compilation
 ***************************************************************************/

// #include "core_config.h"
// #include "k10defs.h"
// #include "sunburst.h"

#include "dh_export.h"

/************
 * Literals
 ************/

#define ADM_FRU_VENDOR_ID_LEN    8
#define ADM_FRU_PRODUCT_ID_LEN  16
#define ADM_FRU_PRD_REV_LEN      9
#define ADM_FRU_ADTL_INQ_LEN    13



/************
 *  Types
 ************/


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

// returns the FRU state, see sunburst.h for values
UINT_32 CALL_TYPE  dyn_fru_state(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of blocks reserved for use by the SP
LBA_T CALL_TYPE  dyn_fru_private_space_sectors(OPAQUE_PTR handle, UINT_32 fru);

// returns TRUE if the vendor ID, product ID, and product revision level
// are valid
BOOL CALL_TYPE  dyn_fru_inquiry_block_valid(OPAQUE_PTR handle, UINT_32 fru);

// fills in the buffer with the vendor ID string
// the buffer_size parameter indicates the maximum size accepted by the 
// caller
// the return value indicates the actual number of bytes entered into 
// the buffer (ADM_PG34_VENDOR_ID_LEN)
UINT_32 CALL_TYPE  dyn_fru_vendor_id(OPAQUE_PTR handle,
                          UINT_32 fru,
                          UINT_32 buffer_size,
                          TEXT *buffer);

// fills in the buffer with the product ID string
// the buffer_size parameter indicates the maximum size accepted by the 
// caller
// the return value indicates the actual number of bytes entered into 
// the buffer (ADM_PG34_PRODUCT_ID_LEN)
UINT_32 CALL_TYPE  dyn_fru_product_id(OPAQUE_PTR handle,
                           UINT_32 fru,
                           UINT_32 buffer_size,
                           TEXT *buffer);

// fills in the buffer with the product revision level string
// the buffer_size parameter indicates the maximum size accepted by the 
// caller
// the return value indicates the actual number of bytes entered into 
// the buffer (ADM_PG34_PRD_REV_LEN)
UINT_32 CALL_TYPE  dyn_fru_product_rev(OPAQUE_PTR handle,
                            UINT_32 fru,
                            UINT_32 buffer_size,
                            TEXT *buffer);

// fills in the buffer with the additional inquiry info (DG part number)
// string
// the buffer_size parameter indicates the maximum size accepted by the 
// caller
// the return value indicates the actual number of bytes entered into 
// the buffer (ADM_PG34_ ADTL_INQ_LEN)
UINT_32 CALL_TYPE  dyn_fru_addl_inquiry_info(OPAQUE_PTR handle,
                                  UINT_32 fru,
                                  UINT_32 buffer_size,
                                  TEXT *buffer);

// returns TRUE if this is an expansion disk
BOOL CALL_TYPE  dyn_fru_expansion_disk(OPAQUE_PTR handle, UINT_32 fru);

// returns TRUE if this disk is bound as a hot spare
BOOL CALL_TYPE  dyn_fru_hot_spare(OPAQUE_PTR handle, UINT_32 fru);

// returns TRUE if this disk needs formatting
BOOL CALL_TYPE  dyn_fru_formatted_state(OPAQUE_PTR handle, UINT_32 fru);

#ifdef C4_INTEGRATED
// returns drive RPM
UINT_32 CALL_TYPE  dyn_fru_drive_rpm(OPAQUE_PTR handle, UINT_32 fru);

// returns drive percentage equalizing
DWORD CALL_TYPE  dyn_fru_percent_eqz(OPAQUE_PTR handle, UINT_32 fru);

#endif /* C4_INTEGRATED - C4ARCH - admin_topology - C4BUG - dead code ? */

// returns TRUE if this disk contains a level 2 cache partition
BOOL CALL_TYPE  dyn_fru_level2_cache(OPAQUE_PTR handle, UINT_32 fru);

// fills in the buffer with the serial number string
// the buffer_size parameter indicates the maximum size accepted by the 
// caller
// the return value indicates the actual number of bytes entered into 
// the buffer (up to LOG_DE_PROD_SERIAL_SIZE, but may be less)
UINT_32 CALL_TYPE  dyn_fru_serial_number(OPAQUE_PTR handle,
                              UINT_32 fru,
                              UINT_32 buffer_size,
                              TEXT *buffer);

// returns percentage zeroed (or INVALID_PERCENTAGE)
UINT_32 CALL_TYPE  dyn_fru_percentage_zeroed(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of sectors unused due to a LUN bound with both
// system & normal disks
LBA_T CALL_TYPE  dyn_fru_unused_sectors(OPAQUE_PTR handle, UINT_32 fru);

// fills in the buffer with the serial number string
// the buffer_size parameter indicates the maximum size accepted by the 
// caller
// the return value indicates the actual number of bytes entered into 
// the buffer (up to LOG_DE_TLA_PART_NUM_SIZE, but may be less)
UINT_32 CALL_TYPE dyn_fru_tla_part_number(OPAQUE_PTR handle,
                                          UINT_32 fru,
                                          UINT_32 buffer_size,
                                          TEXT *buffer);

// Returns the drive class for the given fru
UINT_32 CALL_TYPE dyn_fru_drive_type(OPAQUE_PTR handle, UINT_32 fru);

// Returns the drive maximum speed CAPABILITY for the given fru
UINT_32 CALL_TYPE dyn_fru_drive_speed_capability(OPAQUE_PTR handle, UINT_32 fru);

UINT_32 CALL_TYPE dyn_ofd_drive_fail_code(OPAQUE_PTR handle, UINT_32 fru);

UINT_32 CALL_TYPE dyn_fru_prior_product_rev(OPAQUE_PTR handle, UINT_32 fru, UINT_32 buffer_size, TEXT *buffer);

UINT_32 CALL_TYPE dyn_fru_ofd_state(OPAQUE_PTR handle, UINT_32 fru);

/*
 * FRU Performance
 */

// returns the number of hard read errors on this FRU
UINT_32 CALL_TYPE  dyn_fru_hard_read_errors(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of hard write errors on this FRU
UINT_32 CALL_TYPE  dyn_fru_hard_write_errors(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of soft read errors on this FRU
UINT_32 CALL_TYPE  dyn_fru_soft_read_errors(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of soft write errors on this FRU
UINT_32 CALL_TYPE  dyn_fru_soft_write_errors(OPAQUE_PTR handle, UINT_32 fru);

// returns the average disk request service time for this FRU
UINT_32 CALL_TYPE  dyn_fru_average_service_time(OPAQUE_PTR handle,
                                                UINT_32 fru);

// returns the average disk sector address difference for this FRU
// NOTE: this may need to return LBA_T in the future
UINT_32 CALL_TYPE  dyn_fru_average_address_diff(OPAQUE_PTR handle,
                                                UINT_32 fru);

// returns the number of read operations for this FRU
UINT_32 CALL_TYPE  dyn_fru_read_count(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of write operations for this FRU
UINT_32 CALL_TYPE  dyn_fru_write_count(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of read retries for this FRU
UINT_32 CALL_TYPE  dyn_fru_read_retries(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of write retries for this FRU
UINT_32 CALL_TYPE  dyn_fru_write_retries(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of remapped sectors for this FRU
UINT_32 CALL_TYPE  dyn_fru_num_remapped_sectors(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of sectors read from this FRU
UINT_32 CALL_TYPE  dyn_fru_blocks_read(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of sectors written to this FRU
UINT_32 CALL_TYPE  dyn_fru_blocks_written(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of sectors seeked while accessing this FRU
ULONGLONG CALL_TYPE  dyn_fru_sum_of_blocks_seeked(OPAQUE_PTR handle,
                                                  UINT_32 fru);

// returns the sum of the queue lengths seen by requests arriving for
// this FRU
UINT_32 CALL_TYPE  dyn_fru_sum_queue_len_on_arrival(OPAQUE_PTR handle,
                                         UINT_32 fru);

// returns the number of requests which encountered at least one other
// request in progress on arrival
UINT_32 CALL_TYPE  dyn_fru_arrivals_to_nonzero_q(OPAQUE_PTR handle,
                                                 UINT_32 fru);

// returns the number clock ticks this FRU was busy
UINT_32 CALL_TYPE  dyn_fru_busy_ticks(OPAQUE_PTR handle, UINT_32 fru);

// returns the number clock ticks this FRU was idle
UINT_32 CALL_TYPE  dyn_fru_idle_ticks(OPAQUE_PTR handle, UINT_32 fru);

// returns TRUE is spin down has been qualified on this drive type
BOOL CALL_TYPE dyn_fru_spindown_qualified(OPAQUE_PTR handle, UINT_32 fru);

// returns TRUE if this particular drive is allowed to be spun down
BOOL CALL_TYPE dyn_fru_power_saving_capable(OPAQUE_PTR handle, UINT_32 fru);

// returns the drive's spinup/spindown state
DH_STANDBY_STATE CALL_TYPE dyn_fru_spindown_state(OPAQUE_PTR handle, UINT_32 fru);

// returns the number clock ticks this FRU was spinning
UINT_32 CALL_TYPE dyn_fru_spinning_ticks(OPAQUE_PTR handle, UINT_32 fru);

// returns the number clock ticks this FRU was in the standby state
UINT_32 CALL_TYPE  dyn_fru_standby_ticks(OPAQUE_PTR handle, UINT_32 fru);

// returns the number of times this fru was spun up from the standby state
UINT_32 CALL_TYPE  dyn_fru_spinups(OPAQUE_PTR handle, UINT_32 fru);

#if defined(__cplusplus)
}
#endif
#endif /* ADM_FRU_API_H */

