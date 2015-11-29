#ifndef ADM_RAID_GROUP_API_H
#define ADM_RAID_GROUP_API_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001 - 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  adm_raid_group_api.h
 ***************************************************************************
 *
 *  Description
 *
 *      This file contains the external interface to the RAID Group-related
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

/************
 * Literals
 ************/


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

// returns the number of free sectors available on this RAID group
LBA_T CALL_TYPE dyn_rg_free_capacity(OPAQUE_PTR handle, UINT_32 group_id);

// returns the total number of sectors available on this RAID group
LBA_T CALL_TYPE dyn_rg_physical_capacity(OPAQUE_PTR handle,
                                         UINT_32 group_id);

// returns the number of sectors available in the largest free space on
// this RAID group
LBA_T CALL_TYPE dyn_rg_largest_contig_free_space(OPAQUE_PTR handle,
                                                 UINT_32 group_id);

// returns percentage complete for expansion or defragmentation (0-99)
// returns ADM_INVALID_PERCENTAGE if no expansion or defragmentation
// is in progress
UINT_32 CALL_TYPE dyn_rg_percent_expanded(OPAQUE_PTR handle,
                                          UINT_32 group_id);

// returns TRUE if the RAID Group is expanding
BOOL CALL_TYPE dyn_rg_expand_lun(OPAQUE_PTR handle, UINT_32 group_id);

// returns TRUE if the RAID Group contains Hi5 LUNs
BOOL CALL_TYPE dyn_rg_l2c_defined(OPAQUE_PTR handle, UINT_32 group_id);

// returns TRUE if the Hi5 Log cannot be used for lack of extended
// memory
BOOL CALL_TYPE dyn_rg_memory_needed(OPAQUE_PTR handle, UINT_32 group_id);


// State of the Hi5 Log
UINT_32 CALL_TYPE dyn_rg_hi5_log_state(OPAQUE_PTR handle, UINT_32 group_id);


// Returns the logical capacity of the RAID group
// Logical capacity is the physical capacity minus private space, parity 
// drives, and hidden LUNs (Hi5 LOG)
LBA_T CALL_TYPE dyn_rg_logical_capacity(OPAQUE_PTR handle,
                                        UINT_32 group_id);

UINT_32 CALL_TYPE dyn_rg_drive_type(OPAQUE_PTR handle, UINT_32 group_id);

LBA_T CALL_TYPE dyn_rg_min_hot_spare_size(OPAQUE_PTR handle, UINT_32 group_id);

LBA_T CALL_TYPE dyn_rg_max_hot_spare_size(OPAQUE_PTR handle, UINT_32 group_id);

// Returns TRUE if the RAID Group is in the standby state
BOOL CALL_TYPE dyn_rg_in_standby(OPAQUE_PTR handle, UINT_32 group_id);

// TRUE if RAID Group can be spun down
BOOL CALL_TYPE dyn_rg_power_saving_capable(OPAQUE_PTR handle, UINT_32 group_id);

// For admin use: returns RAID Group idle time in minutes.
UINT_64 CALL_TYPE dyn_rg_get_idle_time_for_standby(OPAQUE_PTR handle,
                                                   UINT_32 group);

// Returns Element size of RAID Group 
UINT_32 CALL_TYPE dyn_rg_element_size(OPAQUE_PTR handle, UINT_32 group_id);

#if defined(__cplusplus)
}
#endif
#endif /* ADM_RAID_GROUP_API_H */

