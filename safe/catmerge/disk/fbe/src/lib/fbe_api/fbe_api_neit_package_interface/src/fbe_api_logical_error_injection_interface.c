/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_logical_error_injection_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_logical_error_injection interface 
 *  for the neit package.
 *
 * @ingroup fbe_api_neit_package_interface_class_files
 * @ingroup fbe_api_logical_error_injection_interface
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_trace.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_logical_error_injection_service.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include <ctype.h>


/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/**************************************
                Local variables
**************************************/
/******************************
 * @var fbe_api_lei_mode_strings
 ******************************
 * @brief There is one string per type of error mode 
 ******************************/
const fbe_char_t *fbe_api_lei_mode_strings[FBE_LOGICAL_ERROR_INJECTION_MODE_LAST] =
{
    "INV",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_INVALID */
    "CNT",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT */
    "ALW",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_ALWAYS */
    "RND",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_RANDOM */
    "SKP",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP  */
    "SKI",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT  */
    "TNS",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_TRANS */
    "TNR",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED */
    "URM",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_TRANS */
    "SLB",                      /* FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA */
    "UNK",
};

/******************************
 * @var fbe_api_lei_err_type_strings
 ******************************
 * @brief There is one string per type of error type
 ******************************/
const fbe_char_t *fbe_api_lei_err_type_strings[FBE_XOR_ERR_TYPE_MAX] =
{
    "NONE",     /* FBE_XOR_ERR_TYPE_NONE = 0,       0x00 */
    "SFT_MD",   /* FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR, 0x01 */
    "HRD_MD",   /* FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR, 0x02 */
    "RND_MD",   /* FBE_XOR_ERR_TYPE_RND_MEDIA_ERR,  0x03 */
    "CRC",      /* FBE_XOR_ERR_TYPE_CRC,            0x04 */
    "KL_CRC",   /* FBE_XOR_ERR_TYPE_KLOND_CRC,      0x05 */
    "DH_CRC",   /* FBE_XOR_ERR_TYPE_DH_CRC,         0x06 */
    "RD_CRC",   /* FBE_XOR_ERR_TYPE_RAID_CRC,       0x07 */
    "CR_CRC",   /* FBE_XOR_ERR_TYPE_CORRUPT_CRC,    0x08 */
    "WS",       /* FBE_XOR_ERR_TYPE_WS,             0x09 */
    "TS",       /* FBE_XOR_ERR_TYPE_TS,             0x0A */
    "SS",       /* FBE_XOR_ERR_TYPE_SS,             0x0B */
    "BWS",      /* FBE_XOR_ERR_TYPE_BOGUS_WS,       0x0C */
    "BTS",      /* FBE_XOR_ERR_TYPE_BOGUS_TS,       0x0D */
    "BSS",      /* FBE_XOR_ERR_TYPE_BOGUS_SS,       0x0E */
    "1NS",      /* FBE_XOR_ERR_TYPE_1NS,            0x0F */
    "1S",       /* FBE_XOR_ERR_TYPE_1S,             0x10 */
    "1R",       /* FBE_XOR_ERR_TYPE_1R,             0x11 */
    "1D",       /* FBE_XOR_ERR_TYPE_1D,             0x12 */
    "1COD",     /* FBE_XOR_ERR_TYPE_1COD,           0x13 */
    "1COP",     /* FBE_XOR_ERR_TYPE_1COP,           0x14 */
    "1POC",     /* FBE_XOR_ERR_TYPE_1POC,           0x15 */
    "COH",      /* FBE_XOR_ERR_TYPE_COH,            0x16 */
    "COR_DA",   /* FBE_XOR_ERR_TYPE_CORRUPT_DATA,   0x17 */
    "NPOC",     /* FBE_XOR_ERR_TYPE_N_POC_COH,      0x18 */
    "POC_CO",   /* FBE_XOR_ERR_TYPE_POC_COH,        0x19 */
    "UKN_CO",   /* FBE_XOR_ERR_TYPE_COH_UNKNOWN,    0x1A */
    "RB_ERR",   /* FBE_XOR_ERR_TYPE_RB_FAILED,      0x1B */
    "LBA",      /* FBE_XOR_ERR_TYPE_LBA_STAMP,      0x1C */
    "SIG_BIT",    /* FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC  0x1D */ /* Single bit CRC error injection not supported in rderr */
    "MULT_BIT",  /* FBE_XOR_ERR_TYPE_MULTI_BIT_CRC   0x1E */ /* Multi bit CRC error injection not supported in rderr */
    "TIMEOUT",      /* FBE_XOR_ERR_TYPE_TIMEOUT_ERR,      0x1F */
    "COR_CRC",    /* FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED  0x20 */ 
    "COR_DAT",  /* FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED   0x21 */ 
    "MGIC_NUM", /* FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM 0x22 */
    "SEQ_NUM",   /* FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM   0x23 */
    "SIL_DROP",  /* FBE_XOR_ERR_TYPE_SILENT_DROP   0x24 */
    "INVLDTD", /* FBE_XOR_ERR_TYPE_INVALIDATED 0x25 */
    "BAD_CRC", /* FBE_XOR_ERR_TYPE_BAD_CRC 0x26 */
    "DELAY_DN", /* FBE_XOR_ERR_TYPE_DELAY_DOWN 0x27 */
    "DLAY_UP", /* FBE_XOR_ERR_TYPE_DELAY_UP   0x28 */
    "COPY_CRC", /* FBE_XOR_ERR_TYPE_COPY_CRC 0x29 lei does not inject */
    "PVD_MD", /* FBE_XOR_ERR_TYPE_PVD_METADATA 0x2B lei does not inject */
    "IO_UNEXP", /* FBE_XOR_ERR_TYPE_IO_UNEXPECTED 0x2C lei does not inject */
    "INCPLETE_WR", /* FBE_XOR_ERR_TYPE_INCOMPLETE_WRITE 0x2D */
    "UNKN",     /* FBE_XOR_ERR_TYPE_UNKNOWN,        0x2E */                                        
    /* FBE_XOR_ERR_TYPE_MAX,                        0x24 */

};

/*******************************************
                Local functions
********************************************/
/*!***************************************************************
 * fbe_api_lei_enum_to_string()
 ****************************************************************
 * @brief
 *  This function gives string value for the enum.
 *
 * @param e_numb - enum to convert .
 * @param *str_array[] - array of possible strings
 * @param *max_str - max # of strings in array
 *
 * @return fbe_char_t - if enum found str_array[0..max_str]
 *  else if debug mode assert panic,
 *  else NULL.
 *
 * @author
 *  09/21/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
fbe_char_t const *const fbe_api_lei_enum_to_string(fbe_u32_t e_numb,
                                const fbe_char_t * const str_array[],
                                fbe_u32_t max_str)
{
    /* we just need a range check.
     */
    if (e_numb >= max_str)
    {
        /* Enum exceeds range.
         * Print unknown val.
         */
        return str_array[max_str];
    }
    return str_array[e_numb];
}

/*!***************************************************************
 * fbe_api_lei_string_to_enum()
 ****************************************************************
 * @brief
 *  This function gives enum value for string.
 *
 *  compare_str,   [I] - string to search for
 *  *str_array[],  [I] - array of possible strings
 *  max_str,       [I] - max # of strings in array
 *
 * RETURN:
 *  if compare string is found, enum of found string 0..max_str
 *  else -1
 *
 * @author
 *  09/21/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
fbe_u32_t fbe_api_lei_string_to_enum(fbe_char_t* compare_str,
                                const fbe_char_t * const str_array[],
                                fbe_u32_t max_str)
{
    fbe_u32_t index;
    fbe_u32_t i;
    fbe_char_t tmp_buf[10];

    i = 0;
    while (compare_str[i] != 0)
    {
        tmp_buf[i] = toupper(compare_str[i]);
        i++;
    }
    tmp_buf[i] = 0;

    for (index = 0;
         index < max_str;
         index++)
    {
        if (!strcmp(tmp_buf, str_array[index]))
        {
            /* Match!
             */
            return index;
        }
    }
    return max_str-1;
}

/*!***************************************************************
 * fbe_api_lei_default_trace_func()
 ****************************************************************
 * @brief
 *  This function is default trace function for displaying LEI info.
 *
 * @param trace_context - trace context
 * @param fmt - format
 *
 * @return None
 *
 * @author
 *  09/21/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
static void fbe_api_lei_default_trace_func(fbe_trace_context_t trace_context, const fbe_char_t * fmt, ...)
{
    va_list ap;
    fbe_char_t string_to_print[256];

    va_start(ap, fmt);
    _vsnprintf(string_to_print, (sizeof(string_to_print) - 1), fmt, ap);
    va_end(ap);
    fbe_printf("%s", string_to_print);
    return;
}


/*!***************************************************************
 * @fn fbe_api_logical_error_injection_enable()
 ****************************************************************
 * @brief
 *  This function starts the error injection.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_logical_error_injection_enable(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_enable()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_disable()
 ****************************************************************
 * @brief
 *  This function stops the error injection.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_logical_error_injection_disable(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_disable()
 **************************************/



/*!***************************************************************
 * @fn fbe_api_logical_error_injection_enable_poc()
 ****************************************************************
 * @brief
 *  This function enables POC error injection.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  02/15/2011 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t 
fbe_api_logical_error_injection_enable_poc(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_POC,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/***************************************************
 * end fbe_api_logical_error_injection_enable_poc()
 **************************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_disable_poc()
 ****************************************************************
 * @brief
 *  This function disables POC error injection.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  02/15/2011 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t 
fbe_api_logical_error_injection_disable_poc(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_POC,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/*****************************************************
 * end fbe_api_logical_error_injection_disable_poc()
 ****************************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_enable_ignore_corrupt_crc_data_errors()
 ****************************************************************
 * @brief
 *  This function enables ignore errors.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/21/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_api_logical_error_injection_enable_ignore_corrupt_crc_data_errors(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_IGNORE_CORRUPT_CRC_DATA_ERRORS,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/************************************************************
 * end fbe_api_logical_error_injection_enable_ignore_corrupt_crc_data_errors()
 ************************************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_disable_ignore_corrupt_crc_data_errors()
 ****************************************************************
 * @brief
 *  This function disables ignore errors.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/21/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_api_logical_error_injection_disable_ignore_corrupt_crc_data_errors(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_IGNORE_CORRUPT_CRC_DATA_ERRORS,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/*****************************************************
 * end fbe_api_logical_error_injection_disable_ignore_corrupt_crc_data_errors()
 ****************************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_get_stats(
 *          fbe_api_logical_error_injection_get_stats_t *stats_p)
 ****************************************************************
 * @brief
 *  This function returns stats on logical error injection.
 *
 * @param stats_p - ptr to stats to return.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_logical_error_injection_get_stats(fbe_api_logical_error_injection_get_stats_t *stats_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_get_stats_t         stats;
    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_STATS,
                                                           &stats,
                                                           sizeof(fbe_logical_error_injection_get_stats_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    stats_p->b_enabled = stats.b_enabled;
    stats_p->num_objects = stats.num_objects;
    stats_p->num_records = stats.num_records;
    stats_p->num_objects_enabled = stats.num_objects_enabled;
    stats_p->num_errors_injected = stats.num_errors_injected;
    stats_p->correctable_errors_detected = stats.correctable_errors_detected;
    stats_p->uncorrectable_errors_detected = stats.uncorrectable_errors_detected;
    stats_p->num_validations = stats.num_validations;
    stats_p->num_failed_validations = stats.num_failed_validations;
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_get_stats()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_get_object_stats(
 *      fbe_api_logical_error_injection_get_object_stats_t *stats_p,
 *      fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function returns stats on logical error injection.
 *
 * @param stats_p - ptr to stats to return.
 * @param object_id - Object to get stats for.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  2/8/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_logical_error_injection_get_object_stats(fbe_api_logical_error_injection_get_object_stats_t *stats_p,
                                                 fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_get_object_stats_t         stats;

    stats.object_id = object_id;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_OBJECT_STATS,
                                                           &stats,
                                                           sizeof(fbe_logical_error_injection_get_object_stats_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    stats_p->num_read_media_errors_injected = stats.num_read_media_errors_injected;
    stats_p->num_write_verify_blocks_remapped = stats.num_write_verify_blocks_remapped;
    stats_p->num_errors_injected = stats.num_errors_injected;
    stats_p->num_validations = stats.num_validations;
    return status;
}


fbe_status_t 
fbe_api_logical_error_injection_accum_object_stats(fbe_api_logical_error_injection_get_object_stats_t *stats_p,
                                                   fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_get_object_stats_t         stats;

    stats.object_id = object_id;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_OBJECT_STATS,
                                                           &stats,
                                                           sizeof(fbe_logical_error_injection_get_object_stats_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    stats_p->num_read_media_errors_injected += stats.num_read_media_errors_injected;
    stats_p->num_write_verify_blocks_remapped += stats.num_write_verify_blocks_remapped;
    stats_p->num_errors_injected += stats.num_errors_injected;
    stats_p->num_validations += stats.num_validations;
    return status;
}

/**************************************
 * end fbe_api_logical_error_injection_get_object_stats()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_create_record(
 *       fbe_api_logical_error_injection_record_t *record_p)
 ****************************************************************
 * @brief
 *  This function creates a new record.
 *
 * @param record_p - pointer to the error injection record
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_create_record(fbe_api_logical_error_injection_record_t *record_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_create_record_t     record;

    /* Copy the record from the user.
     */
    record.pos_bitmap = record_p->pos_bitmap;
    record.width = record_p->width;
    record.opcode = record_p->opcode;
    record.lba = record_p->lba;
    record.blocks = record_p->blocks;
    record.err_type = record_p->err_type;
    record.err_mode = record_p->err_mode;
    record.err_count = record_p->err_count;
    record.err_limit = record_p->err_limit;
    record.skip_count = record_p->skip_count;
    record.skip_limit = record_p->skip_limit;
    record.err_adj = record_p->err_adj;
    record.start_bit = record_p->start_bit;
    record.num_bits = record_p->num_bits;
    record.bit_adj = record_p->bit_adj;
    record.crc_det = record_p->crc_det;
    record.object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_CREATE_RECORD,
                                                           &record,
                                                           sizeof(fbe_logical_error_injection_create_record_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_create_record()
 **************************************/

/*!***************************************************************
 * fbe_api_logical_error_injection_create_object_record
 ****************************************************************
 * @brief
 *  This function creates a new record.
 *
 * @param record_p - pointer to the error injection record
 * @param object_id - Object to create record for.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  5/12/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_create_object_record(fbe_api_logical_error_injection_record_t *record_p,
                                                                  fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_create_record_t     record;

    /* Copy the record from the user.
     */
    record.pos_bitmap = record_p->pos_bitmap;
    record.width = record_p->width;
    record.opcode = record_p->opcode;
    record.lba = record_p->lba;
    record.blocks = record_p->blocks;
    record.err_type = record_p->err_type;
    record.err_mode = record_p->err_mode;
    record.err_count = record_p->err_count;
    record.err_limit = record_p->err_limit;
    record.skip_count = record_p->skip_count;
    record.skip_limit = record_p->skip_limit;
    record.err_adj = record_p->err_adj;
    record.start_bit = record_p->start_bit;
    record.num_bits = record_p->num_bits;
    record.bit_adj = record_p->bit_adj;
    record.crc_det = record_p->crc_det;
    record.object_id = object_id;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_CREATE_RECORD,
                                                           &record,
                                                           sizeof(fbe_logical_error_injection_create_record_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_create_object_record()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_modify_record(
 *       fbe_api_logical_error_injection_record_t *record_p)
 ****************************************************************
 * @brief
 *  This function modify a given record.
 *
 * @param record_p - pointer to the error injection record
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_modify_record(
                 fbe_api_logical_error_injection_modify_record_t *modify_record_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_modify_record_t       record;

    /* Copy the record from the user.
     */
    record.modify_record.pos_bitmap = modify_record_p->modify_record.pos_bitmap;
    record.modify_record.width = modify_record_p->modify_record.width;
    record.modify_record.lba = modify_record_p->modify_record.lba;
    record.modify_record.blocks = modify_record_p->modify_record.blocks;
    record.modify_record.err_type = modify_record_p->modify_record.err_type;
    record.modify_record.err_mode = modify_record_p->modify_record.err_mode;
    record.modify_record.err_count = modify_record_p->modify_record.err_count;
    record.modify_record.err_limit = modify_record_p->modify_record.err_limit;
    record.modify_record.skip_count = modify_record_p->modify_record.skip_count;
    record.modify_record.skip_limit = modify_record_p->modify_record.skip_limit;
    record.modify_record.err_adj = modify_record_p->modify_record.err_adj;
    record.modify_record.start_bit = modify_record_p->modify_record.start_bit;
    record.modify_record.num_bits = modify_record_p->modify_record.num_bits;
    record.modify_record.bit_adj = modify_record_p->modify_record.bit_adj;
    record.modify_record.crc_det = modify_record_p->modify_record.crc_det;
    
    record.record_handle = modify_record_p->record_handle;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_MODIFY_RECORD,
                                                           &record,
                                                           sizeof(fbe_logical_error_injection_modify_record_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_modify_record()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_delete_record(
 *       fbe_api_logical_error_injection_record_t *record_p)
 ****************************************************************
 * @brief
 *  This function delete the given record.
 *
 * @param record_p - pointer to the error injection record
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_delete_record(
                      fbe_api_logical_error_injection_delete_record_t *delete_record_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_remove_record_t     record;

    /* Copy the record from the user.
     */
    record.record_handle = delete_record_p->record_handle;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DELETE_RECORD,
                                                           &record,
                                                           sizeof(fbe_logical_error_injection_remove_record_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_delete_record()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_disable_records(
 *       fbe_u32_t start_index,
 *       fbe_u32_t num_to_clear)
 ****************************************************************
 * @brief
 *  This function creates a new record.
 *
 * @param start_index - Start index in this table to clear.
 * @param num_to_clear - Number of records to disable.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  2/5/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_disable_records(fbe_u32_t start_index,
                                                             fbe_u32_t num_to_clear)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_disable_records_t   disable_records;

    /* Copy info from the user..
     */
    disable_records.start_index = start_index;
    disable_records.count = num_to_clear;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_RECORDS,
                                                           &disable_records,
                                                           sizeof(fbe_logical_error_injection_disable_records_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_disable_records()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_get_records(
 *       fbe_api_logical_error_injection_get_records_t *record_p)
 ****************************************************************
 * @brief
 *  This function creates a new record.
 *
 * @param record_p - pointer to injection record
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_get_records(fbe_api_logical_error_injection_get_records_t *record_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_get_records_t       get_records;
    fbe_u32_t record_index = 0;

    /* First get the count of records.
     */
    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORDS,
                                                           &get_records,
                                                           sizeof(fbe_logical_error_injection_get_records_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy over the count.
     */
    record_p->num_records = get_records.num_records;

    /* Copy the records to the user.
     * We copy up to the max of what the user passed in or 
     * the max of what was returned. 
     */
    while ((record_index < record_p->num_records) &&
           (record_index < get_records.num_records))
    {
        record_p->records[record_index].pos_bitmap = get_records.records[record_index].pos_bitmap;
        record_p->records[record_index].width = get_records.records[record_index].width;
        record_p->records[record_index].lba = get_records.records[record_index].lba;
        record_p->records[record_index].blocks = get_records.records[record_index].blocks;
        record_p->records[record_index].err_type = get_records.records[record_index].err_type;
        record_p->records[record_index].err_mode = get_records.records[record_index].err_mode;
        record_p->records[record_index].err_count = get_records.records[record_index].err_count;
        record_p->records[record_index].err_limit = get_records.records[record_index].err_limit;
        record_p->records[record_index].skip_count = get_records.records[record_index].skip_count;
        record_p->records[record_index].skip_limit = get_records.records[record_index].skip_limit;
        record_p->records[record_index].err_adj = get_records.records[record_index].err_adj;
        record_p->records[record_index].start_bit = get_records.records[record_index].start_bit;
        record_p->records[record_index].num_bits = get_records.records[record_index].num_bits;
        record_p->records[record_index].bit_adj = get_records.records[record_index].bit_adj;
        record_p->records[record_index].crc_det = get_records.records[record_index].crc_det;
        record_p->records[record_index].opcode = get_records.records[record_index].opcode;
        
        record_index++;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_get_records()
 **************************************/

/*!***************************************************************************
 * @fn fbe_api_logical_error_injection_generate_edge_hook_bitmask(
 *        fbe_u32_t positions_to_disable_bitmask,
 *        fbe_u32_t *edge_hook_bitmask_p)
 *****************************************************************************
 * @brief
 *  Simply generates the edge hook bitmask.
 *
 * @param positions_to_disable_bitmask - Bitmask of positions that should
 *          not have the edge hook set.
 * @param edge_hook_bitmask_p - Pointer to resulting bitmask
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/03/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_generate_edge_hook_bitmask(fbe_u32_t positions_to_disable_bitmask,
                                                                        fbe_u32_t *edge_hook_bitmask_p)
{
    /* validate arguments
     */
    if (edge_hook_bitmask_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: edge_hook_bitmask_p is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Remove the disabled positions
     */
    *edge_hook_bitmask_p = FBE_U32_MAX & ~positions_to_disable_bitmask;
    return FBE_STATUS_OK;
}
/******************************************************************
 * end fbe_api_logical_error_injection_generate_edge_hook_bitmask()
 ******************************************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_modify_object(
 *        fbe_object_id_t object_id,
 *        fbe_u32_t edge_hook_enable_bitmask,
 *        fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function `modifies' an logical error injection object with
 *  the values specified.
 *
 * @param object_id - object ID
 * @param edge_hook_enable_bitmask - Bitmask of which positions to enable
 * @param injection_lba_adjustment - Adjust the lba to inject to by this amount
 * @param package_id - Package ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/02/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_modify_object(fbe_object_id_t object_id,
                                                           fbe_u32_t edge_hook_enable_bitmask,
                                                           fbe_lba_t injection_lba_adjustment,
                                                           fbe_package_id_t package_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_logical_error_injection_modify_objects_t modify_object;

    modify_object.filter.filter_type = FBE_LOGICAL_ERROR_INJECTION_FILTER_TYPE_OBJECT;
    modify_object.filter.class_id = FBE_CLASS_ID_INVALID;
    modify_object.filter.object_id = object_id;
    modify_object.filter.package_id = package_id;
    modify_object.filter.edge_index = FBE_U32_MAX;
    modify_object.edge_hook_enable_bitmask = edge_hook_enable_bitmask;
    modify_object.lba_injection_adjustment = injection_lba_adjustment;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_MODIFY_OBJECTS,
                                                           &modify_object,
                                                           sizeof(fbe_logical_error_injection_modify_objects_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************************************
 * end fbe_api_logical_error_injection_modify_object()
 ******************************************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_enable_object(
 *        fbe_object_id_t object_id,
 *        fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function starts the error injection on a single object.
 *
 * @param object_id - object ID
 * @param package_id - Package ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_enable_object(fbe_object_id_t object_id,
                                                           fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_enable_objects_t    enable_objects;

    enable_objects.filter.filter_type = FBE_LOGICAL_ERROR_INJECTION_FILTER_TYPE_OBJECT;
    enable_objects.filter.class_id = FBE_CLASS_ID_INVALID;
    enable_objects.filter.object_id = object_id;
    enable_objects.filter.package_id = package_id;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_OBJECTS,
                                                           &enable_objects,
                                                           sizeof(fbe_logical_error_injection_enable_objects_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_enable_object()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_disable_object(
 *       fbe_object_id_t object_id,
 *       fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function stops the error injection.
 *
 * @param object_id - object ID
 * @param package_id - Package ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_disable_object(fbe_object_id_t object_id,
                                                            fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_disable_objects_t    disable_objects;

    disable_objects.filter.filter_type = FBE_LOGICAL_ERROR_INJECTION_FILTER_TYPE_OBJECT;
    disable_objects.filter.class_id = FBE_CLASS_ID_INVALID;
    disable_objects.filter.object_id = object_id;
    disable_objects.filter.package_id = package_id;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_OBJECTS,
                                                           &disable_objects,
                                                           sizeof(fbe_logical_error_injection_disable_objects_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_disable_object()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_enable_class(
 *       fbe_class_id_t class_id,
 *       fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function starts the error injection on a class of objects.
 *
 * @param class_id - class ID
 * @param package_id - package ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_enable_class(fbe_class_id_t class_id,
                                                          fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_enable_objects_t    enable_objects;

    enable_objects.filter.filter_type = FBE_LOGICAL_ERROR_INJECTION_FILTER_TYPE_CLASS;
    enable_objects.filter.class_id = class_id;
    enable_objects.filter.object_id = FBE_OBJECT_ID_INVALID;
    enable_objects.filter.package_id = package_id;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_OBJECTS,
                                                           &enable_objects,
                                                           sizeof(fbe_logical_error_injection_enable_objects_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_enable_class()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_disable_class(
 *        fbe_class_id_t class_id,
 *        fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function stops the error injection on a class of objects.
 *
 * @param class_id - class ID
 * @param package_id - package ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_disable_class(fbe_class_id_t class_id,
                                                           fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_disable_objects_t    disable_objects;

    disable_objects.filter.filter_type = FBE_LOGICAL_ERROR_INJECTION_FILTER_TYPE_CLASS;
    disable_objects.filter.class_id = class_id;
    disable_objects.filter.object_id = FBE_OBJECT_ID_INVALID;
    disable_objects.filter.package_id = package_id;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_OBJECTS,
                                                           &disable_objects,
                                                           sizeof(fbe_logical_error_injection_disable_objects_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_disable_class()
 **************************************/

/*!***************************************************************
 * fbe_api_logical_error_injection_destroy_objects()
 ****************************************************************
 * @brief
 *  Destroy all the objects in the logical error injection service.
 *
 * @param None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  8/31/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_destroy_objects(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DESTROY_OBJECTS,
                                                           NULL,
                                                           0, /* No buffer */
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_destroy_objects()
 **************************************/

/*!***************************************************************
 * fbe_api_logical_error_injection_get_table_info()
 ****************************************************************
 * @brief
 *  This function gets the table information for the given table.
 *
 * @param table_index - index of table to load.
 * @param b_simulation - FBE_TRUE to load a simulation table.
 *                       FBE_FALSE otherwise.
 * @param tables_info_p
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  9/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_get_table_info(fbe_u32_t table_index, 
                                                             fbe_bool_t b_simulation,
                                                             fbe_api_logical_error_injection_table_description_t *tables_info_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_sg_element_t                                sg_list[2];
    fbe_logical_error_injection_load_table_t        load_table;
    fbe_u8_t                                        sg_count = 0;

    load_table.b_simulation = b_simulation;
    load_table.table_index = table_index;

    /* Initialize the sg list */
    fbe_sg_element_init(sg_list, 
                        sizeof(fbe_api_logical_error_injection_table_description_t), 
                        tables_info_p);
    sg_count++;
    /* no more elements*/
    fbe_sg_element_terminate(&sg_list[sg_count]);
    sg_count++;
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_TABLE_INFO,
                                                                        &load_table,
                                                                        sizeof(fbe_logical_error_injection_load_table_t),
                                                                        FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                                        sg_list,
                                                                        sg_count,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    tables_info_p = (fbe_api_logical_error_injection_table_description_t*)fbe_sg_element_address(sg_list);
    if(tables_info_p != NULL)
    {
        fbe_trace_func_t trace_func = fbe_api_lei_default_trace_func;
        fbe_trace_context_t trace_context = NULL;
        /* take table descriptor from the table */
        trace_func(trace_context, "%s\n", (char*)tables_info_p);
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_get_table_info()
 **************************************/

/*!***************************************************************
 * fbe_api_logical_error_injection_get_table_info()
 ****************************************************************
 * @brief
 *  This function gets the table information for the given table.
 *
 * @param b_simulation - FBE_TRUE to load a simulation table.
 *                       FBE_FALSE otherwise.
 * @param tables_info_p
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  9/21/2010 - Created. Swati Fursule
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_get_active_table_info(fbe_bool_t b_simulation,
                                                             fbe_api_logical_error_injection_table_description_t *tables_info_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_sg_element_t                                sg_list[2];
    fbe_logical_error_injection_load_table_t        load_table;
    fbe_u8_t                                        sg_count = 0;

    load_table.b_simulation = b_simulation;
    load_table.table_index = 0; /*dummy*/

    /* Initialize the sg list */
    fbe_sg_element_init(sg_list, 
                        sizeof(fbe_api_logical_error_injection_table_description_t), 
                        tables_info_p);
    sg_count++;
    /* no more elements*/
    fbe_sg_element_terminate(&sg_list[sg_count]);
    sg_count++;
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_ACTIVE_TABLE_INFO,
                                                                        &load_table,
                                                                        sizeof(fbe_logical_error_injection_load_table_t),
                                                                        FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                                        sg_list,
                                                                        sg_count,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    tables_info_p = (fbe_api_logical_error_injection_table_description_t*)fbe_sg_element_address(sg_list);
    if(tables_info_p != NULL)
    {
        fbe_trace_func_t trace_func = fbe_api_lei_default_trace_func;
        fbe_trace_context_t trace_context = NULL;
        /* take table descriptor from the table */
        trace_func(trace_context, "%s\n", (char*)tables_info_p);
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_get_active_table_info()
 **************************************/

/*!***************************************************************
 * fbe_api_logical_error_injection_load_table()
 ****************************************************************
 * @brief
 *  This function creates a new record.
 *
 * @param table_index - index of table to load.
 * @param b_simulation - FBE_TRUE to load a simulation table.
 *                       FBE_FALSE otherwise.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  1/5/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_load_table(fbe_u32_t table_index, fbe_bool_t b_simulation)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_logical_error_injection_load_table_t        load_table;

    load_table.b_simulation = b_simulation;
    load_table.table_index = table_index;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_LOAD_TABLE,
                                                           &load_table,
                                                           sizeof(fbe_logical_error_injection_load_table_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_load_table()
 **************************************/

/*!**************************************************************
 *          fbe_api_logical_error_injection_display_r6_w5()
 ****************************************************************
 *
 * @brief   Display logical error injection service raid6 specific data
 * @param   trace_func
 * @param   trace_context
 * @param   data
 * @param   is_number
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_api_logical_error_injection_display_r6_w5(fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_u32_t data, 
                                                   fbe_bool_t is_number)
{
    /*if (data == RG_R6_ERR_PARM_RND)
    {
        trace_func(trace_context, "  RND");
    }
    else if (data == RG_R6_ERR_PARM_UNUSED)
    {
        trace_func(trace_context, " DEAD");
    }
    else
    */if (is_number)
    {
        trace_func(trace_context, "%5x", data);
    }
    else
    {
        trace_func(trace_context, "%5s", data ? "Yes" : "No"); 
    }
}

/*!**************************************************************
 *          fbe_api_logical_error_injection_display_r6_w4()
 ****************************************************************
 *
 * @brief   Display logical error injection service raid6 specific data
 * @param   trace_func
 * @param   trace_context
 * @param   data
 * @param   is_number
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_api_logical_error_injection_display_r6_w4(fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_u32_t data, 
                                                   fbe_bool_t is_number)
{
    /*if (data == RG_R6_ERR_PARM_RND)
    {
        trace_func(trace_context, " RND");
    }
    else*/
    if (is_number)
    {                              
        trace_func(trace_context, "%4x", data);
    }
    else
    {
        trace_func(trace_context, "%4s", data ? "Yes" : "No"); 
    }
}
/*!**************************************************************
 *          fbe_api_logical_error_injection_display_stats()
 ****************************************************************
 *
 * @brief   Display statistics of logical error injection service
 * @param   argument stats
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_api_logical_error_injection_display_stats(
                    fbe_api_logical_error_injection_get_stats_t stats
                    )
{
    fbe_trace_func_t trace_func = fbe_api_lei_default_trace_func;
    fbe_trace_context_t trace_context = NULL;
    trace_func(trace_context, "Statistics information on the LEI service\n");
    trace_func(trace_context, "\tLogical Error Injection status : %s\n", 
               (stats.b_enabled ? "ENABLED" : "DISABLED"));
    trace_func(trace_context, "\tNumber of records active for error injection: %d\n", 
               stats.num_records);
    trace_func(trace_context, "\tNumber of Objects for error injection: %d\n", 
               stats.num_objects);
    trace_func(trace_context, "\tNumber of Objects enabled for error injection: %d\n", 
               stats.num_objects_enabled);
    trace_func(trace_context, "\tNumber of Errors Injected: %d\n", 
               (int)stats.num_errors_injected);
    trace_func(trace_context, "\tNumber of Correctable Errors Detected: %d\n", 
               (int)stats.correctable_errors_detected);
    trace_func(trace_context, "\tNumber of Uncorrectable Errors Detected: %d\n", 
               (int)stats.uncorrectable_errors_detected);
    trace_func(trace_context, "\tNumber of Validation performed: %d\n", 
               (int)stats.num_validations);
    trace_func(trace_context, "\tNumber of Validation that failed: %d\n", 
               (int)stats.num_failed_validations);
    trace_func(trace_context, "%s", "\n");
    return;
}
/******************************************
 * end fbe_cli_rginfo_parse_cmds()
 ******************************************/

/*!**************************************************************
 *          fbe_api_logical_error_injection_display_obj_stats()
 ****************************************************************
 *
 * @brief   Display object statistics of logical error injection service
 * @param   argument obj_stats
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_api_logical_error_injection_display_obj_stats(
            fbe_api_logical_error_injection_get_object_stats_t obj_stats
            )
{
    fbe_trace_func_t trace_func = fbe_api_lei_default_trace_func;
    fbe_trace_context_t trace_context = NULL;
    trace_func(trace_context, "Object Statistics information on the LEI service\n");
    trace_func(trace_context, "\tNumber of read media errors injected: %d\n", 
               (int)obj_stats.num_read_media_errors_injected);
    trace_func(trace_context, "\tTotal number of blocks remapped "
                   " as a result of a write verify command.: %d\n", 
               (int)obj_stats.num_write_verify_blocks_remapped);
    trace_func(trace_context, "\tTotal number of errors injected by the tool: %d\n", 
               (int)obj_stats.num_errors_injected);
    trace_func(trace_context, "\tTotal number of validation calls on this object: %d\n", 
               (int)obj_stats.num_validations);
    trace_func(trace_context, "%s", "\n");
    return;
}
/******************************************
 * end fbe_api_logical_error_injection_display_obj_stats()
 ******************************************/

/*!**************************************************************
 *          fbe_api_logical_error_injection_error_display()
 ****************************************************************
 *
 * @brief   Display Error records for logical error injection service
 * @param   argument get_records
 * @param   argument start
 * @param   argument end
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_api_logical_error_injection_error_display(
                    fbe_api_logical_error_injection_get_records_t get_records,
                    fbe_u32_t start,
                    fbe_u32_t end
                    )
{
    fbe_u32_t record;
    fbe_trace_func_t trace_func = fbe_api_lei_default_trace_func;
    fbe_trace_context_t trace_context = NULL;
    trace_func(trace_context, "Error Records  information on the LEI service\n");
    /* take table descriptor from the table 
    if ( rg_error_sim.table_desc != NULL )
    {
        trace_func(trace_context, "%s", rg_error_sim.table_desc);
    }*/
    trace_func(trace_context, "\n\n");
    
    trace_func(trace_context, 
               "                                         Err  Err  Err  Err  Err  Str  Num  Bit  CRC\n");
    trace_func(trace_context, 
               " Rec  Pos  Wth      LBA    Blocks        Type Mod  Cnt  Lmt  Adj  Bit  Bit  Adj  Det\n"); 
    trace_func(trace_context, "----------------------------------------"
                   "--------------------------------------------\n");
    for(record=0; record<get_records.num_records; record++)
    {
        trace_func(trace_context, "%4d", record);
        fbe_api_logical_error_injection_display_r6_w5(trace_func, trace_context,
            get_records.records[record].pos_bitmap, FBE_TRUE);
        fbe_api_logical_error_injection_display_r6_w5(trace_func, trace_context,
            get_records.records[record].width,      FBE_TRUE);

        trace_func(trace_context, " %08llX %9llx",
               get_records.records[record].lba,
               get_records.records[record].blocks);
        trace_func(trace_context, "%12s %3s ",
               fbe_api_lei_enum_to_string(get_records.records[record].err_type,
               fbe_api_lei_err_type_strings,FBE_XOR_ERR_TYPE_MAX),
               fbe_api_lei_enum_to_string(get_records.records[record].err_mode,
               fbe_api_lei_mode_strings,FBE_LOGICAL_ERROR_INJECTION_MODE_LAST)
               );
        trace_func(trace_context, "%4x %4x ",
               get_records.records[record].err_count,
               get_records.records[record].err_limit);
        trace_func(trace_context, "%4x %4x %4x ", 
                   get_records.records[record].err_adj,
                   get_records.records[record].start_bit,
                   get_records.records[record].num_bits);
        fbe_api_logical_error_injection_display_r6_w4(trace_func, trace_context,
            get_records.records[record].bit_adj,   FBE_FALSE);
        fbe_api_logical_error_injection_display_r6_w5(trace_func, trace_context,
            get_records.records[record].crc_det,   FBE_FALSE);
        trace_func(trace_context, "\n");
    }
    return;
}
/******************************************
 * end fbe_api_logical_error_injection_error_display()
 ******************************************/

/*!**************************************************************
 *          fbe_api_logical_error_injection_table_info_display()
 ****************************************************************
 *
 * @brief   Display table information logical error injection service
 * @param   argument get_table_p
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_api_logical_error_injection_table_info_display(
                    fbe_api_logical_error_injection_table_t* get_table_p
                    )
{
    fbe_trace_func_t trace_func = fbe_api_lei_default_trace_func;
    fbe_trace_context_t trace_context = NULL;
    /* take table descriptor from the table */
    if ( get_table_p->description != NULL )
    {
        trace_func(trace_context, "%s", get_table_p->description);
    }
    return;
}
/******************************************
 * end fbe_api_logical_error_injection_table_info_display()  
 ******************************************/

/*!***************************************************************
 * fbe_api_logical_error_injection_remove_object()
 ****************************************************************
 * @brief
 *  Remove any edge hooks associated with the object specified then
 *  remove the object from the logical error injection service.
 * 
 * @param object_id - object ID
 * @param package_id - Package ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/10/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_logical_error_injection_remove_object(fbe_object_id_t object_id,
                                                           fbe_package_id_t package_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_logical_error_injection_remove_object_t remove_object_info;
    fbe_api_control_operation_status_info_t     status_info;

    remove_object_info.object_id = object_id;
    remove_object_info.package_id = package_id;
    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_OBJECT,
                                                           &remove_object_info,
                                                           sizeof(fbe_logical_error_injection_remove_object_t),
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/********************************************************
 * end fbe_api_logical_error_injection_remove_object()
 ********************************************************/

/*!***************************************************************
 * @fn fbe_api_logical_error_injection_enable()
 ****************************************************************
 * @brief
 *  This function disable table lba normalization.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/08/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_api_logical_error_injection_disable_lba_normalize(void)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_LBA_NORMALIZE,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_logical_error_injection_enable()
 **************************************/

/*******************************************************
 * end file fbe_api_logical_error_injection_interface.c
 *******************************************************/ 
