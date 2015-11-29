#ifndef FBE_ESES_H
#define FBE_ESES_H
/****************************************************************************** 
 * Copyright (C) EMC Corp. 2008 
 * All rights reserved. 
 * Licensed material - Property of EMC Corporation. 
 *****************************************************************************/ 
 
/*!****************************************************************************/ 
/** @file ses.h 
 * 
 * ESES Definitions. 
 * 
 * This file contains structures and other definitions for ESES.  It includes all the structures
 * for SAS, SCSI and SES as documented in the ESES specification V 1.0.
 *
 * Structures beginning with <code>ses_pg_</code> are the SES page structures.  Other structures are
 * fields and descriptors within the pages.
 *
 * The structures for SES pages define only the parts of the pages that are constant, usually the
 * part up to the first variable-length field. Other structures are defined for descriptors and
 * elements in parts of the page that are not at a constant offset, and inlines are provided for
 * fetching those fields in the page.
 *
 * The symbols for the page codes, field names and descriptors used in this file, 
 * exactly match the names in the ESES specification with the following standard abbreviations:
 *
 * <table border="0" cellspacing="1" cellpadding="0" align="center">
 * <tr><td>   addl    <td> additional     </tr>
 * <tr><td>   buf     <td> buffer         </tr>
 * <tr><td>   cmn     <td> common         </tr>
 * <tr><td>   comp    <td> component      </tr>
 * <tr><td>   config  <td> configuration  </tr>
 * <tr><td>   conn    <td> connector      </tr>
 * <tr><td>   desc    <td> descriptor     </tr>
 * <tr><td>   ctrl    <td> control        </tr>
 * <tr><td>   desc    <td> descriptor     </tr>
 * <tr><td>   dev     <td> device         </tr>
 * <tr><td>   diag    <td> diagnostic     </tr>
 * <tr><td>   elec    <td> electronics    </tr>
 * <tr><td>   elem    <td> element        </tr>
 * <tr><td>   encl    <td> enclosure      </tr>
 * <tr><td>   esc     <td> enclosure services controller </tr>
 * <tr><td>   exp     <td> expander       </tr>
 * <tr><td>   fw      <td> firmware       </tr>
 * <tr><td>   gen     <td> generation     </tr>
 * <tr><td>   hdr     <td> header         </tr>
 * <tr><td>   id      <td> identifier     </tr>
 * <tr><td>   info    <td> information    </tr>
 * <tr><td>   init    <td> initialization </tr>
 * <tr><td>   lang    <td> language       </tr>
 * <tr><td>   len     <td> length         </tr>
 * <tr><td>   log     <td> logical        </tr>
 * <tr><td>   mgr     <td> manager        </tr>
 * <tr><td>   num     <td> number [of]    </tr>
 * <tr><td>   pg      <td> page           </tr>
 * <tr><td>   proc    <td> process        </tr>
 * <tr><td>   prod    <td> product        </tr>
 * <tr><td>   ps      <td> power supply   </tr>
 * <tr><td>   rdy     <td> ready          </tr>
 * <tr><td>   rel     <td> relative       </tr>
 * <tr><td>   rqst    <td> request        </tr>
 * <tr><td>   rqsted  <td> requested      </tr>
 * <tr><td>   subencl <td> subenclosure   </tr>
 * <tr><td>   stat    <td> status         </tr>
 * <tr><td>   stats   <td> statistics     </tr>
 * <tr><td>   str     <td> string         </tr>
 * <tr><td>   supp    <td> supported      </tr>
 * <tr><td>   svc     <td> service        </tr>
 * <tr><td>   temp    <td> temporary or temperature </tr>
 * <tr><td>   ver     <td> version        </tr>
 * <tr><td>   vltg    <td> voltage        </tr>
 * </table>
 *
 * and variations by prefixing with "un", adding "s" or "ed", etc.
 *
 * Descriptions of some globals refer to the time they are set: "Startup" means that they are
 * initialized when the SES library is initialized and never changed.  "Config time" means they
 * are initialized each time the configuration changes, which corresponds to an increment of the
 * generation code.  Access to all functions and globals declared in this file is limited to the
 * EMA thread unless otherwise noted.  
 *
 * NOTE: This file uses little endian for Intel compiler.
 ****************************************************************************/

/*
 * Include Files
 */
#include "fbe/fbe_types.h"
#include "stddef.h"
#include "swap_exports.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/eses_interface.h"


#define MOCKUP_VOYAGER  0
#define FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_A1             "Power Supply A1"
#define FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_B1             "Power Supply B1"
#define FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_A0             "Power Supply A0"
#define FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_B0             "Power Supply B0"
#define FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_A              "Power Supply A"
#define FBE_ESES_SUBENCL_TEXT_POWER_SUPPLY_B              "Power Supply B"

/*
 *  constants related to the pages
 */
typedef enum fbe_eses_const_e
{   
    /* Diplay element */
    FBE_ESES_NUM_ELEMS_PER_TWO_DIGIT_DISPLAY = 2, 
    FBE_ESES_NUM_ELEMS_PER_ONE_DIGIT_DISPLAY = 1,     
    /* All the pages with the common header. */
    FBE_ESES_PAGE_LENGTH_OFFSET = 3,
    FBE_ESES_PAGE_SIZE_ADJUST   = 4,  // num bytes:page code + subencl id + page length
    FBE_ESES_PAGE_HEADER_SIZE   = 8,
    FBE_ESES_PAGE_MAX_SIZE      = 2500,/*changed on 10/11/11 by Chang, Rui.*/
    FBE_ESES_GENERATION_CODE_SIZE = 4,
    /*status/control page*/
    FBE_ESES_CTRL_STAT_ELEM_SIZE =  4,
    /*status page*/
    ESES_ENCLOSURE_STATUS_PAGE_MAX_SIZE = 400,
    FBE_ESES_ENTIRE_CONNECTOR_PHYSICAL_LINK = 0xFF,
    /*additional status page*/
    FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE = 4,
    FBE_ESES_ARRAY_DEV_SLOT_PROT_SPEC_INFO_HEADER_SIZE = 4,
    FBE_ESES_SAS_EXP_PROT_SPEC_INFO_HEADER_SIZE = 12,
    FBE_ESES_ESC_ELEC_PROT_SPEC_INFO_HEADER_SIZE = 4,
    ESES_ENCLOSURE_ADDL_STATUS_PAGE_MAX_SIZE = 950,

    /*EMC Enclosure status/control page */
    FBE_ESES_EMC_CTRL_STAT_FIRST_INFO_ELEM_GROUP_OFFSET = 12,
    FBE_ESES_EMC_CONTROL_SAS_CONN_INFO_ELEM_SIZE = 14,
    FBE_ESES_EMC_CONTROL_TRACE_BUF_INFO_ELEM_SIZE = 29,
    FBE_ESES_EMC_CONTROL_ENCL_TIME_INFO_ELEM_SIZE = 8,
    FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE = 4,
    FBE_ESES_EMC_CTRL_STAT_PS_INFO_ELEM_SIZE = 5,
    FBE_ESES_EMC_CTRL_STAT_SPS_INFO_ELEM_SIZE = 6,
    FBE_ESES_EMC_CTRL_STAT_ENCL_TIME_ZONE_UNSPECIFIED = 96,
    FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE = 3,
#if 0
    /*EMC Enclosure status/control page */
    FBE_ESES_EMC_CTRL_STAT_FIRST_INFO_ELEM_GROUP_OFFSET = 12,
    FBE_ESES_EMC_CTRL_STAT_SAS_CONN_INFO_ELEM_SIZE = 13,
    FBE_ESES_EMC_CTRL_STAT_TRACE_BUF_INFO_ELEM_SIZE = 29,
    FBE_ESES_EMC_CTRL_STAT_ENCL_TIME_INFO_ELEM_SIZE = 8,
    FBE_ESES_EMC_CTRL_STAT_GENERAL_INFO_ELEM_SIZE = 4,
    FBE_ESES_EMC_CTRL_STAT_PS_INFO_ELEM_SIZE = 5,
    FBE_ESES_EMC_CTRL_STAT_ENCL_TIME_ZONE_UNSPECIFIED = 96,
    FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE = 3,
#endif
    FBE_ESES_DRIVE_POWER_CYCLE_DURATION_MAX     = 127,  // 7 bits.
    FBE_ESES_DRIVE_POWER_CYCLE_DURATION_DEFAULT = 0,  // The duration for the power cycle in 0.5 second increments.  
                                                      // A value of 0 will result in the default time of 5 seconds.  
    /*download control page*/
    FBE_ESES_DL_CONTROL_UCODE_DATA_LENGTH_OFFSET =  23, // last byte (offset) before image data begins
    FBE_ESES_DL_CONTROL_UCODE_DATA_OFFSET       =   24, // offset where the image data begins
    FBE_ESES_DL_CONTROL_MAX_PAGE_LENGTH         = 4096, // max page length in the send diag page
    FBE_ESES_DL_CONTROL_MAX_UCODE_DATA_LENGTH   = 4072, // max ucode data length in the send diag page
    FBE_ESES_MICROCODE_STATUS_PAGE_MAX_SIZE     =   23, // max page length for microcode status page
    FBE_ESES_DL_CONTROL_TUNNEL_UCODE_DATA_OFFSET = 46,        // offset where the image data begins
    FBE_ESES_DL_CONTROL_MAX_TUNNEL_UCODE_DATA_LENGTH = 2000,  // max ucode data length in the tunnel send diag page
    FBE_ESES_DL_CONTROL_TUNNEL_GET_DOWNLOAD_STATUS_PG_LEN = 22, // Size of the tunneled receive diag status page. 

    /* EMC Statistics status/control page size */
    FBE_ESES_EMC_STATS_CTRL_STAT_COMM_FIELD_LENGTH          = 2,
    FBE_ESES_EMC_STATS_CTRL_STAT_FIRST_ELEM_OFFSET          = 8,

    /*config page*/
    FBE_ESES_SUBENCL_DESC_LENGTH_OFFSET  = 3,
    FBE_ESES_VER_DESC_SIZE               = 20,
    FBE_ESES_VER_DESC_REVISION_SIZE      = 5,
    FBE_ESES_VER_DESC_IDENTIFIER_SIZE    = 12,
    FBE_ESES_BUF_DESC_SIZE               = 4,
    FBE_ESES_ELEM_TYPE_DESC_HEADER_SIZE  = 4,
    FBE_ESES_ELEM_TYPE_NAME_MAX_LEN      = 23,
    FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE  = 16,

    /* FW */
    FBE_ESES_FW_REVISION_1_0_SIZE  = 5,
    FBE_ESES_FW_REVISION_2_0_SIZE  = 16,
    FBE_ESES_FW_REVISION_SIZE      = 16,        // this should be max value of the various CDES-x FW Rev size

    FBE_ESES_FW_IDENTIFIER_SIZE    = 12,

    /* CDES-1 Microcode file format */
    FBE_ESES_MCODE_IMAGE_HEADER_SIZE_OFFSET      = 0,
    FBE_ESES_MCODE_IMAGE_HEADER_SIZE_BYTES       = 4,

    FBE_ESES_MCODE_IMAGE_SIZE_OFFSET             = 16,
    FBE_ESES_MCODE_IMAGE_SIZE_BYTES              = 4,

    FBE_ESES_MCODE_IMAGE_COMPONENT_TYPE_OFFSET   = 24,
    FBE_ESES_MCODE_IMAGE_COMPONENT_TYPE_SIZE_BYTES   = 1,
     
    FBE_ESES_MCODE_IMAGE_REV_OFFSET              = 40,
    FBE_ESES_MCODE_IMAGE_REV_SIZE_BYTES          = 5,

    FBE_ESES_MCODE_NUM_SUBENCL_PRODUCT_ID_OFFSET = 69,
    FBE_ESES_MCODE_SUBENCL_PRODUCT_ID_OFFSET     = 70,

    /* CDES-2 Microcode file format */
    FBE_ESES_CDES2_MCODE_IMAGE_HEADER_SIZE_OFFSET  = 8,
    FBE_ESES_CDES2_MCODE_IMAGE_HEADER_SIZE_BYTES   = 4,

    FBE_ESES_CDES2_MCODE_DATA_SIZE_OFFSET  = 20,
    FBE_ESES_CDES2_MCODE_DATA_SIZE_BYTES   = 4,

    FBE_ESES_CDES2_MCODE_IMAGE_REV_OFFSET          = 48,
    /* The image rev size is actually 32 bytes. But we only use 16 bytes.*/
    FBE_ESES_CDES2_MCODE_IMAGE_REV_SIZE_BYTES      = 16,

    FBE_ESES_CDES2_MCODE_IMAGE_COMPONENT_TYPE_OFFSET   = 12,
    FBE_ESES_CDES2_MCODE_IMAGE_COMPONENT_TYPE_SIZE_BYTES   = 4,

    FBE_ESES_CDES2_MCODE_NUM_SUBENCL_PRODUCT_ID_OFFSET = 104,
    FBE_ESES_CDES2_MCODE_SUBENCL_PRODUCT_ID_OFFSET     = 108,

    /* string out page */
    FBE_ESES_STR_OUT_PAGE_HEADER_SIZE    = 4,
    FBE_ESES_STR_OUT_PAGE_STR_OUT_DATA_MAX_LEN = 80,
    FBE_ESES_STR_OUT_PAGE_DEBUG_CMD_NUM_ELEMS_PER_BYTE = 4,
    FBE_ESES_STR_OUT_PAGE_DEBUG_CMD_NUM_BITS_PER_ELEM = 2,

    /* threshold in page */
    FBE_ESES_THRESH_IN_PAGE_OVERALL_STATUS_ELEM_SIZE    = 4,    
    FBE_ESES_THRESH_IN_PAGE_INDIVIDUAL_STATUS_ELEM_SIZE    = 4,
    FBE_ESES_THRESHOLD_OUT_PAGE_OVERALL_CONTROL_ELEMENT_SIZE    = 4,
    FBE_ESES_THRESHOLD_OUT_PAGE_INDIVIDUAL_CONTROL_ELEMENT_SIZE    = 4,
    
    /* mode parameter list */
    FBE_ESES_MODE_PARAM_LIST_HEADER_SIZE = 8,
    FBE_ESES_MODE_PAGE_COMMON_HEADER_SIZE = 2,
    /* EMC ESES persistent mode page */
    FBE_ESES_EMC_ESES_PERSISTENT_MODE_PAGE_SIZE = 16,
    FBE_ESES_EMC_ESES_PERSISTENT_MODE_PAGE_LEN  = 0x0E, // The number of bytes to following the page lenth The page length is different from page size. 
                                                        // It does not include the bytes before the page_length bytes and the page_length bytes.
    /* EMC ESES non persistent mode page */
    FBE_ESES_EMC_ESES_NON_PERSISTENT_MODE_PAGE_SIZE = 16,
    FBE_ESES_EMC_ESES_NON_PERSISTENT_MODE_PAGE_LEN  = 0x0E, // The page length is different from page size. 
                                                            // It does not include the bytes before the page_length bytes and the page_length bytes.
    /* Read Buffer */
    FBE_ESES_READ_BUF_DESC_SIZE  = 4,
    /* Trace Buffer */
    FBE_ESES_TRACE_BUF_MAX_SIZE = 12288,   // 12KB.

    FBE_ESES_TRACE_BUF_MIN_TRANSFER_SIZE = 2048, //2KB.
    FBE_ESES_TRACE_BUF_MAX_TRANSFER_SIZE = 4096, //4KB.

    /* Sense Data for Illegal Request */
    FBE_ESES_SENSE_DATA_ILLEGAL_REQUEST_START_OFFSET = 15,
    FBE_ESES_SENSE_DATA_ILLEGAL_REQUEST_FILED_POINTER_FIRST_BYTE_OFFSET = 16,
    FBE_ESES_SENSE_DATA_ILLEGAL_REQUEST_FILED_POINTER_SECOND_BYTE_OFFSET = 17,
}fbe_eses_const_t;

typedef enum fbe_eses_desc_type_e{
    FBE_ESES_DESC_TYPE_ARRAY_DEVICE_SLOT = 0,
    FBE_ESES_DESC_TYPE_SAS_EXP_ESC_ELEC = 1
}fbe_eses_desc_type_t;

typedef enum fbe_eses_enclosure_board_type_e{
    FBE_ESES_ENCLOSURE_BOARD_TYPE_UNKNOWN       = 0x0000,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_MAGNUM        = 0x0002,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_DERRINGER     = 0x0003,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_VIPER         = 0x0004,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_SENTRY        = 0x0005,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_EVERGREEN     = 0x0006,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_PINECONE      = 0x0007,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_VOYAGER_ICM   = 0x0008,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_VOYAGER_EE    = 0x0009,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_VOYAGER_EE_REV_A  = 0x000A,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_VIKING        = 0x000B,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_VIKING_RSVD0  = 0x000C,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_SKYDIVE       = 0x000D,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_JETFIRE_BEM   = 0x000E,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_BEACHCOMBER   = 0x000F,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_SILVERBOLT    = 0x0010,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_STARDUST      = 0x0011,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_CAYENNE       = 0x0012,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_ANCHO         = 0x0013,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_TABASCO       = 0x0014,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_OBERON        = 0x0015,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_NAGA          = 0x0016,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_LAPETUS       = 0x0017,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_EUROPA        = 0x0018,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_COLOSSUS      = 0x0019,
    FBE_ESES_ENCLOSURE_BOARD_TYPE_INVALID       = 0xFFFF, 
}fbe_eses_enclosure_board_type_t;       

typedef enum fbe_eses_enclosure_platform_type_e {
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_UNKNOWN        = 0x00,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_VIPER          = 0x01,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_DERRINGER      = 0x02,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_MAGNUM         = 0x03,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SENTRY15       = 0x04,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SENTRY25       = 0x05,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_EVERGREEN12    = 0x06,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_EVERGREEN25    = 0x07,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_PINECONE       = 0x08,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_VOYAGER        = 0x0A,    
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_VIKING         = 0x0B,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_JETFIRE12      = 0x0C,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_JETFIRE25      = 0x0D,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SKYDIVE12      = 0x0E,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SKYDIVE25      = 0x0F,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_BEACHCOMBER12  = 0x10,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_BEACHCOMBER25  = 0x11,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SILVERBOLT12   = 0x12,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_SILVERBOLT25   = 0x13,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_STARDUST       = 0x14,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_CAYENNE        = 0x15,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_ANCHO          = 0x16,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_TABASCO        = 0x17,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_OBERSON25      = 0x18,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_OBERON12       = 0x19,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_NAGA           = 0x1A,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_HYPERION       = 0x1B,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_EUROPA12       = 0x1C,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_EUROPA25       = 0x1D,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_COLOSSUS       = 0x1E,
    FBE_ESES_ENCLOSURE_PLATFORM_TYPE_INVALID        = 0xFF
}fbe_eses_enclosure_platform_type_t;

typedef enum fbe_eses_conn_type_e{
    FBE_ESES_CONN_TYPE_UNKNOWN              = 0x0,
    FBE_ESES_CONN_TYPE_SAS_4X               = 0x1,
    FBE_ESES_CONN_TYPE_MINI_SAS_4X          = 0x2,
    FBE_ESES_CONN_TYPE_QSFP_PLUS            = 0x3,
    FBE_ESES_CONN_TYPE_MINI_SAS_4X_ACTIVE   = 0x4,
    FBE_ESES_CONN_TYPE_MINI_SAS_HD_4X       = 0x5,
    FBE_ESES_CONN_TYPE_MINI_SAS_HD_8X       = 0x6,
    FBE_ESES_CONN_TYPE_MINI_SAS_HD_16X      = 0x7,
    FBE_ESES_CONN_TYPE_SAS_DRIVE            = 0x20,
    FBE_ESES_CONN_TYPE_VIRTUAL_CONN         = 0x2F,
    FBE_ESES_CONN_TYPE_INTERNAL_CONN        = 0x3F
}fbe_eses_conn_type_t;

#define ESES_REVISION_1_0                     0x01
#define ESES_REVISION_2_0                     0x02
#define MAX_SUPPORTED_ESES_VENDOR_DESCRIPTOR  ESES_REVISION_2_0

// Pack the structures to prevent padding.
#pragma pack(push, fbe_eses, 1) 

/**************************************
 * RECEIVE_DIAGNOSTIC_RESULTS command.
 **************************************/
typedef struct fbe_eses_receive_diagnostic_results_cdb_s
{
    /* Byte 0 */
    fbe_u8_t operation_code;
    /* Byte 1 */
    fbe_u8_t page_code_valid : 1; // Byte 1, bit 0
    fbe_u8_t rsvd : 7;   // Byte 1, bit 1-7
    /* Byte 2 */
    fbe_u8_t page_code;
    /* Byte 3 */
    fbe_u8_t alloc_length_msbyte;
    /* Byte 4 */
    fbe_u8_t alloc_length_lsbyte;
    /* Byte 5 */
    fbe_u8_t control;
}fbe_eses_receive_diagnostic_results_cdb_t;

/**************************************
 * SEND_DIAGNOSTIC command.
 **************************************/
typedef struct fbe_eses_send_diagnostic_cdb_s
{
    /* Byte 0 */
    fbe_u8_t operation_code;    // SCSI operation code (1dh)
    /* Byte 1 */
    fbe_u8_t unit_offline :1;   // ignored
    fbe_u8_t device_offline :1; // ignored
    fbe_u8_t self_test :1;      // Perform the Diag Op in the SelfTestcode
    fbe_u8_t rsvd :1;           // Reserved bit
    fbe_u8_t page_format :1;    // Page Format for Single Diag Page
    fbe_u8_t self_test_code : 3;// not supported, must be 0
    /* Byte 2 */
    fbe_u8_t rsvd1;             // Reserved byte
    /* Byte 3 */
    fbe_u8_t param_list_length_msbyte; // The number of bytes to be written to the EMA (MSB)
    /* Byte 4 */
    fbe_u8_t param_list_length_lsbyte; // The number of bytes to be written to the EMA (LSB)
    /* Byte 5 */
    fbe_u8_t control;
}fbe_eses_send_diagnostic_cdb_t;

/**************************************
 * READ_BUFFER command.
 **************************************/
typedef struct fbe_eses_read_buf_cdb_s
{
    fbe_u8_t operation_code;    // Byte 0, SCSI operation code (3Ch)
    fbe_u8_t mode           :5; // Byte 1, bit 0-4, mode
    fbe_u8_t reserved       :3; // Byte 1, bit 5-7, ignored
    fbe_u8_t buf_id;            // Byte 2, buffer id
    fbe_u8_t buf_offset_msbyte;           // Byte 3, buffer offset (MSB)  
    fbe_u8_t buf_offset_midbyte;          // Byte 4, buffer offset (The byte between MSB and LSB) 
    fbe_u8_t buf_offset_lsbyte;           // Byte 5, buffer offset (LSB) 
    fbe_u8_t alloc_length_msbyte; // Byte 6, the maximum number of bytes to read from the EMA (MSB)
    fbe_u8_t alloc_length_midbyte; // Byte 7, the maximum number of bytes to read from the EMA (The byte between MSB and LSB)
    fbe_u8_t alloc_length_lsbyte; // Byte 8, the maximum number of bytes to read from the EMA (LSB)
    fbe_u8_t control; // Byte 9
}fbe_eses_read_buf_cdb_t;

/**************************************
 * WRITE_BUFFER command.
 **************************************/
typedef struct fbe_eses_write_buf_cdb_s
{
    fbe_u8_t operation_code;    // Byte 0, SCSI operation code (3Ch)
    fbe_u8_t mode           :5; // Byte 1, bit 0-4, mode
    fbe_u8_t reserved       :3; // Byte 1, bit 5-7, ignored
    fbe_u8_t buf_id;            // Byte 2, buffer id
    fbe_u8_t buf_offset_msbyte;           // Byte 3, buffer offset (MSB)  
    fbe_u8_t buf_offset_midbyte;          // Byte 4, buffer offset (The byte between MSB and LSB) 
    fbe_u8_t buf_offset_lsbyte;           // Byte 5, buffer offset (LSB) 
    fbe_u8_t param_list_length_msbyte; // Byte 6, the number of bytes to be written to the EMA (MSB)
    fbe_u8_t param_list_length_midbyte; // Byte 7, the number of bytes to be written to the EMA (The byte between MSB and LSB)
    fbe_u8_t param_list_length_lsbyte; // Byte 8, the number of bytes to be written to the EMA (LSB)
    fbe_u8_t control; // Byte 9
}fbe_eses_write_buf_cdb_t;


/**************************************
 * MODE SENSE (10) command.
 **************************************/
typedef struct fbe_eses_mode_sense_10_cdb_s
{
    fbe_u8_t operation_code;    // Byte 0, SCSI operation code (5Ah)
    fbe_u8_t rsvd1          :3; // Byte 1, bit 0-2, reserved
    fbe_u8_t dbd            :1; // Byte 1, bit 3
    fbe_u8_t llbaa          :1; // Byte 1, bit 4
    fbe_u8_t rsvd2          :3; // Byte 1, bit 5-7, reserved
    fbe_u8_t pg_code        :6; // Byte 2, bit 0-5, the mode page to be returned.
    fbe_u8_t pc             :2; // Byte 2, bit 6-7 
    fbe_u8_t subpg_code;        // Byte 3, the mode subpage code to be returned.
    fbe_u8_t rsvd3[3];       // Byte 4-6,  
    fbe_u8_t alloc_length_msbyte; // Byte 7, the number of bytes to be written to the EMA (MSB)
    fbe_u8_t alloc_length_lsbyte; // Byte 8, the number of bytes to be written to the EMA (LSB)
    fbe_u8_t control;             // Byte 9
}fbe_eses_mode_sense_10_cdb_t;

/**************************************
 * MODE SELECT (10) command.
 **************************************/
typedef struct fbe_eses_mode_select_10_cdb_s
{
    fbe_u8_t operation_code;    // Byte 0, SCSI operation code (55h)
    fbe_u8_t sp             :1; // Byte 1, bit 0
    fbe_u8_t rsvd1          :3; // Byte 1, bit 1-3, reserved
    fbe_u8_t pf             :1; // Byte 1, bit 4
    fbe_u8_t rsvd2          :3; // Byte 1, bit 5-7, reserved
    fbe_u8_t rsvd3[5];          // Byte 2-6    
    fbe_u8_t param_list_length_msbyte; // Byte 7, the number of bytes to be written to the EMA (MSB)
    fbe_u8_t param_list_length_lsbyte; // Byte 8, the number of bytes to be written to the EMA (LSB)
    fbe_u8_t control;             // Byte 9
}fbe_eses_mode_select_10_cdb_t;

#pragma pack(pop, fbe_eses) /* Go back to default alignment.*/

/*****************************************************************************
 * !!!!!!!!!!!!!!!!!!!!!The following is copied from ses.h !!!!!!!!!!!!!!!!!!
 * Changed UINT8 to fbe_u8_t to meet the fbe naming convention.
 *****************************************************************************

/** Macro to generate a compile-time error if the size of \p name is not \p length.
 *  Use this to verify size of structures where the exact size is important
 *  (e.g., for structures defining messages exchanged with separately compiled software).
 *
 *  <p>"array size cannot be negative" means size is smaller than \p length
 *  <p>"array size cannot be zero"     means size is bigger than \p length
 *  <p>"array size must be greater than zero" means either
 */
#if !defined (SIZE_CHECK)
#define SIZE_CHECK(name, length) CSX_ASSERT_AT_COMPILE_TIME_GLOBAL_SCOPE(sizeof(name) == length)
#endif

typedef fbe_u8_t PAD;


#pragma pack(push, naa, 1)
   
/*****************************************************************************/ 
/** NAA identifier for a piece of hardware, used in SES and other places.
 * The value is specific to the vendor of the hardware.
 * For EMC, the values are defined below.  
 *****************************************************************************/

typedef struct {
    fbe_u8_t vendor_specific_id_A_high : 4;   ///< Byte 0, bits 0-3,      always 0x2
    fbe_u8_t NAA                       : 4;   ///< Byte 0, bits 4-7 
    fbe_u8_t vendor_specific_id_A_low;        ///< Byte 1
    fbe_u8_t IEEE_company_id[3];              ///<     Byte 2-4,              0x000097 for EMC.      
    fbe_u8_t vendor_specific_id_B[3];         ///< Byte 5-7,              zero if hardware not present.
}naa_id_struct;

SIZE_CHECK(naa_id_struct, 8);

#pragma pack(pop, naa)

// Pack the structures to prevent padding.

#pragma pack(push, ses_interface, 1)



/****************************************************************************
 ****************************************************************************
 *****                                                                  *****   
 *****               BASIC TYPES, ENUMS AND GLOBALS                     *****
 *****                                                                  *****   
 ***** The items defined as typedefs here are enums instead of simple   *****
 ***** redefinitions of basic types because that enforces better strong *****
 ***** typing, even though we don't assign names to all possible values.*****
 ***** To set a variable of one of these types to a number you have to  *****
 ***** use a cast.                                                      *****
 ****************************************************************************
 ****************************************************************************/


/******************************************************************************//**
 * Supported SES page codes.
 *
 * 00h - 0Fh Standard SES pages          <br>
 * 10h - 1Fh Vendor-specific (EMC) pages <br>
 * 20h - 2Fh Reserved for SES-2          <br>
 * 80h - FFh Vendor-specific for SPC-4 
 *******************************************************************************/
typedef enum {
    // Standard SES pages 0x00 - 0x0F
    SES_PG_CODE_SUPPORTED_DIAGS_PGS              = 0x00, ///< 00h
    SES_PG_CODE_CONFIG                      = 0x01, ///< 01h
    SES_PG_CODE_ENCL_CTRL                   = 0x02, ///< 02h
    SES_PG_CODE_ENCL_STAT                   = 0x02, ///< 02h
    SES_PG_CODE_HELP_TEXT                   = 0x03, ///< 03h
    SES_PG_CODE_STR_IN                      = 0x04, ///< 04h
    SES_PG_CODE_STR_OUT                     = 0x04, ///< 04h
    SES_PG_CODE_THRESHOLD_IN                = 0x05, ///< 05h
    SES_PG_CODE_THRESHOLD_OUT               = 0x05, ///< 05h
    SES_PG_CODE_ELEM_DESC                   = 0x07, ///< 07h
    SES_PG_CODE_ENCL_BUSY                   = 0x09, ///< 09h
    SES_PG_CODE_ADDL_ELEM_STAT              = 0x0A, ///< 0Ah
    SES_PG_CODE_DOWNLOAD_MICROCODE_CTRL     = 0x0E, ///< 0Eh
    SES_PG_CODE_DOWNLOAD_MICROCODE_STAT     = 0x0E, ///< 0Eh
    // Vendor-specific SES pages (EMC) 0x10-0x1E
    SES_PG_CODE_EMC_ENCL_CTRL               = 0x10, ///< 10h
    SES_PG_CODE_EMC_ENCL_STAT               = 0x10, ///< 10h
    SES_PG_CODE_EMC_STATS_CTRL              = 0x11, ///< 11h
    SES_PG_CODE_EMC_STATS_STAT              = 0x11, ///< 11h
    //                                        0x12-0x1F available
    SES_PG_CODE_EMC_ESES_PLATFORM_SPECIFIC  = 0x1F, ///< 1Fh
    // Reserved for SES-2              0x20 - 0x2F
    SES_PG_CODE_EMC_ESES_PERSISTENT_MODE_PG = 0x20,
    SES_PG_CODE_EMC_ESES_NON_PERSISTENT_MODE_PG = 0x21,
    SES_PG_CODE_ALL_SUPPORTED_MODE_PGS      = 0x3F,
    // Vendor-specific SPC-4 pages     0x80 - 0xFF
    SES_PG_CODE_EMM_SYS_LOG_PG              = 0x80, ///< 80h PMC
    SES_PG_CODE_DELAY_TEST_PG               = 0x81, ///< 81h PMC
    SES_PG_CODE_FW_STAT_PG                  = 0x82, ///< 82h PMC
    SES_PG_CODE_TUNNEL_DIAG_CTRL            = 0x83, ///< 83h PMC
    SES_PG_CODE_TUNNEL_DIAG_STATUS          = 0x83, ///< 83h PMC

    /// The maximum page code for status pages we store persistently
    SES_PG_CODE_INVALID                     = 0xFF
} ses_pg_code_enum;

/******************************************************************************//**
 * Download Microcode Mode
 *
 * This code appears in the \ref fbe_eses_download_control_page_header_t 
 * "mode" byte.
 *******************************************************************************/
typedef enum {
    FBE_ESES_DL_CONTROL_MODE_DOWNLOAD = 0x0E, // download, defer activate
    FBE_ESES_DL_CONTROL_MODE_ACTIVATE = 0x0F, // activate deferred ucode
} fbe_eses_download_microcode_mode;

/******************************************************************************//**
 * Element status codes.
 *
 * This code appears in the \ref ses_cmn_stat_struct "common status byte" in each \ref
 * ses_stat_elem_struct "status element" in the \ref ses_pg_encl_stat_struct "Enclosure Status"
 * diagnostic page.
 *******************************************************************************/
typedef enum {
    SES_STAT_CODE_UNSUPP        = 0x0, ///< 0h, unable to detect status.
    SES_STAT_CODE_OK            = 0x1, ///< 1h, element present and has no errors.
    SES_STAT_CODE_CRITICAL      = 0x2, ///< 2h, fatal condition for element.
    SES_STAT_CODE_NONCRITICAL   = 0x3, ///< 3h, warning condition detected for element.
    SES_STAT_CODE_UNRECOV       = 0x4, ///< 4h, unrecoverable condition detected for element.
    SES_STAT_CODE_NOT_INSTALLED = 0x5, ///< 5h, element not installed.
    SES_STAT_CODE_UNKNOWN       = 0x6, ///< 6h, sensor failed or status unavailable.
    SES_STAT_CODE_UNAVAILABLE   = 0x7, ///< 7h, installed with no errors, but not turned on.
} ses_elem_stat_code_enum;

/****************************************************************************/
/** SES Element Types.  These values appear in the \ref ses_type_desc_hdr_struct.elem_type field.
 ****************************************************************************/
typedef enum {
    SES_ELEM_TYPE_PS             = 0x02, ///< Type 02h. Power Supply element.
    SES_ELEM_TYPE_COOLING        = 0x03, ///< Type 03h. Cooling element.
    SES_ELEM_TYPE_TEMP_SENSOR    = 0x04, ///< Type 04h. Temperature Sensor element.
    SES_ELEM_TYPE_ALARM          = 0x06, ///< Type 06h. Alarm element (unused in ESES).
    SES_ELEM_TYPE_ESC_ELEC       = 0x07, ///< Type 07h. Enclosure Serivces Controller Electronics element.
    SES_ELEM_TYPE_UPS            = 0x0B, ///< Type 0Bh. Uninterruptible Power Supply element.
    SES_ELEM_TYPE_DISPLAY        = 0x0C, ///< Type 0Ch. Display element.
    SES_ELEM_TYPE_ENCL           = 0x0E, ///< Type 0Eh. Enclosure element.
    SES_ELEM_TYPE_LANG           = 0x10, ///< Type 10h. Language element.
    SES_ELEM_TYPE_ARRAY_DEV_SLOT = 0x17, ///< Type 17h. Array Device Slot element.
    SES_ELEM_TYPE_SAS_EXP        = 0x18, ///< Type 18h. SAS Expander element.
    SES_ELEM_TYPE_SAS_CONN       = 0x19, ///< Type 19h. SAS Connector element.
    SES_ELEM_TYPE_EXP_PHY        = 0x81, ///< Type 81h. Expander phy element (EMC-specific).
    SES_ELEM_TYPE_INVALID        = 0xFF, /// used to an initialization value.
} ses_elem_type_enum;

/****************************************************************************/
/** SES Buffer Types.  These values appear in the
 * \ref ses_buf_desc_struct.buf_type field.
 ****************************************************************************/
typedef enum ses_buf_type_enum {
    SES_BUF_TYPE_EEPROM       = 0,
    SES_BUF_TYPE_ACTIVE_TRACE = 1,
    SES_BUF_TYPE_SAVED_TRACE  = 2,
    SES_BUF_TYPE_EVENT_LOG    = 3,
    SES_BUF_TYPE_SAVED_DUMP   = 4,
    SES_BUF_TYPE_ACTIVE_RAM   = 5,
    SES_BUF_TYPE_REGISTERS    = 6,
    SES_BUF_TYPE_INVALID      = 0xFF
} ses_buf_type_enum;

/****************************************************************************/
/** SES Buffer Indexes.  These values appear in the
 * \ref ses_buf_desc_struct.buf_index field.
 ****************************************************************************/
typedef enum ses_buf_index_eeprom_enum {
    SES_BUF_INDEX_LOCAL_SXP_EEPROM   = 0,
    SES_BUF_INDEX_BASEBOARD_EEPROM   = 1,
    SES_BUF_INDEX_LOCAL_LCC_EEPROM   = 2,
    SES_BUF_INDEX_PEER_LCC_EEPROM    = 3,
    SES_BUF_INDEX_CONN_0_EEPROM      = 4,
    SES_BUF_INDEX_CONN_1_EEPROM      = 5,
    SES_BUF_INDEX_CONN_2_EEPROM      = 6,
    SES_BUF_INDEX_CONN_3_EEPROM      = 7,
    SES_BUF_INDEX_LOCAL_PS0_EEPROM   = 8,
    SES_BUF_INDEX_LOCAL_PS1_EEPROM   = 9,
    SES_BUF_INDEX_PEER_PS0_EEPROM    = 10,
    SES_BUF_INDEX_PEER_PS1_EEPROM    = 11,
    SES_BUF_INDEX_SSC_EEPROM         = 12,
    SES_BUF_INDEX_INVALID_EEPROM     = 0xFF
} ses_buf_index_eeprom_enum;

/****************************************************************************/
/** SES Cooling fan speed codes.  These values appear in the Cooling
 * \ref ses_ctrl_elem_cooling_struct "control" and
 * \ref ses_stat_elem_cooling_struct "status" elements.
 ****************************************************************************/
typedef enum {
    SES_FAN_SPEED_STOPPED     = 0,
    SES_FAN_SPEED_LOWEST      = 1,
    SES_FAN_SPEED_2NDLOWEST   = 2,
    SES_FAN_SPEED_3RDLOWEST   = 3,
    SES_FAN_SPEED_MEDIUM      = 4,
    SES_FAN_SPEED_3RDHIGHEST  = 5,
    SES_FAN_SPEED_2NDHIGHEST  = 6,
    SES_FAN_SPEED_HIGHEST     = 7
} ses_fan_speed_code_enum;

/****************************************************************************/
/** Subenclosure type field in \ref ses_subencl_desc_struct "subenclosure descriptor"
 *  in the \ref ses_pg_config_struct "Configuration diagnostic page".
 ****************************************************************************/

typedef enum ses_subencl_type_enum {
    SES_SUBENCL_TYPE_PS      = 0x02, ///< Type 02h, Power Supply
    SES_SUBENCL_TYPE_COOLING = 0x03, ///< Type 03h, Cooling
    SES_SUBENCL_TYPE_LCC     = 0x07, ///< Type 07h, LCC
    SES_SUBENCL_TYPE_UPS     = 0x0B, ///< Type 0Bh, UPS
    SES_SUBENCL_TYPE_CHASSIS = 0x0E, ///< Type 0Eh, Chassis
    SES_SUBENCL_TYPE_INVALID = 0xFF  /// 
} ses_subencl_type_enum;

/****************************************************************************/
/** Component type field in \ref ses_ver_desc_struct "version descriptor"
 *  in the \ref ses_subencl_desc_struct "subenclosure descriptor"
 *  in the \ref ses_pg_config_struct "Configuration diagnostic page".
 ****************************************************************************/

typedef enum {
    SES_COMP_TYPE_EXPANDER_FW_EMA         = 0, ///< Type  0, expander firmware 
    SES_COMP_TYPE_EXPANDER_BOOT_LOADER_FW = 1, ///< Type  1, expander boot loader
    SES_COMP_TYPE_INIT_STR                = 2, ///< Type  2, init string
    SES_COMP_TYPE_FPGA_IMAGE              = 3, ///< Type  3, FPGA image
    SES_COMP_TYPE_PS_FW                   = 4, ///< Type  4, power supply firmware
    SES_COMP_TYPE_OTHER_EXPANDER          = 5, ///< Type  5, other expander image
    SES_COMP_TYPE_LCC_MAIN                = 6, ///< Type  6, main (combined) image
    SES_COMP_TYPE_SPS_FW                  = 7, ///< Type  7, SPS image
    SES_COMP_TYPE_COOLING_FW              = 8, ///< Type  8, Cooling Module firmware
    SES_COMP_TYPE_BBU_FW                  = 9, ///< Type  9, BBU firmware
    SES_COMP_TYPE_SPS_SEC_FW              = 10,///< Type 10, Secondary SPS firmware
    SES_COMP_TYPE_SPS_BAT_FW              = 11,///< Type 11, SPS Battery firmware
    SES_COMP_TYPE_OTHER_HW_VER            = 30,///< Type 30, hardware version
    SES_COMP_TYPE_OTHER_FW                = 31,///< Type 31, other firwmare image
    SES_COMP_TYPE_UNKNOWN                 = 255,
} ses_comp_type_enum;

/****************************************************************************/
/** Identifier for a subenclosure, defined in the
 *  \ref ses_pg_config_struct "Configuration" diagnostic page and appearing in
 *  a number of other SES pages.  This identifier is unique within the environment
 *  for a given EMA, but different EMAs may use different identifiers for the same
 *  subenclosure.  
 *
 * If not \ref SES_SUBENCL_ID_NONE, an item of this type is guaranteed to
 * fit into a \p fbe_u8_t.
 * The first ID is \ref SES_SUBENCL_ID_PRIMARY and the number of them (not including
 * the primary) is in ses_pg_config_struct.num_secondary_subencls.
 * These IDs are assigned contiguously.
 ****************************************************************************/
typedef enum {
    SES_SUBENCL_ID_FIRST   = 0,       ///< First subenclosure ID
    SES_SUBENCL_ID_PRIMARY = 0,       ///< Primary subenclosure = local LCC
    SES_NUM_SUBENCLS_0     = 0,       ///< Count of zero subenclosures
    SES_SUBENCL_ID_NONE    = 0xFF,      ///< An ID indicating no subenclosure (can't appear SES page)
} ses_subencl_id;

/****************************************************************************/
/** We use to assign side id on the basis of component index and number of 
 *  components for respective subenclosure. This identifier is unique within the environment
 *  for a given EMA.
 *
 * If not \ref FBE_ESES_SUBENCL_SIDE_ID_INVALID, an item of this type is guaranteed to
 * fit into a \p fbe_u8_t.
 * These IDs are assigned while processing configuration page.
 ****************************************************************************/
typedef enum {
    FBE_ESES_SUBENCL_SIDE_ID_A         = 0,       
    FBE_ESES_SUBENCL_SIDE_ID_B         = 1,       
    FBE_ESES_SUBENCL_SIDE_ID_C         = 2, 
    FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE  = 0x1F,  // Midplane.
    FBE_ESES_SUBENCL_SIDE_ID_INVALID   = 0xFF,  
} fbe_eses_subencl_side_id_t;

/****************************************************************************/
/** Index of a \ref ses_stat_elem_struct "status" or \ref ses_ctrl_elem_struct "control" element
 * in the \ref ses_pg_encl_stat_struct "Enclosure Status" or
 * \ref ses_pg_encl_ctrl_struct "Enclosure Control" diagnostic page.  This index
 * is used frequently in SES to identify an individual element and is unique across all elements.
 * The index of an individual element is the position of the element in the page,
 * relative to all other individual elements, not including overall status or control elements.
 *
 * It is guaranteed that this number will fit into a \p fbe_u8_t.
 * The first index is \ref SES_ELEM_INDEX_FIRST and the number of them is equal to the total
 * of the \ref ses_type_desc_hdr_struct.num_possible_elems "num_possible_elems" fields in the
 * \ref ses_type_desc_hdr_struct "type descriptor headers" in the 
 * \ref ses_pg_config_struct "Configuration" diagnostic page.  The indexes are contiguous, but,
 * because they do not count overall status elements, they cannot be used directly to index
 * into the array of status or control elements in the Enclosure Status or Control pages.
 * \see ses_pg_encl_stat_struct.stat_elem
 ****************************************************************************/
typedef enum {
    SES_ELEM_INDEX_FIRST = 0,      ///< First possible element index
    SES_NUM_ELEMS_0      = 0,      ///< Count of zero elements
    SES_ELEM_INDEX_NONE  = 0xFF,   ///< An index indicating no or unknown element
} ses_elem_index;

typedef enum{
    FBE_ESES_TRACE_BUF_ACTION_CTRL_NO_CHANGE = 0,
    FBE_ESES_TRACE_BUF_ACTION_CTRL_CLEAR_BUF = 2,
    FBE_ESES_TRACE_BUF_ACTION_CTRL_SAVE_BUF = 4,
}fbe_eses_trace_buf_action_ctrl_t;

typedef enum{
    FBE_ESES_TRACE_BUF_ACTION_STATUS_ACTIVE = 0,
    FBE_ESES_TRACE_BUF_ACTION_STATUS_UNEXPECTED_EXCEPTION = 1,
    FBE_ESES_TRACE_BUF_ACTION_STATUS_ASSERTION_FAILURE_RESET = 2,
    FBE_ESES_TRACE_BUF_ACTION_STATUS_RESET_ISSUED = 3,
    FBE_ESES_TRACE_BUF_ACTION_STATUS_CLIENT_INIT_SAVE_BUF = 4,
    FBE_ESES_TRACE_BUF_ACTION_STATUS_WATCHDOG_FIRED = 5,
    FBE_ESES_TRACE_BUF_ACTION_STATUS_NMI_OCCURRED = 6,
    FBE_ESES_TRACE_BUF_ACTION_STATUS_ASSERTION_FAILURE_NO_RESET = 7,
    FBE_ESES_TRACE_BUF_ACTION_STATUS_UNUSED_BUF = 0xFF,
}fbe_eses_trace_buf_action_status_t;

typedef enum{
    FBE_ESES_READ_BUF_MODE_DATA = 0x02,
    FBE_ESES_READ_BUF_MODE_DESC = 0x03,
    FBE_ESES_READ_BUF_MODE_INVALID = 0x1F,    
}fbe_eses_read_buf_mode_t;

typedef enum{
    FBE_ESES_WRITE_BUF_MODE_DATA = 0x02,
    FBE_ESES_WRITE_BUF_MODE_INVALID = 0x1F,    
}fbe_eses_write_buf_mode_t;

// Margining test mode in Power Supply Information Control Element
typedef enum{
    FBE_ESES_MARGINING_TEST_MODE_CTRL_NO_CHANGE          = 0x00,
    FBE_ESES_MARGINING_TEST_MODE_CTRL_START_TEST         = 0x01,
    FBE_ESES_MARGINING_TEST_MODE_CTRL_STOP_TEST          = 0x02,
    FBE_ESES_MARGINING_TEST_MODE_CTRL_CLEAR_TEST_RESULTS = 0x03,
    FBE_ESES_MARGINING_TEST_MODE_CTRL_DISABLE_AUTO_TEST  = 0x04,
    FBE_ESES_MARGINING_TEST_MODE_CTRL_ENABLE_AUTO_TEST   = 0x05,
}fbe_eses_margining_test_mode_ctrl_t;

typedef enum{
    FBE_ESES_EXP_PORT_UNKNOWN  = 0x00,
    FBE_ESES_EXP_PORT_UPSTREAM = 0x01, // primary port.
    FBE_ESES_EXP_PORT_DOWNSTREAM = 0x02,  // expansion port.
    FBE_ESES_EXP_PORT_UNIVERSAL = 0x03,
}fbe_eses_exp_port_t;

// defines for PowerCycleRequest field
typedef enum
{
    FBE_ESES_ENCL_POWER_CYCLE_REQ_NONE = 0,
    FBE_ESES_ENCL_POWER_CYCLE_REQ_BEGIN,
    FBE_ESES_ENCL_POWER_CYCLE_REQ_CANCEL,
    FBE_ESES_ENCL_POWER_CYCLE_REQ_RETURN_CC
} fbe_eses_power_cycle_request_t;

// defines for Information Element Type.
typedef enum
{
    FBE_ESES_INFO_ELEM_TYPE_SAS_CONN = 0,
    FBE_ESES_INFO_ELEM_TYPE_TRACE_BUF,
    FBE_ESES_INFO_ELEM_TYPE_ENCL_TIME,
    FBE_ESES_INFO_ELEM_TYPE_GENERAL,
    FBE_ESES_INFO_ELEM_TYPE_DRIVE_POWER,
    FBE_ESES_INFO_ELEM_TYPE_PS,
    FBE_ESES_INFO_ELEM_TYPE_SPS
} fbe_eses_info_elem_type_t;

// defines for expander reset reason.
typedef enum
{
    FBE_ESES_RESET_REASON_UNKNOWN          = 0,
    FBE_ESES_RESET_REASON_WATCHDOG         = 1,
    FBE_ESES_RESET_REASON_SOFTRESET        = 2,
    FBE_ESES_RESET_REASON_EXTERNAL         = 3,
} fbe_eses_reset_reason_t;

// defines for enclosure shutdown reason.
typedef enum
{
    FBE_ESES_SHUTDOWN_REASON_NOT_SCHEDULED                  = 0,
    FBE_ESES_SHUTDOWN_REASON_CLIENT_REQUESTED_POWER_CYCLE   = 1,
    FBE_ESES_SHUTDOWN_REASON_CRITICAL_TEMP_FAULT            = 2,
    FBE_ESES_SHUTDOWN_REASON_CRITICAL_COOLIG_FAULT          = 3,
    FBE_ESES_SHUTDOWN_REASON_PS_NOT_INSTALLED               = 4,
    FBE_ESES_SHUTDOWN_REASON_UNSPECIFIED_HW_NOT_INSTALLED   = 5,
    FBE_ESES_SHUTDOWN_REASON_UNSPECIFIED_REASON             = 127,
} fbe_eses_shutdown_reason_t;


#define FBE_ESES_EXPANDER_ACTION_CLEAR_RESET  0xf

/****************************************************************************
 ****************************************************************************
 *****                                                                  *****   
 *****                    SES PAGE SUPPORTING STRUCTURES                *****
 *****                                                                  *****   
 ***** Identifiers taken directly from ESES 1.0 specification.          *****
 ***** See note at top of file regarding abbreviations.                 *****
 *****                                                                  *****   
 ****************************************************************************
 ****************************************************************************/


/*****************************************************************************/
/** Download Microcode Control Page - does not include the image data area.
 *  This is used as part of firmware download.
 *****************************************************************************/

typedef struct fbe_eses_download_control_page_header
{
    fbe_u8_t    page_code;          // [0] page code (0Eh)
    fbe_u8_t    subenclosure_id;    // [1]
    fbe_u8_t    page_length_msb;    // [2] page length msb
    fbe_u8_t    page_length_lsb;    // [3] page length lsb
    fbe_u32_t   config_gen_code;    // [4-7]
    fbe_u8_t    mode;               // [8] defer or activate
    fbe_u16_t   reserved;           // [9-10]
    fbe_u8_t    buffer_id;          // [11] currently not used
    fbe_u32_t   buffer_offset;      // [12-15] offset to start this transfer
    fbe_u32_t   image_length;       // [16-19] total image size
    fbe_u32_t   transfer_length;    // [20-23] transfer size
} fbe_eses_download_control_page_header_t;

SIZE_CHECK(fbe_eses_download_control_page_header_t, 24);

// defines for protocol field of the tunnel page.
typedef enum
{
    FBE_ESES_TUNNEL_PROT_SMP  = 0x0,
    FBE_ESES_TUNNEL_PROT_SES  = 0x1,
    FBE_ESES_TUNNEL_PROT_SCSI = 0x2,
} fbe_eses_tunnel_prot_t;

// defines for RQST type field of the tunnel page.
typedef enum
{
    FBE_ESES_TUNNEL_RQST_TYPE_LOCAL       = 0x0,
    FBE_ESES_TUNNEL_RQST_TYPE_LOCAL_PEER  = 0x1,
    FBE_ESES_TUNNEL_RQST_TYPE_REMOTE_PEER = 0x2,
} fbe_eses_tunnel_rqst_type_t;

typedef enum {
    FBE_ESES_TUNNEL_DIAG_STATUS_SUCCESS           = 0x0,
    FBE_ESES_TUNNEL_DIAG_STATUS_PROC_LOCAL        = 0x1,
    FBE_ESES_TUNNEL_DIAG_STATUS_PROC_PEER         = 0x2,
    FBE_ESES_TUNNEL_DIAG_STATUS_ABORTED           = 0x3,
    FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_RESOURCES    = 0x4,
    FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_COMM         = 0x5,
    FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_COLLISION    = 0x6,
    FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_REQ_INV      = 0x7,
    FBE_ESES_TUNNEL_DIAG_STATUS_FAIL_BUSY         = 0x8,
} fbe_eses_tunnel_diag_status_t;

/*****************************************************************************/
/** Tunnel Send Diagnostic Page (83h) - does not include the image data area.
 *****************************************************************************/
typedef struct
{
    fbe_u8_t                                page_code;       // byte 0 page code (83h)
    fbe_u8_t                                prot        : 2; // byte 1 bit 0-1 protocol
    fbe_u8_t                                rqst_type   : 3; // byte 1 bit 2-4 RQST type
    PAD                                                 : 3; // byte 1 bit 5-7
    fbe_u8_t                                pg_len_msb;      // byte 2 page length msb 
    fbe_u8_t                                pg_len_lsb;      // byte 3 page length lsb 
    fbe_u8_t                                cmd_len_msb;     // byte 4 command allocation length msb
    fbe_u8_t                                cmd_len_lsb;     // byte 5 command allocation length lsb
    fbe_u8_t                                data_len_msb;    // byte 6 data allocation length msb
    fbe_u8_t                                data_len_lsb;    // byte 7 data allocation length lsb
    fbe_u64_t                               dest_sas_addr;   // byte 8-15 destination address
    fbe_eses_send_diagnostic_cdb_t          cmd;
    fbe_eses_download_control_page_header_t data;
} fbe_eses_tunnel_diag_ctrl_page_header_t;

SIZE_CHECK(fbe_eses_tunnel_diag_ctrl_page_header_t, 46);

/*****************************************************************************/
/** Tunnel Receive Diagnostic Status Page
 *****************************************************************************/
typedef struct
{
    fbe_u8_t                                page_code;       // byte 0 page code (83h)
    fbe_u8_t                                prot        : 2; // byte 1 bit 0-1 protocol
    fbe_u8_t                                rqst_type   : 3; // byte 1 bit 2-4 RQST type
    PAD                                                 : 3; // byte 1 bit 5-7
    fbe_u8_t                                pg_len_msb;      // byte 2 page length msb 
    fbe_u8_t                                pg_len_lsb;      // byte 3 page length lsb
    fbe_u8_t                                cmd_len_msb;     // byte 4 command allocation length msb
    fbe_u8_t                                cmd_len_lsb;     // byte 5 command allocation length lsb
    fbe_u8_t                                data_len_msb;    // byte 6 data allocation length msb
    fbe_u8_t                                data_len_lsb;    // byte 7 data allocation length lsb
    fbe_u64_t                               dest_sas_addr;   // byte 8-15 destination address
    fbe_eses_receive_diagnostic_results_cdb_t cmd;
} fbe_eses_tunnel_diag_status_page_header_t;

SIZE_CHECK(fbe_eses_tunnel_diag_status_page_header_t, 22);

/*****************************************************************************/
/** Tunnel Command Status Page (83h) 
 *****************************************************************************/
typedef struct
{
    fbe_u8_t                                page_code;       // byte 0 page code (83h)
    fbe_u8_t                                prot        : 2; // byte 1 bit 0-1 protocol
    fbe_u8_t                                rqst_type   : 3; // byte 1 bit 2-4 RQST type
    PAD                                                 : 3; // byte 1 bit 5-7
    fbe_u16_t                               pg_len;          // byte 2-3 page length       
    fbe_u8_t                                status;          // byte 4 status 
    fbe_u16_t                               data_len;        // byte 5-6 data allocation length
    fbe_u16_t                               resp_len;        // byte 7-8 response allocation length 
} fbe_eses_tunnel_cmd_status_page_header_t;

SIZE_CHECK(fbe_eses_tunnel_cmd_status_page_header_t, 9);

/****************************************************************************/
/** Identifier for a subenclosure, defined in the
 *  \ref ses_pg_config_struct "Configuration" diagnostic page and appearing in
 *  a number of other SES pages.  This identifier is unique within the environment
 *  for a given EMA, but different EMAs may use different identifiers for the same
 *  subenclosure.  
 *
 * If not \ref SES_SUBENCL_ID_NONE, an item of this type is guaranteed to
 * fit into a \p fbe_u8_t.
 * The first ID is \ref SES_SUBENCL_ID_PRIMARY and the number of them (not including
 * the primary) is in ses_pg_config_struct.num_secondary_subencls.
 * These IDs are assigned contiguously.
 ****************************************************************************/
typedef enum {
    ESES_DOWNLOAD_STATUS_NONE               = 0x00, // Interim - no download in  progress
    ESES_DOWNLOAD_STATUS_IN_PROGRESS        = 0x01, // Interim - download in progress
    ESES_DOWNLOAD_STATUS_UPDATING_FLASH     = 0x02, // Interim - transfer complete, updating flash
    ESES_DOWNLOAD_STATUS_UPDATING_NONVOL    = 0x03, // Interim - updating non-vol with image
    ESES_DOWNLOAD_STATUS_IMAGE_IN_USE       = 0x10, // Complete - subenclosure will attempt to use image
    ESES_DOWNLOAD_STATUS_NEEDS_ACTIVATE     = 0x13, // Complete - needs activation or hard reset
    ESES_DOWNLOAD_STATUS_ACTIVATE_IN_PROGRESS = 0x70, // temporary useage
    ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD   = 0x80, // Error - page field error
    ESES_DOWNLOAD_STATUS_ERROR_CHECKSUM     = 0x81, // Error - image checksum error, image discarded
    ESES_DOWNLOAD_STATUS_ERROR_TIMEOUT      = 0x82, // Error - download microcode timeout, partial image discarded
    ESES_DOWNLOAD_STATUS_ERROR_IMAGE        = 0x83, // Error - internal error, new image required
    ESES_DOWNLOAD_STATUS_ERROR_BACKUP       = 0x84, // Error - internal error, backup image available
    ESES_DOWNLOAD_STATUS_NO_IMAGE           = 0x85, // Error - activation with no deferred image loaded
    ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED    = 0xF0, // Error - activation failed
    ESES_DOWNLOAD_STATUS_MAX                = ESES_DOWNLOAD_STATUS_NO_IMAGE  // last valid status code
} fbe_eses_download_status_code;

/*****************************************************************************/
/** Download Microcode Status Descriptor
 *  This is the status descriptor that's returned with the Download 
 *  Microcode Status Page.
 *  
 *****************************************************************************/
typedef struct fbe_download_status_desc_s
{
    fbe_u8_t    rsvd;                   // [0]
    fbe_u8_t    subencl_id;             // [1]          
    fbe_u8_t    status;                 // [2]
    fbe_u8_t    addl_status;            // [3]
    fbe_u32_t   max_size;               // [4-7]
    fbe_u8_t    rsvd1[3];               // [8-10]
    fbe_u8_t    expected_buffer_id;     // [11]
    fbe_u32_t   expected_buffer_offset; // [12-15]
} fbe_download_status_desc_t;

/*****************************************************************************/
/** Download Microcode Status Page
 *  Requested page from Receive Diagnostic results.
 *  Transmits imformation about the download operation.
 *****************************************************************************/

typedef struct fbe_eses_download_status_page_s
{
    fbe_u8_t    page_code;          // [0] page code (0Eh)
    fbe_u8_t    number_of_subencls; // [1] number of subencl status descriptors (1 max)
    fbe_u16_t   pg_len;             // [2-3] Byte 2-3,
    fbe_u32_t   gen_code;           // [4-7]
    fbe_download_status_desc_t dl_status; // [8-23] download status descriptor
} fbe_eses_download_status_page_t;


/*****************************************************************************/
/** Subenclosure product revision level in the 
 *  \ref ses_subencl_desc_struct "subenclosure descriptor" in the
 *  \ref ses_pg_config_struct "Configuration diagnostic page".
 * This number contains 4 bytes in vendor-specific format.  If the subenclosure
 * contains a "main" downloadable image this is 4 ASCII hex digits representing a
 * 16-bit binary number.  Bits 15-0 of the number map to:
 *
 * bits 15-11 major number 0-31<br>
 * bits 10-4  minor number 00-99<br>
 * bits 3-0   debug version 0,a-o
 ****************************************************************************/
#define SES_SUBENCL_REV_LEVEL_DIGITS  4
typedef struct {
    fbe_u8_t digits[SES_SUBENCL_REV_LEVEL_DIGITS]; 
} ses_subencl_prod_rev_level_struct;

SIZE_CHECK(ses_subencl_prod_rev_level_struct, 4);

/****************************************************************************/
/** Version descriptor in \ref ses_subencl_desc_struct "subenclosure descriptor"
 * in the \ref ses_pg_config_struct "Configuration diagnostic page".
 * Each descriptor describes the version of some hardware or firmware in the
 * subenclosure, usually associated with a particular element.
 ****************************************************************************/

typedef struct
{
        CHAR   comp_rev_level[5];     ///< Byte 2-6
        CHAR   comp_id[12];           ///< Byte 7-18
} ses_cdes_1_0_revision_struct;
typedef struct
{
    CHAR      comp_rev_level[16];     ///< Byte 2-17
    CHAR      reserved;               ///< Byte 18
} ses_cdes_2_0_revision_struct;

typedef union
{
    ses_cdes_1_0_revision_struct    cdes_1_0_rev;
    ses_cdes_2_0_revision_struct    cdes_2_0_rev;
} ses_cdes_revision_union;

typedef struct {
    fbe_u8_t  elem_index         : 8; ///< Byte 0, element in \ref ses_pg_encl_stat_struct
                                                ///<   "Enclosure Status diagnostic page" to which
                                                ///<   this descriptor pertains
    fbe_u8_t  comp_type          : 5; ///< Byte 1,\ref ses_comp_type_enum bits 0-4
    fbe_u8_t  downloadable       : 1; ///< Byte 1, bit 5
    fbe_u8_t  updated            : 1; ///< Byte 1, bit 6
    fbe_u8_t  main               : 1; ///< Byte 1, bit 7
    ses_cdes_revision_union cdes_rev; ///< Bytes 2-18
    fbe_u8_t  buf_id;                 ///< Byte 19
}ses_ver_desc_struct;

SIZE_CHECK(ses_ver_desc_struct, 20);


/****************************************************************************/
/** Subenclosure descriptor in the \ref ses_pg_config_struct "Configuration diagnostic page".
 *
 * There is one of these in the page for each subenclosure, as reported by
 * \ref ses_pg_config_struct.num_secondary_subencls, plus 1 for the primary subenclosure.
 *
 * The descriptor is variable-length.
 * This structure defines only the first part of the descriptor.  The remainder contains
 * the following fields.  Comments below name the functions you can use to get pointers to
 * these fields.
 *
 * <pre>
    fbe_u8_t           num_buf_descs;                   // \ref fbe_eses_fbe_eses_get_ses_num_buf_descs_p()
    ses_buf_desc    buf_desc[num_buf_descs];         // \ref fbe_eses_get_ses_buf_desc_p()
    fbe_u8_t           num_vpd_pgs;                     // \ref fbe_eses_get_ses_num_vpd_pgs_p()
    fbe_u8_t           vpd_pg_list[num_vpd_pgs];        // \ref fbe_eses_get_ses_vpd_pg_list_p()
    fbe_u8_t           subencl_text_len;                // \ref fbe_eses_get_ses_subencl_text_len_p()
    CHAR            subencl_text[subencl_text_len];  // \ref fbe_eses_get_ses_subencl_text_p()
 * </pre>
 *
 * To get from one descriptor to the next one, use
 * \ref fbe_eses_get_next_ses_subencl_desc_p().
 *
 * This descriptor may appear as a placeholder in the page
 * even if the subenclosure is not present.
 * If not present, \ref subencl_vendor_id is blank (ASCII spaces). Otherwise, unless specified,
 * fields are valid even if the subenclosure is not present.
 ****************************************************************************/
typedef struct {
    fbe_u8_t num_encl_svc_procs    : 3; ///< Byte  0, bit 0-2
    PAD                         : 1; //   Byte  0, bit 3
    fbe_u8_t rel_encl_svcs_proc_id : 3; ///< Byte  0, bit 4-6
    PAD                         : 1; //   Byte  0, bit 7
    fbe_u8_t subencl_id            : 8; ///< Byte  1,           
    fbe_u8_t num_elem_type_desc_hdrs;   ///< Byte  2,          0 if not present
    fbe_u8_t subencl_desc_len;          ///< Byte  3,           
    naa_id_struct   
          subencl_log_id;            ///< Byte  4-11,       see \ref naa_id_struct
    CHAR  subencl_vendor_id[8];      ///< Byte 12-19,       all spaces if subenclosure not present
    CHAR  subencl_prod_id[16];       ///< Byte 20-35
    ses_subencl_prod_rev_level_struct 
          subencl_prod_rev_level;     ///< Byte 36-39,       all spaces if subenclosure not present
    fbe_u8_t subencl_type          : 8; ///< Byte 40,\ref ses_subencl_type_enum 
    fbe_u8_t side                  : 5; ///< Byte 41, bit 0-4, side number or 0x1F
    fbe_u8_t reserved              : 2; ///< Byte 41, bit 5-6, reserved
    fbe_u8_t FRU                   : 1; ///< Byte 41, bit 7,   subenclosure is a FRU
    CHAR  side_name;                 ///< Byte 42,          side name
    fbe_u8_t container_subencl_id;      ///< Byte 43,          same as \p subencl_id if no container
    fbe_u8_t manager_subencl_id;        ///< Byte 44,          same as \p subencl_id if no other manager
    fbe_u8_t peer_subencl_id;           ///< Byte 45,          0xFF if no peer
    fbe_u8_t encl_rel_subencl_uid;      ///< Byte 46,          unique ID across current EMAs 
    CHAR  subencl_ser_num[16];       ///< Byte 47-62,       undefined if not present
    fbe_u8_t num_ver_descs;             ///< Byte 63,           
    ///                                             Byte 64+,
    /// version descriptors.  There are \ref num_ver_descs of these, one for
    /// each version of hardware or software described.
    ses_ver_desc_struct ver_desc[1];               
    
 /* The remaining fields appear after the above array.  Use macros below
    to get pointers to these fields. 

    fbe_u8_t           num_buf_descs;
    ses_buf_desc    buf_desc[num_buf_descs];
    fbe_u8_t           num_vpd_pgs;
    fbe_u8_t           vpd_pg_list[num_vpd_pgs];
    fbe_u8_t           subencl_text_len;
    CHAR            subencl_text[subencl_text_len];
 */
    
} ses_subencl_desc_struct;

SIZE_CHECK(ses_subencl_desc_struct, 64+sizeof(ses_ver_desc_struct));

/****************************************************************************/
/** Buffer descriptor field in \ref ses_subencl_desc_struct "subenclosure descriptor" in the
 * \ref ses_pg_config_struct "Configuration diagnostic page".
 *
 * To obtain a pointer to the array of these, use fbe_eses_get_ses_buf_desc_p().
 * To get a pointer to the number of array elements use fbe_eses_get_ses_num_buf_descs_p().
 ****************************************************************************/
typedef struct {
    fbe_u8_t buf_id;            ///< Byte 0,           ID used in READ/WRITE BUFER
    fbe_u8_t buf_type      : 7; ///< Byte 1, bits 0-6, type of the buffer \ref ses_buf_type_enum
    fbe_u8_t writable      : 1; ///< Byte 1, bit 7,    WRITE BUFER allowed     
    fbe_u8_t buf_index;         ///< Byte 2,           meaning depends on buf_type
    fbe_u8_t buf_spec_info;     ///< Byte 3,           not used at this time
} ses_buf_desc_struct;

SIZE_CHECK(ses_buf_desc_struct, 4);


/****************************************************************************/
/** Type descriptor header in the 
 * \ref ses_pg_config_struct "Configuration diagnostic page".
 *
 * There is one entry per element type per type descriptor text per subenclosure. 
 *
 * These headers appear in the page immediately following the last subenclosure descriptor.
 * To get a pointer to the array of these, you must iterate beyond the last
 * subenclosure descriptor using \ref fbe_eses_get_next_ses_subencl_desc_p().
 ****************************************************************************/
typedef struct {
    fbe_u8_t elem_type             : 8; ///< Byte 0, element type, \ref ses_elem_type_enum
    fbe_u8_t num_possible_elems;        ///< Byte 1, number of possible elements of this type in the subenclosure
    fbe_u8_t subencl_id            : 8; ///< Byte 2, subenclosure ID
    fbe_u8_t type_desc_text_len;        ///< Byte 3, length of the type descriptor text
} ses_type_desc_hdr_struct;

SIZE_CHECK(ses_type_desc_hdr_struct, 4);

typedef struct fbe_eses_elem_group_s
{
    // this element type is defined by expander interface.
    fbe_u8_t    elem_type;
    // the number of elements within this group.
    fbe_u8_t num_possible_elems;
    // subenclosure this group belongs to.
    fbe_u8_t subencl_id;    
    // the element index for its first member.
    fbe_u8_t first_elem_index;
    // byte offset in the control/status page.
    fbe_u16_t byte_offset;    
} fbe_eses_elem_group_t;

/****************************************************************************/
/** Common status byte in each \ref ses_stat_elem_struct "status element"
 * in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page.
 * This appears as the first byte of each status element.
 ****************************************************************************/
typedef struct {
    fbe_u8_t elem_stat_code : 4; ///< Bit 0-3, element status code,\ref ses_elem_stat_code_enum
    fbe_u8_t swap           : 1; ///< Bit 4,   element was swapped
    PAD                  : 1; //   Bit 5
    fbe_u8_t prdfail        : 1; ///< Bit 6,   predicted failure
    PAD                  : 1; //   Bit 7
} ses_cmn_stat_struct;

SIZE_CHECK(ses_cmn_stat_struct, 1);


/****************************************************************************/
/** Common control byte in each \ref ses_ctrl_elem_struct "control element"
 * in the \ref ses_pg_encl_ctrl_struct "Enclosure Control" diagnostic page.
 * This appears as the first byte of each control element.
 ****************************************************************************/
typedef struct {
    PAD             : 4; ///< Bit 0-3
    fbe_u8_t  rst_swap : 1; ///< Bit 4,  reset the \ref ses_cmn_stat_struct.swap bit
    fbe_u8_t  disable  : 1; ///< Bit 5,  disable element
    fbe_u8_t  prdfail  : 1; ///< Bit 6,  set predicted failure (not used in ESES)
    fbe_u8_t  select   : 1; ///< Bit 7,  use this element
} ses_cmn_ctrl_struct;

SIZE_CHECK(ses_cmn_ctrl_struct, 1);

/****************************************************************************/
/** Status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page.
 *  This is the generic format for a 4-byte status element.  
 *  Each element type has its own specific structure.  For these,
 *  see other structures with names beginning with <code>ses_stat_elem_</code>
 ****************************************************************************/
typedef struct {
    ses_cmn_stat_struct cmn_stat; ///< Byte 0,   common status byte
    fbe_u8_t               bytes[3]; ///< Byte 1-3, element-specific
} ses_stat_elem_struct;

SIZE_CHECK(ses_stat_elem_struct, 4);

/****************************************************************************/
/** Control element in the \ref ses_pg_encl_ctrl_struct "Enclosure Control" diagnostic page.
 *  This is the generic format for a 4-byte control element.
 *  Each element type has its own specific structure.  For these,
 *  see other structures with names beginning with <code>ses_ctrl_elem_</code>
 ****************************************************************************/
typedef struct {
    ses_cmn_ctrl_struct cmn_ctrl; ///< Byte 0,   common control byte
    fbe_u8_t               bytes[3]; ///< Byte 1-3, element-specific
} ses_ctrl_elem_struct;

SIZE_CHECK(ses_ctrl_elem_struct, 4);

/****************************************************************************/
/**  SES page header.
 * This describes the first 8 bytes common to most SES pages, used for functions
 * that process this common information.
 ****************************************************************************/
typedef struct {
    fbe_u8_t pg_code            : 8; ///< Byte 0,   the page code, \ref ses_pg_code_enum
    PAD                      : 8; ///< Byte 1,   page code-specific field
    fbe_u16_t pg_len;              ///< Byte 2-3, length of rest of page
    fbe_u32_t gen_code;            ///< Byte 4-7, [expected] generation code
                                            ///<           (not used in all pages)
} ses_common_pg_hdr_struct;

SIZE_CHECK(ses_common_pg_hdr_struct, 8);

#define FBE_ESES_GET_PAGE_SIZE(page_size, pg_hdr_p) \
(page_size = swap16(pg_hdr_p->pg_len) + FBE_ESES_PAGE_SIZE_ADJUST)

/****************************************************************************
 ****************************************************************************
 *****                                                                  *****   
 *****                 BYTE SWAP FOR ENDIAN-NESS PURPOSES               *****
 ***** Because SES (like the rest of SCSI) is a big endian protocol,    *****
 ***** need to swap the bytes for multi-byte fields.                    *****
 ****************************************************************************
 ****************************************************************************/
static __inline fbe_u16_t fbe_eses_get_pg_size(ses_common_pg_hdr_struct *d) 
{
    //Add 4 because pg_len is the length of the rest after the page length bytes.
    //There are 4 bytes before it.
    return (fbe_u16_t)(swap16(d->pg_len) + 4);
}

static __inline void fbe_eses_set_pg_len(ses_common_pg_hdr_struct *d, fbe_u16_t page_size) 
{
     d->pg_len = swap16(page_size - 4);
     return;
}

static __inline fbe_u16_t fbe_eses_get_pg_gen_code(ses_common_pg_hdr_struct *d) 
{
    return (fbe_u16_t)(swap32(d->gen_code));
}

static __inline void fbe_eses_set_pg_gen_code(ses_common_pg_hdr_struct *d, fbe_u32_t generation_code) 
{
    d->gen_code = swap32(generation_code);
    return;
}

/********************************************************************
 ********************************************************************
 ***                                                              ***
 ***                    ELEMENT DEFINITIONS                       ***
 ***                                                              ***
 *** These describe status and control elements for the Enclosure ***
 *** Status and Control SES pages                                 *** 
 ***                                                              ***
 ********************************************************************
 ********************************************************************/

/** Power Supply control element in the \ref ses_pg_encl_ctrl_struct "Enclosure Control" diagnostic page. */
typedef struct {
    ses_cmn_ctrl_struct
         cmn_ctrl;        ///< byte 0,         common control byte
    PAD              : 7; ///< byte 1, bit 0-6
    fbe_u8_t rqst_ident : 1; ///< byte 1, bit 7,  activate identify pattern
    PAD              : 8; ///< byte 2 
    PAD              : 5; ///< byte 3, bit 0-4
    fbe_u8_t rqst_on    : 1; ///< byte 3, bit 5,  turn on/off PS
    fbe_u8_t rqst_fail  : 1; ///< byte 3, bit 6,  activate fail pattern (unused)
    PAD              : 1; ///< byte 3, bit 7   
} ses_ctrl_elem_ps_struct;

SIZE_CHECK(ses_ctrl_elem_ps_struct, 4);

/** Power Supply status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct
         cmn_stat;           ///< byte 0,         common status byte
    PAD                 : 7; ///< byte 1, bit 0-6
    fbe_u8_t ident         : 1; ///< byte 1, bit 7,  identify pattern active
    PAD                 : 1; ///< byte 2, bit 0
    fbe_u8_t dc_over_curr  : 1; ///< byte 2, bit 1,  DC over current
    fbe_u8_t dc_under_vltg : 1; ///< byte 2, bit 2,  DC under voltage
    fbe_u8_t dc_over_vltg  : 1; ///< byte 2, bit 3,  DC over voltage
    PAD                 : 4; ///< byte 2, bit 4-7
    fbe_u8_t dc_fail       : 1; ///< byte 3, bit 0,  at least one DC power failed
    fbe_u8_t ac_fail       : 1; ///< byte 3, bit 1,  input power failed
    fbe_u8_t temp_warn     : 1; ///< byte 3, bit 2,  over temperature warning
    fbe_u8_t overtmp_fail  : 1; ///< byte 3, bit 3,  over temperature failure
    fbe_u8_t off           : 1; ///< byte 3, bit 4,  no DC power at all
    fbe_u8_t rqsted_on     : 1; ///< byte 3, bit 5,  installed and turned on
    fbe_u8_t fail          : 1; ///< byte 3, bit 6,  failure pattern active
    fbe_u8_t hot_swap      : 1; ///< byte 3, bit 7,  hot swap doesn't remove power
}ses_stat_elem_ps_struct;

SIZE_CHECK(ses_stat_elem_ps_struct, 4);

/** Cooling control element in the \ref ses_pg_encl_ctrl_struct "Enclosure Control" diagnostic page. */
typedef struct {
    ses_cmn_ctrl_struct
         cmn_ctrl;               ///< byte 0,        common control byte
    PAD                     : 7; ///< byte 1, bit 0-6
    fbe_u8_t rqst_ident        : 1; ///< byte 1, bit 7, activate identify pattern
    PAD                     : 8; ///< byte 2,   
    fbe_u8_t rqsted_speed_code : 3; ///< byte 3, bit 0-2, requested speed code, \ref ses_fan_speed_code_enum
    PAD                     : 2; ///< byte 3, bit 3-4
    fbe_u8_t rqst_on           : 1; ///< byte 3, bit 5, turn on/off PS
    fbe_u8_t rqst_fail         : 1; ///< byte 3, bit 6, activate fail pattern (unused)
    PAD                     : 1; ///< byte 3, bit 7,   
} ses_ctrl_elem_cooling_struct;

SIZE_CHECK(ses_ctrl_elem_cooling_struct, 4);

/** Cooling status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct
           cmn_stat;                   ///< byte 0,            common status byte
    fbe_u8_t  actual_fan_speed_high  : 2; ///< byte 1,  bit 0-1 , speed in 10's of rpms
    PAD                           : 5; ///< byte 1,  bit 2-6
    fbe_u8_t   ident                 : 1; ///< byte 1,  bit 7,  identify pattern active
    fbe_u8_t  actual_fan_speed_low      ; ///< byte 2,  speed in 10's of rpms
    fbe_u8_t  actual_speed_code      : 3; ///< byte 3,  bit 0-2, actual speed code, \ref ses_fan_speed_code_enum
    PAD                           : 1; ///< byte 3,  bit 3
    fbe_u8_t   off                   : 1; ///< byte 3,  bit 4,  no cooling
    fbe_u8_t   rqsted_on             : 1; ///< byte 3,  bit 5,  installed and turned on
    fbe_u8_t   fail                  : 1; ///< byte 3,  bit 6,  failure pattern active
    PAD                           : 1; ///< byte 3,  bit 7
} ses_stat_elem_cooling_struct;

SIZE_CHECK(ses_stat_elem_cooling_struct, 4);

/** Temperature Sensor control element in the \ref ses_pg_encl_ctrl_struct "Enclosure Control" diagnostic page. */
//typedef struct {
//   ses_cmn_ctrl_struct
//         cmn_ctrl;        ///< byte 0,        common control byte
//    BOOL  rqst_ident : 1;  ///< byte 1, bit 7, activate identify pattern (unused)
//    PAD              : 23; //   byte 1, bit 6-0 to byte 3
//} ses_ctrl_elem_temp_sensor_struct;

//SIZE_CHECK(ses_ctrl_elem_temp_sensor_struct, 4);

typedef struct {
    ses_cmn_ctrl_struct
          cmn_ctrl;        ///< byte 0,        common control byte
    PAD               : 7; ///< byte 1, bit 0-6
    fbe_u8_t  rqst_ident : 1; ///< byte 1, bit 7, activate identify pattern (unused)
    PAD               :8;  ///< byte 2
    PAD               :8;  ///< byte 3
} ses_ctrl_elem_temp_sensor_struct;

SIZE_CHECK(ses_ctrl_elem_temp_sensor_struct, 4);

/** Temperature Sensor status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct
          cmn_stat;        ///< byte 0,         common status byte
    PAD               : 7; ///< byte 1, bit 0-6
    fbe_u8_t  ident      : 1; ///< byte 1, bit 7,  identify pattern active
    fbe_u8_t  temp       : 8; ///< byte 2,         temperature + 20C
    fbe_u8_t  ut_warning : 1; ///< byte 3, bit 0,  unused
    fbe_u8_t  ut_failure : 1; ///< byte 3, bit 1,  unused
    fbe_u8_t  ot_warning : 1; ///< byte 3, bit 2,  Exceed high warning threshold
    fbe_u8_t  ot_failure : 1; ///< byte 3, bit 3,  Exceed high critical threshold
    PAD               : 4; ///< byte 3, bit 4-7
} ses_stat_elem_temp_sensor_struct;

SIZE_CHECK(ses_stat_elem_temp_sensor_struct, 4);

/** ESC Electronics control element in the \ref ses_pg_encl_ctrl_struct "Enclosure Control" diagnostic page. */
typedef struct {
    ses_cmn_ctrl_struct
         cmn_ctrl;          ///< byte 0,         common control byte
    PAD                : 6; ///< byte 1, bit 0-5
    fbe_u8_t rqst_fail    : 1; ///< byte 1, bit 6,  activate failure pattern
    fbe_u8_t rqst_ident   : 1; ///< byte 1, bit 7,  activate identify pattern
    fbe_u8_t select_elem  : 1; ///< byte 2, bit 0,  unused
    PAD                : 7; ///< byte 2, bit 1-7
    PAD                : 8; ///< byte 3         
} ses_ctrl_elem_esc_elec_struct;

SIZE_CHECK(ses_ctrl_elem_esc_elec_struct, 4);

/** ESC Electronics status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct
         cmn_stat;      ///< byte 0,         common status byte
    PAD            : 6; ///< byte 1, bit 0-5
    fbe_u8_t fail     : 1; ///< byte 1, bit 6,  failure pattern active
    fbe_u8_t ident    : 1; ///< byte 1, bit 7,  identify pattern active
    fbe_u8_t report   : 1; ///< byte 2, bit 0,  TRUE for primary LCC, FALSE for peer 
    PAD            : 7; ///< byte 2, bit 1-7 
    PAD            : 7; ///< byte 3, bit 0-6
    fbe_u8_t hot_swap : 1; ///< byte 3, bit 7,  swappable without removing power
} ses_stat_elem_esc_elec_struct;

SIZE_CHECK(ses_stat_elem_esc_elec_struct, 4);

/** Array Device Slot control element in the \ref ses_pg_encl_ctrl_struct "Enclosure Control" diagnostic page. */
typedef struct {
    ses_cmn_ctrl_struct
         cmn_ctrl;                 ///< byte 0,         common control byte
    fbe_u8_t rqst_r_r_abort       : 1; ///< byte 1, bit 0,  copied to \ref ses_stat_elem_esc_elec_struct "status" element
    fbe_u8_t rqst_rebuild_remap   : 1; ///< byte 1, bit 1,  copied to \ref ses_stat_elem_esc_elec_struct "status" element
    fbe_u8_t rqst_in_failed_array : 1; ///< byte 1, bit 2,  copied to \ref ses_stat_elem_esc_elec_struct "status" element
    fbe_u8_t rqst_in_crit_array   : 1; ///< byte 1, bit 3,  copied to \ref ses_stat_elem_esc_elec_struct "status" element
    fbe_u8_t rqst_cons_check      : 1; ///< byte 1, bit 4,  copied to \ref ses_stat_elem_esc_elec_struct "status" element
    fbe_u8_t rqst_hot_spare       : 1; ///< byte 1, bit 5,  copied to \ref ses_stat_elem_esc_elec_struct "status" element
    fbe_u8_t rqst_rsvd_dev        : 1; ///< byte 1, bit 6,  copied to \ref ses_stat_elem_esc_elec_struct "status" element
    fbe_u8_t rqst_ok              : 1; ///< byte 1, bit 7,  activate online pattern (not displayed by Viper)
    PAD                        : 1; //   byte 2, bit 0
    fbe_u8_t rqst_ident           : 1; ///< byte 2, bit 1,  activate identify pattern
    fbe_u8_t rqst_rmv             : 1; ///< byte 2, bit 2,  activate identify pattern
    fbe_u8_t rqst_insert          : 1; ///< byte 2, bit 3,  copied to \ref ses_stat_elem_esc_elec_struct "status" element
    PAD                        : 2; //   byte 2, bit 4-5
    fbe_u8_t do_not_remove        : 1; ///< byte 2, bit 6,  copied to \ref ses_stat_elem_esc_elec_struct "status" element
    fbe_u8_t active               : 1; ///< byte 2, bit 7,  unused
    PAD                        : 2; //   byte 3, bit 0-1
    fbe_u8_t enable_byp_b         : 1; ///< byte 3, bit 2,  unused
    fbe_u8_t enable_byp_a         : 1; ///< byte 3, bit 3,  unused
    fbe_u8_t dev_off              : 1; ///< byte 3, bit 4,  power off slot
    fbe_u8_t rqst_fault           : 1; ///< byte 3, bit 5,  activate failure pattern
    PAD                        : 2; //   byte 3, bit 6-7
} ses_ctrl_elem_array_dev_slot_struct;

SIZE_CHECK(ses_ctrl_elem_array_dev_slot_struct, 4);

/** Array Device Slot status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct
         cmn_stat;                  ///< byte 0,         common status byte
    fbe_u8_t r_r_abort             : 1; ///< byte 1, bit 0,  copied from <code>rqst_r_r_abort</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t rebuild_remap         : 1; ///< byte 1, bit 1,  copied from <code>rqst_rebuild_remap</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t in_failed_array       : 1; ///< byte 1, bit 2,  copied from <code>rqst_in_failed_array</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t in_crit_array         : 1; ///< byte 1, bit 3,  copied from <code>rqst_in_crit_array</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t cons_check            : 1; ///< byte 1, bit 4,  copied from <code>rqst_cons_check</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t hot_spare             : 1; ///< byte 1, bit 5,  copied from <code>rqst_hot_spare</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t rsvd_dev              : 1; ///< byte 1, bit 6,  copied from <code>rqst_rsvd_dev</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t ok                    : 1; ///< byte 1, bit 7,  online pattern active (not displayed by Viper)
    fbe_u8_t report                : 1; ///< byte 2, bit 0,  unused
    fbe_u8_t ident                 : 1; ///< byte 2, bit 1,  identify pattern active
    fbe_u8_t rmv                   : 1; ///< byte 2, bit 2,  copied from <code>rqst_rmv</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t rdy_to_insert         : 1; ///< byte 2, bit 3,  copied from <code>rqst_insert</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t encl_bypassed_b       : 1; ///< byte 2, bit 4,  unused
    fbe_u8_t encl_bypassed_a       : 1; ///< byte 2, bit 5,  unused
    fbe_u8_t do_not_remove         : 1; ///< byte 2, bit 6,  copied from <code>do_not_remove</code> in \ref ses_ctrl_elem_esc_elec_struct "control" element
    fbe_u8_t app_client_bypassed_a : 1; ///< byte 2, bit 7,  unused
    fbe_u8_t dev_bypassed_b        : 1; ///< byte 3, bit 0,  unused
    fbe_u8_t dev_bypassed_a        : 1; ///< byte 3, bit 1,  unused
    fbe_u8_t bypassed_b            : 1; ///< byte 3, bit 2,  unused
    fbe_u8_t bypassed_a            : 1; ///< byte 3, bit 3,  unused
    fbe_u8_t dev_off               : 1; ///< byte 3, bit 4,  slot powered off
    fbe_u8_t fault_requested       : 1; ///< byte 3, bit 5,  failure pattern active due to local rqst_fault
    fbe_u8_t fault_sensed          : 1; ///< byte 3, bit 6,  failure pattern active due to peer client request or EMA-detected fault
    fbe_u8_t app_client_bypassed_b : 1; ///< byte 3, bit 7,  unused
} ses_stat_elem_array_dev_slot_struct;

SIZE_CHECK(ses_stat_elem_array_dev_slot_struct, 4);

/*Expander Phy control element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_ctrl_struct
         cmn_stat;                        ///< byte 0, common control byte
    fbe_u16_t reserved;                   ///< byte 2-3
    PAD                             : 6;  ///< byte 3, bit 0-5
    fbe_u8_t spinup_sas             : 1;  ///< byte 3, bit 6
    fbe_u8_t force_enable           : 1;  ///< byte 3, bit 7
} ses_ctrl_elem_exp_phy_struct;     
 
SIZE_CHECK(ses_ctrl_elem_exp_phy_struct, 4);

/*Expander Phy status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct
         cmn_stat;                        ///< byte 0, common status byte
    fbe_u8_t exp_index;              ///< byte 1,index of sas expander or esc electronics element representing the expander containing the PHY    
    fbe_u8_t phy_id                 : 7;  ///< byte 2, bit 0-6, Logical identifier of the PHY
    fbe_u8_t force_disabled         : 1;  ///< byte 2, bit 7,  indicates EMA disabled PHY that it self-diagnoised
    fbe_u8_t reserved               : 3;  ///< byte 3, bit 0-2, 
    fbe_u8_t carrier_detect         : 1;  ///< byte 3, bit 3, set if something other than storage device inserted in slot
    fbe_u8_t sata_spinup_hold       : 1;  ///< byte 3, bit 4, set if SATA drive inserted in a slot that is in spinup hold state
    fbe_u8_t spinup_enabled         : 1;  ///< byte 3, bit 5, set 
    fbe_u8_t link_rdy               : 1;  ///< byte 3, bit 6, set if the link is ready on the Phy
    fbe_u8_t phy_rdy                : 1;  ///< byte 3, bit 7, corresponds to the SAS_PHY_Ready state
} ses_stat_elem_exp_phy_struct;
 
SIZE_CHECK(ses_stat_elem_exp_phy_struct, 4);

/*SAS connector status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct        cmn_stat;  ///< byte 0, common status byte
    fbe_u8_t conn_type              : 7;  ///< byte 1, bit 0-6    
    fbe_u8_t ident                  : 1;  ///< byte 1, bit 7, 
    fbe_u8_t conn_physical_link;          ///< byte 2, 
    fbe_u8_t                        : 6;  ///< byte 3, bit 0-5, 
    fbe_u8_t fail                   : 1;  ///< byte 3, bit 6, 
    fbe_u8_t                        : 1;  ///< byte 3, bit 4, set if SATA drive inserted in a slot that is in spinup hold state
} ses_stat_elem_sas_conn_struct;

SIZE_CHECK(ses_stat_elem_sas_conn_struct, 4);

/*SAS connector control element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_ctrl_struct cmn_stat;         ///< byte 0, common control byte
    PAD                             : 7;      
    fbe_u8_t rqst_ident             : 1;  ///byte1 -bit 7
    PAD                             : 8;  ///byte 2
    PAD                             : 6;  ///byte 3 bits 0-5
    fbe_u8_t rqst_fail              : 1;  /// byte 3-bit 6
    PAD                             : 1;  ///< byte 3 bits 6
} ses_ctrl_elem_sas_conn_struct;     
 
SIZE_CHECK(ses_ctrl_elem_exp_phy_struct, 4);
/*SAS expander status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct        cmn_stat;  ///< byte 0, common status byte
    fbe_u8_t                        : 6;  ///< byte 1, bit 0-5,
    fbe_u8_t fail                   : 1;  ///< byte 1, bit 6,
    fbe_u8_t ident                  : 1;  ///< byte 1, bit 7, 
    fbe_u16_t reserved;                   ///< byte 2-3
} ses_stat_elem_sas_exp_struct;

SIZE_CHECK(ses_stat_elem_sas_exp_struct, 4);

/*SPS/UPS status element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct         cmn_stat;        ///< byte 0, common status byte
    fbe_u8_t                    battery_status;  ///< byte 1, Battery Status
    fbe_u8_t intf_fail          : 1;  ///< byte 2, bit 0,
    fbe_u8_t warn               : 1;  ///< byte 2, bit 1,
    fbe_u8_t ups_fail           : 1;  ///< byte 2, bit 2,
    fbe_u8_t dc_fail            : 1;  ///< byte 2, bit 3,
    fbe_u8_t ac_fail            : 1;  ///< byte 2, bit 4,
    fbe_u8_t ac_qual            : 1;  ///< byte 2, bit 5,
    fbe_u8_t ac_hi              : 1;  ///< byte 2, bit 6,
    fbe_u8_t ac_lo              : 1;  ///< byte 2, bit 7,
    fbe_u8_t bpf                : 1;  ///< byte 3, bit 0, 
    fbe_u8_t batt_fail          : 1;  ///< byte 3, bit 1, 
    fbe_u8_t reserved           : 4;  ///< byte 3, bits 2-5
    fbe_u8_t fail               : 1;  ///< byte 3, bit 6, 
    fbe_u8_t ident              : 1;  ///< byte 3, bit 7, 
} ses_stat_elem_sas_sps_struct;

SIZE_CHECK(ses_stat_elem_sas_sps_struct, 4);

/*Enclosure element in the \ref ses_pg_encl_stat_struct "Enclosure Status" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct        cmn_stat;  ///< byte 0, common status byte
    fbe_u8_t                        : 7;  ///< byte 1, bit 0-6,
    fbe_u8_t ident                  : 1;  ///< byte 1, bit 7, 
    fbe_u8_t warning_indicaton      : 1;  ///< byte 2, bit 0, 
    fbe_u8_t failure_indicaton      : 1;  ///< byte 2, bit 1, 
    fbe_u8_t time_until_power_cycle : 6;  ///< byte 2, bit 2-7, 
    fbe_u8_t warning_rqsted         : 1;  ///< byte 3, bit 1, 
    fbe_u8_t failure_rqsted         : 1;  ///< byte 3, bit 1, 
    fbe_u8_t rqsted_power_off_duration : 6;  ///< byte 3, bit 2-7,
} ses_stat_elem_encl_struct;

SIZE_CHECK(ses_stat_elem_encl_struct, 4);


/*Enclosure element in the \ref ses_pg_encl_ctrl_struct "Enclosure Control" diagnostic page. */
typedef struct {
    ses_cmn_ctrl_struct        cmn_ctrl; ///< byte 0, common status byte
    PAD                             : 7; ///< byte 1, bit 0-6
    fbe_u8_t rqst_ident             : 1; ///< byte 1, bit 7,  activate identify pattern
    fbe_u8_t power_cycle_delay      : 6; ///< byte 2, bit 0-5,
    fbe_u8_t power_cycle_rqst       : 2; ///< byte 2, bit 6-7,
    fbe_u8_t rqst_warning           : 1; ///< byte 3, bit 0, 
    fbe_u8_t rqst_failure           : 1; ///< byte 3, bit 1, 
    fbe_u8_t power_off_duration     : 6; ///< byte 3, bit 2-7,
} ses_ctrl_elem_encl_struct;

SIZE_CHECK(ses_ctrl_elem_encl_struct, 4);


/*Display element in the \ref ses_pg_encl_ctrl_struct "Display Control" diagnostic page. */
typedef struct {
    ses_cmn_stat_struct        cmn_stat; ///< byte 0, common status byte
    fbe_u8_t display_mode_status    : 2; ///< byte 1, bit 0-1
    PAD                             : 4; ///< byte 1, bit 2-5
    fbe_u8_t fail                   : 1; ///< byte 1, bit 6,
    fbe_u8_t ident                  : 1; ///< byte 1, bit 7,  activate identify pattern
    fbe_u8_t display_char_stat;          ///< byte 2
    PAD                             : 8; ///< byte 3
} ses_stat_elem_display_struct;

SIZE_CHECK(ses_stat_elem_display_struct, 4);

typedef struct {
    ses_cmn_ctrl_struct        cmn_ctrl; ///< byte 0, common status byte
    fbe_u8_t display_mode           : 2; ///< byte 1, bit 0-1
    PAD                             : 4; ///< byte 1, bit 2-5
    fbe_u8_t rqst_fail              : 1; ///< byte 1, bit 6,
    fbe_u8_t rqst_ident             : 1; ///< byte 1, bit 7,  activate identify pattern
    fbe_u8_t display_char;               ///< byte 2
    PAD                             : 8; ///< byte 3
} ses_ctrl_elem_display_struct;

SIZE_CHECK(ses_ctrl_elem_display_struct, 4);

#define SES_DISPLAY_MODE_DISPLAY_CHAR   2

#define SES_SPS_IN_BUF_HEADER_LEN       6

typedef struct
{
    fbe_u16_t   version;                    // current version is 0
    fbe_u16_t   cmd_token;                  // token provided by writer
    fbe_u8_t    reinit_connection   :1;     // request SPS reset
    fbe_u8_t    raw                 :1;     // process passthru without adding \n
    fbe_u8_t    pad                 :6;
    fbe_u8_t    command_len;                // length of command string
    fbe_char_t  command;                    // the command (size based on command_len)
} dev_sps_in_buffer_struct_t;

SIZE_CHECK(dev_sps_in_buffer_struct_t, 7);


typedef struct
{
    fbe_u16_t   version;                    // current version is 0
    fbe_u16_t   cmd_frame_cnt;              // counter of frames processed
    fbe_u16_t   cond;                       // the last COND? read
    fbe_u16_t   battime;                    // the last BATTIME read
    fbe_u16_t   cmd_token;                  // token provided by writer
    fbe_u8_t    cond_valid          :1;     // cond vaid?
    fbe_u8_t    battime_valid       :1;     // battime valid?
    fbe_u8_t    response_valid      :1;     // response valid?
    fbe_u8_t    cmd_frame_valid     :1;     // command formatting
    fbe_u8_t    reinit_inprogress   :1;     // SPS reinit/reset in process
    fbe_u8_t    cmd_inprocess       :1;     // command in process
    fbe_u8_t    pad                 :2;
    fbe_u8_t    response_len;               // lenth of response
    fbe_char_t  response;                   // the response (size based on response_len) -- null terminated
} dev_sps_out_buffer_struct_t;

SIZE_CHECK(dev_sps_out_buffer_struct_t, 13);


/*
 * SPS EEPROM info 
 * There are different layout with different Expander FW revision 
 */
// Initial revision (Exp FW 1.41)
typedef struct 
{
   fbe_u32_t    major;
   fbe_u32_t    minor;
} dev_sps_fw_rev ;

typedef struct 
{
    fbe_u8_t        sps_type;                       ///< type : dev_sps_type_enum
    fbe_u8_t        supplier_name[5];               ///< vendor name from MFG?
    fbe_u8_t        supplier_model_number[11];      ///< vendor number from MFG?
    fbe_u8_t        pad[3];                         ///
    dev_sps_fw_rev  firmware_rev;                   ///< Primary MCU FW rev from MFG?
    dev_sps_fw_rev  firmware_rev_sec;               ///< Secondary MCU FW rev from MFG?
    fbe_u8_t        firmware_date[10];              ///< FW date from MFG?
    fbe_u8_t        partnum[11];                    ///< SPS part # date from PN?
    fbe_u8_t        revision[3];                    ///< SPS rev date from REV?
    fbe_u8_t        serialnum[16];                  ///< SPS ser # from SN?
    fbe_u8_t        prod_id[16];                    ///< Vendor product ID.  For EMC, defined in ESES.
    fbe_u8_t        batt_model_number[9];           ///< Battery model # from B_MFG?
    fbe_u8_t        pad1[3];                        ///
    dev_sps_fw_rev  batt_firmware_rev;              ///< Battery FW rev # from B_MFG?
    fbe_u32_t       ffid;                           ///< SPS FFID from FFID?
    fbe_u32_t       batt_ffid;                      ///< Battery Pack FFID from B_FFID?
} dev_sps_rev_info_struct;

SIZE_CHECK(dev_sps_rev_info_struct, 120);

// Revision 0 (Exp FW 1.42)
typedef struct  
{
    fbe_u32_t   pmbus_dev_version;              ///< pmbus extension dev format -- 4
    fbe_u8_t    tbd;                            ///< PMBUS content is under review
} dev_sps_pmbus_struct;

typedef struct  
{
    fbe_u32_t   sps_dev_version;                ///< sps extension dev format -- 4
    fbe_u32_t   batt_ffid;                      ///< Battery Pack FFID from B_FFID? -- 4
    fbe_u8_t    primary_firmware_rev[5];        ///< Primary MCU FW rev from MFG? -- 5
    fbe_u8_t    secondary_firmware_rev[5];      ///< Secondary MCU FW rev from MFG? -- 5
    fbe_u8_t    firmware_date[10];              ///< FW date from MFG? -- 10
    fbe_u8_t    sps_celldate[10];               ///< cell date fromo CD? -- 10
    fbe_u8_t    batt_emc_serialnum[16];         ///< SPS ser # from B_SN? -- 16
    fbe_u8_t    batt_emc_revision[3];           ///< Battery rev  from B_REV? -- 3
    fbe_u8_t    batt_model_number[9];           ///< Battery model # from B_MFG? -- 9
    fbe_u8_t    batt_emc_partnum[16];           ///< Battery part # from B_PN? -- 16
    fbe_u8_t    batt_mfg_name[32];              ///< vendor name from B_MFG? -- 32
    fbe_u8_t    batt_firmware_rev[5];           ///< Battery FW rev # from B_MFG? -- 5
    fbe_u8_t    batt_firmware_date[10];         ///< FW date from B_MFG? -- 10
    fbe_u8_t    sps_type;                       ///< type : dev_sps_type_enum - 1
} dev_sps_sps_struct;

typedef union  
{
    dev_sps_pmbus_struct    pmbus;
    dev_sps_sps_struct      sps;
} dev_sps_per_dev_union;

typedef struct 
{
    fbe_u32_t   version;                    ///< virtual eeprom dev format -- 4
    fbe_u32_t   ffid;                       ///< SPS FFID from FFID? -- 4
    fbe_u8_t    emc_serialnum[16];          ///< SPS ser # from SN?   -- 16
    fbe_u8_t    emc_partnum[16];            ///< SPS part #  from PN? -- 16
    fbe_u8_t    emc_revision[3];            ///< SPS rev  from REV?   -- 3
    fbe_u8_t    mfg_date[10];               ///<  -- 10
    fbe_u8_t    mfg_revision[3];            ///<  -- 3
    fbe_u8_t    mfg_prod_id[32];            ///< Vendor product ID.  For EMC, defined in ESES. -- 32
    fbe_u8_t    mfg_model_number[16];       ///< vendor model number from MFG? -- 16
    fbe_u8_t    mfg_location[32];           ///< where manufactured -- 32
    fbe_u8_t    mfg_serialnum[32];          ///< -- 32
    fbe_u8_t    mfg_name[32];               ///< vendor name from MFG? -- 32
    dev_sps_per_dev_union   per_dev;
} dev_sps_rev0_info_struct;

SIZE_CHECK(dev_sps_rev0_info_struct, 330);

typedef union
{
    dev_sps_rev_info_struct     spsEeprom;
    dev_sps_rev0_info_struct    spsEepromRev0;
} sps_eeprom_info_struct;


/* 
 * EEPROM Common Area, Version Zero
 */

typedef struct 
{
    fbe_u32_t   version;                     
    fbe_u32_t   ffid;                        
    fbe_u8_t    emc_serialnum[16];           
    fbe_u8_t    emc_partnum[16];             
    fbe_u8_t    emc_assembly_revision[3];             
    fbe_u8_t    mfg_date[10];                
    fbe_u8_t    vendor_revision[3];          
    fbe_u8_t    vendor_part_number[32];      
    fbe_u8_t    vendor_model_number[16];     
    fbe_u8_t    mfg_location[32];           
    fbe_u8_t    vendor_serialnum[32];        
    fbe_u8_t    vendor_name[32];
} dev_eeprom_rev0_info_struct;

SIZE_CHECK(dev_eeprom_rev0_info_struct, 200);


///\todo declare the rest of the elements

/* 
 * Get the pointer to the control/status element in the control/status page.
 * d - the pointer to the group overall control/status element.
 * elem - the element number in the group. (starting from 1)
 */
static __inline fbe_u8_t *fbe_eses_get_ctrl_stat_elem_p(fbe_u8_t *d, fbe_u8_t elem){
    return (fbe_u8_t *)((fbe_u8_t *)d + elem * FBE_ESES_CTRL_STAT_ELEM_SIZE);
}


/********************************************************************
 ********************************************************************
 ***                                                              ***
 ***              INLINE FUNCTION DEFINITIONS                     ***
 ***                                                              ***
 *** These functions help to get pointers to various fields in    ***
 *** SES pages that aren't at fixed offsets.                      *** 
 ***                                                              ***
 ********************************************************************
 ********************************************************************/

/****************************************************************************/
/** Return a pointer to the number of \ref ses_buf_desc_struct "buffer descriptors" field
 *  in the specified \ref ses_subencl_desc_struct "subenclosure descriptor" in the
 *  \ref ses_pg_config_struct "Configuration diagnostic page".
 *
 * Use \ref fbe_eses_get_ses_buf_desc_p() "fbe_eses_get_ses_buf_desc_p(\p d)" to get a pointer to the array of descriptors.
 *
 * The subenclosure descriptor at \p d need not be within a SES page.  The
 * <tt>d-\></tt>\ref ses_subencl_desc_struct.num_ver_descs "num_ver_descs" field
 * in the descriptor must be filled in.
 ****************************************************************************/

static __inline fbe_u8_t *fbe_eses_get_num_buf_descs_p(ses_subencl_desc_struct *d) {
    return (fbe_u8_t *)(((fbe_u8_t *) d) + offsetof(ses_subencl_desc_struct, ver_desc) +
                d->num_ver_descs * sizeof(ses_ver_desc_struct));
}

/****************************************************************************/
/** Return a pointer to the \ref ses_buf_desc_struct "buffer descriptor" array
 *  in the specified \ref ses_subencl_desc_struct "subenclosure descriptor" in the
 *  \ref ses_pg_config_struct "Configuration diagnostic page".
 *
 * Use \ref fbe_eses_get_ses_num_buf_descs_p() "fbe_eses_get_ses_num_buf_descs_p(\p d)"
 * to get the number of descriptors in the array.
 *
 * The subenclosure descriptor at \p d need not be within a SES page.  The
 * <tt>d-\></tt>\ref ses_subencl_desc_struct.num_ver_descs "num_ver_descs" field
 * in the descriptor must be filled in.
 ****************************************************************************/

static __inline ses_buf_desc_struct *fbe_eses_get_first_buf_desc_p(ses_subencl_desc_struct *d) {
    return (ses_buf_desc_struct *) (fbe_eses_get_num_buf_descs_p(d) + 1);
}

/****************************************************************************/
/** Return a pointer to the number of VPD pages field in the VPD page list
 *  in the specified \ref ses_subencl_desc_struct "subenclosure descriptor" in the
 *  \ref ses_pg_config_struct "Configuration diagnostic page".
 *
 * Use fbe_eses_get_ses_vpd_pg_list_p(\p d) to get the list.
 *
 * The subenclosure descriptor at \p d need not be within a SES page.  The
 * <tt>d-\></tt>\ref ses_subencl_desc_struct.num_ver_descs "num_ver_descs" field and
 * the number of buffer descriptors field returned by \ref fbe_eses_get_ses_num_buf_descs_p()
 * "fbe_eses_get_ses_num_buf_descs_p(\p d)" in the descriptor must be filled in.
 ****************************************************************************/

static __inline fbe_u8_t *fbe_eses_get_ses_num_vpd_pgs_p(ses_subencl_desc_struct *d) {
    return (fbe_u8_t *)(((char *) fbe_eses_get_first_buf_desc_p(d)) +
            sizeof(ses_buf_desc_struct) * *fbe_eses_get_num_buf_descs_p(d));
}

/****************************************************************************/
/** Return a pointer to the VPD page list 
 *  in the specified \ref ses_subencl_desc_struct "subenclosure descriptor" in the
 *  \ref ses_pg_config_struct "Configuration diagnostic page".
 *
 * The VPD page list has no structure defined for it -- it's an array of bytes
 * containing VPD page codes.  Use
 * \ref fbe_eses_get_ses_num_vpd_pgs_p() "fbe_eses_get_ses_num_vpd_pgs_p(\p d)" to get its length.
 *
 * The subenclosure descriptor at \p d need not be within a SES page.  The
 * <tt>d-></tt>\ref ses_subencl_desc_struct.num_ver_descs "num_ver_descs" field and
 * the number of buffer descriptors field returned by \ref fbe_eses_get_ses_num_buf_descs_p()
 * "fbe_eses_get_ses_num_buf_descs_p(\p d)" in the descriptor must be filled in.
 ****************************************************************************/

static __inline fbe_u8_t *fbe_eses_get_ses_vpd_pg_list_p(ses_subencl_desc_struct *d) {
    return (fbe_u8_t *)(fbe_eses_get_ses_num_vpd_pgs_p(d) + 1);
}

/****************************************************************************/
/** Return a pointer to the subenclosure text length field
 *  in the specified \ref ses_subencl_desc_struct "subenclosure descriptor" in the
 *  \ref ses_pg_config_struct "Configuration diagnostic page".
 *
 *  Use \ref fbe_eses_get_ses_subencl_text_p() "fbe_eses_get_ses_subencl_test_p(\p d)" to get the text.
 *
 * The subenclosure descriptor at \p d need not be within a SES page.  The
 * <tt>d-></tt>\ref ses_subencl_desc_struct.num_ver_descs "num_ver_descs" field,
 * the number of buffer descriptors field returned by \ref fbe_eses_get_ses_num_buf_descs_p()
 * "fbe_eses_get_ses_num_buf_descs_p(\p d)", and the number of VPD pages returned by
 * \ref fbe_eses_get_ses_num_vpd_pgs_p() "fbe_eses_get_ses_num_vpd_pgs_p(\p d)" in the
 * descriptor must be filled in.
 ****************************************************************************/

static __inline fbe_u8_t *fbe_eses_get_ses_subencl_text_len_p(ses_subencl_desc_struct *d) {
    return fbe_eses_get_ses_vpd_pg_list_p(d) + *fbe_eses_get_ses_num_vpd_pgs_p(d);
}

/****************************************************************************/
/** Return a pointer to the subenclosure text field
 *  in the specified \ref ses_subencl_desc_struct "subenclosure descriptor" in the
 *  \ref ses_pg_config_struct "Configuration diagnostic page".
 *
 * The subenclosure text field is a string of bytes.  Use
 * \ref fbe_eses_get_ses_subencl_text_len_p() "fbe_eses_get_ses_subencl_text_len_p(\p d)" to get its length.
 *
 * The subenclosure descriptor at \p d need not be within a SES page.  The
 * <tt>d-></tt>\ref ses_subencl_desc_struct.num_ver_descs "num_ver_descs" field,
 * the number of buffer descriptors field returned by \ref fbe_eses_get_ses_num_buf_descs_p()
 * "fbe_eses_get_ses_num_buf_descs_p(\p d)", and the number of VPD pages returned by
 * \ref fbe_eses_get_ses_num_vpd_pgs_p() "fbe_eses_get_ses_num_vpd_pgs_p(\p d)" in the
 * descriptor must be filled in.
 ****************************************************************************/

static __inline CHAR *fbe_eses_get_ses_subencl_text_p(ses_subencl_desc_struct *d) {
    return (CHAR *)(fbe_eses_get_ses_subencl_text_len_p(d) + 1);
}

/****************************************************************************/
/** Return a pointer to the next
 *  \ref ses_subencl_desc_struct "subenclosure descriptor" in the
 *  \ref ses_pg_config_struct "Configuration diagnostic page" following the
 *  specified one at \p d.  If this is the last subenclosure descriptor in the page,
 *  the returned value points to the type descriptor header list, an array of
 *  \ref ses_type_desc_hdr_struct.  It is the caller's responsibility to
 *  count the number of subenclosure descriptors processed to determine whether the
 *  return value should be cast to a \ref ses_type_desc_hdr_struct.
 *
 * This function requires that the subenclosure descriptor at \p d be located
 * in a subenclosure descriptor list in a SES page, and that the
 * <tt>d-></tt>\ref ses_subencl_desc_struct.subencl_desc_len "subencl_desc_len" field be filled in.
 ****************************************************************************/

static __inline ses_subencl_desc_struct *fbe_eses_get_next_ses_subencl_desc_p(ses_subencl_desc_struct *d) {
    return (ses_subencl_desc_struct *)(((fbe_u8_t *) d) + d->subencl_desc_len + 4);
}

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 *******                                                              *******
 *******                   SES PAGE DEFINITIONS                       *******
 *******                                                              *******
 ******* Identifiers taken directly from ESES 1.0 specification.      *******
 ******* See note at top of file regarding abbreviations.             *******
 *******                                                              *******   
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************/

/****************************************************************************
 ****************************************************************************//**
 ** Supported Diagnostics Pages diagnostic page (00h).
 **
 ** This is a status page that returns a list of supported page codes.
 ****************************************************************************
 ****************************************************************************/
   
typedef struct {
    fbe_u8_t  pg_code : 8; ///< Byte 0,   \ref SES_PG_CODE_SUPPORTED_DIAGS_PGS, \ref ses_pg_code_enum
    PAD            : 8; //   Byte 1
    fbe_u16_t pg_len;   ///< Byte 2-3, length of rest of page
    fbe_u8_t  pg_code_list[1]; ///< Byte 4,   first byte of \ref pg_len bytes of
                                       ///<           \ref ses_pg_code_enum "ses_pg_code_enum"s.
} ses_pg_supported_diags_pgs_struct;

SIZE_CHECK(ses_pg_supported_diags_pgs_struct, 5);

/****************************************************************************
 ****************************************************************************//**
 ** Configuration diagnostic page (01h).
 **
 ** This is a status page that returns the configuration of the enclosure,
 ** including a list of subenclosures and the types and numbers of elements within them.
 **
 ** This structure defines only the first part of the page up to the start of the first \ref
 ** ses_subencl_desc_struct "subenclosure descriptor". To get to each subsequent subenclosure
 ** descriptor, use \ref fbe_eses_get_next_ses_subencl_desc_p(). After all the subenclosure descriptors,
 ** the following appear:
 **
 ** <pre>
 **    ses_type_desc_hdr_struct type_desc_hdr[num_hdrs]; // Type descriptor headers
 **    CHAR                    type_desc_text[];        // Type descriptor texts
 ** </pre>
 **
 ** The value of <code>num_hdrs</code> can be determined by adding up the values of the \ref
 ** ses_subencl_desc_struct.num_elem_type_desc_hdrs "num_elem_type_desc_hdrs" in all the \ref
 ** ses_subencl_desc_struct "subenclosure descriptors".
 **
 ** The <tt>type_desc_text</tt> field is a series of <tt>num_hdrs</tt> variable-length ASCII
 ** strings, one for each <tt>type_desc_hdr</tt>. The length of the <tt>i</tt>th string is in
 ** <tt>type_desc_hdr[i].\ref ses_type_desc_hdr_struct.type_desc_text_len "type_desc_text_len"</tt>.
 ***************************************************************************
 ****************************************************************************/

typedef struct {
    fbe_u8_t  pg_code : 8;            ///< Byte 0    \ref SES_PG_CODE_CONFIG, \ref ses_pg_code_enum
    fbe_u8_t  num_secondary_subencls; ///< Byte 1,   number does not include primary
    fbe_u16_t pg_len;              ///< Byte 2-3, length of rest of page
    fbe_u32_t  gen_code;            ///< Byte 4-7, generation code bumped on any change in this page
    ///                                                  Byte 8+,  subenclosure descriptors.
    /// There is always one descriptor for the primary subenclosure (always an LCC)
    /// plus one for each secondary subenclosure (\ref num_secondary_subencls).  They
    /// are variable length, so you have to go through them in order to get to the next
    /// one, using \ref fbe_eses_get_next_ses_subencl_desc_p().
    ses_subencl_desc_struct subencl_desc;
    
    // Following this:
    //    ses_type_desc_hdr_struct type_desc_hdr[num_hdrs]; // Type descriptor headers
    //   CHAR                    type_desc_text[];        // Type descriptor texts
    // See comments above.
} ses_pg_config_struct;

SIZE_CHECK(ses_pg_config_struct, 8 + sizeof(ses_subencl_desc_struct));

/****************************************************************************
 ****************************************************************************//**
 ** Enclosure Status diagnostic page (02h).
 **
 ** This is a status page that reports overall health of the enclosure and
 ** contains a 4-byte status element for each element within
 ** each subenclosure defined in the
 ** \ref ses_pg_config_struct "Configuration diagnostic page".
 ****************************************************************************
 ****************************************************************************/

typedef struct {
    fbe_u8_t pg_code      : 8; ///< Byte 0            \ref SES_PG_CODE_ENCL_STAT, \ref ses_pg_code_enum
    fbe_u8_t unrecov      : 1; ///< Byte 1, bit 0, unrecoverable condition active; see \ref ses_elem_stat_code_enum
    fbe_u8_t crit         : 1; ///< Byte 1, bit 1, critical condition active; see \ref ses_elem_stat_code_enum
    fbe_u8_t non_crit     : 1; ///< Byte 1, bit 2, non-critical condition active; see \ref ses_elem_stat_code_enum
    fbe_u8_t info         : 1; ///< Byte 1, bit 3,
                            ///< set once on most changes to this page per I_T nexus
    fbe_u8_t invop        : 1; ///< Byte 1, bit 4,    always 0 in ESES
    PAD                : 3; //   Byte 1, bits 7-5
    fbe_u16_t pg_len;        ///< Byte 2-3,length of rest of page
    fbe_u32_t gen_code;      ///< Byte 4-7,generation code; see ses_pg_config_struct.gen_code
    ///                                         Byte 8-11+,    status elements.
    /// For each \ref ses_type_desc_hdr_struct "type descriptor header" in each
    /// \ref ses_subencl_desc_struct "subenclosure descriptor" in the
    /// \ref ses_pg_config_struct "Configuration diagnostic page", 
    /// there is one overall status elements for that type, followed by
    /// one individual status element for each element, as indicated by
    /// \ref ses_type_desc_hdr_struct.num_possible_elems.
    /// The sequence of individual status elements (not including overall
    /// status elements) defines the \ref SES_ELEM_INDEX "element index"
    /// for each element.  Because elements are fixed-length, you can
    /// directly index into the i'th status element by referencing \p stat_elem[i].
    /// but, because of the overall status elements, the index into this array is not
    /// equal to the element's \ref ses_elem_index.
    fbe_u8_t stat_elem[1]; ///// \ref ses_stat_elem_struct    
} ses_pg_encl_stat_struct;

SIZE_CHECK(ses_pg_encl_stat_struct, 9);

/****************************************************************************
 ****************************************************************************//**
 ** Enclosure Control diagnostic page (02h).
 **
 ** This is a control page that contains a 4-byte control element for each element within
 ** each subenclosure defined in the \ref ses_pg_config_struct "Configuration diagnostic page".
 **
 ** The \ref non_crit, \ref crit, and \ref unrecov bits defined here allow the client
 ** to set or reset the specified condition to be reported in the \ref ses_pg_encl_stat_struct
 ** "Enclosure Status" diagnostic page.  However a value of FALSE may be ignored
 ** if the EMA has independently determined that a condition of the specified type exists.
 ****************************************************************************
 ****************************************************************************/

typedef struct {
    fbe_u8_t pg_code        : 8; ///< Byte 0,        \ref SES_PG_CODE_ENCL_CTRL, \ref ses_pg_code_enum
    fbe_u8_t unrecov        : 1; ///< Byte 1, bit 0, set/reset ses_pg_encl_stat_struct.unrecov
    fbe_u8_t crit           : 1; ///< Byte 1, bit 1, set/reset ses_pg_encl_stat_struct.crit
    fbe_u8_t non_crit       : 1; ///< Byte 1, bit 2, set/reset ses_pg_encl_stat_struct.non_crit
    fbe_u8_t info           : 1; ///< Byte 1, bit 3, unused in ESES
    PAD                  : 4; //   Byte 1, bit 4-7
    fbe_u16_t pg_len;          ///< Byte 2-3,      length of rest of page
    fbe_u32_t gen_code;        ///< Byte 4-7,      expected generation code
    ///                                            Byte 8-11+,    control elements
    /// This is an array of elements in exactly the same order as the status elements in the
    /// ses_pg_encl_stat_struct.stat_elem array.  Because elements are fixed-length, you can
    /// directly index into the i'th control element by referencing \p ctrl_elem[i].
    fbe_u8_t ctrl_elem[1];//// \ref ses_ctrl_elem_struct
} ses_pg_encl_ctrl_struct;

SIZE_CHECK(ses_pg_encl_stat_struct, 9);

/********************************
 * Additional Status Page.
 ********************************/
/******************************************************************************
 * Additional element status descriptor in the additional status page.
 ******************************************************************************/
typedef struct ses_addl_elem_stat_hdr_desc_s
{
    fbe_u8_t protocol_id : 4;   //Byte 0, bit 0-3
    fbe_u8_t eip     : 1;       //Byte 0, bit 4
    fbe_u8_t         : 2;       //Byte 0, bit 6,5
    fbe_u8_t invalid : 1;       //Byte 0, bit 7
    fbe_u8_t desc_len;          //Byte 1
    fbe_u8_t reserved;          //Byte 2
    fbe_u8_t elem_index;        //Byte 3
    /* Protocol_specific information */
    
}ses_addl_elem_stat_desc_hdr_struct;

/******************************************************************************
 * Array Device Slot Protocol Specific Info  
 * in the additional element status descriptor of the additional status page.
 ******************************************************************************/
typedef struct ses_array_dev_slot_prot_spec_info_s
{
    fbe_u8_t num_phy_descs;  // Byte 0   
    fbe_u8_t not_all_phys : 1;   // Byte 1, bit 0
    fbe_u8_t           : 5;   // Byte 1, bit 1-5
    fbe_u8_t desc_type : 2;   // Byte 1, bit 6-7
    fbe_u8_t reserved;       // byte 2
    fbe_u8_t dev_slot_num;   // Byte 3
    /* Array Device Phy descriptor */
}ses_array_dev_slot_prot_spec_info_struct;

/******************************************************************************
 * Array Device Phy Descriptor
 * a part of Array Device Slot Protocol Specific Info 
 * in the additional element status descriptor of the additional status page.
 ******************************************************************************/
typedef struct ses_array_dev_phy_desc_s{
    fbe_u8_t ignored[12];    // Byte 0-11, will be defined later.
    fbe_u64_t sas_address;   // Byte 12-19
    fbe_u8_t phy_id;         // Byte 20
    fbe_u8_t reserved[7];    // Byte 21-27
}ses_array_dev_phy_desc_struct;

/******************************************************************************
 * SAS Expander Protocol Specific Info  
 * in the additional element status descriptor of the additional status page.
 ******************************************************************************/
typedef struct ses_sas_exp_prot_spec_info_s
{
    fbe_u8_t num_exp_phy_descs; // Byte 0   
    fbe_u8_t           : 6;     // Byte 1, bit 0-5
    fbe_u8_t desc_type : 2;     // Byte 1, bit 6-7
    fbe_u16_t reserved;         // Byte 2-3
    fbe_u64_t sas_address;      // Byte 4-11
    /* Expander Phy descriptor */
}ses_sas_exp_prot_spec_info_struct;

/******************************************************************************
 * Expander Phy Descriptor in Enclosure service Controller Electronics 
 * Protocol Specific Info  
 * in the additional element status descriptor of the additional status page.
 ******************************************************************************/
typedef struct ses_sas_exp_phy_desc_s{
    fbe_u8_t conn_elem_index;     // Byte 0
    fbe_u8_t other_elem_index;    // Byte 1
}ses_sas_exp_phy_desc_struct;


/******************************************************************************
 * ESC ELectronics Protocol Specific Info  
 * in the additional element status descriptor of the additional status page.
 ******************************************************************************/
typedef struct ses_esc_elec_prot_spec_info_s
{
    fbe_u8_t num_exp_phy_descs;  // Byte 0   
    fbe_u8_t           : 6;     // Byte 1, bit 0-5
    fbe_u8_t desc_type : 2;     // Byte 1, bit 6-7
    fbe_u16_t reserved;         // byte 2-3
    /* Expander Phy descriptor */
}ses_esc_elec_prot_spec_info_struct;

/******************************************************************************
 * Expander Phy Descriptor in ESC ELectronics Protocol Specific Info  
 * in the additional element status descriptor of the additional status page.
 ******************************************************************************/
typedef struct ses_esc_elec_exp_phy_desc_s{
    fbe_u8_t phy_id;    // Byte 0
    fbe_u8_t reserved;   // Byte 1
    fbe_u8_t conn_elem_index;     // Byte 2
    fbe_u8_t other_elem_index;    // Byte 3
    fbe_u64_t sas_address; // Byte 4-11   
}ses_esc_elec_exp_phy_desc_struct;


static __inline ses_addl_elem_stat_desc_hdr_struct *fbe_eses_get_next_addl_elem_stat_desc_p(ses_addl_elem_stat_desc_hdr_struct *d) {
    return (ses_addl_elem_stat_desc_hdr_struct *)(((fbe_u8_t *) d) + d->desc_len + 2);
}

/* Get the additional element status descriptor length. */
static __inline fbe_u16_t fbe_eses_get_addl_elem_stat_desc_len(ses_addl_elem_stat_desc_hdr_struct *d) 
{
    return (fbe_u16_t)(d->desc_len + FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE);
}

/* Get the pointer to the first additional element status descriptor in the additional status page. */
static __inline ses_addl_elem_stat_desc_hdr_struct * fbe_eses_get_first_addl_elem_stat_desc_p(fbe_u8_t *d) 
{
    return (ses_addl_elem_stat_desc_hdr_struct *)((fbe_u8_t *)d + FBE_ESES_PAGE_HEADER_SIZE);
}

/* Get the pointer to the array device slot or esc electronics protocol specific info. */
static __inline fbe_u8_t *fbe_eses_get_prot_spec_info_p(ses_addl_elem_stat_desc_hdr_struct *d)
{
    return (fbe_u8_t *)((fbe_u8_t *)d + FBE_ESES_ADDL_ELEM_STATUS_DESC_HDR_SIZE);
}

/* Get the pointer to the first array device slot phy descriptor. */
static __inline ses_array_dev_phy_desc_struct *fbe_eses_get_first_array_dev_slot_phy_desc_p(ses_array_dev_slot_prot_spec_info_struct *d)
{
    return (ses_array_dev_phy_desc_struct *)((fbe_u8_t *)d + FBE_ESES_ARRAY_DEV_SLOT_PROT_SPEC_INFO_HEADER_SIZE);
}

/* Get the pointer to the first sas expander phy descriptor. */
static __inline ses_sas_exp_phy_desc_struct *fbe_eses_get_first_sas_exp_phy_desc_p(ses_sas_exp_prot_spec_info_struct *d)
{
    return (ses_sas_exp_phy_desc_struct *)((fbe_u8_t *)d + FBE_ESES_SAS_EXP_PROT_SPEC_INFO_HEADER_SIZE);
}

/* Get the pointer to the next sas expander phy descriptor. */
static __inline ses_sas_exp_phy_desc_struct *fbe_eses_get_next_sas_exp_phy_desc_p(ses_sas_exp_phy_desc_struct *d)
{
    return (ses_sas_exp_phy_desc_struct *)((fbe_u8_t *)d + sizeof(ses_sas_exp_phy_desc_struct));
}

/* Get the pointer to the first esc electronic phy descriptor. */
static __inline ses_esc_elec_exp_phy_desc_struct *fbe_eses_get_first_esc_elec_phy_desc_p(ses_esc_elec_prot_spec_info_struct *d)
{
    return (ses_esc_elec_exp_phy_desc_struct *)((fbe_u8_t *)d + FBE_ESES_ESC_ELEC_PROT_SPEC_INFO_HEADER_SIZE);
}


/********************************
 * End of Additional Status Page.
 ********************************/

/****************************************************************************
 ****************************************************************************//**
 ** EMC Enclosure Control/Status diagnostic page (10h).
 **
 ** This is a Control/Status page that contains a number of elements of various sizes that
 ** augment configuration and status information in the standard
 ** \ref ses_pg_encl_stat_struct "Enclosure Status diagnostic page".
 **
 ** This structure defines only the first fixed-length portion of the page.  The
 ** page ends this way:
 **
 ** <pre>
 **     ses_sas_conn_info_elem_struct    \ref conn_info[\ref num_sas_conn_info_elems]
 **     UINT8                           num_trace_buf_info_elems; // number of trace buffer information elements
 **     ses_trace_buf_info_elem_struct info_elems[num_trace_buf_info_elems];
 **     ses_encl_time_elem_struct       encl_time;                 // enclosure time
 **     ses_drive_power_info_struct     drive_power_info;          // drive power info/control
 ** </pre>
 **     
 ****************************************************************************
 ****************************************************************************/

/************************************
 * SAS Connector Information Element 
 ************************************/
typedef struct ses_sas_conn_inf_elem_s{
    fbe_u8_t conn_elem_index;       // Byte 0;
    fbe_u64_t attached_sas_address; // Byte 1-8; Reserved in a control element.
    fbe_u8_t           : 4;         // Byte 9, bit 0-3;
    fbe_u8_t port_use  : 2;         // Byte 9, bit 4-5; 
    fbe_u8_t port_type : 2;         // Byte 9, bit 6-7; Reserved in a control element.
    fbe_u8_t wide_port_id : 4;      // Byte 10, bit 0-3;
    fbe_u8_t recovery     : 1;      // Byte 10, bit 4;  Reserved in a control element.
    fbe_u8_t enable       : 1;      // Byte 10, bit 5;
    fbe_u8_t              : 2;      // Byte 10, bit 6-7;
    fbe_u8_t conn_id;               // Byte 11; Reserved in a control element.
    fbe_u8_t attached_phy_id;       // Byte 12; 
    // In newer versions of CDES (version 0.21) we also have the next field.
    fbe_u8_t attached_sub_encl_id;  // Byte 13; Used in Voyager but not in Viper/Derringer
} ses_sas_conn_info_elem_struct;


SIZE_CHECK(ses_sas_conn_info_elem_struct, 14);


/************************************
 * SPS/UPS Information Element 
 ************************************/
typedef struct ses_sps_inf_elem_s{
    fbe_u8_t sps_elem_index;                    ///< byte 0;
    fbe_u8_t reserved_0         : 6;            ///< byte 1, bits 0-5 reserved
    fbe_u8_t sps_battime_valid  : 1;            ///< byte 1, bit 6, SPS Status valid
    fbe_u8_t sps_status_valid   : 1;            ///< byte 1, bit 7, SPS Batttime valid
    fbe_u16_t sps_status;                       ///< bytes 2-3, SPS Status
    fbe_u16_t sps_battime;                      ///< bytes 4-5, SPS Battime
} ses_sps_info_elem_struct;


SIZE_CHECK(ses_sps_info_elem_struct, 6);

/************************************
 * Trace Buffer Information Element 
 ************************************/
typedef struct ses_trace_buf_info_elem_s
{
    fbe_u8_t buf_id;      // Byte 0
    fbe_u8_t buf_action;  // Byte 1
    fbe_u8_t elem_index;   // Byte 2
    fbe_u8_t rev_level[5];    // Byte 3-7, used in a status element only and reserved in a control element
    fbe_u8_t timestamp[21];    // Byte 8-28, used in a status element only and reserved in a control element
}ses_trace_buf_info_elem_struct;

SIZE_CHECK(ses_trace_buf_info_elem_struct, 29);

/************************************
 * Enclosure Time Element 
 ************************************/
typedef struct ses_encl_time_elem_s
{
    fbe_u8_t year  : 7;  // Byte 0, bit 0-6
    fbe_u8_t valid : 1;  // Byte 0, bit 7
    fbe_u8_t month;      // Byte 1
    fbe_u8_t day;        // Byte 2
    fbe_u8_t time_zone;   // Byte 3
    fbe_u32_t milliseconds; // Byte 4-7    
}ses_encl_time_elem_struct;

SIZE_CHECK(ses_encl_time_elem_struct, 8);

/************************************
 * General Information Element Array device slot
 ************************************/
typedef struct ses_general_info_elem_array_dev_slot_s
{
    fbe_u8_t elem_index;        // Byte 0
    fbe_u8_t reserved1 : 6;     // Byte 1, bit 0-5
    fbe_u8_t battery_backed : 1;     // Byte 1, bit 6
    fbe_u8_t fru : 1;           // Byte 1, bit 7, reserved in a control element.
    fbe_u8_t duration : 7;      // Byte 2, bit 0-6
    fbe_u8_t power_cycle : 1;   // Byte 2, bit 7
    fbe_u8_t reserved2;         // Byte 3
}ses_general_info_elem_array_dev_slot_struct;

SIZE_CHECK(ses_general_info_elem_array_dev_slot_struct, 4);

/************************************
 * General Information Element Expander
 ************************************/
typedef struct ses_general_info_elem_expander_s
{
    fbe_u8_t elem_index;        // Byte 0
    fbe_u8_t reset_reason : 4;  // Byte 1, bit 0-3
    fbe_u8_t reserved1 : 3;     // Byte 1, bit 4-6
    fbe_u8_t fru : 1;           // Byte 1, bit 7, reserved in a control element.
    fbe_u8_t reserved2;         // Byte 2
    fbe_u8_t reserved3;         // Byte 3
}ses_general_info_elem_expander_struct;

SIZE_CHECK(ses_general_info_elem_expander_struct, 4);


/********************************************
 * General Information Element common fields
 ********************************************/
typedef struct ses_general_info_elem_cmn_s
{
    fbe_u8_t elem_index;        // Byte 0
    fbe_u8_t reserved1 : 7;     // Byte 1, bit 0-6
    fbe_u8_t fru : 1;           // Byte 1, bit 7, reserved in a control element.
    fbe_u8_t reserved2;         // Byte 2
    fbe_u8_t reserved3;         // Byte 3
}ses_general_info_elem_cmn_struct;

SIZE_CHECK(ses_general_info_elem_cmn_struct, 4);


/********************************************
 * General Information Element for 
 * Temperature Sensor
 ********************************************/
typedef struct ses_general_info_elem_temp_sensor_s
{
    fbe_u8_t  elem_index;        // Byte 0
    fbe_u8_t  valid : 1;         // Byte 1, bit 0
    fbe_u8_t  reserved1 : 6;     // Byte 1, bit 1-6
    fbe_u8_t  fru : 1;           // Byte 1, bit 7, reserved in a control element.
    fbe_u16_t temp;              // Bytes 2-3 
}ses_general_info_elem_temp_sensor_struct;

SIZE_CHECK(ses_general_info_elem_temp_sensor_struct, 4);


/************************************
 * Power Supply Information Element
 ************************************/
typedef struct ses_ps_info_elem_s
{
    fbe_u8_t    ps_elem_index;           // Byte 0
    fbe_u8_t    input_power_valid : 1;   // Byte 0, bit 0, reserved in control element
    fbe_u8_t    low_power_mode_b : 1;    // Byte 0, bit 1, reserved in control element
    fbe_u8_t    low_power_mode_a : 1;    // Byte 0, bit 2, reserved in control element
    fbe_u8_t    margining_test_mode : 4; // Byte 0, bit 3-6, reserved in control element
    fbe_u8_t    reserved : 1;            // Byte 1, bit 7
    fbe_u8_t    margining_test_results;  // Byte 2,
    fbe_u16_t   input_power;             // Byte 3-4.
}ses_ps_info_elem_struct;

SIZE_CHECK(ses_ps_info_elem_struct, 5);

/********************************************
 * General Information Element for 
 * ESC Electronics
 ********************************************/
typedef struct ses_general_info_elem_esc_electronics_s
{
    fbe_u8_t  elem_index;        // Byte 0
    fbe_u8_t  ecb_fault : 1;     // Byte 1, bit 0, reserved in a control element.
    fbe_u8_t  reserved1 : 6;     // Byte 1, bit 1-6
    fbe_u8_t  fru : 1;           // Byte 1, bit 7, reserved in a control element.
    fbe_u8_t  reserved2;         // Byte 2
    fbe_u8_t  reserved3;         // Byte 3 
}ses_general_info_elem_esc_electronics_struct;

/************************************
 * Information Element Group Header 
 ************************************/
typedef struct fbe_eses_info_elem_group_hdr_s
{
    fbe_u8_t info_elem_type;  // Byte 0
    fbe_u8_t num_info_elems;  // Byte 1
    fbe_u8_t info_elem_size;  // Byte 2     
}fbe_eses_info_elem_group_hdr_t;

SIZE_CHECK(fbe_eses_info_elem_group_hdr_t, 3);

/************************************************
 * EMC Enclosure Control/Status Diagnostic Page.
 ************************************************/
typedef struct ses_pg_emc_encl_stat_s 
{    
    ses_common_pg_hdr_struct hdr;  // Byte 0-7, the common page header;
    fbe_u8_t reserved[2];          // Byte 8-9;
    fbe_u8_t shutdown_reason : 7;  // Byte 10, bit 0-6, reserved in a control page;
    fbe_u8_t partial         : 1;  // Byte 10, bit 7, reserved in a control page;
    fbe_u8_t num_info_elem_groups; // Byte 11, num of # information element groups;
    // information element groups
} ses_pg_emc_encl_stat_struct;

typedef ses_pg_emc_encl_stat_struct ses_pg_emc_encl_ctrl_struct; 

static __inline fbe_eses_info_elem_group_hdr_t *fbe_eses_get_first_info_elem_group_hdr_p(ses_pg_emc_encl_stat_struct *d) {
    return (fbe_eses_info_elem_group_hdr_t *)((fbe_u8_t *)d + FBE_ESES_EMC_CTRL_STAT_FIRST_INFO_ELEM_GROUP_OFFSET);
}

static __inline fbe_eses_info_elem_group_hdr_t *fbe_eses_get_next_info_elem_group_hdr_p(fbe_eses_info_elem_group_hdr_t *d) {
    return (fbe_eses_info_elem_group_hdr_t *)((fbe_u8_t *)d + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE + 
                                              (d->num_info_elems) * (d->info_elem_size));
}

static __inline ses_sas_conn_info_elem_struct *fbe_eses_get_first_sas_conn_info_elem_p(fbe_eses_info_elem_group_hdr_t *d) {
    return (ses_sas_conn_info_elem_struct *)((fbe_u8_t *)d + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE);
}
static __inline ses_sas_conn_info_elem_struct *fbe_eses_get_next_sas_conn_info_elem_p(ses_sas_conn_info_elem_struct *d,
                                                                              fbe_u8_t elem_size) {
    return (ses_sas_conn_info_elem_struct *)((fbe_u8_t *)d + elem_size);
}

static __inline ses_trace_buf_info_elem_struct *fbe_eses_get_first_trace_buf_info_elem_p(fbe_eses_info_elem_group_hdr_t *d) {
    return (ses_trace_buf_info_elem_struct *)((fbe_u8_t *)d + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE);
}

static __inline ses_trace_buf_info_elem_struct *fbe_eses_get_next_trace_buf_info_elem_p(ses_trace_buf_info_elem_struct *d,
                                                                                fbe_u8_t elem_size) {
    return (ses_trace_buf_info_elem_struct *)((fbe_u8_t *)d + elem_size);
} 

static __inline ses_ps_info_elem_struct *fbe_eses_get_first_ps_info_elem_p(fbe_eses_info_elem_group_hdr_t *d) {
    return (ses_ps_info_elem_struct *)((fbe_u8_t *)d + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE);
}

static __inline ses_sps_info_elem_struct *fbe_eses_get_first_sps_info_elem_p(fbe_eses_info_elem_group_hdr_t *d) {
    return (ses_sps_info_elem_struct *)((fbe_u8_t *)d + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE);
}

static __inline ses_ps_info_elem_struct *fbe_eses_get_next_ps_info_elem_p(ses_ps_info_elem_struct *d) {
    return (ses_ps_info_elem_struct *)((fbe_u8_t *)d + FBE_ESES_EMC_CTRL_STAT_PS_INFO_ELEM_SIZE);
} 

static __inline ses_sps_info_elem_struct *fbe_eses_get_next_sps_info_elem_p(ses_sps_info_elem_struct *d) {
    return (ses_sps_info_elem_struct *)((fbe_u8_t *)d + FBE_ESES_EMC_CTRL_STAT_SPS_INFO_ELEM_SIZE);
} 

static __inline ses_general_info_elem_cmn_struct *fbe_eses_get_first_general_info_elem_p(fbe_eses_info_elem_group_hdr_t *d) {
    return (ses_general_info_elem_cmn_struct *)((fbe_u8_t *)d + FBE_ESES_EMC_CTRL_STAT_INFO_ELEM_GROUP_HEADER_SIZE);
}

static __inline ses_general_info_elem_cmn_struct *fbe_eses_get_next_general_info_elem_p(ses_general_info_elem_cmn_struct *d) {
    return (ses_general_info_elem_cmn_struct *)((fbe_u8_t *)d + FBE_ESES_EMC_CONTROL_GENERAL_INFO_ELEM_SIZE);
}

/**********************************************
 * End of EMC Enclosure Status diagnostic page.
 **********************************************/

/****************************************************************************
 ****************************************************************************
 ** EMC Statistics Control/Status diagnostic page (11h).
 **
 ** This is a Control/Status page that contains a number of elements of various sizes that
 ** augment configuration and status information in the standard
 ** \ref ses_pg_emc_statistics_struct "Statistics diagnostic page".
 **
 ****************************************************************************
 ****************************************************************************/

/***********************************************
 * Common Statistics Field for all Element Types
 ***********************************************/
typedef struct ses_common_statistics_field_s
{
    fbe_u8_t elem_offset;  // Byte 0
    fbe_u8_t stats_len;    // Byte 1. (n-1)
}ses_common_statistics_field_t;

SIZE_CHECK(ses_common_statistics_field_t, 2);

/***********************************************
 * Cooling Element Statistics
 ***********************************************/
typedef struct fbe_eses_cooling_stats_s
{
    ses_common_statistics_field_t common_stats; // Bytes 0-1
    fbe_u8_t    fail;                           // Byte 2
}fbe_eses_cooling_stats_t;

/***********************************************
 * Power Supply Element Statistics
 ***********************************************/
typedef struct fbe_eses_ps_stats_s
{
    ses_common_statistics_field_t common_stats; // Bytes 0-1
    fbe_u8_t    dc_over;                        // Byte 2
    fbe_u8_t    dc_under;                       // Byte 3
    fbe_u8_t    fail;                           // Byte 4
    fbe_u8_t    ot_fail;                        // Byte 5
    fbe_u8_t    ac_fail;                        // Byte 6
    fbe_u8_t    dc_fail;                        // Byte 7
}fbe_eses_ps_stats_t;

/***********************************************
 * Temperature Sensor Element Statistics
 ***********************************************/
typedef struct fbe_eses_temperature_stats_s
{
    ses_common_statistics_field_t common_stats; // Bytes 0-1
    fbe_u8_t    ot_fail;                        // Byte 2
    fbe_u8_t    ot_warn;                        // Byte 2
}fbe_eses_temperature_stats_t;

/***********************************************
 * Expander Phy Element Statistics
 ***********************************************/
typedef struct fbe_eses_exp_phy_stats_s
{
    ses_common_statistics_field_t common_stats; // Bytes 0-1
    fbe_u8_t    rsvd;                           // Byte 2
    fbe_u32_t   invalid_dword;                  // 3-6
    fbe_u32_t   disparity_error;                // 7-10
    fbe_u32_t   loss_dword_sync;                // 11-14
    fbe_u32_t   phy_reset_fail;                 // 15-18
    fbe_u32_t   code_violation;                 // 19-22
    fbe_u8_t    phy_change;                     // 23
    fbe_u16_t   crc_pmon_accum;                 // 24-25
    fbe_u16_t   in_connect_crc;                 // 26-27
}fbe_eses_exp_phy_stats_t;

/***********************************************
 * Array Device Slot Element Statistics
 ***********************************************/
typedef struct fbe_eses_device_slot_stats_s
{
    ses_common_statistics_field_t common_stats; // Bytes 0-1
    fbe_u8_t    insert_count;                   // Byte 2
    fbe_u8_t    power_down_count;               // Byte 3
}fbe_eses_device_slot_stats_t;

/***********************************************
 * SAS Expander Element Statistics
 ***********************************************/
typedef struct fbe_eses_sas_exp_stats_s
{
    ses_common_statistics_field_t common_stats; // Bytes 0-1
    fbe_u16_t    exp_change;                      // Byte 2-3
}fbe_eses_sas_exp_stats_t;

//SIZE_CHECK(ses_common_statistics_field_t, 9);

typedef union fbe_eses_statistcs_s
{
        ses_common_statistics_field_t   common;     // bytes 0-1 common to all the structs in this union
        fbe_eses_ps_stats_t             ps_stats;
        fbe_eses_cooling_stats_t        cool_stats;
        fbe_eses_temperature_stats_t    temp_stats;
        fbe_eses_exp_phy_stats_t        exp_phy_stats;
        fbe_eses_device_slot_stats_t    device_slot_stats;
        fbe_eses_sas_exp_stats_t        sas_phy_stats;
} fbe_eses_page_statistics_u;


/************************************************************************
 * Main Structure for EMC Statistics Control/Status Diagnostic Page (11h) 
 ************************************************************************/
typedef struct ses_pg_emc_stats_stat_s 
{    
    ses_common_pg_hdr_struct hdr;   // Byte 0-7, the common page header;
    fbe_eses_page_statistics_u statistics;
} ses_pg_emc_statistics_struct;

static __inline ses_common_statistics_field_t *fbe_eses_get_first_elem_stats_p(ses_pg_emc_statistics_struct *d) {
    return (ses_common_statistics_field_t *)((fbe_u8_t *)d + FBE_ESES_EMC_STATS_CTRL_STAT_FIRST_ELEM_OFFSET);
}

static __inline ses_common_statistics_field_t *fbe_eses_get_next_elem_stats_p(ses_common_statistics_field_t *d) {
    return (ses_common_statistics_field_t *)((fbe_u8_t *)d + FBE_ESES_EMC_STATS_CTRL_STAT_COMM_FIELD_LENGTH + d->stats_len);
}

/************************************************************************
 * Main Structure for EMC Threshold In Page 
 ************************************************************************/
typedef struct ses_pg_emc_thresh_stat_s 
{    
    ses_common_pg_hdr_struct hdr;   // Byte 0-7, the common page header;
    fbe_expander_threshold_overall_info_t  overall_thresh_stat_elem;
} ses_pg_emc_threshold_struct;

/*******************************************************
 * EMC Statistics Control/Status Diagnostic Page. - End
 *******************************************************/

/**********************************************************************************************/

/***********************************
 * STRING OUT DIAGNOSTIC PAGE (04h)
 ***********************************/
typedef struct fbe_eses_pg_str_out_s{
    fbe_u8_t page_code;  // Byte 0 
    fbe_u8_t obsolete;   // Byte 1
    fbe_u16_t page_length; // Byte 2-3
    fbe_u8_t reserved : 7; // Byte 4, bit 0-6
    fbe_u8_t echo : 1;     // Byte 4, bit 7
    fbe_u8_t str_out_data[1];// The first byte of the string out data.
    //Below are the rest of the string out data.
}fbe_eses_pg_str_out_t;

/***********************************
 * End of STRING OUT DIAGNOSTIC PAGE (04h)
 ***********************************/

/************************************************
 * MODE PARAMETER LIST.
 ************************************************/
typedef struct fbe_eses_mode_param_list_s 
{    
    fbe_u16_t mode_data_len;  // Byte 0-1, reserved for mode select.
    fbe_u8_t medium_type;    // Byte 2;
    fbe_u8_t dev_specific_param; // Byte 3;
    fbe_u8_t reserved[2]; // Byte 4-5;
    fbe_u16_t block_desc_len;    // Byte 6-7;
    // below are mode pages.
} fbe_eses_mode_param_list_t;

/************************************************
 * COMMON MODE PAGE HEADER.
 ************************************************/
typedef struct fbe_eses_pg_common_mode_pg_hdr_s 
{    
    fbe_u8_t pg_code          :6;  // Byte 0, bit 0-5;
    fbe_u8_t spf              :1;  // Byte 0, bit 6;
    fbe_u8_t ps               :1;  // Byte 0, bit 7; 
                                   // In mode sense, set to 1 to indicate that parameter in this page can be saved in non-volatile memory (persistent). 
                                   // set to 0 to indicate that parameter in this page can be saved in non-volatile memory (non-persistent); 
                                   // In mode select, reserved.
    fbe_u8_t pg_len;               // Byte 1, the number of bytes to follow.
}fbe_eses_mode_pg_common_hdr_t;

/************************************************
 * EMC ESES Persistent(eep) Mode Page(20h).
 ************************************************/
typedef struct fbe_eses_pg_eep_mode_pg_s 
{    
    fbe_eses_mode_pg_common_hdr_t mode_pg_common_hdr; // Byte 0-1
    fbe_u8_t rsvd1            :4;  // Byte 2, bit 0-3;
    fbe_u8_t bad_exp_recovery_enabled : 1; // Byte 2, bit 4;
    fbe_u8_t ha_mode          :1;  // Byte 2, bit 5;
    fbe_u8_t ssu_disable      :1;  // Byte 2, bit 6;
    fbe_u8_t disable_indicator_ctrl :1; // Byte 2, bit 7;
    fbe_u8_t rsvd2[13];                 // Byte 3-15;
} fbe_eses_pg_eep_mode_pg_t;

/************************************************
 * EMC ESES Non Persistent(eenp) Mode Page(21h).
 ************************************************/
typedef struct fbe_eses_pg_eenp_mode_pg_s
{    
    fbe_eses_mode_pg_common_hdr_t mode_pg_common_hdr; // Byte 0-1
    fbe_u8_t sps_dev_supported                :1;  // Byte 2, bit 0;
    fbe_u8_t rsvd1                            :2;  // Byte 2, bit 1-2;
    fbe_u8_t include_drive_connectors         :1;  // Byte 2, bit 3;
    fbe_u8_t disable_auto_shutdown            :1;  // Byte 2, bit 4;
    fbe_u8_t disable_auto_cooling_ctrl        :1;  // Byte 2, bit 5;
    fbe_u8_t activity_led_ctrl                :1;  // Byte 2, bit 6;
    fbe_u8_t test_mode                        :1;  // Byte 2, bit 7;
    fbe_u8_t rsvd2[13];            // Byte 3-15;
} fbe_eses_pg_eenp_mode_pg_t;

static __inline fbe_eses_mode_pg_common_hdr_t *fbe_eses_get_first_mode_page(fbe_eses_mode_param_list_t *d) {
    return (fbe_eses_mode_pg_common_hdr_t *)((fbe_u8_t *)d + FBE_ESES_MODE_PARAM_LIST_HEADER_SIZE);
}

static __inline fbe_eses_mode_pg_common_hdr_t *fbe_eses_get_next_mode_page(fbe_eses_mode_pg_common_hdr_t *d) {
    return (fbe_eses_mode_pg_common_hdr_t *)((fbe_u8_t *)d + d->pg_len + FBE_ESES_MODE_PAGE_COMMON_HEADER_SIZE);
}

/**********************************************
 * End of MODE PARAMETER LIST.
 **********************************************/

/************************************************
 * READ BUFFER DESCRIPTOR
 ************************************************/
typedef struct fbe_eses_read_buf_desc_s
{    
    fbe_u8_t offset_boundary; // Byte 0
    fbe_u8_t buf_capacity[3];   // Byte 1-3
} fbe_eses_read_buf_desc_t;

static __inline fbe_u32_t fbe_eses_get_buf_capacity(fbe_eses_read_buf_desc_t *d) {
    return (fbe_u32_t)((d->buf_capacity[0] << 16) | (d->buf_capacity[1] << 8) | (d->buf_capacity[2]));
}

/**********************************************
 * End of READ BUFFER DESCRIPTOR.
 **********************************************/
#pragma pack(pop, ses_interface)

/****************************************************************************/
/** A union that acts as a superclass of all SES pages.  This is used to make it possible
 *  to write functions returning SES pages without having to use a cast, e.g.:
 * <pre>
 *     ses_pg_config_struct *config_p;        // a config page
 *     ses_pg_p fbe_eses_get_ses_pg(ses_pg_code_enum); // returns a SES page of specified type
 *
 *     ses_pg_p p = fbe_eses_get_ses_pg(SES_PG_CODE_CONFIG); // returns config page
 *     config_p = &p->config;                       // no cast needed
 * </pre>
 * The construct "p->config" is like a cast but limits cast to impossible things.
 *
 * This also allows functions taking multiple types of SES pages to be declared
 * using a type instead of void, improving documentation:
 * <pre>
 *     void ses_pg_print(void *);        // unconstrained parameter -- bad
 *     void ses_pg_print(ses_pg_p);      // typed parameter -- good
 *     ses_pg_print((ses_pg_p)config_p); // but caller must cast parameter manually
 * </pre>
 * It can also shorten tediously long names in declarations but at the slight
 * risk of an undetected error:
 * <pre>
 *     ses_pg_config_struct *conf_p;  // ugly
 *     conf_p->pg_len = 20;
 *
 *     ses_pg_p conf_p;               // shorter, but true type unknown
 *     conf_p->config.pg_len    = 20; // correct
 *     conf_p->encl_stat.pg_len = 20; // undetected mistake if pg_len is a valid field but
 *                                    // in a different place in encl_stat page
 * </pre>
 * The type is declared as a pointer type so that you don't accidentally declare
 * an object of the union type, just a pointer to one.
 ****************************************************************************/
typedef union {
    ses_common_pg_hdr_struct     hdr;
    //ses_pg_supp_diags_pgs_struct supp_diags_pgs;
    ses_pg_config_struct         config;
    ses_pg_encl_stat_struct      encl_stat;
    ses_ctrl_elem_struct         encl_ctrl;
} *ses_pg_p;

/*************************
 * Control/Status Element
 *************************/
typedef struct fbe_eses_ctrl_stat_elem_s
{
    union {
        // control element
        ses_ctrl_elem_array_dev_slot_struct  array_dev_slot_ctrl;
       
        // status element
        ses_stat_elem_array_dev_slot_struct  array_dev_slot_status;
        ses_stat_elem_exp_phy_struct         exp_phy_status;
        ses_stat_elem_sas_conn_struct        sas_conn_status;
        ses_stat_elem_ps_struct              ps_status;
        ses_stat_elem_sas_exp_struct     exp_status; 
       
    } data;
}fbe_eses_ctrl_stat_elem_t;



/* fbe_eses_main.c */
fbe_status_t fbe_eses_build_receive_diagnostic_results_cdb(fbe_u8_t *cdb, 
                                                           fbe_u16_t cdb_buffer_size,
                                                           fbe_u16_t response_buffer_size,
                                                           ses_pg_code_enum page_code);

fbe_status_t fbe_eses_build_send_diagnostic_cdb(fbe_u8_t *cdb, 
                                                fbe_u16_t cdb_buffer_size,
                                                fbe_u16_t cmd_buffer_size);

fbe_status_t fbe_eses_build_read_buf_cdb(fbe_u8_t *cdb, 
                                                fbe_u8_t cdb_buffer_size, 
                                                fbe_eses_read_buf_mode_t mode,
                                                fbe_u8_t buf_id,
                                                fbe_u32_t buf_offset,
                                                fbe_u32_t response_buffer_size);

fbe_status_t fbe_eses_build_mode_sense_10_cdb(fbe_u8_t *cdb, 
                                                          fbe_u16_t cdb_buffer_size, 
                                                          fbe_u16_t response_buffer_size,
                                                          ses_pg_code_enum page_code);

fbe_status_t fbe_eses_build_mode_select_10_cdb(fbe_u8_t *cdb, 
                                                       fbe_u16_t cdb_buffer_size, 
                                                       fbe_u16_t cmd_buffer_size);

fbe_status_t fbe_eses_build_write_buf_cdb(fbe_u8_t *cdb, 
                                                fbe_u8_t cdb_buffer_size, 
                                                fbe_eses_write_buf_mode_t mode,
                                                fbe_u8_t buf_id,
                                                fbe_u32_t data_offset,
                                                fbe_u32_t data_buf_size);

char * fbe_eses_decode_trace_buf_action_status(fbe_eses_trace_buf_action_status_t trace_buf_action_status);
char * elem_type_to_text(fbe_u8_t elem_type);

#endif /* #define FBE_ESES_H */


