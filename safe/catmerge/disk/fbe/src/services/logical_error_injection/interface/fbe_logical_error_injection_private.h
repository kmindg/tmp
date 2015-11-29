#ifndef FBE_LOGICAL_ERROR_INJECTION_PRIVATE_H
#define FBE_LOGICAL_ERROR_INJECTION_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_error_injection_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the logical_error_injection service.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_logical_error_injection_service.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_random.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
#define INVALID_INPUT    -1
#define RG_ERROR_INVALID_DATA 0x00BADBAD
#define RG_ERROR_CORRUPT_WORD_COUNT    5
#define FBE_LOGICAL_ERROR_INJECTION_MAX_TABLES       17
#define FBE_LOGICAL_ERROR_INJECTION_DESCRIPTION_MAX_CHARS 128

#define FBE_LOGICAL_ERROR_INJECTION_RECORD_ROUND_BLOCKS 64


/*!*******************************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_MAX_SCAN_SECONDS
 *********************************************************************
 * @brief Max time the delay thread will wait.
 *        This is arbitrarily one day.
 *
 *********************************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_MAX_SCAN_SECONDS 3600 * 24
#define FBE_LOGICAL_ERROR_INJECTION_MAX_SCAN_MS FBE_LOGICAL_ERROR_INJECTION_MAX_SCAN_SECONDS * FBE_TIME_MILLISECONDS_PER_SECOND


/* String description of error insertion modes
 */
#define RG_ERR_SIM_MODES_DESC "\n\
Valid error insertion modes are:\n\
   CNT - inject errors up to limit\n\
   ALW - always inject error\n\
   RND - inject errors randomly (1 in errLimit)\n\
   SKP - skip errLimit matches, then inject errLimit errors, repeat\n\
   TNS - transitory, inject error on first attempt, not on retry\n\
   TNR - randomly transitory.\n"

#define RG_ERR_SIM_TYPE_DESC "\n\
Valid error insertion modes are:\n\
   SOFT - soft media errors\n\
   HARD - hard media errors\n\
   RNDMED - randomly choose hard/soft media errors\n\
   CRC  - checksum error\n\
   TS   - timestamp consistency error\n\
   WS   - writestamp consistency error\n\
   BWS  - bogus writestamp error\n\
   BTS  - bogus timestamp error\n"

/*!*******************************************************************
 * @def LEI_COND
 *********************************************************************
 * @brief Logical error injection condition We define this as the 
 *        default condition handler.
 *        We overload this so that in test code we can
 *        test when these conditions are false.
 * 
 *********************************************************************/
#define LEI_COND(m_cond) (m_cond)

/***************************************************
 *   This structure contains the common parameters 
 *   used by multiple validation steps/functions. 
 ***************************************************/
typedef struct fbe_logical_error_injection_validate_parms_s
{
    fbe_bool_t b_print;                /* Whether to print matching record. */
    fbe_u16_t bitmap_phys_const;  /* Physical bitmap based on error table. */
    fbe_u16_t bitmap_phys;        /* Physical bitmap based on error table. */
    fbe_u16_t bitmap_deg;         /* Physical bitmap of degraded drive(s). */
    fbe_u16_t bitmap_parity_phys; /* Physical bitmap of parity drives. */
    fbe_u16_t bitmap_parity_log;  /* Logical  bitmap of parity drives. */
    fbe_u16_t err_adj_phys;       /* Physical bitmap of error adjacency. */
    fbe_bool_t correctable_tbl;        /* Correctable-ness based on error table. */
    fbe_bool_t correctable_reg;        /* Correctable-ness based on error region. */
    fbe_xor_error_type_t err_type;     /* Error type w/o FBE_XOR_ERR_TYPE_FLAG_MASK. */
    fbe_u16_t bitmap_width;       /* Bitmap of all disks in the strip.  */
}
fbe_logical_error_injection_validate_parms_t;

/***************************************************
 * Initialize the error sim lbas by setting
 * the current address offset - 7/00 - KLS
 ***************************************************/
#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
#define RG_ERROR_SIM_OFFSET 0x8d000
#else
#define RG_ERROR_SIM_OFFSET 0x2000
#endif /* UMODE_ENV */

extern const char *rg_err_mode_strings[FBE_LOGICAL_ERROR_INJECTION_MODE_LAST];
extern const char *rg_err_type_strings[FBE_XOR_ERR_TYPE_MAX];

/* Set up the correctable or uncorrectable bitmap of an fbe_xor_error_t
 * depending on whether the error is correctable or not.
 */
#define RG_ERR_BITMAP_SET(is_correctable, c_bitmap, u_bitmap, bitmap) \
    do                                                                \
    {                                                                 \
        if (is_correctable)                                           \
        {                                                             \
            c_bitmap |= bitmap;                                       \
        }                                                             \
        else                                                          \
        {                                                             \
            u_bitmap |= bitmap;                                       \
        }                                                             \
    } while(0);

/* RAID 6 specific defines and macros.
 *
 * The following is the logical layout of a 16 drive RAID6 LUN. For 4, 6, 
 * and 10 drive RAID6 LUNs, drives [2,13], [4,13], and [8,14] are virtual 
 * respectively. In order to be able to specify error types independent 
 * of array width, the logical position is always 0 for Non-S only data, 
 * 14 for row parity, and 15 for diagonal parity. 
 *
 *  +--------- RG_R6_POS_LOG_DIAG
 *  |   +----- RG_R6_POS_LOG_ROW          RG_R6_POS_LOG_NS -----+
 *  |   |                                                       |
 *  V   V                                                       V
 * 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
 * --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  
 * DP  RP  NS  NS  NS  NS  NS  NS  NS  NS  NS  NS  NS  NS  NS  NS
 *          S   S   S   S   S   S   S   S   S   S   S   S   S
 * <------------------ FBE_RAID_MAX_DISK_ARRAY_WIDTH -------------------->
 *         <---------- RG_R6_DRV_PHYS_MAX  --------------------->
 *         <---------- RG_R6_MASK_LOC_DATA --------------------->
 *         <---------- RG_R6_MASK_LOC_S    ----------------->
 *     <> RG_R6_MASK_LOG_ROW                 RG_R6_MASK_LOG_NS <>
 * <>     RG_R6_MASK_LOG_DIAG
 * <----> RG_R6_MASK_LOG_PAR
 *
 *
 * RG_R6_DRV_PHYS_MAX:      maximum number of data drives in a LUN
 *
 * Following are constants describing logical positions of drives.
 * RG_R6_POS_LOG_NS:        logical index of Non-S only      drive
 * RG_R6_POS_LOG_ROW:       logical index of row      parity drive
 * RG_R6_POS_LOG_DIAG:      logical index of diagonal parity drive
 * RG_R6_MASK_LOG_NS:       logical mask  of Non-S only      drive
 * RG_R6_MASK_LOG_ROW:      logical mask  of row      parity drive
 * RG_R6_MASK_LOG_PAR:      logical mask  of          parity drives
 * RG_R6_MASK_LOG_DIAG:     logical mask  of diagonal parity drive
 * RG_R6_MASK_LOG_DATA:     logical mask  of Non-S & S data  drives
 * RG_R6_MASK_LOG_S:        logical mask  of S data only     drives
 *
 * Following are constants that have to do with EVENODD algorithm.
 * RG_R6_SYM_PER_SECTOR:    number of symbol per sector
 * RG_R6_SYM_SIZE_BYTES:    number of bytes  per symbol
 * RG_R6_SYM_SIZE_LONGS:    number of LONGs  per symbol
 * RG_R6_SYM_SIZE_BITS:     number of bits   per symbol
 *
 * Following are predefined values of error table parameters. 
 * RG_R6_ERR_PARM_NO:       value for a logical NO
 * RG_R6_ERR_PARM_YES:      valud for a logical YES
 * RG_R6_ERR_PARM_RND:      value for a parameter to be randomized
 * RG_R6_ERR_PARM_UNUSED:   value for a parameter that is unused
 *
 * NUM_BITS_fbe_u16_t:       number of bits in an fbe_u16_t word
 * NUM_BITS_fbe_u32_t:       number of bits in an fbe_u32_t word
 */
#define RG_R6_DRV_PHYS_MAX      (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2)

#define RG_R6_POS_LOG_NS        (0)
#define RG_R6_POS_LOG_ROW       (14)
#define RG_R6_POS_LOG_DIAG      (15)

#define RG_R6_MASK_LOG_NS       (1 << RG_R6_POS_LOG_NS)
#define RG_R6_MASK_LOG_ROW      (1 << RG_R6_POS_LOG_ROW)
#define RG_R6_MASK_LOG_DIAG     (1 << RG_R6_POS_LOG_DIAG)
#define RG_R6_MASK_LOG_PAR      (RG_R6_MASK_LOG_ROW | RG_R6_MASK_LOG_DIAG)
#define RG_R6_MASK_LOG_DATA     (~RG_R6_MASK_LOG_PAR)
#define RG_R6_MASK_LOG_S        (RG_R6_MASK_LOG_DATA & ~RG_R6_MASK_LOG_NS)

#define RG_R6_SYM_PER_SECTOR    (16)
#define RG_R6_SYM_SIZE_BYTES    (FBE_BE_BYTES_PER_BLOCK / RG_R6_SYM_PER_SECTOR)
#define RG_R6_SYM_SIZE_LONGS    (RG_R6_SYM_SIZE_BYTES / 4)
#define RG_R6_SYM_SIZE_BITS     (RG_R6_SYM_SIZE_BYTES * 8)

#define RG_R6_ERR_PARM_NO       (0)
#define RG_R6_ERR_PARM_YES      (1)
#define RG_R6_ERR_PARM_RND      (0xFFFF)
#define RG_R6_ERR_PARM_UNUSED   (0xDEAD)

/* This function checks whether pos_bitmap and err_adj are valid. 
 * pos_bitmap and err_adj must be specified including randomization. 
 * And pos_bitmap must be part of err_adj. 
 * Possible combinations are:
 * 1) 1NS:  (bitmap & _DATA) != 0 && (bitmap & _PAR ) == 0
 * 2) 1S:   (bitmap & _S   ) != 0 && (bitmap & _PAR ) == 0
 * 3) 1COD: (bitmap & _DATA) != 0 && (bitmap & _PAR ) == 0
 * 4) 1COP: (bitmap & _PAR ) != 0 && (bitmap & _DATA) == 0
 * 5) 1POC: (bitmap & _PAR ) != 0 && (bitmap & _DATA) == 0
 */
static __inline fbe_bool_t rg_r6_pos_bitmap_err_adj_chk(fbe_u16_t pos_bitmap, 
                                           fbe_u16_t err_adj, 
                                           fbe_u16_t mask_non0, 
                                           fbe_u16_t mask_0)
{
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
    else if ((pos_bitmap & mask_non0) != 0 && 
             (pos_bitmap & mask_0   ) == 0 && 
             (pos_bitmap &  err_adj ) != 0 && 
             (pos_bitmap & ~err_adj ) == 0)
    {
        is_valid = FBE_TRUE;
    }
    else
    {
        is_valid = FBE_FALSE;
    }

    return is_valid;
}

/* This macro injects error to fbe_u16_t or fbe_u32_t types.
 *
 * if bit_adj == RG_R6_ERR_PARM_YES
 *  For a given range identified by the mask,
 *  invert the bits in the range.
 *
 * else bit_adj != RG_R6_ERR_PARM_YES
 *  For a given range identified by the mask,
 *  insert a random set of bits.
 */
#define RG_R6_ERR_INJECT_RANGE(size, data_p, bit_adj, mask) \
    do                                                      \
    {                                                       \
        fbe_u##size##_t data_new;                               \
        if ((bit_adj) == RG_R6_ERR_PARM_YES)                \
        {                                                   \
            data_new = ~(*(data_p)) & (mask);               \
        }                                                   \
        else                                                \
        {                                                   \
            data_new = (fbe_u##size##_t)fbe_random() & (mask);   \
        }                                                   \
        *(data_p) = (*(data_p) & ~(mask)) | data_new;       \
    } while (0);

/* This function injects error into a 32-bit word. 
 */
static __inline void rg_r6_err_inject_uint32(fbe_u32_t* data_p, fbe_bool_t bit_adj)
{
    if (bit_adj == RG_R6_ERR_PARM_YES)
    {
        /* If error bits must be adjacent, invert all 32-bits.
         */
        *data_p = ~(*data_p);
    }
    else
    {
        /* If error bits may not be adjacent, randomize 32-bits. 
         */
        *data_p = fbe_random();
    }
}

/*!**************************************************
 * @typedef fbe_logical_error_injection_table_description_t
 ***************************************************
 * @brief string to hold the description of a table.
 ***************************************************/
typedef fbe_char_t fbe_logical_error_injection_table_description_t[FBE_LOGICAL_ERROR_INJECTION_DESCRIPTION_MAX_CHARS];

/*! @struct fbe_logical_error_injection_log_record_t  
 *  @brief Simple log of errors encountered.
 *         We use this during unit testing to validate that
 *         we saw the appropriate combinations of errors.
 */
typedef struct fbe_logical_error_injection_log_record_s
{
    /*! Lba of the error encountered
     */
    fbe_lba_t lba;

    /*! Type of error encountered.
     */
    fbe_u32_t err_type;

    /*! Position in LUN that took error.
     */
    fbe_u32_t position;

    /*! Opcode that encountered error.
     */
    fbe_u32_t opcode;

    /*! Lun that took error.
     */
    fbe_u32_t unit;
}
fbe_logical_error_injection_log_record_t;

/*! @def FBE_LOGICAL_ERROR_INJECTION_MAX_LOG_RECORDS 
 *  @brief Max number of error log records we allow. 
 */
#define FBE_LOGICAL_ERROR_INJECTION_MAX_LOG_RECORDS 100

/*! @struct fbe_logical_error_injection_log_t  
 *  @brief Simple log of errors encountered.
 *         We use this during unit testing to validate that
 *         we saw the appropriate combinations of errors.
 */
typedef struct fbe_logical_error_injection_log_s
{
    /*! The next index to add a log entry. 
     * When we hit the end of the array, we wrap to zero. 
     */
    fbe_u32_t index;
    /*! Array of records that represents the log.
     */
    fbe_logical_error_injection_log_record_t log[FBE_LOGICAL_ERROR_INJECTION_MAX_LOG_RECORDS];
}
fbe_logical_error_injection_log_t;
/*!**************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_MAP_MEMORY
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 ***************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_MAP_MEMORY(memory_p, byte_count)((void*)0)

/*!**************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_UNMAP_MEMORY
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 ***************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_UNMAP_MEMORY(memory_p, byte_count)((void)0)

/*!**************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_VIRTUAL_ADDRESS_AVAIL
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 *   For now everything is virtual.
 ***************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_VIRTUAL_ADDRESS_AVAIL(sg_ptr)(FBE_TRUE)

/* These are not used, but are kept as placeholders.
 */
#define FBE_LOGICAL_ERROR_SGL_MAP_MEMORY(m_sg_element, m_offset, arg3)(m_sg_element->address + m_offset)
#define FBE_LOGICAL_ERROR_SGL_GET_PHYSICAL_ADDRESS(arg1, arg2)((void)0)
#define FBE_LOGICAL_ERROR_SGL_UNMAP_MEMORY(sg, addr)((void)0)

/*!*******************************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_MAX_TRACE_CHARS
 *********************************************************************
 * @brief Max number of chars we can buffer to trace.
 *
 *********************************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_MAX_TRACE_CHARS 256

/*!*******************************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_TS_TRACE_ERROR_PARAMS
 *********************************************************************
 * @brief Parameters we send when tracing an logical_error_injection ts for error.
 *
 *********************************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_TS_TRACE_ERROR_PARAMS FBE_TRACE_LEVEL_ERROR, __LINE__, __FUNCTION__


/*!*************************************************
 * @typedef fbe_logical_error_injection_record_t
 ***************************************************
 * @brief statistics information on the lei service.
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_record_s
{
    fbe_raid_position_bitmask_t pos_bitmap;      /*!< Bitmap for each position to error */
    fbe_u16_t width;              /*!< Array width */
    fbe_u32_t opcode;             /*!< Opcodes to error */
    fbe_lba_t lba;                /*!< Physical address to begin errs    */
    fbe_block_count_t blocks;     /*!< Blocks to extend the errs for     */
    fbe_xor_error_type_t err_type; /*!< Type of errors, see above      */
    fbe_logical_error_injection_mode_t err_mode; /*!< Type of error injection, see above */
    fbe_u16_t err_count;                         /*!< Count of errs injected so far     */
    fbe_u16_t err_limit;                         /*!< Limit of errors to inject         */
    fbe_u16_t skip_count;                        /*!< Limit of errors to skip          */
    fbe_u16_t skip_limit;                        /*!< Limit of errors to inject         */
    fbe_raid_position_bitmask_t err_adj; /*!< Error adjacency bitmask           */
    fbe_u16_t start_bit;                 /*!< Starting bit of an error          */
    fbe_u16_t num_bits;                  /*!< Number of bits of an error        */
    fbe_u16_t bit_adj;                   /*!< Whether erroneous bits adjacent   */
    fbe_u16_t crc_det;                   /*!< Whether error is CRC detectable   */
    fbe_object_id_t object_id; /*!< Object id to inject on. */
}
fbe_logical_error_injection_record_t;

/*!*******************************************************************
 * @enum fbe_logical_error_injection_table_flags_t
 *********************************************************************
 * @brief One flag for each table flag.
 *
 *********************************************************************/
typedef enum fbe_logical_error_injection_table_flags_s
{
    FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_CORRECTABLE_TABLE    = 0x00000001,
    FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_UNCORRECTABLE_TABLE  = 0x00000002,
    FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_ALL_RAID_TYPES       = 0x00000004,
    FBE_LOGICAL_ERROR_INJECTION_TABLE_FLAG_RAID6_ONLY           = 0x00000008,
}
fbe_logical_error_injection_table_flags_t;


/*!*******************************************************************
 * @struct fbe_logical_error_injection_static_table_t
 *********************************************************************
 * @brief This contains a single error injection table.
 *
 *********************************************************************/
typedef struct fbe_logical_error_injection_static_table_s
{
    fbe_logical_error_injection_table_description_t description;
    fbe_logical_error_injection_table_flags_t flags;
    fbe_logical_error_injection_record_t error_info[FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS];
}
fbe_logical_error_injection_static_table_t;

/*!*******************************************************************
 * @struct fbe_logical_error_injection_static_tables_t
 *********************************************************************
 * @brief This contains a single error injection table.
 *
 *********************************************************************/
typedef struct fbe_logical_error_injection_static_tables_s
{
    fbe_logical_error_injection_static_table_t *table[FBE_LOGICAL_ERROR_INJECTION_MAX_TABLES];
}
fbe_logical_error_injection_static_tables_t;

/*!*******************************************************************
 * @struct fbe_logical_error_injection_thread_t
 *********************************************************************
 * @brief This is a thread for the logical error injection service
 *
 *********************************************************************/
typedef struct fbe_logical_error_injection_thread_s
{
    /*! Allows a thread to be queued.
     */
    fbe_queue_element_t queue_element;

    /*! This is the handle to our thread object.
     */
    fbe_thread_t thread_handle;

    /*! This is the queue of requests for the thread to process.
     */
    fbe_queue_head_t packet_queue_head;

    /*! This is the lock to protect local data.
     */
    fbe_spinlock_t lock;

    /*! Used to signal the thread.
     */
    fbe_rendezvous_event_t event;

    /*! Used to tell the thread to stop
     */
    fbe_bool_t stop;
}
fbe_logical_error_injection_thread_t;

/*!*******************************************************************
 * @struct fbe_logical_error_injection_packet_ts_t
 *********************************************************************
 * @brief Structure we use to keep track of a queued packet.
 *
 *********************************************************************/
typedef struct fbe_logical_error_injection_packet_ts_s
{
    fbe_queue_element_t queue_element;
    fbe_object_id_t object_id;
    fbe_time_t arrival_time;
    fbe_time_t expiration_time;
    fbe_packet_t *packet_p;
}
fbe_logical_error_injection_packet_ts_t;

/*!*******************************************************************
 * @struct fbe_logical_error_injection_service_t
 *********************************************************************
 * @brief
 *  This is the definition of the logical_error_injection service, which is used to
 *  generate I/O.
 *
 *********************************************************************/
typedef struct fbe_logical_error_injection_service_s
{
    fbe_base_service_t base_service;

    fbe_spinlock_t lock; /* Protects our data. */
    fbe_bool_t b_enabled; /*!< TRUE if we are enabled. */

    /*! This is the list of fbe_logical_error_injection_object_t structures, 
     * which track all logical_error_injection activity to a given object. 
     */
    fbe_queue_head_t active_object_head;

    fbe_u32_t num_objects; /*!< umber of active objects. */
    fbe_u32_t num_objects_enabled; /*!< Number of active objects injecting errors. */
    fbe_u32_t num_records; /*!< Number of active records. */

    fbe_u64_t num_errors_injected; /*!< Number of errors injected.  */
    fbe_u64_t correctable_errors_detected; /*!< Number of correctable errors detected.  */
    fbe_u64_t uncorrectable_errors_detected; /*!< Number of uncorrectable errors detected.  */
    fbe_u64_t num_validations; /*<! Number of successful validations. */
    fbe_u64_t num_failed_validations; /*<! Number of failed validations. */

    fbe_logical_error_injection_record_t error_info[FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS];
    fbe_logical_error_injection_table_description_t table_desc; /*! Description for active table. */
    fbe_logical_error_injection_table_flags_t table_flags;    /*!< Defines characteristics of the table. */
    fbe_bool_t b_initialized;    /*< Has this struct been initalized */
    fbe_lba_t max_lba;        /*< Largest LBA in the table. */

    fbe_bool_t poc_injection; /*!< Indicate wether POC errors are allowed to be injected.*/

    /*! When the validation fails, this points
     * to the error region that was found to be invalid.
     * We need to save this since we retry all the reads
     * and need to save this context for later displaying
     * the blocks that are relevant to this error region.
     */
    fbe_xor_error_region_t * unmatched_record_p;

    /*! Log structure, which contains a log of 
     * errors injected.  Currently we only log 
     * the media errors we injected on read/remaps. 
     */
    fbe_logical_error_injection_log_t *log_p;

    /*! Number of passes after which to trace. 
     * If set to 0, we do not trace per pass. 
     */
    fbe_u32_t num_passes_per_trace;

    /*! Number of ios after which to trace. 
     *  If set to 0, we do not trace per io. 
     */
    fbe_u32_t num_ios_per_trace;

    fbe_u32_t num_seconds_per_trace; /*!< Num seconds to allow pass before tracing again. */
    fbe_time_t last_trace_time; /*!< Last time we traced. */
    fbe_bool_t b_ignore_corrupt_crc_data_errs; /*!< Indicates whether crc and data errors need to be ignored */

    fbe_logical_error_injection_thread_t delay_thread; /* thread used to delay packets */

}
fbe_logical_error_injection_service_t;

fbe_logical_error_injection_service_t *fbe_get_logical_error_injection_service(void);

/* Rdgen service lock accessors.
 */
static __inline void fbe_logical_error_injection_lock(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();

    fbe_spinlock_lock(&logical_error_injection_p->lock);
    return;
}
static __inline void fbe_logical_error_injection_unlock(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();

    fbe_spinlock_unlock(&logical_error_injection_p->lock);
    return;
}

/* Accessors for error_info.
 */
static __forceinline void fbe_logical_error_injection_get_error_info(fbe_logical_error_injection_record_t **error_info_p,
                                                              fbe_u32_t index) 
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *error_info_p = &logical_error_injection_p->error_info[index];
    return;
}
/* Accessors for unmatched_record_p.
 */
static __forceinline void fbe_logical_error_injection_get_unmatched_record(fbe_xor_error_region_t **unmatched_record_p_p) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    *unmatched_record_p_p = service_p->unmatched_record_p;
    return;
}
static __forceinline void fbe_logical_error_injection_set_unmatched_record(fbe_xor_error_region_t *unmatched_record_p) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    if (service_p->unmatched_record_p == NULL)
    {
        /* We only allow the unmatched record to get set one time,
         * after setting it, we retry the reads and fbe_panic.
         * There should be no possibility of it getting changed again.
         */
        service_p->unmatched_record_p = unmatched_record_p;
    }
    return;
}
/* Accessors for poc_injection.
 */
static __forceinline void fbe_logical_error_injection_get_poc_injection(fbe_bool_t *poc_injection_p) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    *poc_injection_p = service_p->poc_injection;
    return;
}
static __forceinline void fbe_logical_error_injection_set_poc_injection(fbe_bool_t poc_injection) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    service_p->poc_injection = poc_injection;
    return;
}

static __forceinline void fbe_logical_error_injection_get_ignore_corrupt_crc_data_errs(fbe_bool_t *b_ignore_corrupt_crc_data_errors_p) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    *b_ignore_corrupt_crc_data_errors_p = service_p->b_ignore_corrupt_crc_data_errs;
    return;
}

static __forceinline void fbe_logical_error_injection_set_ignore_corrupt_crc_data_errs(fbe_bool_t b_ignore_corrupt_crc_data_errors) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    service_p->b_ignore_corrupt_crc_data_errs = b_ignore_corrupt_crc_data_errors;
    return;
}
/* Accessors for max_lba.
 */
static __forceinline void fbe_logical_error_injection_get_max_lba(fbe_lba_t *max_lba_p) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    *max_lba_p = service_p->max_lba;
    return;
}
static __forceinline void fbe_logical_error_injection_set_max_lba(fbe_lba_t max_lba) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    service_p->max_lba = max_lba;
    return;
}
/* Accessors for table_flags.
 */
static __forceinline void fbe_logical_error_injection_get_table_flags(fbe_logical_error_injection_table_flags_t *table_flags_p) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    *table_flags_p = service_p->table_flags;
    return;
}
static __forceinline void fbe_logical_error_injection_set_table_flags(fbe_logical_error_injection_table_flags_t table_flags) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    service_p->table_flags = table_flags;
    return;
}


/* Accessors for the number of records.
 */
static __inline void fbe_logical_error_injection_init_num_records(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_records = 0;
    return;
}
static __inline void fbe_logical_error_injection_inc_num_records(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_records++;
    return;
}
static __inline void fbe_logical_error_injection_dec_num_records(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_records--;
    return;
}
static __inline void fbe_logical_error_injection_get_num_records(fbe_u32_t *num_records_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *num_records_p = logical_error_injection_p->num_records;
    return;
}

/* Accessors to enable.
 */
static __inline void fbe_logical_error_injection_enable(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->b_enabled = FBE_TRUE;
    return;
}
static __inline void fbe_logical_error_injection_disable(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->b_enabled = FBE_FALSE;
    return;
}
static __inline void fbe_logical_error_injection_get_enable(fbe_bool_t *b_enabled_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *b_enabled_p = logical_error_injection_p->b_enabled;
    return;
}

/* Accessors for table_desc.
 */
static __forceinline void fbe_logical_error_injection_get_table_description(fbe_logical_error_injection_table_description_t **table_desc_p) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    *table_desc_p = &service_p->table_desc;
    return;
}

/* Accessors for b_initialized.
 */
static __forceinline void fbe_logical_error_injection_get_initialized(fbe_bool_t *b_initialized_p) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    *b_initialized_p = service_p->b_initialized;
    return;
}
static __forceinline void fbe_logical_error_injection_set_initialized(fbe_bool_t b_initialized) 
{
    fbe_logical_error_injection_service_t *service_p = fbe_get_logical_error_injection_service();
    service_p->b_initialized = b_initialized;
    return;
}


/* Accessors to how many passes to trace after.
 */
static __inline void fbe_logical_error_injection_set_trace_per_pass_count(fbe_u32_t passes)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_passes_per_trace = passes;
    return;
}
static __inline fbe_u32_t fbe_logical_error_injection_get_trace_per_pass_count(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    return logical_error_injection_p->num_passes_per_trace;
}

/* Accessors to how many ios to trace after.
 */
static __inline void fbe_logical_error_injection_set_trace_per_io_count(fbe_u32_t passes)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_ios_per_trace = passes;
    return;
}
static __inline fbe_u32_t fbe_logical_error_injection_get_trace_per_io_count(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    return logical_error_injection_p->num_ios_per_trace;
}

/* Accessors to how much time to trace after.
 */
static __inline void fbe_logical_error_injection_set_seconds_per_trace(fbe_u32_t seconds)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_seconds_per_trace = seconds;
    return;
}
static __inline fbe_u32_t fbe_logical_error_injection_get_seconds_per_trace(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    return logical_error_injection_p->num_seconds_per_trace;
}

/* Accessors to last trace time.
 */
static __inline void fbe_logical_error_injection_set_last_trace_time(fbe_time_t time)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->last_trace_time = time;
    return;
}
static __inline fbe_time_t fbe_logical_error_injection_get_last_trace_time(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    return logical_error_injection_p->last_trace_time;
}

/* Accessors for the number of objects.
 */
static __inline void fbe_logical_error_injection_inc_num_objects(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_objects++;
    return;
}
static __inline void fbe_logical_error_injection_dec_num_objects(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_objects--;
    return;
}
static __inline void fbe_logical_error_injection_get_num_objects(fbe_u32_t *num_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *num_p = logical_error_injection_p->num_objects;
    return;
}

/* Accessors for the number of objects enabled.
 */
static __inline void fbe_logical_error_injection_inc_num_objects_enabled(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_objects_enabled++;
    return;
}
static __inline void fbe_logical_error_injection_dec_num_objects_enabled(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_objects_enabled--;
    return;
}
static __inline void fbe_logical_error_injection_get_num_objects_enabled(fbe_u32_t *num_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *num_p = logical_error_injection_p->num_objects_enabled;
    return;
}
static __inline void fbe_logical_error_injection_get_active_object_head(fbe_queue_head_t **head_pp)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *head_pp = &logical_error_injection_p->active_object_head;
    return;
}

/* Accessors for the number of failed validations.
 */
static __inline void fbe_logical_error_injection_inc_num_failed_validations(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_failed_validations++;
    return;
}
static __inline void fbe_logical_error_injection_dec_num_failed_validations(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_failed_validations--;
    return;
}
static __inline void fbe_logical_error_injection_get_num_failed_validations(fbe_u64_t *num_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *num_p = logical_error_injection_p->num_failed_validations;
    return;
}
static __inline void fbe_logical_error_injection_set_num_failed_validations(fbe_u64_t num)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_failed_validations = num;
    return;
}
/* Accessors for the number of validations.
 */
static __inline void fbe_logical_error_injection_inc_num_validations(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_validations++;
    return;
}
static __inline void fbe_logical_error_injection_dec_num_validations(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_validations--;
    return;
}
static __inline void fbe_logical_error_injection_get_num_validations(fbe_u64_t *num_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *num_p = logical_error_injection_p->num_validations;
    return;
}
static __inline void fbe_logical_error_injection_set_num_validations(fbe_u64_t num)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_validations = num;
    return;
}
/* Accessors for the number of errors detected.
 */
static __inline void fbe_logical_error_injection_inc_num_errors_injected(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_errors_injected++;
    return;
}
static __inline void fbe_logical_error_injection_dec_num_errors_injected(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_errors_injected--;
    return;
}
static __inline void fbe_logical_error_injection_get_num_errors_injected(fbe_u64_t *num_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *num_p = logical_error_injection_p->num_errors_injected;
    return;
}
static __inline void fbe_logical_error_injection_set_num_errors_injected(fbe_u64_t num)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->num_errors_injected = num;
    return;
}
/* Accessors for the number of errors injected.
 */
static __inline void fbe_logical_error_injection_inc_correctable_errors_detected(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->correctable_errors_detected++;
    return;
}
static __inline void fbe_logical_error_injection_dec_correctable_errors_detected(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->correctable_errors_detected--;
    return;
}
static __inline void fbe_logical_error_injection_get_correctable_errors_detected(fbe_u64_t *num_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *num_p = logical_error_injection_p->correctable_errors_detected;
    return;
}
static __inline void fbe_logical_error_injection_set_correctable_errors_detected(fbe_u64_t num)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->correctable_errors_detected = num;
    return;
}
/* Accessors for the number of errors detected.
 */
static __inline void fbe_logical_error_injection_inc_uncorrectable_errors_detected(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->uncorrectable_errors_detected++;
    return;
}
static __inline void fbe_logical_error_injection_dec_uncorrectable_errors_detected(void)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->uncorrectable_errors_detected--;
    return;
}
static __inline void fbe_logical_error_injection_get_uncorrectable_errors_detected(fbe_u64_t *num_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    *num_p = logical_error_injection_p->uncorrectable_errors_detected;
    return;
}
static __inline void fbe_logical_error_injection_set_uncorrectable_errors_detected(fbe_u64_t num)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    logical_error_injection_p->uncorrectable_errors_detected = num;
    return;
}

/*!*******************************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_OBJECT_DESTROY_WAIT_MSEC
 *********************************************************************
 * @brief Milliseconds to wait when trying to destroy an object
 *        that still has errors getting injected.
 *
 *********************************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_OBJECT_DESTROY_WAIT_MSEC 500

/*!*******************************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_OBJECT_DESTROY_MAX_MSEC
 *********************************************************************
 * @brief Max milliseconds to wait when trying to destory an object
 *        and error injection is going on.
 *
 *********************************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_OBJECT_DESTROY_MAX_MSEC 60000

/*!*******************************************************************
 * @struct fbe_logical_error_injection_object_t
 *********************************************************************
 * @brief
 *  This is the definition of the logical_error_injection object, which is used to track
 *  error injections to a given object.
 *
 *********************************************************************/
typedef struct fbe_logical_error_injection_object_s
{
    /*! We make this first so it is easy to enqueue, dequeue since 
     * we can simply cast the queue element to an object. 
     */
    fbe_queue_element_t queue_element;

    fbe_bool_t b_enabled; /*!< FBE_TRUE if error injection is enabled. */

    /*! This is the spinlock to protect the logical_error_injection object queues.
     */
    fbe_spinlock_t lock;

    /*! This is the object we are testing.
     */
    fbe_object_id_t object_id;

    /*! This is package this object is in.
     */
    fbe_object_id_t package_id;

    /*! The first media error that we injected for this range. 
     */
    fbe_lba_t first_media_error_lba[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /*! The last media error we injected for this range.
     */
    fbe_lba_t last_media_error_lba[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /*! Number of read errors injected.
     */
    fbe_u64_t num_verify_read_media_errors_injected;

    /* Total number of blocks remapped via write verify.
     */
    fbe_u64_t num_verify_write_verify_blocks_remapped; 

    /* Total number of injections in progress NOW.
     */
    fbe_u32_t errors_in_progress; 

    /*! Total number of errors injected.
     */
    fbe_u64_t num_errors_injected;

    /*! Total number of validation calls from raid for this object.
     */
    fbe_u64_t num_validations;

    /*! This is the count of active records on this object.
     */
    fbe_u32_t active_record_count;

    /*! This is the number of total I/Os completed on this object 
     */
    fbe_u32_t num_ios;

    /*! This is the object capacity.
     */
    fbe_lba_t capacity;

    /*! This is the object's stripe size.
     */
    fbe_u32_t stripe_size;

    /*! Edge hook bitmask. 
     */
    fbe_u32_t edge_hook_bitmask;

    /*! Error injection lba adjustment.  Add this amount to the lba to 
     *  inject on before injecting/checking
     */
    fbe_lba_t injection_lba_adjustment;

    fbe_class_id_t class_id;
}
fbe_logical_error_injection_object_t;


static __inline void fbe_logical_error_injection_object_lock(fbe_logical_error_injection_object_t *object_p)
{
    fbe_spinlock_lock(&object_p->lock);
    return;
}
static __inline void fbe_logical_error_injection_object_unlock(fbe_logical_error_injection_object_t *object_p)
{
    fbe_spinlock_unlock(&object_p->lock);
    return;
}
static __inline void fbe_logical_error_injection_enqueue_object(fbe_logical_error_injection_object_t *object_p)
{
    fbe_logical_error_injection_service_t *logical_error_injection_p = fbe_get_logical_error_injection_service();
    fbe_queue_push(&logical_error_injection_p->active_object_head, &object_p->queue_element);
    return;
}

static __inline void fbe_logical_error_injection_dequeue_object(fbe_logical_error_injection_object_t *object_p)
{
    fbe_queue_remove(&object_p->queue_element);
    return;
}
static __inline fbe_bool_t fbe_logical_error_injection_object_is_enabled(fbe_logical_error_injection_object_t *object_p)
{
    return object_p->b_enabled;
}

static __inline void fbe_logical_error_injection_object_set_enabled(fbe_logical_error_injection_object_t *object_p,
                                                                    fbe_bool_t b_enabled)
{
    object_p->b_enabled = b_enabled;
}
/* Accessors for the number of num_verify_read_media_errors_injected.
 */
static __inline void fbe_logical_error_injection_object_inc_in_progress(fbe_logical_error_injection_object_t *object_p)
{
    object_p->errors_in_progress++;
    return;
}
static __inline void fbe_logical_error_injection_object_dec_in_progress(fbe_logical_error_injection_object_t *object_p)
{
    object_p->errors_in_progress--;
    return;
}
static __inline void fbe_logical_error_injection_object_get_in_progress(fbe_logical_error_injection_object_t *object_p,
                                                                        fbe_u32_t *count_p)
{
    *count_p = object_p->errors_in_progress;
    return;
}
static __inline void fbe_logical_error_injection_object_set_in_progress(fbe_logical_error_injection_object_t *object_p,
                                                                        fbe_u32_t count)
{
    object_p->errors_in_progress = count;
    return;
}
/* Accessors for the number of num_verify_read_media_errors_injected.
 */
static __inline void fbe_logical_error_injection_object_inc_num_rd_media_errors(fbe_logical_error_injection_object_t *object_p)
{
    object_p->num_verify_read_media_errors_injected++;
    return;
}
static __inline void fbe_logical_error_injection_object_dec_num_rd_media_errors(fbe_logical_error_injection_object_t *object_p)
{
    object_p->num_verify_read_media_errors_injected--;
    return;
}
static __inline void fbe_logical_error_injection_object_get_num_rd_media_errors(fbe_logical_error_injection_object_t *object_p,
                                                                         fbe_u64_t *count_p)
{
    *count_p = object_p->num_verify_read_media_errors_injected;
    return;
}
static __inline void fbe_logical_error_injection_object_set_num_rd_media_errors(fbe_logical_error_injection_object_t *object_p,
                                                                         fbe_u64_t count)
{
    object_p->num_verify_read_media_errors_injected = count;
    return;
}
/* Accessors for the number of num_verify_write_verify_blocks_remapped.
 */
static __inline void fbe_logical_error_injection_object_inc_num_wr_remaps(fbe_logical_error_injection_object_t *object_p)
{
    object_p->num_verify_write_verify_blocks_remapped++;
    return;
}
static __inline void fbe_logical_error_injection_object_dec_num_wr_remaps(fbe_logical_error_injection_object_t *object_p)
{
    object_p->num_verify_write_verify_blocks_remapped--;
    return;
}
static __inline void fbe_logical_error_injection_object_get_num_wr_remaps(fbe_logical_error_injection_object_t *object_p,
                                                                   fbe_u64_t *count_p)
{
   *count_p = object_p->num_verify_write_verify_blocks_remapped;
   return;
}
static __inline void fbe_logical_error_injection_object_set_num_wr_remaps(fbe_logical_error_injection_object_t *object_p,
                                                                   fbe_u64_t count)
{
    object_p->num_verify_write_verify_blocks_remapped = count;
    return;
}

/* Accessors for the number of num_errors_injected.
 */
static __inline void fbe_logical_error_injection_object_inc_num_errors(fbe_logical_error_injection_object_t *object_p)
{
    object_p->num_errors_injected++;
    return;
}
static __inline void fbe_logical_error_injection_object_dec_num_errors(fbe_logical_error_injection_object_t *object_p)
{
    object_p->num_errors_injected--;
    return;
}
static __inline void fbe_logical_error_injection_object_get_num_errors(fbe_logical_error_injection_object_t *object_p,
                                                                fbe_u64_t *count_p)
{
   *count_p = object_p->num_errors_injected;
   return;
}
static __inline void fbe_logical_error_injection_object_set_num_errors(fbe_logical_error_injection_object_t *object_p,
                                                                fbe_u64_t count)
{
    object_p->num_errors_injected = count;
    return;
}

/* Accessors for the number of num_validations.
 */
static __inline void fbe_logical_error_injection_object_inc_num_validations(fbe_logical_error_injection_object_t *object_p)
{
    object_p->num_validations++;
    return;
}
static __inline void fbe_logical_error_injection_object_dec_num_validations(fbe_logical_error_injection_object_t *object_p)
{
    object_p->num_validations--;
    return;
}
static __inline void fbe_logical_error_injection_object_get_num_validations(fbe_logical_error_injection_object_t *object_p,
                                                                fbe_u64_t *count_p)
{
   *count_p = object_p->num_validations;
   return;
}
static __inline void fbe_logical_error_injection_object_set_num_validations(fbe_logical_error_injection_object_t *object_p,
                                                                fbe_u64_t count)
{
    object_p->num_validations = count;
    return;
}

/* Accessors for the number of passes.
 */
static __inline void fbe_logical_error_injection_object_init_active_record_count(fbe_logical_error_injection_object_t *object_p)
{
    object_p->active_record_count = 0;
    return;
}
static __inline void fbe_logical_error_injection_object_inc_active_record_count(fbe_logical_error_injection_object_t *object_p)
{
    object_p->active_record_count++;
    return;
}
static __inline void fbe_logical_error_injection_object_dec_active_record_count(fbe_logical_error_injection_object_t *object_p)
{
    object_p->active_record_count--;
    return;
}
static __inline fbe_u32_t fbe_logical_error_injection_object_get_active_record_count(fbe_logical_error_injection_object_t *object_p)
{
    return object_p->active_record_count;
}

/* Lock and Unlock lei thread
 */
static __inline void fbe_logical_error_injection_thread_lock(fbe_logical_error_injection_thread_t *thread_p)
{
    fbe_spinlock_lock(&thread_p->lock);
    return;
}
static __inline void fbe_logical_error_injection_thread_unlock(fbe_logical_error_injection_thread_t *thread_p)
{
    fbe_spinlock_unlock(&thread_p->lock);
    return;
}


/*!**************************************************************
 * fbe_logical_error_injection_get_completion_context()
 ****************************************************************
 *
 * @brief
 *    This function returns the packet completion context for
 *    the specified packet's current level.
 *
 * @param   in_packet_p  -  pointer to control packet
 *
 * @return  fbe_packet_completion_context_t
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

static __inline fbe_packet_completion_context_t
fbe_logical_error_injection_get_completion_context(
                                                    fbe_packet_t* in_packet_p
                                                  )
{
    /* return completion context for this packet's current level
     */
    return in_packet_p->stack[in_packet_p->current_level].completion_context;

}   /* end fbe_logical_error_injection_get_completion_context() */


/*************************
 * end file fbe_logical_error_injection_private.h
 *************************/
#endif /* end FBE_LOGICAL_ERROR_INJECTION_PRIVATE_H */
