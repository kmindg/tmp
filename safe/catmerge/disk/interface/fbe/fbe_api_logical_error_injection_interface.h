#ifndef FBE_API_LOGICAL_ERROR_INJECTION_INTERFACE_H
#define FBE_API_LOGICAL_ERROR_INJECTION_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_logical_error_injection_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_logical_error_injection_interfac.c.
 * 
 * @ingroup fbe_api_neit_package_interface_class_files
 * 
 * @version
 *   10/10/08    sharel - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe_logical_error_injection_service.h"


/*!*******************************************************************
 * @def FBE_API_LEI_MAX_RECORDS
 *********************************************************************
 * @brief Max number of records this service allows.
 *
 *********************************************************************/
#define FBE_API_LEI_MAX_RECORDS 256

/*!*******************************************************************
 * @def FBE_API_LOGICAL_ERROR_INJECTION_MAX_TABLES
 *********************************************************************
 * @brief Max number of tables this service allows.
 *
 *********************************************************************/
#define FBE_API_LOGICAL_ERROR_INJECTION_MAX_TABLES       17
/*!*******************************************************************
 * @def FBE_API_LOGICAL_ERROR_INJECTION_DESCRIPTION_MAX_CHARS
 *********************************************************************
 * @brief Max number of description length.
 *
 *********************************************************************/
#define FBE_API_LOGICAL_ERROR_INJECTION_DESCRIPTION_MAX_CHARS 128

//----------------------------------------------------------------
// Define the top level group for the FBE NEIT Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_api_neit_package_class FBE NEIT Package APIs 
 *  @brief 
 *    This is the set of definitions for FBE NEIT Package APIs.
 * 
 *  @ingroup fbe_api_neit_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Logical Error Injection Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_logical_error_injection_usurper_interface FBE API Logical Error Injection Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Logical Error Injection class
 *    usurper interface.
 *  
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!********************************************************************* 
 * @struct fbe_api_logical_error_injection_get_stats_t 
 *  
 * @brief 
 *   Statistics information on the lei service.
 *
 * @ingroup fbe_api_logical_error_injection_interface
 * @ingroup fbe_api_logical_error_injection_get_stats
 **********************************************************************/
typedef struct fbe_api_logical_error_injection_get_stats_s
{
    fbe_bool_t b_enabled; /*!< FBE_TRUE if service is injecting. */

    fbe_u32_t num_objects; /*!< Number of objects we are injecting on. */
    fbe_u32_t num_records; /*!< Number of records active. */
    fbe_u32_t num_objects_enabled; /*< Number of objects injecting errors. */

    fbe_u64_t num_errors_injected; /*!< Number of errors we injected. */
    fbe_u64_t correctable_errors_detected; /*!< c_errors encountered by validation.  */
    fbe_u64_t uncorrectable_errors_detected; /*!< u_errors encountered by validation.  */
    fbe_u64_t num_validations;     /*!< Number of error validations performed. */
    fbe_u64_t num_failed_validations; /*!< Number of validations that failed. */
}
fbe_api_logical_error_injection_get_stats_t;

/*!********************************************************************* 
 * @struct fbe_api_logical_error_injection_get_object_stats_t 
 *  
 * @brief 
 *   Statistics information on a given object.
 *
 * @ingroup fbe_api_logical_error_injection_interface
 * @ingroup fbe_api_logical_error_injection_get_object_stats
 **********************************************************************/
typedef struct fbe_api_logical_error_injection_get_object_stats_s
{
    fbe_u64_t num_read_media_errors_injected;  /*!< Number of read media errors injected. */

    /*! Total number of blocks remapped as a result of a write verify command. */
    fbe_u64_t num_write_verify_blocks_remapped; 

    /*! Total number of errors injected by the tool. */ 
    fbe_u64_t num_errors_injected;

    /*! Total number of validation calls on this object. */ 
    fbe_u64_t num_validations;
}
fbe_api_logical_error_injection_get_object_stats_t;

/*!********************************************************************* 
 * @enum fbe_api_logical_error_injection_mode_t 
 *  
 * @brief 
 *  This enum has one bit position for an individual
 *  display mode.
 *  
 *  IMPORTANT: This has to match the definition in the service.
 *
 * @ingroup fbe_api_logical_error_injection_interface
 * @ingroup fbe_api_logical_error_injection_mode
 **********************************************************************/
typedef enum fbe_api_logical_error_injection_mode_s
{
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID = 0x0,               /*!< INVALID Mode */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT = 0x1,                 /*!< Count */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS = 0x2,                /*!< Always */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_RANDOM = 0x3,                /*!< Random */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP = 0x4,                  /*!< Skip */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT = 0x5,           /*!< Insert */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS = 0x6,                 /*!< Transition */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_TRANS_RND = 0x7,             /*!< Sometimes transitory, sometimes not. */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED = 0x8, /*!< Inject until the error gets remapped. */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA = 0x9,       /*!< Inject same LBA */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_UNKNOWN = 0xa,               /*!< Unknown */
    FBE_API_LOGICAL_ERROR_INJECTION_MODE_LAST                         /*!< Last */
}
fbe_api_logical_error_injection_mode_t;

/*!********************************************************************* 
 * @enum fbe_api_logical_error_injection_type_t 
 *  
 * @brief 
 *  This maps the valid user injection types to
 *  the types that error injection understands.
 *
 * @ingroup fbe_api_logical_error_injection_interface
 * @ingroup fbe_api_logical_error_injection_type
 **********************************************************************/
typedef enum fbe_api_logical_error_injection_type_s
{
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID = 0x0,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC = FBE_XOR_ERR_TYPE_CRC,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RAID_CRC  = FBE_XOR_ERR_TYPE_RAID_CRC,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SINGLE_BIT_CRC = FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC  = FBE_XOR_ERR_TYPE_MULTI_BIT_CRC,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MULTI_BIT_CRC_LBA_STAMP  = FBE_XOR_ERR_TYPE_MULTI_BIT_WITH_LBA_STAMP,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP = FBE_XOR_ERR_TYPE_LBA_STAMP,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_CRC = FBE_XOR_ERR_TYPE_CORRUPT_CRC,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CORRUPT_DATA = FBE_XOR_ERR_TYPE_CORRUPT_DATA,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_WRITE_STAMP = FBE_XOR_ERR_TYPE_WS,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIME_STAMP  = FBE_XOR_ERR_TYPE_TS,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SHED_STAMP  = FBE_XOR_ERR_TYPE_SS,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_WRITE_STAMP = FBE_XOR_ERR_TYPE_BOGUS_WS,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_TIME_STAMP  = FBE_XOR_ERR_TYPE_BOGUS_TS,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_BOGUS_SHED_STAMP  = FBE_XOR_ERR_TYPE_BOGUS_SS,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1NS = FBE_XOR_ERR_TYPE_1NS,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1S  = FBE_XOR_ERR_TYPE_1S,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1R  = FBE_XOR_ERR_TYPE_1R,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1D  = FBE_XOR_ERR_TYPE_1D,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COD = FBE_XOR_ERR_TYPE_1COD,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1COP = FBE_XOR_ERR_TYPE_1COP,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_1POC = FBE_XOR_ERR_TYPE_1POC,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH  = FBE_XOR_ERR_TYPE_COH,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR  = FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR = FBE_XOR_ERR_TYPE_TIMEOUT_ERR,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR = FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP = FBE_XOR_ERR_TYPE_SILENT_DROP,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALIDATED = FBE_XOR_ERR_TYPE_INVALIDATED,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_MAGIC_NUM_MISMATCH = FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_RM_SEQ_NUM_MISMATCH = FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN = FBE_XOR_ERR_TYPE_DELAY_DOWN,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP = FBE_XOR_ERR_TYPE_DELAY_UP,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_IO_UNEXPECTED = FBE_XOR_ERR_TYPE_IO_UNEXPECTED,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INCOMPLETE_WRITE = FBE_XOR_ERR_TYPE_INCOMPLETE_WRITE,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_KEY_ERROR = FBE_XOR_ERR_TYPE_KEY_ERROR,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_KEY_NOT_FOUND = FBE_XOR_ERR_TYPE_KEY_NOT_FOUND,
    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_ENCRYPTION_NOT_ENABLED = FBE_XOR_ERR_TYPE_ENCRYPTION_NOT_ENABLED,

    FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LAST
}
fbe_api_logical_error_injection_type_t;

/*!********************************************************************* 
 * @struct fbe_api_logical_error_injection_record_t 
 *  
 * @brief 
 *   Record from lei service.
 *
 * @ingroup fbe_api_logical_error_injection_interface
 * @ingroup fbe_api_logical_error_injection_record
 **********************************************************************/
typedef struct fbe_api_logical_error_injection_record_s
{

    fbe_u16_t pos_bitmap;         /*!< Bitmap for each position to error */
    fbe_u16_t width;              /*!< Array width */
    fbe_lba_t lba;                /*!< Physical address to begin errs    */
    fbe_block_count_t blocks;     /*!< Blocks to extend the errs for     */

    fbe_api_logical_error_injection_type_t err_type; /*!< Type of errors, see above      */
    fbe_api_logical_error_injection_mode_t err_mode; /*!< Type of error injection, see above */
    fbe_u16_t err_count;          /*!< Count of errs injected so far     */
    fbe_u16_t err_limit;          /*!< Limit of errors to inject         */

    fbe_u16_t skip_count;         /*!< Limit of errors to skip          */
    fbe_u16_t skip_limit;         /*!< Limit of errors to inject         */
    fbe_u16_t err_adj;            /*!< Error adjacency bitmask           */
    fbe_u16_t start_bit;          /*!< Starting bit of an error          */

    fbe_u16_t num_bits;           /*!< Number of bits of an error        */
    fbe_u16_t bit_adj;            /*!< Whether erroneous bits adjacent   */
    fbe_u16_t crc_det;            /*!< Whether error is CRC detectable   */
    fbe_u32_t opcode;             /*!< If not 0 or invalid, only inject for this opcode. */
    fbe_object_id_t object_id;    /*!< Object id to inject on. */
}
fbe_api_logical_error_injection_record_t;

/*!********************************************************************* 
 * @struct fbe_api_logical_error_injection_get_records_t 
 *  
 * @brief 
 *   Structure to fetch records from lei service.
 *
 * @ingroup fbe_api_logical_error_injection_interface
 * @ingroup fbe_api_logical_error_injection_get_records
 **********************************************************************/
typedef struct fbe_api_logical_error_injection_get_record_s
{
    fbe_u32_t num_records; /*!< Number of validations that failed. */

    /*! Error injection record. */
    fbe_api_logical_error_injection_record_t records[FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS];
}
fbe_api_logical_error_injection_get_records_t;

/*!**************************************************
 * @typedef fbe_api_logical_error_injection_table_description_t
 ***************************************************
 * @brief string to hold the description of a table.
 ***************************************************/
typedef fbe_u8_t fbe_api_logical_error_injection_table_description_t[FBE_API_LOGICAL_ERROR_INJECTION_DESCRIPTION_MAX_CHARS];

/*!*******************************************************************
 * @struct fbe_logical_error_injection_static_table_t
 *********************************************************************
 * @brief This contains a single error injection table.
 *
 *********************************************************************/
typedef struct fbe_api_logical_error_injection_table_s
{
    fbe_api_logical_error_injection_table_description_t description;
}
fbe_api_logical_error_injection_table_t;

/*!********************************************************************* 
 * @struct fbe_api_logical_error_injection_get_tables_s 
 *  
 * @brief 
 *   Structure to fetch tables from lei service.
 *
 * @ingroup fbe_api_logical_error_injection_interface
 * @ingroup fbe_api_logical_error_injection_get_records
 **********************************************************************/
typedef struct fbe_api_logical_error_injection_get_tables_s
{
    /*fbe_u32_t num_tables; !< Number of tables. */
    /*! Error injection tables. */
    fbe_api_logical_error_injection_table_t tables[FBE_API_LOGICAL_ERROR_INJECTION_MAX_TABLES];
}
fbe_api_logical_error_injection_get_tables_t;

/*!*************************************************
 * @typedef fbe_api_logical_error_injection_modify_record_t
 ***************************************************
 * @brief Info to create a record.
 *
 ***************************************************/
typedef struct fbe_api_logical_error_injection_modify_record_s
{
    fbe_u64_t record_handle;
    fbe_api_logical_error_injection_record_t modify_record;
}
fbe_api_logical_error_injection_modify_record_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_delete_record_t
 ***************************************************
 * @brief Info to create a record.
 *
 ***************************************************/
typedef struct fbe_api_logical_error_injection_delete_record_s
{
    fbe_u64_t record_handle;
}
fbe_api_logical_error_injection_delete_record_t;


/*! @} */ /* end of group fbe_api_logical_error_injection_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Logical Error Injection Interface.  
// This is where all the function prototypes for the Logical Error Injection API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_logical_error_injection_interface FBE API Logical Error Injection Interface
 *  @brief 
 *    This is the set of FBE API Logical Error Injection Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_logical_error_injection_interface.h.
 *
 *  @ingroup fbe_api_neit_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t fbe_api_logical_error_injection_enable(void);

fbe_status_t fbe_api_logical_error_injection_disable(void);
fbe_status_t fbe_api_logical_error_injection_enable_poc(void);
fbe_status_t fbe_api_logical_error_injection_disable_poc(void);
fbe_status_t fbe_api_logical_error_injection_enable_ignore_corrupt_crc_data_errors(void);
fbe_status_t fbe_api_logical_error_injection_disable_ignore_corrupt_crc_data_errors(void);
fbe_status_t fbe_api_logical_error_injection_get_stats(fbe_api_logical_error_injection_get_stats_t *stats);
fbe_status_t fbe_api_logical_error_injection_get_object_stats(fbe_api_logical_error_injection_get_object_stats_t *stats_p,
                                                              fbe_object_id_t object_id);
fbe_status_t fbe_api_logical_error_injection_create_record(fbe_api_logical_error_injection_record_t *record_p);
fbe_status_t fbe_api_logical_error_injection_create_object_record(fbe_api_logical_error_injection_record_t *record_p,
                                                                  fbe_object_id_t object_id);
fbe_status_t fbe_api_logical_error_injection_get_records(fbe_api_logical_error_injection_get_records_t *record_p);
fbe_status_t fbe_api_logical_error_injection_modify_record(
                 fbe_api_logical_error_injection_modify_record_t *modify_record_p);
fbe_status_t fbe_api_logical_error_injection_delete_record(
                      fbe_api_logical_error_injection_delete_record_t *delete_record_p);
fbe_status_t fbe_api_logical_error_injection_get_table_info(fbe_u32_t table_index, 
                                                             fbe_bool_t b_simulation,
                                                             fbe_api_logical_error_injection_table_description_t *tables_info_p);
fbe_status_t fbe_api_logical_error_injection_get_active_table_info(fbe_bool_t b_simulation,
                                                             fbe_api_logical_error_injection_table_description_t *tables_info_p);
fbe_status_t fbe_api_logical_error_injection_generate_edge_hook_bitmask(fbe_u32_t positions_to_disable_bitmask,
                                                                        fbe_u32_t *edge_hook_bitmask_p);
fbe_status_t fbe_api_logical_error_injection_modify_object(fbe_object_id_t object_id,
                                                           fbe_u32_t edge_hook_enable_bitmask,
                                                           fbe_lba_t injection_lba_adjustment,
                                                           fbe_package_id_t package_id);
fbe_status_t fbe_api_logical_error_injection_enable_object(fbe_object_id_t object_id,
                                                           fbe_package_id_t package_id);
fbe_status_t fbe_api_logical_error_injection_disable_object(fbe_object_id_t object_id,
                                                           fbe_package_id_t package_id);
fbe_status_t fbe_api_logical_error_injection_enable_class(fbe_class_id_t class_id,
                                                           fbe_package_id_t package_id);
fbe_status_t fbe_api_logical_error_injection_disable_class(fbe_class_id_t class_id,
                                                           fbe_package_id_t package_id);
fbe_status_t fbe_api_logical_error_injection_load_table(fbe_u32_t table_index, fbe_bool_t b_simulation);
fbe_status_t fbe_api_logical_error_injection_disable_records(fbe_u32_t start_index,
                                                             fbe_u32_t num_to_clear);
fbe_status_t fbe_api_logical_error_injection_destroy_objects(void);
void fbe_api_logical_error_injection_display_stats(
                    fbe_api_logical_error_injection_get_stats_t stats
                    );
void fbe_api_logical_error_injection_display_obj_stats(
            fbe_api_logical_error_injection_get_object_stats_t obj_stats
            );
void fbe_api_logical_error_injection_error_display(
                    fbe_api_logical_error_injection_get_records_t get_records,
                    fbe_u32_t start,
                    fbe_u32_t end
                    );
void fbe_api_logical_error_injection_table_info_display(
                    fbe_api_logical_error_injection_table_t* get_table_p
                    );
fbe_u32_t fbe_api_lei_string_to_enum(fbe_char_t* compare_str,
                                const fbe_char_t * const str_array[],
                                fbe_u32_t max_str);
fbe_char_t const *const fbe_api_lei_enum_to_string(fbe_u32_t e_numb,
                                const fbe_char_t * const str_array[],
                                fbe_u32_t max_str);
fbe_status_t fbe_api_logical_error_injection_remove_object(fbe_object_id_t object_id,
                                                           fbe_package_id_t package_id);
fbe_status_t fbe_api_logical_error_injection_disable_lba_normalize(void);


/*! @} */ /* end of group fbe_api_logical_error_injection_interface */


//----------------------------------------------------------------
// Define the group for all FBE NEIT Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API NEIT 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_neit_package_interface_class_files FBE NEIT Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE NEIT Package Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------



fbe_status_t 
fbe_api_logical_error_injection_accum_object_stats(fbe_api_logical_error_injection_get_object_stats_t *stats_p,
                                                   fbe_object_id_t object_id);

#endif /*FBE_API_LOGICAL_ERROR_INJECTION_INTERFACE_H*/


