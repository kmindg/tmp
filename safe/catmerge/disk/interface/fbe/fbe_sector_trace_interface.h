#ifndef FBE_SECTOR_TRACE_INTERFACE_H
#define FBE_SECTOR_TRACE_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/***************************************************************************
 *
 * @brief
 *
 *  This file contains structures and definitions needed by the
 *  Fbe Sector Tracing facility.
 *
 * @author
 *
 *  01/13/2010  - Omer Miranda 
 *
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_encryption.h"

/*************************
 * LITERAL DEFINITIONS
 *************************/
#define FBE_SECTOR_TRACE_MAX_COUNT          ((fbe_u32_t)-1) 
/*
 *  FBE_SECTOR_TRACE_ERROR_FLAG_LOC   - This flag allows the `location' (function and line)
 *                    to be prepended to the trace entry.  ALL generated
 *                    trace entries will have the location prepended!!
 */
#define FBE_SECTOR_TRACE_ERROR_FLAG_LOC   0x80000000         /* If this flag is set add function and line information */

enum {
    /*! Number of Milliseconds in which we are allowed to fill the buffer. 
     *  If we have filled the buffer in less then this amount of time,
     *  then we will wait for this time to expire before filling the buffer again.
     */
    FBE_SECTOR_TRACE_FILL_BUFFER_QUANTA_MS = 1000,

    /*! Total size of our trace buffer. 
     */
    FBE_SECTOR_TRACE_ENTRIES = 2 * 1024,
    /*! Number of trace entries consumed for one sector. 
     */
    FBE_SECTOR_TRACE_ENTRIES_PER_SECTOR = 34,
};
/*
 *  fbe_sector_trace_error_level
 *      These are the different levels of severity or debug for sector tracing
 *      
 */
typedef enum fbe_sector_trace_error_level_e
{
    FBE_SECTOR_TRACE_ERROR_LEVEL_CRITICAL = 0,
    FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR,
    FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING,
    FBE_SECTOR_TRACE_ERROR_LEVEL_INFO,
    FBE_SECTOR_TRACE_ERROR_LEVEL_DATA,
    FBE_SECTOR_TRACE_ERROR_LEVEL_DEBUG,
    FBE_SECTOR_TRACE_ERROR_LEVEL_VERBOSE,
    FBE_SECTOR_TRACE_ERROR_LEVEL_COUNT,
    FBE_SECTOR_TRACE_ERROR_LEVEL_MAX = (FBE_SECTOR_TRACE_ERROR_LEVEL_VERBOSE),
    
    FBE_SECTOR_TRACE_DEFAULT_ERROR_LEVEL = FBE_SECTOR_TRACE_ERROR_LEVEL_DATA    
}fbe_sector_trace_error_level_t;

typedef enum fbe_sector_trace_error_flags_e
{
    FBE_SECTOR_TRACE_ERROR_FLAG_NONE    = 0x00000000,   /* No problems detected. */
    FBE_SECTOR_TRACE_ERROR_FLAG_UCRC    = 0x00000001,   /* CRC Error where we don't recognize pattern */
    FBE_SECTOR_TRACE_ERROR_FLAG_COH     = 0x00000002,   /* consistency error */
    FBE_SECTOR_TRACE_ERROR_FLAG_TS      = 0x00000004,   /* timestamp error */
    FBE_SECTOR_TRACE_ERROR_FLAG_WS      = 0x00000008,   /* writestamp error */
    FBE_SECTOR_TRACE_ERROR_FLAG_SS      = 0x00000010,   /* shedstamp error */
    FBE_SECTOR_TRACE_ERROR_FLAG_POC     = 0x00000020,   /* Parity of checksums error. (R6) */
    FBE_SECTOR_TRACE_ERROR_FLAG_NPOC    = 0x00000040,   /* Multiple Parity of checksums error. (R6)  */
    FBE_SECTOR_TRACE_ERROR_FLAG_UCOH    = 0x00000080,   /* An unknown coherency error occurred. (R6) */
    FBE_SECTOR_TRACE_ERROR_FLAG_ZERO    = 0x00000100,   /* Should have been zeroed and was not. (R6) */
    FBE_SECTOR_TRACE_ERROR_FLAG_LBA     = 0x00000200,   /* An LBA stamp error (w/o CRC) has occurred */
    FBE_SECTOR_TRACE_ERROR_FLAG_RCRC    = 0x00000400,   /* Retried crc error. */
    FBE_SECTOR_TRACE_ERROR_FLAG_UNU1    = 0x00000800,   /* Currently not used */
    FBE_SECTOR_TRACE_ERROR_FLAG_RINV    = 0x00001000,   /* RAID Purposefully invalidated this sector (DL) */
    FBE_SECTOR_TRACE_ERROR_FLAG_RAID    = 0x00002000,   /* Generic RAID error (rebuild failed, injection mismatch etc.)*/
    FBE_SECTOR_TRACE_ERROR_FLAG_BAD_CRC = 0x00004000,   /* Client sent RAID a sector with bad CRC */
    FBE_SECTOR_TRACE_ERROR_FLAG_ECOH    = 0x00008000,   /* Expected COH eror*/ 
    FBE_SECTOR_TRACE_ERROR_FLAG_MAX     = (FBE_SECTOR_TRACE_ERROR_FLAG_ECOH), /* Terminator for enumeration */
    FBE_SECTOR_TRACE_ERROR_FLAG_MASK    = (FBE_SECTOR_TRACE_ERROR_FLAG_MAX  | (FBE_SECTOR_TRACE_ERROR_FLAG_MAX - 1)),
    FBE_SECTOR_TRACE_ERROR_FLAG_COUNT   = (        4 + \
                                                  4 + \
                                                 4 + \
                                                4        ),
    FBE_SECTOR_TRACE_DEFAULT_ERROR_FLAGS = (FBE_SECTOR_TRACE_ERROR_FLAG_UCRC | FBE_SECTOR_TRACE_ERROR_FLAG_COH  | FBE_SECTOR_TRACE_ERROR_FLAG_TS   |
                                            FBE_SECTOR_TRACE_ERROR_FLAG_WS   | FBE_SECTOR_TRACE_ERROR_FLAG_SS   | FBE_SECTOR_TRACE_ERROR_FLAG_POC  |
                                            FBE_SECTOR_TRACE_ERROR_FLAG_NPOC | FBE_SECTOR_TRACE_ERROR_FLAG_UCOH | FBE_SECTOR_TRACE_ERROR_FLAG_ZERO |
                                            FBE_SECTOR_TRACE_ERROR_FLAG_LBA  | FBE_SECTOR_TRACE_ERROR_FLAG_RCRC | FBE_SECTOR_TRACE_ERROR_FLAG_RAID |
                                            FBE_SECTOR_TRACE_ERROR_FLAG_BAD_CRC | FBE_SECTOR_TRACE_ERROR_FLAG_ECOH                                                                     )
}fbe_sector_trace_error_flags_t;

/* Defines used to display trace information */
#define FBE_SECTOR_TRACE_ERROR_FLAGS_VERBOSE_LEN (127 + 1) 
#define FBE_SECTOR_TRACE_ERROR_FLAGS_STRING_LEN (4 + 2)     /* Maximum of 4 characters for name plus space and term */  
/* maximum length of mask to string */
#define FBE_SECTOR_TRACE_ERROR_FLAGS_TOTAL_LEN  ((FBE_SECTOR_TRACE_ERROR_FLAG_COUNT * FBE_SECTOR_TRACE_ERROR_FLAGS_STRING_LEN) + 1) 
/* maximum length of error level string */
#define FBE_SECTOR_TRACE_ERROR_LEVEL_TOTAL_LEN  (127 + 1) 

/* These define some default params for our trace functions. 
 * This makes is much easier to maintain if we need to change 
 * these default params for any reason. 
 */
/*                                          Trace Location? function   line Initial Call?   Sector Start?    */
#define FBE_SECTOR_TRACE_INITIAL_PARAMS     FBE_FALSE, __FUNCTION__, __LINE__, FBE_TRUE,    FBE_FALSE
#define FBE_SECTOR_TRACE_INITIAL_LOCATION   FBE_TRUE,  __FUNCTION__, __LINE__, FBE_TRUE,    FBE_FALSE
#define FBE_SECTOR_TRACE_PARAMS             FBE_FALSE, __FUNCTION__, __LINE__, FBE_FALSE,   FBE_FALSE
#define FBE_SECTOR_TRACE_PARAMS_LOCATION    FBE_TRUE,  __FUNCTION__, __LINE__, FBE_FALSE,   FBE_FALSE
#define FBE_SECTOR_TRACE_DATA_START         FBE_FALSE, __FUNCTION__, __LINE__, FBE_FALSE,   FBE_TRUE
#define FBE_SECTOR_TRACE_DATA_PARAMS        FBE_FALSE, __FUNCTION__, __LINE__, FBE_FALSE,   FBE_FALSE
#define FBE_SECTOR_TRACE_LOCATION_BUFFERS   fbe_sector_trace_get_function_string(), fbe_sector_trace_get_line_string()

typedef enum fbe_sector_trace_control_code_e {
	FBE_SECTOR_TRACE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_SECTOR_TRACE),
	FBE_SECTOR_TRACE_CONTROL_CODE_GET_INFO,
	FBE_SECTOR_TRACE_CONTROL_CODE_SET_LEVEL,
    FBE_SECTOR_TRACE_CONTROL_CODE_SET_FLAGS,
    FBE_SECTOR_TRACE_CONTROL_CODE_RESET_COUNTERS,
    FBE_SECTOR_TRACE_CONTROL_CODE_GET_COUNTERS,
    FBE_SECTOR_TRACE_CONTROL_CODE_SET_STOP_ON_ERROR_FLAGS,
    FBE_SECTOR_TRACE_CONTROL_CODE_STOP_SYSTEM_ON_ERROR,
    FBE_SECTOR_TRACE_CONTROL_CODE_SET_ENCRYPTION_MODE,
	FBE_SECTOR_TRACE_CONTROL_CODE_LAST
} fbe_sector_trace_control_code_t;

/*!*******************************************************************
 * @struct fbe_sector_trace_get_info_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_SECTOR_TRACE_CONTROL_CODE_GET_INFO control code.
 *
 *********************************************************************/
typedef struct fbe_sector_trace_get_info_s
{
    fbe_bool_t                      b_sector_trace_initialized;
    fbe_sector_trace_error_level_t  default_sector_trace_level; /*! Default sector trace level */
    fbe_sector_trace_error_level_t  current_sector_trace_level; /*! Current sector trace level */
    fbe_sector_trace_error_flags_t  default_sector_trace_flags; /*! Default sector trace flags */
    fbe_sector_trace_error_flags_t  current_sector_trace_flags; /*! Current sector trace flags */
    fbe_sector_trace_error_flags_t  stop_on_error_flags;        /*! PANIC if these sector trace errors are encountered */
    fbe_bool_t                      b_stop_on_error_flags_enabled; /*! If True PANIC if these sector traces are encountered */
}
fbe_sector_trace_get_info_t;

/*!*******************************************************************
 * @struct fbe_sector_trace_get_counters_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_SECTOR_TRACE_CONTROL_CODE_GET_COUNTERS control code.
 *
 *********************************************************************/
typedef struct fbe_sector_trace_get_counters_t
{
    fbe_u32_t                       total_invocations;
    fbe_u32_t                       total_errors_traced;
    fbe_u32_t                       total_sectors_traced;
    fbe_u32_t                       error_level_counters[FBE_SECTOR_TRACE_ERROR_LEVEL_COUNT];
    fbe_u32_t                       error_type_counters[FBE_SECTOR_TRACE_ERROR_FLAG_COUNT];
}
fbe_sector_trace_get_counters_t;

/*!*******************************************************************
 * @struct fbe_sector_trace_set_level_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_SECTOR_TRACE_CONTROL_CODE_SET_LEVEL control code.
 *
 *********************************************************************/
typedef struct fbe_sector_trace_set_level_s
{
    fbe_sector_trace_error_level_t  new_sector_trace_level; /*! New sector trace level */
	fbe_bool_t persist;
}
fbe_sector_trace_set_level_t;

/*!*******************************************************************
 * @struct fbe_sector_trace_set_flags_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_SECTOR_TRACE_CONTROL_CODE_SET_FLAGS control code.
 *
 *********************************************************************/
typedef struct fbe_sector_trace_set_flags_s
{

    fbe_sector_trace_error_flags_t  new_sector_trace_flags; /*! New sector trace flags */
	fbe_bool_t persist;
}
fbe_sector_trace_set_flags_t;

/*!*******************************************************************
 * @struct fbe_sector_trace_set_stop_on_error_flags_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_SECTOR_TRACE_CONTROL_CODE_SET_STOP_ON_ERROR_FLAGS control code.
 *
 *********************************************************************/
typedef struct fbe_sector_trace_set_stop_on_error_flags_s
{

    fbe_sector_trace_error_flags_t  new_sector_trace_stop_on_error_flags; /*! New sector trace flags */
    fbe_u32_t  additional_event;
    fbe_bool_t persist;
}
fbe_sector_trace_set_stop_on_error_flags_t;

/*!*******************************************************************
 * @struct fbe_sector_trace_stop_system_on_error_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_SECTOR_TRACE_CONTROL_CODE_STOP_SYSTEM_ON_ERROR control code.
 *
 *********************************************************************/
typedef struct fbe_sector_trace_stop_system_on_error_s
{
    fbe_bool_t                      b_stop_system;
}
fbe_sector_trace_stop_system_on_error_t;

/*!*******************************************************************
 * @struct fbe_sector_trace_reset_counters_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_SECTOR_TRACE_CONTROL_CODE_RESET_COUNTERS control code.
 *
 *********************************************************************/
typedef struct fbe_sector_trace_reset_counters_s
{
    fbe_u32_t                       unused; /*! Currently unused */
}
fbe_sector_trace_reset_counters_t;

/*!*******************************************************************
 * @struct fbe_sector_trace_encryption_mode_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_SECTOR_TRACE_CONTROL_CODE_SET_ENCRYPTION_MODE control code.
 *
 *********************************************************************/
typedef struct fbe_sector_trace_encryption_mode_s
{
    fbe_system_encryption_mode_t encryption_mode;
    fbe_bool_t                   b_is_external_build;
}
fbe_sector_trace_encryption_mode_t;

/*!*******************************************************************
 * @struct fbe_sector_trace_is_external_build_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_SECTOR_TRACE_CONTROL_CODE_IS_EXTENAL_BUILD control code.
 *
 *********************************************************************/
typedef struct fbe_sector_trace_is_extenal_build_s
{
    fbe_bool_t                   b_is_external_build;
}
fbe_sector_trace_is_external_build_t;

/*                                                               
 *  Prototypes from fbe_sector_trace.c
 */
void fbe_sector_trace_lock(void);
void fbe_sector_trace_unlock(void);
fbe_bool_t fbe_sector_trace_is_external_build(void);
fbe_bool_t fbe_sector_trace_can_trace(fbe_u32_t entries);
fbe_bool_t fbe_sector_trace_can_trace_data(void);
fbe_char_t *fbe_sector_trace_get_function_string(void);
fbe_char_t *fbe_sector_trace_get_line_string(void);
fbe_bool_t fbe_sector_trace_is_enabled(fbe_sector_trace_error_level_t trace_err_level,
                                       fbe_sector_trace_error_flags_t trace_err_flag);
fbe_status_t fbe_sector_trace_entry(fbe_sector_trace_error_level_t error_level,
                                    fbe_sector_trace_error_flags_t error_flag,
                                    fbe_bool_t trace_location,
                                    const fbe_char_t *function_p,
                                    const fbe_u32_t line,
                                    fbe_bool_t b_initial_error_trace,
                                    fbe_bool_t sector_trace_start,
                                    const fbe_char_t *entry_fmt_p,
                                    ...);
fbe_status_t fbe_sector_trace_check_stop_on_error(fbe_sector_trace_error_flags_t error_flag);

/****************************************
 * END fbe_sector_trace_interface.h
 ****************************************/
#endif /*  FBE_SECTOR_TRACE_INTERFACE_H */
