#ifndef FBE_LOGICAL_ERROR_INJECTION_H
#define FBE_LOGICAL_ERROR_INJECTION_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_error_injection.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interface for the logical error injection service.
 *  (formerly rderr).
 *
 * @version
 *   12/21/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_topology_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

typedef enum fbe_logical_error_injection_control_code_e {
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_RDGEN),

    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_POC,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_POC,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_MODIFY_OBJECTS,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_OBJECTS,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_OBJECTS,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_IGNORE_CORRUPT_CRC_DATA_ERRORS,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_IGNORE_CORRUPT_CRC_DATA_ERRORS,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DESTROY_OBJECTS,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_STATS,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_OBJECT_STATS,

    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_CREATE_RECORD,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORDS,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_MODIFY_RECORD,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DELETE_RECORD,

    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_TABLE_INFO,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_ACTIVE_TABLE_INFO,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_LOAD_TABLE,
    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_RECORDS,

    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_OBJECT,

    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_LBA_NORMALIZE,

    FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_LAST
} fbe_logical_error_injection_control_code_t;

/*!*******************************************************************
 * @def LEI_RAW_MIRROR_OBJECT_ID
 *********************************************************************
 * @brief Logical error injection service object ID for raw mirror testing.
 *        Raw mirrors do not use an object interface. Errors are injected
 *        on raw mirror PVDs; however I/O is driven through the raw mirror,
 *        so it needs an object ID to facilitate error injection. This dummy 
 *        object ID is used for this purpose.
 * 
 *********************************************************************/
#define LEI_RAW_MIRROR_OBJECT_ID 0xFFFFFFFF

/*!*******************************************************************
 * @enum fbe_logical_error_injection_filter_type_t
 *********************************************************************
 * @brief
 *  Describes the type of object to inject errors on.
 *********************************************************************/
typedef enum fbe_logical_error_injection_filter_type_e
{
    FBE_LOGICAL_ERROR_INJECTION_FILTER_TYPE_INVALID = 0,

    /*! Run I/O to a specific object.
     */
    FBE_LOGICAL_ERROR_INJECTION_FILTER_TYPE_OBJECT,

    /*! Run I/O to all objects of a given class. 
     */
    FBE_LOGICAL_ERROR_INJECTION_FILTER_TYPE_CLASS,

    FBE_LOGICAL_ERROR_INJECTION_FILTER_TYPE_LAST
}
fbe_logical_error_injection_filter_type_t;

/*!*******************************************************************
 * @struct fbe_logical_error_injection_filter_t
 *********************************************************************
 * @brief
 *  Describes the object to inject on.
 *********************************************************************/
typedef struct fbe_logical_error_injection_filter_s
{
    /*! Determines if we are running to:  
     * all objects, to all objects of a given class or to a specific object. 
     */
    fbe_logical_error_injection_filter_type_t filter_type;

    /*! The object ID of the object to inject for. 
     *  This can be FBE_OBJECT_ID_INVALID, in cases where
     *  we are not injecting on a specific object.
     */
    fbe_object_id_t object_id; 

    /*! The edge of this object to inject on. 
     *  Or FBE_U32_MAX if all edges. 
     */
    fbe_u32_t edge_index;

    /*! This can be FBE_CLASS_ID_INVALID if class is not used.
     */
    fbe_class_id_t class_id;

    /*! This must always be set and is the package we will run I/O against.
     */
    fbe_package_id_t package_id;
}
fbe_logical_error_injection_filter_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_get_stats_t
 ***************************************************
 * @brief statistics information on the lei service.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_STATS.
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_get_stats_s
{
    fbe_bool_t b_enabled; /*!< FBE_TRUE if service is injecting. */

    fbe_u32_t num_objects; /*!< Number of objects we are injecting on. */

    fbe_u32_t num_records; /*!< Number of records active. */

    fbe_u32_t num_objects_enabled; /*< Number of objects injecting errors. */

    fbe_u64_t num_errors_injected; /*< Number of errors we injected. */
    fbe_u64_t correctable_errors_detected; /*!< c_errors encountered by validation.  */
    fbe_u64_t uncorrectable_errors_detected; /*!< u_errors encountered by validation.  */
    fbe_u64_t num_validations;     /*< Number of error validations performed. */
    fbe_u64_t num_failed_validations; /*<! Number of validations that failed. */
}
fbe_logical_error_injection_get_stats_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_get_object_stats_t
 ***************************************************
 * @brief statistics information on a given object.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_OBJECT_STATS.
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_get_object_stats_s
{
    fbe_object_id_t object_id; /*!< Object to get stats on. */

    fbe_u64_t num_read_media_errors_injected;

    /*! Total number of blocks remapped as a result of a write verify command. */
    fbe_u64_t num_write_verify_blocks_remapped; 

    /*! Total number of errors injected on this object. */ 
    fbe_u64_t num_errors_injected;

    /*! Total number of validation calls on this object. */ 
    fbe_u64_t num_validations;
}
fbe_logical_error_injection_get_object_stats_t;


/***************************************************
 * fbe_logical_error_injection_mode
 *  This enum has one bit position for an individual
 *  display mode.
 * 
 *  IMPORTANT: This has to match the definition in the fbe_api.
 *
 ***************************************************/
typedef enum fbe_logical_error_injection_mode_s
{
    FBE_LOGICAL_ERROR_INJECTION_MODE_INVALID = 0x0,
    FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT = 0x1,
    FBE_LOGICAL_ERROR_INJECTION_MODE_ALWAYS = 0x2,
    FBE_LOGICAL_ERROR_INJECTION_MODE_RANDOM = 0x3,
    FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP = 0x4,
    FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT = 0x5,
    FBE_LOGICAL_ERROR_INJECTION_MODE_TRANS = 0x6,
    FBE_LOGICAL_ERROR_INJECTION_MODE_TRANS_RND = 0x7, /* Sometimes transitory, sometimes not. */
    FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED = 0x8, /* Inject once and disable. */
    FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA = 0x9,       /* Inject same LBA */
    FBE_LOGICAL_ERROR_INJECTION_MODE_UNKNOWN = 0xa,
    FBE_LOGICAL_ERROR_INJECTION_MODE_LAST
}
fbe_logical_error_injection_mode_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_get_record_t
 ***************************************************
 * @brief Info to fetch record.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORDS
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_get_record_s
{
    fbe_u64_t record_handle;      /*!< Unique identifier to use for modify/delete. */
    fbe_u16_t pos_bitmap;         /*!< Bitmap for each position to error */
    fbe_u16_t width;              /*!< Array width */
    fbe_u32_t opcode;             /*!< Opcodes to error */
    fbe_lba_t lba;                /*!< Physical address to begin errs    */
    fbe_block_count_t blocks;             /*!< Blocks to extend the errs for     */
    fbe_xor_error_type_t err_type; /*!< Type of errors, see above      */
    fbe_logical_error_injection_mode_t err_mode; /*!< Type of error injection, see above */
    fbe_u16_t err_count;          /*!< Count of errs injected so far     */
    fbe_u16_t err_limit;          /*!< Limit of errors to inject         */
    fbe_u16_t skip_count;         /*!< Limit of errors to skip          */
    fbe_u16_t skip_limit;         /*!< Limit of errors to inject         */
    fbe_u16_t err_adj;            /*!< Error adjacency bitmask           */
    fbe_u16_t start_bit;          /*!< Starting bit of an error          */
    fbe_u16_t num_bits;           /*!< Number of bits of an error        */
    fbe_u16_t bit_adj;            /*!< Whether erroneous bits adjacent   */
    fbe_u16_t crc_det;            /*!< Whether error is CRC detectable   */
}
fbe_logical_error_injection_get_record_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_create_record_t
 ***************************************************
 * @brief Info to create a record.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_CREATE_RECORD
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_create_record_s
{
    fbe_u16_t pos_bitmap;         /*!< Bitmap for each position to error */
    fbe_u16_t width;              /*!< Array width */
    fbe_u32_t opcode;             /*!< Opcodes to error */
    fbe_lba_t lba;                /*!< Physical address to begin errs    */
    fbe_block_count_t blocks;             /*!< Blocks to extend the errs for     */
    fbe_xor_error_type_t err_type; /*!< Type of errors, see above      */
    fbe_logical_error_injection_mode_t err_mode; /*!< Type of error injection, see above */
    fbe_u16_t err_count;          /*!< Count of errs injected so far     */
    fbe_u16_t err_limit;          /*!< Limit of errors to inject         */
    fbe_u16_t skip_count;         /*!< Limit of errors to skip          */
    fbe_u16_t skip_limit;         /*!< Limit of errors to inject         */
    fbe_u16_t err_adj;            /*!< Error adjacency bitmask           */
    fbe_u16_t start_bit;          /*!< Starting bit of an error          */
    fbe_u16_t num_bits;           /*!< Number of bits of an error        */
    fbe_u16_t bit_adj;            /*!< Whether erroneous bits adjacent   */
    fbe_u16_t crc_det;            /*!< Whether error is CRC detectable   */
    fbe_object_id_t object_id;    /*!< Object id to inject on. */
}
fbe_logical_error_injection_create_record_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_load_table_t
 ***************************************************
 * @brief Info to load a new static table.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_LOAD_TABLE
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_load_table_s
{
    fbe_u16_t table_index;         /*!< Table index of static table to load. */
    fbe_bool_t b_simulation;       /*!< FBE_TRUE if simulation. */
}
fbe_logical_error_injection_load_table_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_disable_records_t
 ***************************************************
 * @brief Info to clear out part or all of this table.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_RECORDS
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_disable_records_s
{
    fbe_u16_t start_index;         /*!< Start index to begin clearing. */
    fbe_u16_t count;               /*!< Number of records to clear. */
}
fbe_logical_error_injection_disable_records_t;


/*!*************************************************
 * @typedef fbe_logical_error_injection_remove_record_t
 ***************************************************
 * @brief Info to create a record.
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_remove_record_s
{
    fbe_u64_t record_handle;
}
fbe_logical_error_injection_remove_record_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_modify_record_t
 ***************************************************
 * @brief Info to modify a record.
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_modify_record_s
{
    fbe_u64_t record_handle;
    fbe_logical_error_injection_create_record_t modify_record;
}
fbe_logical_error_injection_modify_record_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_enable_objects_t
 ***************************************************
 * @brief Struct for enabling error injection
 *        on a given object or set of objects.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_OBJECTS
 *
 ***************************************************/
typedef struct fbe_logical_error_injection_enable_objects_s
{
    fbe_logical_error_injection_filter_t filter;
}
fbe_logical_error_injection_enable_objects_t;

/*!*************************************************
 * @typedef fbe_logical_error_injection_disable_objects_t
 ***************************************************
 * @brief Struct for disabling error injection
 *        on a given object or set of objects.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_DISABLE_OBJECTS
 ***************************************************/
typedef struct fbe_logical_error_injection_disable_objects_s
{
    fbe_logical_error_injection_filter_t filter;
}
fbe_logical_error_injection_disable_objects_t;

/*!***************************************************************************
 * @typedef fbe_logical_error_injection_modify_objects_t()
 *****************************************************************************
 * @brief Struct for modifying the edge information etc, for an object
 *        that will have error injection enabled.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ENABLE_OBJECTS
 *****************************************************************************/
typedef struct fbe_logical_error_injection_modify_objects_s
{
    fbe_logical_error_injection_filter_t filter;
    fbe_u32_t                            edge_hook_enable_bitmask;
    fbe_lba_t                            lba_injection_adjustment;
}
fbe_logical_error_injection_modify_objects_t;

/*!********************************************************************* 
 * @struct fbe_logical_error_injection_remove_object_t 
 *  
 * @brief 
 *   Structure used to remove object
 *
 * @ingroup fbe_logical_error_injection_interface
 **********************************************************************/
typedef struct fbe_logical_error_injection_remove_object_s
{
    fbe_object_id_t     object_id;  /*!< Object id the remove all injection for. */
    /*! This must always be set and is the package we will run I/O against.
     */
    fbe_package_id_t    package_id;
}
fbe_logical_error_injection_remove_object_t;

/*!*******************************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS
 *********************************************************************
 * @brief Max number of records this service allows.
 *
 *********************************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS 256

/*!*************************************************
 * @typedef fbe_logical_error_injection_get_records_t
 ***************************************************
 * @brief Structure to fetch records from lei service.
 *        Used with FBE_LOGICAL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORDS.
 ***************************************************/
typedef struct fbe_logical_error_injection_get_records_s
{
    fbe_u32_t num_records; /*! Input to indicate how many records follow.*/
    fbe_logical_error_injection_get_record_t records[FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS];
}
fbe_logical_error_injection_get_records_t;

/*!*******************************************************************
 * @def FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_SECONDS
 *********************************************************************
 * @brief This is the max we will allow an I/O to be delayed for.
 *        The system really is not designed for I/Os to be delayed
 *        for longer than this so we enforce this policy.
 *
 *********************************************************************/
#define FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_SECONDS 45
#define FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_MS (FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_SECONDS * FBE_TIME_MILLISECONDS_PER_SECOND)
#endif /* FBE_LOGICAL_ERROR_INJECTION_H */

/*************************
 * end file fbe_logical_error_injection.h
 *************************/
