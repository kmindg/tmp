#ifndef SAS_PHYSICAL_DRIVE_CONFIG_H
#define SAS_PHYSICAL_DRIVE_CONFIG_H

/******************************MODE SENSE/SELECT***************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"

/* The following are Mode Sense / Mode Select pages. */
#define FBE_MS_VENDOR_PG00     0x00  /* unit attention/vendor specific page */
#define FBE_MS_RECOV_PG01      0x01  /* error recovery control page */
#define FBE_MS_DISCO_PG02      0x02  /* disconnect/reconnect page */
#define FBE_MS_FORMAT_PG03     0x03  /* format parameters page */
#define FBE_MS_GEOMETRY_PG04   0x04  /* rigid disk drive geometry page */
#define FBE_MS_VERRECOV_PG07   0x07  /* verify error recovery page */
#define FBE_MS_CACHING_PG08    0x08  /* caching parameters page */
#define FBE_MS_CONTROL_PG0A    0x0A  /* control mode page */
#define FBE_MS_FIBRE_PG19      0x19  /* fibre control page */
#define FBE_MS_POWER_PG1A      0x1A  /* power condition page */
#define FBE_MS_REPORT_PG1C     0x1C  /* method of reporting informational exceptions page */

/* 
 *  The following are Mode Sense / Mode Select Opcode.
 */
#define FBE_MS_SAVE_PAGES 0x11
#define FBE_MS_NO_SAVE_PAGES 0x10
   /* These are the 2 values possible for byte 1 of a Mode Select CDB.
    * The choice between the two is based on whether you will follow up
    * the Mode Select with a Format command.  If a format is not to follow
    * the mode select, then save pages.  Otherwise, do NOT save pages,
    * because some vendors will prematurely save the block descriptor
    * and thus appear formatted before they have been (which will cause
    * dead drive if our code is re-started while the target is formatting!
    */

#define FBE_MAX_MODE_SENSE_BYTES 0xFF
#define FBE_MIN_MODE_SENSE_BYTES 44
   /* These are the maximum and minimum byte counts for Mode Sense
    * data transfers.  The Max of 0xFF is the SCSI max.  The min
    * is the sum of the following (based on CCS sizes): header (4),
    * block descriptor (8), page 1 (8), page 3 (24).  We must have
    * ALL of the above named parts to successfully parse the Mode Sense
    * data that we are concerned with.
    */

/* The following are mode sense byte 2 bit 6 and 7 PC (page control) field
 * settings. Note that the values need to be shifted left (<<6) and ORed (|) 
 * with page code to put into a byte field.
 */
#define FBE_MS_PC_CURRENT_VALUES         0x00    /* current values of mode params*/
#define FBE_MS_PC_CHANGEABLE_VALUES      0x01    /* requests mask of changeable params */
#define FBE_MS_PC_DEFAULT_VALUES         0x02    /* requests deafult values */
#define FBE_MS_PC_SAVED_VALUES           0x03    /* requests saved page params */

/* The following is mode sense byte 2 bit 0-5 page code setting.
 */
#define FBE_MS_ALL_MODE_PAGES            0x3F    /* return all mode pages */

/* For Hornet's Nest and Fibre Channel, the memory allocated for mode
 * sense needs to be a multiple of the sector size, because Tachyon can
 * only use integral sectors in the sg list.
 */
#define FBE_MODE_SENSE_ALLOC_BYTES BE_BYTES_PER_BLOCK

#define FBE_MIN_MODE_SENSE_PAGE4_BYTES 31
        /* hdr(4) + bd(8) + 19(min pg. 4 bytes) = 31
         */

/*
 *  The following are Mode Sense / Mode Select  Header Descriptor values.
 */
#define FBE_MS_HEADER_LEN                     4
#define FBE_MS_NUM_BLKS_OFFSET                0
#define FBE_MS_NUMBER_OF_BLOCKS_OFFSET        1
#define FBE_MS_MEDIUM_TYPE_OFFSET             1
#define FBE_MS_DEVICE_SPECIFIC_PARAM_OFFSET   2
#define FBE_MS_BLOCK_DESCR_LEN_OFFSET         3
#define FBE_MS_BLOCK_LEN_OFFSET               5

#define FBE_MS10_HEADER_LEN                     8
#define FBE_MS10_MEDIUM_TYPE_OFFSET             2
#define FBE_MS10_DEVICE_SPECIFIC_PARAM_OFFSET   3
#define FBE_MS10_BLOCK_DESCR_LEN_OFFSET         6

/*
 * Mask to get the page number out of the byte.
 */
#define FBE_MS_PAGE_NUM_MASK    ((fbe_u8_t) 0x3F)

/*
 * Mode Page 01  - Error Recovery Parameters
 */
#define FBE_PG01_ERR_CNTL_OFFSET                      2
#define FBE_PG01_READ_RETRY_OFFSET                    3
#define FBE_PG01_RECOVERY_LIMIT_OFFSET0              10
#define FBE_PG01_RECOVERY_LIMIT_OFFSET1              11

/* Byte 2 control byte setting: bit 1(DTE) 0, bit 2(PER) 1, bit 5(TB) 0,
 * new R24: bit 0(DCR) 0, bit 6(ARRE) 0, bit 7(AWRE) 0.
 */
#define FBE_PG01_ERR_CNTL_DTE_MASK     ((fbe_u8_t) 0x02)
#define FBE_PG01_ERR_CNTL_PER_MASK     ((fbe_u8_t) 0x04)
#define FBE_PG01_ERR_CNTL_TB_MASK      ((fbe_u8_t) 0x20)
#define FBE_PG01_ERR_CNTL_DCR_MASK     ((fbe_u8_t) 0x01)
#define FBE_PG01_ERR_CNTL_ARRE_MASK    ((fbe_u8_t) 0x40)
#define FBE_PG01_ERR_CNTL_AWRE_MASK    ((fbe_u8_t) 0x80)

/* The current spec has the TB bit in mode page 1 as a don't care for 
 * FC drives. Changes made for 146018 address the ATA version of this issue 
 * but do not correctly implement the don't care case for FC drives.
 * Rather it forces it to zero, which doesn't cause functional problems but 
 * does deviate from the drive spec and is a hassle for manufacturing.
 * So removed the ERR_CNTL_TB_MASK bit from the define of the mask.
 * This will make the bit a don't care. (#151246)
 */
#define FBE_PG01_ERR_CNTL_MASK         ((fbe_u8_t) FBE_PG01_ERR_CNTL_AWRE_MASK | \
                                                 FBE_PG01_ERR_CNTL_ARRE_MASK | \
                                                 FBE_PG01_ERR_CNTL_PER_MASK  | \
                                                 FBE_PG01_ERR_CNTL_DTE_MASK  | \
                                                 FBE_PG01_ERR_CNTL_DCR_MASK)

/* Klondike drives's TB bit is pre-set to 1. We would skip this bit.
 */
#define FBE_PG01_ATA_ERR_CNTL_MASK      ((fbe_u8_t) FBE_PG01_ERR_CNTL_PER_MASK | \
                                                  FBE_PG01_ERR_CNTL_DTE_MASK)
#define FBE_PG01_ERR_CNTL_BYTE          ((fbe_u8_t) FBE_PG01_ERR_CNTL_PER_MASK)

   /* These two values indicate the 2 valid values for the
    * error control byte (byte 2) in page one.  They tell the
    * drive not to auto-remap, exhaust retries before applying
    * ECC correction, report soft errors, do not stop the op in
    * the case of a soft error, and report all soft errors (as
    * Check Condititons).  Also, there are 2 valid values because
    * the TB bit is dont-care (TB=1 means send the bad block's data,
    * and (TB=0) means don't bother.
    */

/*
 *  Default settings for leviathan drives (9GB Low Profile, 18GB half-height)
 *
 *     max retry per lba is 2.2s or 11 steps in recovery algorithm
 *     maximum time for recovery per SCSI command 3s
 */
#define FBE_PG01_READ_RETRY_COUNT_BYTE     ((fbe_u8_t) 0x0B)
#define FBE_PG01_RECOVERY_LIMIT0_BYTE      ((fbe_u8_t) 0x0C)
#define FBE_PG01_RECOVERY_LIMIT1_BYTE      ((fbe_u8_t) 0x00)


/*
 *  Default settings for IBM fibre drives.
 *
 *     maximum time for recovery is exactly 3s
 *     minus 5ms due to drive specs.
 */
#define FBE_PG01_IBM_RECOVERY_LIMIT0_BYTE   ((fbe_u8_t) 0x0B)
#define FBE_PG01_IBM_RECOVERY_LIMIT1_BYTE   ((fbe_u8_t) 0xB3)

/*
 * Mode Page 02 - Disconnect/Reconnect Parameters
 */
#define FBE_PG02_BFR_OFFSET                     2
#define FBE_PG02_BER_OFFSET                     3

#define FBE_PG02_DEFAULT_BFR       ((fbe_u8_t) 0xFF)
#define FBE_PG02_DEFAULT_BER       ((fbe_u8_t) 0xFF)
#define FBE_PG02_IBM_DEFAULT_BFR   ((fbe_u8_t) 0x00)
#define FBE_PG02_IBM_DEFAULT_BER   ((fbe_u8_t) 0x00)
   /* These values define the buffer-fill and buffer-empty ratios
    * used by the DH to program the specific devices to NOT disconnect
    * on 16 sector writes (and not re-connect on reads until buffer is full).
    * IBM Uses adaptive caching.
    */
/*
 * Mode Page 03 - Format Device Parameters
 *
 * Since Fibre Flare does not format drives, this page is not used.
 */
/*
 * Mode Page 04 - Format Device Parameters
 *
 * Since Fibre Flare does not use spindle synch, this page is not used.
 */
/*
 * Mode Page 07 - Verify Error Recovery
 */
#define FBE_PG07_ERR_CNTL_OFFSET                  2
#define FBE_PG07_READ_RETRY_OFFSET                3
#define FBE_PG07_RECOVERY_LIMIT_OFFSET0          10
#define FBE_PG07_RECOVERY_LIMIT_OFFSET1          11

/* R19 changed sniff verify to send VERIFY (0x2F) command, which was controled
 * by page 7. Enabling PER bit allows drives to report recoverd errors.
 */
#define FBE_PG07_ERR_CNTL_PER_MASK      FBE_PG01_ERR_CNTL_PER_MASK
#define FBE_PG07_ERR_CNTL_DCR_MASK      FBE_PG01_ERR_CNTL_DCR_MASK
#define FBE_PG07_ERR_CNTL_MASK          ((fbe_u8_t) FBE_PG07_ERR_CNTL_PER_MASK | \
                                                  FBE_PG07_ERR_CNTL_DCR_MASK)

/*
 * Settings for leviathan drives 
 *     These values will be the same as mode page 1 settings
 *     if in the future there is a need to change this please make sure that 
 *     the drives will support different settings on pages 1 and 7 because
 *     presently seagate drives don't
 */
#define FBE_PG07_READ_RETRY_COUNT_BYTE   FBE_PG01_READ_RETRY_COUNT_BYTE
#define FBE_PG07_RECOVERY_LIMIT0_BYTE    FBE_PG01_RECOVERY_LIMIT0_BYTE
#define FBE_PG07_RECOVERY_LIMIT1_BYTE    FBE_PG01_RECOVERY_LIMIT1_BYTE

/*
 * Settings for IBM drives
 *    see comment for default leviathan drives
 */
#define FBE_PG07_IBM_RECOVERY_LIMIT0_BYTE   FBE_PG01_IBM_RECOVERY_LIMIT0_BYTE
#define FBE_PG07_IBM_RECOVERY_LIMIT1_BYTE   FBE_PG01_IBM_RECOVERY_LIMIT1_BYTE

/*
 * Mode Page 08 - Caching Parameters
 * Starting from R22, We force set DISC bit to improve idle prefetch
 * performance on FC drives. 
 */
#define FBE_PG08_RD_WR_CACHE_OFFSET                    2
#define FBE_PG08_CACHE_MAX_PREFETCH_OFFSET_0           8
#define FBE_PG08_CACHE_MAX_PREFETCH_OFFSET_1           9
#define FBE_PG08_CACHE_SEG_COUNT_OFFSET               13

#define FBE_PG08_WR_CACHE_MASK             ((fbe_u8_t) 0x04)
#define FBE_PG08_RD_CACHE_MASK             ((fbe_u8_t) 0x01)
#define FBE_PG08_DISC_MASK                 ((fbe_u8_t) 0x10)
#define FBE_PG08_RD_WR_CACHE_MASK          ((fbe_u8_t) (FBE_PG08_WR_CACHE_MASK |\
                                                      FBE_PG08_RD_CACHE_MASK))
#define FBE_PG08_RD_WR_CACHE_DISC_MASK     ((fbe_u8_t) (FBE_PG08_WR_CACHE_MASK |\
                                                      FBE_PG08_RD_CACHE_MASK |\
                                                      FBE_PG08_DISC_MASK))
#define FBE_PG08_MAX_PREFETCH_MASK         ((fbe_u8_t) 0xff)

#define FBE_PG08_CACHE_SEG_COUNT_BYTE      ((fbe_u8_t) 0x03)
#define FBE_PG08_RD_CACHE_BYTE             ((fbe_u8_t) 0x00)
/* Changed to RCD (Read Cache Disable) for the RD_CACHE bit. */
#define FBE_PG08_RCD_DISC_BYTE             ((fbe_u8_t) FBE_PG08_RD_CACHE_BYTE | FBE_PG08_DISC_MASK)

#define FBE_PG08_RD_WR_CACHE_BYTE          ((fbe_u8_t) FBE_PG08_RD_CACHE_BYTE | FBE_PG08_WR_CACHE_MASK)
   /* This value indicates that the write-immediates are disabled
    * and read-aheads are enabled for a device that supports Page 8.
    */

/* Added to make sure that the DRA bit of byte 12 is cleared */
#define FBE_PG08_CACHE_PARAMS            ((fbe_u8_t) 0x0C)
#define FBE_PG08_CACHE_PARAMS_MASK       ((fbe_u8_t) 0x20)
#define FBE_PG08_CACHE_PARAMS_VALUE      ((fbe_u8_t) 0x00)
/*
 * Mode Page 0A - Control Mode Parameters
 */

/* The desired value of the QAM/QERR byte in page 0xA is a global variable
 * for debug purposes.  By default, we set it up as follows:
 *    QAM Set:       Allow drive to reorder queued commands any way it wants.
 *    QERR cleared:  Do not abort queued commands after a check condition.
 */
#define FBE_PG0A_QERR_OFFSET                       3

#define FBE_PG0A_QAM_BIT_MASK        ((fbe_u8_t) 0x10)    /* Queue Algorithm Modifier */

#if 0
/* Queue Aging Time Limit for Seagate drives */
#define FBE_PG0A_SEAGATE_QUEUE_TIME_OFFSET0  ((fbe_u8_t) 0xA)
#define FBE_PG0A_SEAGATE_QUEUE_TIME_OFFSET1  ((fbe_u8_t) 0xB)
#define FBE_PG0A_QUEUE_DEFAULT0              ((fbe_u8_t) 0x00)
#define FBE_PG0A_QUEUE_DEFAULT               ((fbe_u8_t) 0x06)
#endif

/*
 * Mode Page 19 - Fibre Control Parameters
 * Fibre Channel Constants
 */
#define FBE_PG19_FC_PROTOCOL_OFFSET                2 /* IBM drives only */
#define FBE_PG19_FC_OFFSET                         3

#define FBE_PG19_FC_PROTOCOL_MASK    ((fbe_u8_t) 0x07)    /* IBM drives only */
#define FBE_PG19_FC_PROTOCOL_VALUE   ((fbe_u8_t) 0x00)    /* IBM drives only */
#define FBE_PG19_FC_DTOLI_BIT_MASK   ((fbe_u8_t) 0x01)
#define FBE_PG19_FC_DTIPE_BIT_MASK   ((fbe_u8_t) 0x02)
#define FBE_PG19_FC_ALWLI_BIT_MASK   ((fbe_u8_t) 0x04)
#define FBE_PG19_FC_DSA_BIT_MASK     ((fbe_u8_t) 0x08)
#define FBE_PG19_FC_DLM_BIT_MASK     ((fbe_u8_t) 0x10)
#define FBE_PG19_FC_DDIS_BIT_MASK    ((fbe_u8_t) 0x20)
#define FBE_PG19_FC_PLPB_BIT_MASK    ((fbe_u8_t) 0x40)

/* Default Fibre Channel mode page settings for back-end drives -
 *   Disable Target Originated Loop Initialization
 *   Allow Login Without Loop Initialization
 *   Disable Soft Address
 *   Disable Loop Master
 */
#define FBE_PG19_DEFAULT_FC_MASK  ((fbe_u8_t) (FBE_PG19_FC_DTOLI_BIT_MASK | \
                                             FBE_PG19_FC_DTIPE_BIT_MASK | \
                                             FBE_PG19_FC_ALWLI_BIT_MASK | \
                                             FBE_PG19_FC_DSA_BIT_MASK   | \
                                             FBE_PG19_FC_DLM_BIT_MASK   | \
                                             FBE_PG19_FC_DDIS_BIT_MASK  | \
                                             FBE_PG19_FC_PLPB_BIT_MASK))

#define FBE_PG19_DEFAULT_FC_VALUE  ((fbe_u8_t) (FBE_PG19_FC_DTOLI_BIT_MASK | \
                                             FBE_PG19_FC_ALWLI_BIT_MASK | \
                                             FBE_PG19_FC_DSA_BIT_MASK   | \
                                             FBE_PG19_FC_DLM_BIT_MASK))

/*
 *  Mode Page 1A - Power Condition Parameters
 */
#define FBE_PG1A_IDLE_STANDBY_OFFSET               3

#define FBE_PG1A_IDLE_STANDBY_MASK   ((fbe_u8_t) 0x03)
#define FBE_PG1A_IDLE_STANDBY_BYTE   ((fbe_u8_t) 0x00)

   /* This value indicates that IDLE and STANDBY drive condition is disabled.
    */
/*
 *  Mode Page 1C - Informational Exceptions Control Parameters
 */
#define FBE_PG1C_PERF_OFFSET                      2
#define FBE_PG1C_PERF_MASK          ((fbe_u8_t) 0xFF)
#define FBE_PG1C_PERF_BYTE          ((fbe_u8_t) 0x0)
#define FBE_PG1C_PERF_DELAY_OFF     ((fbe_u8_t) 0x08)

#define FBE_PG1C_MRIE_OFFSET                      3
#define FBE_PG1C_MRIE_MASK          ((fbe_u8_t) 0x0F)
#define FBE_PG1C_MRIE_BYTE          ((fbe_u8_t) 0x04)

#define FBE_PG1C_INTERVAL_TIMER_OFFSET_BYTE0     4
#define FBE_PG1C_INTERVAL_TIMER_OFFSET_BYTE1     5
#define FBE_PG1C_INTERVAL_TIMER_OFFSET_BYTE2     6
#define FBE_PG1C_INTERVAL_TIMER_OFFSET_BYTE3     7

/* Interval timer is used on ATA only.
 * The granularity is 100 millisecond intervals, so
 * The below 864000 (0x0D2F00) is 1 Day.
 */
#define FBE_PG1C_INTERVAL_TIMER_BYTE0          ((fbe_u8_t) 0x00)
#define FBE_PG1C_INTERVAL_TIMER_BYTE1          ((fbe_u8_t) 0x0D)
#define FBE_PG1C_INTERVAL_TIMER_BYTE2          ((fbe_u8_t) 0x2F)
#define FBE_PG1C_INTERVAL_TIMER_BYTE3          ((fbe_u8_t) 0x00)

#define FBE_PG1C_REPORT_COUNT_OFFSET_0           8
#define FBE_PG1C_REPORT_COUNT_OFFSET_1           9
#define FBE_PG1C_REPORT_COUNT_OFFSET_2           10
#define FBE_PG1C_REPORT_COUNT_OFFSET_3           11

#define FBE_PG1C_REPORT_COUNT_BYTE_0             ((fbe_u8_t) 0x00)
#define FBE_PG1C_REPORT_COUNT_BYTE_1             ((fbe_u8_t) 0x00)
#define FBE_PG1C_REPORT_COUNT_BYTE_2             ((fbe_u8_t) 0x00)
#define FBE_PG1C_REPORT_COUNT_BYTE_3             ((fbe_u8_t) 0x01)

/*
 *  Mode Page 00 - Vendor Unique Parameters
 */

#define FBE_PG00_QPE_OFFSET                       2
#define FBE_PG00_EICM_OFFSET                     13
#define FBE_PG00_QPE_MASK            ((fbe_u8_t) 0x80)
#define FBE_PG00_HDUP_MASK           ((fbe_u8_t) 0x80)
#define FBE_PG00_EICM_MASK           ((fbe_u8_t) 0x40)

#define FBE_PG00_HDUP_EICM_MASK      (FBE_PG00_HDUP_MASK | FBE_PG00_EICM_MASK)

#define FBE_MS_PRIORITY_QUEUE_TIMER_INVALID		0xFFFF

#if 0
typedef struct fbe_vendor_page_s {
    fbe_u8_t page;
    fbe_u8_t offset;
    fbe_u8_t mask;
    fbe_u8_t value;
} fbe_vendor_page_t;
#endif

#define FBE_MS_MAX_SAS_TBL_ENTRIES_COUNT 40
typedef struct fbe_response_table_s {
    fbe_u32_t block_len;          /* actual block size of this drive */
    fbe_u64_t number_of_blocks;     /* actual capacity of this drive. */

    struct
    {
        fbe_u8_t page;
        fbe_u8_t *pg_ptr;
        fbe_u8_t offset;
        fbe_u8_t *offset_ptr;
        fbe_u8_t value;
    }
    mp[FBE_MS_MAX_SAS_TBL_ENTRIES_COUNT];
} fbe_response_table_t;


extern fbe_vendor_page_t fbe_sam_sas_vendor_tbl[];
extern const fbe_u16_t FBE_MS_MAX_DEF_SAS_TBL_ENTRIES_COUNT;
extern fbe_u8_t fbe_sas_attr_pages[];
extern const fbe_u8_t FBE_MS_MAX_SAS_ATTR_PAGES_ENTRIES_COUNT;
extern fbe_vendor_page_t fbe_def_sas_vendor_tbl[];
extern const fbe_u16_t FBE_MS_MAX_SAM_SAS_TBL_ENTRIES_COUNT;
extern fbe_vendor_page_t fbe_minimal_sas_vendor_tbl[];
extern const fbe_u16_t FBE_MS_MAX_MINIMAL_SAS_TBL_ENTRIES_COUNT;

extern fbe_vendor_page_t fbe_cli_mode_page_override_tbl[];
extern fbe_u16_t fbe_cli_mode_page_override_entries;

#endif /* SAS_PHYSICAL_DRIVE_CONFIG_H */
