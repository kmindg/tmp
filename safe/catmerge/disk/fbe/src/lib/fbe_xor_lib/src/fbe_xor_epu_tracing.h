#ifndef FBE_XOR_EPU_TRACING_H
#define FBE_XOR_EPU_TRACING_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2006-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file xor_epu_tracing.h
 ***************************************************************************
 *
 * @brief
 *   This header file contains structures and definitions
 *   related to tracing for XOR Eval Parity Unit.
 *
 * @notes
 *
 * @author
 *
 *   07/31/06: Rob Foley -- Created.
 *
 *
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/

#include "fbe_xor_private.h"
#include "fbe_xor_epu_tracing.h"
#include "fbe_xor_raid6.h"
#include "fbe/fbe_library_interface.h"

/************************
 * STRUCTURE DEFINITIONS.
 ************************/

/*!*************************************************
 * @struct fbe_xor_epu_tracing_record_t 
 * @brief 
 * This is the individual record we use for keeping track of each loop through EPU.
 **************************************************/
typedef struct fbe_xor_epu_tracing_record_s
{
    fbe_u32_t error_flags[FBE_XOR_ERR_TYPE_MAX];
}
fbe_xor_epu_tracing_record_t;

/**************************************************
 * fbe_xor_epu_state_history_record_t
 * This is the individual record we use for keeping
 * track of state history for each loop through EPU.
 **************************************************/
typedef struct fbe_xor_epu_state_history_record_s
{
    fbe_u16_t fatal_key;
    fbe_u16_t fatal_cnt;
    fbe_u16_t scratch_state;
}
fbe_xor_epu_state_history_record_t;

/* This is the number of times we allow Raid 6 to loop through
 * evaluate parity unit.
 */
#define FBE_XOR_EPU_MAX_LOOPS 5

/**************************************************
 * fbe_xor_epu_tracing_global_t
 * This is the main structure we use for keeping
 * track of the EPU activities.
 **************************************************/
typedef struct fbe_xor_epu_tracing_global_s
{
    fbe_bool_t b_initialized;
    fbe_u16_t loop_count;
    fbe_xor_epu_tracing_record_t record[FBE_XOR_EPU_MAX_LOOPS];
    fbe_xor_epu_state_history_record_t state_history[FBE_XOR_EPU_MAX_LOOPS];
}
fbe_xor_epu_tracing_global_t;

extern fbe_xor_epu_tracing_global_t fbe_xor_epu_tracing;

/************************
 * PROTOTYPE DEFINITIONS.
 ************************/
/* fbe_xor_epu_tracing.c
 */
fbe_status_t fbe_xor_epu_trace_error(fbe_xor_scratch_r6_t *const scratch_p,
                               const fbe_xor_error_t      *const eboard_p,
                               const fbe_u16_t        err_posmask,
                               fbe_xor_error_type_t  err_type, 
                               const fbe_u32_t         instance,
                               const fbe_bool_t            uncorrectable,
                               const char *const     function,
                               const fbe_u32_t         line);

fbe_bool_t fbe_xor_epu_trace_is_invalidated(const fbe_u16_t err_posmask, 
                                  const fbe_sector_trace_error_flags_t trace_flag, 
                                  const fbe_xor_error_t *const eboard_p);

/************************
 * INLINE DEFINITIONS.
 ************************/


/* XOR_QUICK_INIT_EPU_GLOBAL
 * Setup the EPU Global quickly.
 */
static __inline void fbe_xor_epu_trc_quick_init( void )
{
#if 0
    fbe_xor_epu_tracing.b_initialized = FBE_FALSE;
    fbe_xor_epu_tracing.loop_count = 0;
#endif
    return;
}

/* fbe_xor_epu_trc_init_loop
 * Clear out the loop count.
 */
static __inline void fbe_xor_epu_trc_init_loop( void )
{
#if 0
    fbe_xor_epu_tracing.loop_count = 0;
#endif
    return;
}


/* XOR_INITIALIZE_EPU_GLOBAL
 * Setup the EPU Global fully.
 */
static __inline void fbe_xor_epu_trc_init_global(void)
{
#if 0
    memset(&fbe_xor_epu_tracing, 0, sizeof(fbe_xor_epu_tracing_global_t));
    fbe_xor_epu_tracing.b_initialized = FBE_TRUE;
#endif    
    return;
}

static __inline fbe_status_t fbe_xor_epu_trc_inc_loop( void )
{
#if 0
    fbe_xor_epu_tracing.loop_count++;

    if (XOR_COND(fbe_xor_epu_tracing.loop_count >= FBE_XOR_EPU_MAX_LOOPS ))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "fbe_xor_epu_tracing.loop_count 0x%x >= FBE_XOR_EPU_MAX_LOOPS 0x%x \n",
                              fbe_xor_epu_tracing.loop_count,
                              FBE_XOR_EPU_MAX_LOOPS);

        return FBE_STATUS_GENERIC_FAILURE;
    }
#endif
    return FBE_STATUS_OK;
}

/* The purpose of the below function is to
 * display something whenever a given error is encountered.
 * This is a generic routine that can be used anywhere in XOR.
 */
static __inline void fbe_xor_trace_error( const fbe_xor_error_type_t err_type,
                                   const fbe_u32_t instance,
                                   const fbe_lba_t lba )
{
    const char *err_type_string_p;
    const char * fbe_xor_get_error_type_string(const fbe_xor_error_type_t err_type);
    
    err_type_string_p = fbe_xor_get_error_type_string(err_type);
    
    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                          "XOR found %s (0x%x) instance: %d lba:%llX\n",
                          err_type_string_p,
                          err_type,
                          instance,
                          (unsigned long long)lba);
    return;
}
/* end fbe_xor_trace_error */

/* The purpose of the below function is to
 * display something whenever a given error is encountered.
 * This routine should only be used in XOR for R6 epu.
 */
static __inline void fbe_xor_epu_trc_log_error_info(fbe_xor_error_type_t err_type, fbe_u32_t instance )
{
#if 0
    fbe_lba_t fbe_xor_raid6_scratch_get_lba(void);
    fbe_lba_t lba = fbe_xor_raid6_scratch_get_lba();
    
    fbe_xor_trace_error(err_type, instance, lba);
#endif
    return;
}

/* This macro adds the function and line number to the fbe_xor_epu_trace_error
 * call.
 */
#define FBE_XOR_EPU_TRC_LOG_ERROR(m_scratch_p, m_eboard_p, m_err_posmask,  \
                              m_err_type, m_instance, m_uncorrectable) \
    fbe_xor_epu_trace_error(m_scratch_p, m_eboard_p, m_err_posmask,        \
                        m_err_type, m_instance, m_uncorrectable,       \
                        __FUNCTION__, __LINE__)


/* Get an error for the given type.
 */
static __inline fbe_status_t fbe_xor_epu_trc_get_flags( fbe_xor_error_type_t err_type,
                                                 fbe_u32_t * const error_p)
{
    /*! @todo This code no longer works due to the fact that it uses globals.  
     *        This code is commented out since it is not critical. 
     */
#if 0
    fbe_u32_t return_val = 0;
    fbe_u16_t loop_index;

    if (XOR_COND(fbe_xor_epu_tracing.loop_count >= FBE_XOR_EPU_MAX_LOOPS ))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "fbe_xor_epu_tracing.loop_count 0x%x >= FBE_XOR_EPU_MAX_LOOPS 0x%x \n",
                              fbe_xor_epu_tracing.loop_count,
                              FBE_XOR_EPU_MAX_LOOPS);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(loop_index = 0; loop_index < fbe_xor_epu_tracing.loop_count + 1; loop_index++)
    {
        return_val |= fbe_xor_epu_tracing.record[loop_index].error_flags[err_type];
    }

    *error_p = return_val;
#endif
    return FBE_STATUS_OK;
}
/* end fbe_xor_epu_trc_get_flags() */


/* XOR_EPU_TRC_INIT_STATE_HISTORY
 * Save state history for this loop.
 */
static __inline fbe_status_t fbe_xor_epu_trc_save_state_history( fbe_u16_t loop_count,
                                                                 fbe_u16_t fatal_key,
                                                                 fbe_u16_t fatal_cnt,
                                                                 fbe_u16_t scratch_state )
{
    /*! @todo This code no longer works due to the fact that it uses globals.  
     *        This code is commented out since it is not critical. 
     */
#if 0
    /* Validate the loop count.
     */
    if (XOR_COND(loop_count > FBE_XOR_EPU_MAX_LOOPS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "loop_count 0x%x is either less than 0 or greater than FBE_XOR_EPU_MAX_LOOPS 0x%x \n",
                              loop_count,
                              FBE_XOR_EPU_MAX_LOOPS);

        return FBE_STATUS_GENERIC_FAILURE;
    }
                

    fbe_xor_epu_tracing.state_history[loop_count].fatal_key = fatal_key;
    fbe_xor_epu_tracing.state_history[loop_count].fatal_cnt = fatal_cnt;
    fbe_xor_epu_tracing.state_history[loop_count].scratch_state = scratch_state;
#endif
    return FBE_STATUS_OK;
}
#endif
/* end FBE_XOR_EPU_TRACING_H */
