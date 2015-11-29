#ifndef FBE_RAID_LIBRARY_PRIVATE_H
#define FBE_RAID_LIBRARY_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_library_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions local to the raid library methods.
 *
 * @version
 *   5/19/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe_raid_cond_trace.h"
#include "fbe_raid_memory_api.h"
#include "fbe_private_space_layout.h"
#include "fbe_traffic_trace.h"
/*************************
 *   LITERAL DEFINITIONS
 *************************/



/****************************
 *   FUNCTION DECLARATIONS
 ***************************/
extern void fbe_raid_library_trace(fbe_trace_level_t trace_level,
                                   fbe_u32_t line,
                                   const char *const function_p,
                                   fbe_char_t * fail_string_p, ...)
                                   __attribute__((format(__printf_func__,4,5)));

/*!***************************************************************************
 * @enum    fbe_raid_status_t
 *****************************************************************************
 *
 * @brief   This is the enumeration of all possible raid library execution
 *          statuses.  These values are private to the raid library and
 *          (eventually) should be used by all library routines.
 */
typedef enum fbe_raid_status_e
{
    FBE_RAID_STATUS_INVALID = 0,                /*!< Status hasn't been populated */
    FBE_RAID_STATUS_OK,                         /*!< The raid library request completed successfully */
    FBE_RAID_STATUS_OK_TO_CONTINUE,             /*!< Ok to continue processing due to redundancy */
    FBE_RAID_STATUS_RETRY_POSSIBLE,             /*!< The request failed but a retry is possible */
    FBE_RAID_STATUS_WAITING,                    /*!< Waiting for object to either fail or retry request */
    FBE_RAID_STATUS_MEDIA_ERROR_DETECTED,       /*!< Indicates hard error detected on write (possible due to error on `write verify' */
    FBE_RAID_STATUS_TOO_MANY_DEAD,              /*!< Although the raid group isn't shutdown too manu positions are dead to continue */
    FBE_RAID_STATUS_MINING_REQUIRED,            /*!< Additional mining (either region or recovery) verify is required */
    FBE_RAID_STATUS_UNSUPPORTED_CONDITION,      /*!< Some condition has occurred that is no longer support */
    FBE_RAID_STATUS_UNEXPECTED_CONDITION        /*!< Validation check failed (most likely transitioned to `unexpected' state */
}fbe_raid_status_t;

/* These define some default params for our trace functions. 
 * This makes is much easier to maintain if we need to change 
 * these default params for any reason. 
 */
#define FBE_RAID_TRACE_ERROR_PARAMS __LINE__, __FUNCTION__

#define FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL FBE_TRACE_LEVEL_CRITICAL_ERROR, __LINE__, __FUNCTION__
#define FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR FBE_TRACE_LEVEL_ERROR, __LINE__, __FUNCTION__
#define FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING FBE_TRACE_LEVEL_WARNING, __LINE__, __FUNCTION__
#define FBE_RAID_LIBRARY_TRACE_PARAMS_INFO FBE_TRACE_LEVEL_INFO, __LINE__, __FUNCTION__
#define FBE_RAID_LIBRARY_TRACE_PARAMS_DEBUG_HIGH FBE_TRACE_LEVEL_DEBUG_HIGH, __LINE__, __FUNCTION__

#define FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL FBE_TRACE_LEVEL_CRITICAL_ERROR, __LINE__, __FUNCTION__
#define FBE_RAID_SIOTS_TRACE_PARAMS_ERROR FBE_TRACE_LEVEL_ERROR, __LINE__, __FUNCTION__ 
#define FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS  __LINE__, __FUNCTION__ 
#define FBE_RAID_SIOTS_TRACE_PARAMS_WARNING FBE_TRACE_LEVEL_WARNING, __LINE__, __FUNCTION__
#define FBE_RAID_SIOTS_TRACE_PARAMS_INFO FBE_TRACE_LEVEL_INFO, __LINE__, __FUNCTION__
#define FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH FBE_TRACE_LEVEL_DEBUG_HIGH, __LINE__, __FUNCTION__
#define FBE_RAID_SIOTS_TRACE_PARAMS_LEVEL(m_level) m_level, __LINE__, __FUNCTION__

#define FBE_RAID_IOTS_TRACE_PARAMS_CRITICAL FBE_TRACE_LEVEL_CRITICAL_ERROR, __LINE__, __FUNCTION__
#define FBE_RAID_IOTS_TRACE_PARAMS_ERROR FBE_TRACE_LEVEL_ERROR, __LINE__, __FUNCTION__ 
#define FBE_RAID_IOTS_TRACE_UNEXPECTED_ERROR_PARAMS  __LINE__, __FUNCTION__ 
#define FBE_RAID_IOTS_TRACE_PARAMS_WARNING FBE_TRACE_LEVEL_WARNING, __LINE__, __FUNCTION__
#define FBE_RAID_IOTS_TRACE_PARAMS_INFO FBE_TRACE_LEVEL_INFO, __LINE__, __FUNCTION__
#define FBE_RAID_IOTS_TRACE_PARAMS_DEBUG_HIGH FBE_TRACE_LEVEL_DEBUG_HIGH, __LINE__, __FUNCTION__

/* To disable additional checking in checked builds change the
 *  -> #if DBG to #if 0.
 */
#if DBG
#define RAID_DBG_ENABLED (FBE_TRUE)
#else
#define RAID_DBG_ENABLED (FBE_FALSE)
#endif

/* For now memory debug is enabled at the same time as debug mode.
 */
#define RAID_DBG_MEMORY_ENABLED RAID_DBG_ENABLED


/*!*******************************************************************
 * @def RAID_COND_ENA
 *********************************************************************
 * @brief This if statement determines if `inject' conditional are
 *        enabled or not.
 *
 *********************************************************************/
#define RAID_COND_ENA (fbe_raid_cond_get_register_fn())

/*!*******************************************************************
 * @def RAID_DBG_COND
 *********************************************************************
 * @brief We define this as the default condition handler for debug only.
 * 
 *        ***This is only enabled in debug builds to avoid performance overhead.
 *        This code will not even be included in a free build because of the
 *        RAID_DBG_ENABLED is 0.
 * 
 *        We overload this so that in test code we can test when these conditions are false.
 * 
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define RAID_DBG_COND(m_cond) (RAID_DBG_ENABLED && RAID_COND_ENA && fbe_raid_cond_evaluate_exp_only(m_cond,__LINE__,__FUNCTION__))
//#define RAID_DBG_COND(m_cond) (RAID_DBG_ENABLED && fbe_raid_cond_evaluate_exp_only(m_cond,__LINE__,__FUNCTION__))

/*!*******************************************************************
 * @def RAID_DBG_ENA
 *********************************************************************
 * @brief This if statement is only evaluated in debug builds
 *
 *********************************************************************/
#define RAID_DBG_ENA(m_cond)(RAID_DBG_ENABLED && m_cond)

/*!*******************************************************************
 * @def RAID_COND_GEO_ENA
 *********************************************************************
 * @brief This if statement determines if `inject' conditional are
 *        enabled or not for conditions that take the geo and
 *        can differentiate on which object they inject on.
 *
 *********************************************************************/
#define RAID_COND_GEO_ENA (fbe_raid_cond_get_geo_register_fn())

/*!*******************************************************************
 * @def RAID_COND
 *********************************************************************
 * @brief We define this as the default condition handler.
 *        We overload this so that in test code we can
 *        test when these conditions are false.


 * 
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define RAID_COND(m_cond)   (RAID_COND_ENA ? fbe_raid_cond_evaluate_exp_only(m_cond,__LINE__,__FUNCTION__) : (m_cond))

/*!*******************************************************************
 * @def RAID_COND_GEO
 *********************************************************************
 * @brief We define this as the default condition handler.
 *        We overload this so that in test code we can
 *        test when these conditions are false.


 * 
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define RAID_COND_GEO(m_geo, m_cond)   ((RAID_COND_GEO_ENA && !fbe_private_space_layout_object_id_is_system_object(m_geo->object_id)) ? fbe_raid_cond_evaluate_exp_only(m_cond,__LINE__,__FUNCTION__) : (m_cond))
/*!*******************************************************************
 * @def RAID_COND_STATUS
 *********************************************************************
 * @brief We define this as default condition handler to change 
 *        status, if it is required to be done (in particular for the 
 *        case when error testing path is enabled for raid library)
 *
 *********************************************************************/
#define RAID_COND_STATUS(m_cond, status) (RAID_COND_ENA ? fbe_raid_cond_evaluate_exp_and_change_status(m_cond,__LINE__,__FUNCTION__, FBE_STATUS_GENERIC_FAILURE, &status) : (m_cond))

/*!*******************************************************************
 * @def RAID_COND_GEO
 *********************************************************************
 * @brief We define this as the default condition handler.
 *        We overload this so that in test code we can
 *        test when these conditions are false.


 * 
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define RAID_COND_STATUS_GEO(m_geo, m_cond, m_status)   ((RAID_COND_GEO_ENA && !fbe_private_space_layout_object_id_is_system_object(m_geo->object_id)) ? fbe_raid_cond_evaluate_exp_and_change_status(m_cond,__LINE__,__FUNCTION__, FBE_STATUS_GENERIC_FAILURE, &m_status) : (m_cond))

/*!*******************************************************************
 * @def RAID_MEMORY_COND
 *********************************************************************
 * @brief We define this as the default condition handler,.
 *        wherever specific actions related to memory allocations 
 *        and release are to be done. At the moment, its definition 
 *        is same as RAID_COND() but can be changed later to handle 
 *        specific issues related to allocation and release of
 *        memory.
 * 
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define RAID_MEMORY_COND(m_cond)   (m_cond)


/*!*******************************************************************
 * @def RAID_ERROR_COND
 *********************************************************************
 * @brief We define this as a condition handler. This condition 
 *        handler is different from RAID_COND in that it does 
 *        result only logging of error rather than exhibiting
 *        any generic failure. We overload this so that in test 
 *        code we can test when these conditions are false in 
 *        real (not by faking the failure). 
 * 
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define RAID_ERROR_COND(m_cond)  (m_cond)

/*!*******************************************************************
 * @def RAID_GENERATE_COND
 *********************************************************************
 * @brief We define this as a condition handler. This condition 
 *        handler is used in specific cases in generation phase
 *        of siots or nested siots.
 *
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define RAID_GENERATE_COND(m_cond)   (m_cond)

/*!*******************************************************************
 * @def RAID_FINISH_ERROR_COND
 *********************************************************************
 * @brief We define this as a condition handler to be used in specific
 *        cases of completion phase of following:
 *              1. iots
 *              2. siots
 *              3. nested siots.
 *
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define RAID_FINISH_COND(m_cond)   (m_cond)

/*!*******************************************************************
 * @def RAID_FRUTS_COND
 *********************************************************************
 * @brief We define this as a default condition handler for cases
 *        wherever specific actions are to be done related to 
 *        FRUTS operation.;
 *
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define RAID_FRUTS_COND(m_cond)  (m_cond)

/*!*******************************************************************
 * @def FBE_RAID_MAX_TRACE_CHARS
 *********************************************************************
 * @brief max number of chars we will put in a formatted trace string.
 *
 *********************************************************************/
#define FBE_RAID_MAX_TRACE_CHARS 128

/*! @def FBE_RAID_SECTOR_SIZE  
 *  @brief abstraction to determine the block size of a given sg. 
 */
#define FBE_RAID_SECTOR_SIZE(m_address) FBE_BE_BYTES_PER_BLOCK


/* The dead position is initialized to this value.
 */
#define FBE_RAID_INVALID_POS ((fbe_u16_t) - 1)

/* The dead position is initialized to this value.
 */
#define FBE_RAID_INVALID_DEAD_POS ((fbe_u16_t) - 1)
#define FBE_RAID_INVALID_PARITY_POSITION ((fbe_u16_t) - 1)

/* Invalid slot for a journal write.
 */
#define FBE_RAID_INVALID_JOURNAL_SLOT ((fbe_u32_t) - 1)

/*!*******************************************************************
 * @def FBE_RAID_INVALID_POS_BITMASK
 *********************************************************************
 * @brief An invalid value for fbe_raid_position_bitmask_t
 *
 *********************************************************************/
#define FBE_RAID_INVALID_POS_BITMASK 0xFFFF

/*! @def FBE_RAID_INVALID_LBA
 *  @brief Used to indicate that the lba field hasn't been set (for instance
 *         the media error lba). 
 */
#define FBE_RAID_INVALID_LBA FBE_LBA_INVALID

/*! @def 
 *  @brief This is the default starting value for the number of retries that
 *         have been executed for a siots
 */
#define FBE_RAID_DEFAULT_RETRY_COUNT    0

/*!*******************************************************************
 * @def FBE_RAID_FRU_POSITION_BITMASK
 *********************************************************************
 * @brief Bitmask for all possible fru positions.
 *
 *********************************************************************/
#define FBE_RAID_FRU_POSITION_BITMASK 0xFFFF

/* These are the values we use to define the first and second dead positions.
 * These are used by functions, which need to distinguish between
 * the first and second dead position.
 */
typedef enum fbe_raid_degraded_position_e
{
    FBE_RAID_FIRST_DEAD_POS = 0,
    FBE_RAID_SECOND_DEAD_POS = 1,
    
    /* This is the number of degraded offsets we allow a siots to have,
     * one per dead position.  For Raid 6 we allow 2 since we
     * can have two dead drives.
     */
    FBE_RAID_MAX_DEGRADED_POSITIONS = 2
}
fbe_raid_degraded_position_t;

typedef struct fbe_raid_extent_s
{
    fbe_lba_t start_lba;
    fbe_block_count_t size;
    fbe_u32_t position;
}
fbe_raid_extent_t;

/*! @def FBE_RAID_MAX_SG_DATA_ELEMENTS
 *  @brief This is the max size of sg list we support.
 */
#define FBE_RAID_MAX_SG_DATA_ELEMENTS 128

/*! @def FBE_RAID_MAX_SG_ELEMENTS
 *  @brief This is the max size of sg list we support.
 */
#define FBE_RAID_MAX_SG_ELEMENTS FBE_RAID_MAX_SG_DATA_ELEMENTS + 1

/*! @def FBE_RAID_PAGE_SIZE_MIN  
 *  @brief This is our minimum buffer size that all of our
 *         pools use.
 */
#define FBE_RAID_PAGE_SIZE_MIN 4

/*! @def FBE_RAID_PAGE_SIZE_CACHE  
 *  @brief This page size is equal to size of cache physical page size
 */
#define FBE_RAID_PAGE_SIZE_CACHE (16)

/*! @def FBE_RAID_PAGE_SIZE_STD  
 *  @brief This is our standard buffer size that all of our
 *         pools use.
 */
#define FBE_RAID_PAGE_SIZE_STD 32

/*! @def FBE_RAID_PAGE_SIZE_MAX  
 *  @brief This is our max buffer size that all of our
 *         pools use.
 */
#define FBE_RAID_PAGE_SIZE_MAX 64

/*! @def FBE_RAID_MAX_BLKS_AVAILABLE  
 *  @brief This is the max number of buffers we can allocate.
 */
#define FBE_RAID_MAX_BLKS_AVAILABLE (FBE_RAID_MAX_SG_ELEMENTS - 1) * FBE_RAID_PAGE_SIZE_STD

/*!*******************************************************************
 * @def FBE_RAID_MAX_RETRY_MSECS
 *********************************************************************
 * @brief Max number of seconds we will wait to retry.
 *        If we get anything larger than this from below, then
 *        we will use this time as our wait.
 * 
 *        We arbitrarily set this to 5 seconds.
 *
 *********************************************************************/
#define FBE_RAID_MAX_RETRY_MSECS 1000

/*!*******************************************************************
 * @def FBE_RAID_MIN_RETRY_MSECS
 *********************************************************************
 * @brief Min number of seconds we will wait to retry.
 *        If we get anything smaller than this from below, then
 *        we will use this time as our wait.
 * 
 *        We arbitrarily set this to 100 millisecs.
 *********************************************************************/
#define FBE_RAID_MIN_RETRY_MSECS 100

/*!*******************************************************************
 * @def FBE_RAID_MAX_ELEMENT_SIZE
 *********************************************************************
 * @brief This is the largest element size we support.
 *
 *********************************************************************/
#define FBE_RAID_MAX_ELEMENT_SIZE FBE_RAID_SECTORS_PER_ELEMENT

/*!*******************************************************************
 * @def FBE_RAID_SECTORS_PER_PARITY
 *********************************************************************
 * @brief Number of blocks vertically in a standard parity stripe.
 *
 *********************************************************************/
#define FBE_RAID_SECTORS_PER_PARITY (FBE_RAID_ELEMENTS_PER_PARITY * FBE_RAID_SECTORS_PER_ELEMENT)

/* This is the max optimal block size we expect to see.
 */
#define FBE_RAID_MAX_OPTIMAL_BLOCK_SIZE 64

/* Defines the sizes of buffers we are using.
 */
static __inline fbe_u32_t fbe_raid_get_std_buffer_size(void)
{
    return FBE_RAID_PAGE_SIZE_STD;
}

/*!*******************************************************************
 * @def FBE_RAID_MINE_REGION_SIZE
 *********************************************************************
 * @brief Size of a region when mining for a hard error.
 *
 *********************************************************************/
//#define FBE_RAID_MINE_REGION_SIZE FBE_RAID_PAGE_SIZE_STD
#define FBE_RAID_MINE_REGION_SIZE 0x40

/* After this amount of time we will modify 
 * our algorithms to do such things as not strip mine 
 * in an effort to speed up the recovery. 
 */
#define FBE_RAID_IOTS_THRESHOLD_SECONDS 5

/* After this amount of media errors, we will modify our algorithms to do such things as
 * not strip mine in an effort to speed up the recovery. 
 * This value is somewhat arbitrary, but allows us to recover from a reasonable number 
 * of errors before we speed up recovery. 
 */
#define FBE_RAID_SIOTS_THRESHOLD_MEDIA_ERRORS 8

/* Extra alignment for 4k.  Max per position is 4, 2 for the beginning and 2 for the end.
 */
#define FBE_RAID_EXTRA_ALIGNMENT_SGS 4

/* We always use virtual addresses, but we 
 * keep this abstraction that we ported over 
 * in case we ever need to use physical. 
 */
#define fbe_raid_virtual_addr_available(m_sg) (TRUE)
#define fbe_raid_sg_get_physical_address(m_dest_sgl_p, m_phys_addr_local)((void)0)
#define fbe_raid_sg_set_physical_address(m_dest_sgl_p, m_phys_addr_local)((void)0)
#define fbe_raid_sg_declare_physical_address(m_arg)((void)0)

/*!*******************************************************************
 * @enum fbe_raid_option_flags_t
 *********************************************************************
 * @brief Various options we can set on the raid group.
 *
 *********************************************************************/
typedef enum fbe_raid_option_flags_e
{
    FBE_RAID_OPTION_FLAG_INVALID                    = 0,
    FBE_RAID_OPTION_FLAG_SMALL_WRITE_ONLY           = 0x00000001, /*!< Writes only use 468 SM */
    FBE_RAID_OPTION_FLAG_LARGE_WRITE_ONLY           = 0x00000002, /*!< Writes only use MR3 SM */
    FBE_RAID_OPTION_FLAG_DEGRADED_RECOVERY          = 0x00000004, /*!< If degraded go through recovery. */
    FBE_RAID_OPTION_FLAG_PANIC_U_ERROR              = 0x00000008, /*!< If panic on uncorrectable. */
    FBE_RAID_OPTION_FLAG_PANIC_C_ERROR              = 0x00000010, /*!< If panic on correctable. */
    FBE_RAID_OPTION_FLAG_DATA_CHECKING              = 0x00000020, /*!<  If check data. */

    /*! To allow event logging even if we injected the error.
     * By default we don't log injected errors,
     * but if we're testing event logging changes then
     * we *need* to see the errors going into the log.
     */
    FBE_RAID_OPTION_FLAG_LOG_INJECTED_ERRORS        = 0x00000040,

    /*! Panic on uncorrectable errors even when we are degraded.
     * This is necessary since the normal panic_u_error_flag does
     * not take effect if we are injecting errors and are degraded.
     * This flag means to panic on uncorrectable errors even
     * when we are degraded.
     */
    FBE_RAID_OPTION_FLAG_PANIC_U_DEGRADED           = 0x00000080,

    /*! In this mode, we panic whenever we write a block with
     * a bad checksum on it.
     * This is useful for catching cases where we invalidate
     * a block and later end up with an uncorrectable error.
     */
    FBE_RAID_OPTION_FLAG_PANIC_WRITE_INVALID_BLOCKS = 0x00000100,
    /*! This turns on/off the delaying of background io in usersim.
     */
    FBE_RAID_OPTION_FLAG_DELAY_BACKGROUND_IO        = 0x00000200,

    /*! This forces write to go through recovery verify
     * even though there might not be an error.
     * This allows us to test the reconstruct parity
     * and data move that is performed by R5/R6/R3 rvr write.
     */
    FBE_RAID_OPTION_FLAG_FORCE_RVR_WRITE            = 0x00000400,

    /*! This forces read to go through recovery verify
     * even though there might not be an error.
     * This allows us to force a verify as part of every read.
     */
    FBE_RAID_OPTION_FLAG_FORCE_RVR_READ             = 0x00000800,

    /*! @note This flag is only in effect if the FBE_RAID_OPTION_FLAG_DATA_CHECKING 
     *        is set.  If this flag is set we will fail read request that return
     *        `data lost - invalidated' data.  This flag should not be set by
     *        default.
     */
    FBE_RAID_OPTION_FLAG_FAIL_DATA_LOST_READ        = 0x00001000, /*!<  Fail read request that have `data lost - invalidated'*/

    /*! @note This flag is only in effect if the FBE_RAID_OPTION_FLAG_DATA_CHECKING 
     *        is set.  If this flag is set we will fail write request that contain
     *        `data lost - invalidated' data.  This flag should not be set by
     *        default.
     */
    FBE_RAID_OPTION_FLAG_FAIL_DATA_LOST_WRITE       = 0x00002000, /*!<  Fail write request that have `data lost - invalidated'*/

    FBE_RAID_OPTION_FLAG_LAST
}
fbe_raid_option_flags_t;

/*!*******************************************************************
 * @def     FBE_RAID_OPTION_DEFAULT_FLAGS
 *********************************************************************
 * @brief   This defines the default raid option flags.
 *
 *  @note   By default we enable data checking.
 *          Global option flags are not associated with any particular
 *          raid group.
 *
 *********************************************************************/
#define FBE_RAID_OPTION_DEFAULT_FLAGS       (FBE_RAID_OPTION_FLAG_DATA_CHECKING)

static __inline void fbe_raid_adjust_sg_desc(fbe_sg_element_with_offset_t * sgd_p)
{
    if ((sgd_p->offset != 0) &&
        (sgd_p->offset >= (fbe_s32_t) sgd_p->sg_element->count))
    {
        while (sgd_p->offset >= (fbe_s32_t) sgd_p->sg_element->count)
        {
            sgd_p->offset -= (sgd_p->sg_element)->count;
            sgd_p->sg_element = sgd_p->get_next_sg_fn(sgd_p->sg_element);
        }
    }
    return;
}

/*!**************************************************
 * @def FBE_RAID_MAP_MEMORY
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 ***************************************************/
#define FBE_RAID_MAP_MEMORY(memory_p, byte_count)((void*)0)

/*!**************************************************
 * @def FBE_RAID_UNMAP_MEMORY
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 ***************************************************/
#define FBE_RAID_UNMAP_MEMORY(memory_p, byte_count)((void)0)

/*!**************************************************
 * @def FBE_RAID_VIRTUAL_ADDRESS_AVAIL
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 *   For now everything is virtual.
 ***************************************************/
#define FBE_RAID_VIRTUAL_ADDRESS_AVAIL(sg_ptr)(FBE_TRUE)

/* These are not used, but are kept as placeholders.
 */
#define FBE_RAID_SGL_MAP_MEMORY(m_sg_element, m_offset, arg3)(m_sg_element->address + m_offset)
#define FBE_RAID_SGL_GET_PHYSICAL_ADDRESS(arg1, arg2)((void)0)
#define FBE_RAID_SGL_UNMAP_MEMORY(sg, addr)((void)0)

/*!***************************************************************
 * fbe_raid_generate_aligned_request()
 ****************************************************************
 * @brief   Using the starting and ending lbas of the request and
 *          the aligned block size, update the request to be of
 *          an aligned block size multiple.
 *
 * @param   start_lba_p - Address of original start lba
 * @param   end_lba_p - Address of oginal end lba
 * @param   aligned_block_size - Optimal block size for the associated
 *          raid group.
 *
 * @return  None
 *
 * @note
 *
 * @author
 *  12/10/2008  Ron Proulx  - Created.
 *
 ****************************************************************/
static __inline void fbe_raid_generate_aligned_request(fbe_lba_t *const start_lba_p, 
                                                fbe_lba_t *const end_lba_p, 
                                                fbe_u32_t aligned_block_size)
{
    fbe_u32_t additional_start_blocks;
    fbe_u32_t blocks;

    /* Determine the additional start blocks required.
     */
    additional_start_blocks = (fbe_u32_t)(*start_lba_p % aligned_block_size);

    /* Generate the updated starting lba by subtracting the 
     * additional blocks required.
     */
    *start_lba_p = *start_lba_p - additional_start_blocks;
        
    /* Determine current request size.
     */
    blocks = (fbe_u32_t)(*end_lba_p - *start_lba_p + 1);

    /* Generate proper end lba by generating the total number of blocks
     * rounded to optimal block size.
     */
    blocks = (blocks + (aligned_block_size - 1)) / aligned_block_size;
    blocks *= aligned_block_size;
    *end_lba_p = *start_lba_p + blocks - 1;

    return;
}
/* end fbe_raid_generate_aligned_request() */

static __inline fbe_status_t fbe_raid_round_lba_blocks_to_optimal(fbe_lba_t *const lba_p, 
                                                           fbe_block_count_t *const blocks_p,
                                                           fbe_u32_t optimal_block_size)
{
    /* Simply calculate the end lba so we can call the below function.
     */
    fbe_lba_t end_lba = *lba_p + *blocks_p - 1;

    fbe_raid_generate_aligned_request(lba_p, &end_lba, optimal_block_size);

    /* Lba was modified, now just determine the blocks.
     */
    if (RAID_COND(end_lba < *lba_p))
    {
          fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                 "unexpected error : end_lba  0x%llx < *lba_p 0x%llx",
                                 (unsigned long long)end_lba,
                                 (unsigned long long)(*lba_p));
          return FBE_STATUS_GENERIC_FAILURE;
    }

    *blocks_p = (fbe_u32_t)((end_lba - *lba_p) + 1);
    return FBE_STATUS_OK;
}

 /*!*******************************************************************
 * @def FBE_RAID_ERROR_INFO
 *********************************************************************
 * @brief This information is logged in the event log.
 *        The below formats the error info as follows
 *        Use first byte (0) is uesd to store error bit information.
 *        Next byte (1) is used to store error reason information.
 *        Currently, next two bytes (2 and 3) are not used.
 *        They are kept for future use.
 * 
 * byte   |   3   |   2   |   1   |   0   |
 *        |   Not used    |  Reason & Error bits   |
 *
 * @Note: We will get the error reason code and error bits in the m_error_bits
 *        only. Hence we are directly ORing this with m_error_info.
 *
 *********************************************************************/
static __inline fbe_u32_t fbe_raid_error_info(int m_error_bits)
{
    return (m_error_bits);
}

/*!*******************************************************************
 * @def FBE_RAID_MIN_ERROR_BLOCKS
 *********************************************************************
 * @brief This is minimum number of errored blocks. *
 *********************************************************************/
#define FBE_RAID_MIN_ERROR_BLOCKS 1

/*!*******************************************************************
 * @def FBE_RAID_COMMON_MDS_RESOURCE_PRIORITY_OFFSET
 *********************************************************************
 * @brief Resource priority boost for metadata requests relative 
 * to IOTS resource priority.
 *********************************************************************/
#define FBE_RAID_COMMON_MDS_RESOURCE_PRIORITY_BOOST (2)

static __inline void *fbe_raid_memory_id_get_memory_ptr(fbe_memory_id_t mem_id)
{
#if RAID_DBG_MEMORY_ENABLED
    fbe_raid_memory_header_t *hdr_p = (fbe_raid_memory_header_t *)mem_id;
    if (hdr_p == NULL)
    {
        return NULL;
    }
    /* We just skip over the header.
     */
    return hdr_p + 1;
#else
    /* When memory debug is not enabled, there is no memory header.
     */
    return mem_id;
#endif
}
#endif /* FBE_RAID_LIBRARY_PRIVATE_H*/
/*************************
 * end file fbe_raid_library_private.h
 *************************/


