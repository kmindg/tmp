/*****************************************************************************
 * Copyright (C) EMC Corp. 2008
 * All rights reserved.
 * Licensed material - Property of EMC Corporation.
 *****************************************************************************/

/*****************************************************************************
 *  File: pmp_api.h
 *****************************************************************************
 *
 * DESCRIPTION:
 *    This file provides function declarations for the generic API functions
 * that can be called into from outside modules.
 *
 * NOTES:
 *
 * HISTORY:
 *      01.08.2008    Bill Buckley         Created
 *
 *****************************************************************************/
#ifndef __PMP_API_H__
#define __PMP_API_H__

#include "generic_types.h"

#ifdef C4_INTEGRATED
#include "environment.h"
#define C4PMP
#endif /* C4_INTEGRATED - C4HW */
//#endif

/*Function Declarations*/
/*API Function*/
//
//  _PMP is defined in Makefile.csx when PMP is built.
//
#if defined ( _PMP_ )
#define PMPAPI CSX_MOD_EXPORT
#else
#define PMPAPI CSX_MOD_IMPORT
#endif

typedef enum {
    // We get into this state at first startup ever. Also when 
    //  SSD replaced/failed (assuming persistent RAM not preserved) or explicitly 
    //  cleared. Generally, make no assumption about what's there in this state
    PMP_STATE_ZERO = 0,
    
    // Powerloss occured and memory went RAM->SSD->RAM cycle. Not yet 
    //  armed, so another powerloss (TODO: reboot too?) will restore SSD->RAM again.
    // Memory regions off-limit.
    PMP_STATE_RESTORED,
    
    // Powerloss occured and memory didn't go RAM->SSD->RAM cycle. 
    PMP_STATE_RESTORE_FAILED,
    
    // Ready to go
    // Memory regions off-limit.
    PMP_STATE_ARMED,

    // Disabled by config/admin
    PMP_STATE_DISABLED,
    
    // Well, nothing we can do here
    PMP_STATE_FAILED
} PMP_STATE;


PMPAPI PMP_STATE        pmp_state(void);
PMPAPI char*            pmp_stringize_state(PMP_STATE);
PMPAPI PMP_STATE        pmp_reset(void); //< (sync) move to PMP_STATE_ZERO unless in DISABLED or FAILED
PMPAPI PMP_STATE        pmp_arm(void); //< (sync) move to PMP_STATE_ARMED if in ZERO, return new state. Do not call while in RESTORE_FAILED condition.

// PMPAPI csx_status_e     pmp_toggle_pmp_functionality(csx_bool_t toggle_on);

// PMPAPI csx_bool_t       pmp_get_image_valid(void);
// PMPAPI csx_status_e     pmp_clear_image_valid(void);
// PMPAPI csx_bool_t       pmp_was_image_restored(csx_bool_t clear);

PMPAPI csx_bool_t       pmp_register_memory_region(UINT_64 ptr, UINT_64 size);
PMPAPI csx_bool_t       pmp_is_memory_region_registered(UINT_64 ptr, UINT_64 size);
PMPAPI csx_bool_t       pmp_clear_region(UINT_64 ptr, UINT_64 size);
PMPAPI csx_bool_t       pmp_clear_all_regions(void);

//TODO: PMPAPI csx_bool_t       pmp_commit(void (*done)(csx_bool_t, csx_ptrhld_t, csx_ptrhld_t), csx_ptrhld_t arg1, csx_ptrhld_t arg2);

#endif
