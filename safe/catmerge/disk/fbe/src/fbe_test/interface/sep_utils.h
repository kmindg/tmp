#ifndef SEP_UTILS_H
#define SEP_UTILS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sep_utils.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the defines for the SEP utility functions.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe_spare.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_job_service.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_sector_trace_interface.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_common_interface.h"
#include "fbe/fbe_payload_metadata_operation.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "fbe/fbe_base_config.h"
#include "kms_dll.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_neit.h"


/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/

/*!*******************************************************************
 * @def FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE
 *********************************************************************
 * @brief Number of retryable errors to cause a drive to fail.
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *        This number ultimately translates to the number of errors DIEH 
 *        detects before issuing a drive reset, which then causes PDO 
 *        edge to go disabled then broken.  DRIVE_FAULT will be set when 
 *        the 15th error occurs.  DRIVE_FAULT must be cleared to use the 
 *        drive.
 * 
 *********************************************************************/
#define FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE 15

/*!*******************************************************************
 * @def FBE_TEST_NUM_RETRYABLE_TO_DISABLE_DRIVE
 *********************************************************************
 * @brief Number of retryable errors to cause a drive to exit ready and
 *        enter activate (edge goes DISABLED).
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *        This number ultimately translates to the number of errors DIEH 
 *        detects before issuing a drive reset, which then causes PDO 
 *        edge to go disabled.  DRIVE_FAULT will NOT be set.
 * 
 *********************************************************************/
#define FBE_TEST_NUM_RETRYABLE_TO_DISABLE_DRIVE (FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE - 1)

enum {
    /*! Time to wait until we timeout in fbe test. 
     *  This is intentionally long so that if we hit it, something is
     *  definately stuck and it is not a result of many tests running in parallel slowing
     *  things down.
     */
    FBE_TEST_WAIT_TIMEOUT_MS = 120000,
    FBE_TEST_WAIT_TIMEOUT_SECONDS = FBE_TEST_WAIT_TIMEOUT_MS/FBE_TIME_MILLISECONDS_PER_SECOND,
};
typedef struct fbe_test_raid_group_disk_set_s 
{
    fbe_u32_t bus;
    fbe_u32_t enclosure;
    fbe_u32_t slot;
}
fbe_test_raid_group_disk_set_t;

typedef struct fbe_test_packages_s
{
    fbe_bool_t physical;
    fbe_bool_t neit;
    fbe_bool_t sep;
    fbe_bool_t esp;
    fbe_bool_t kms;
}
fbe_test_packages_t;


typedef void (*fbe_test_create_raid_group_fn)(fbe_u32_t *object_number,
                                              fbe_object_id_t *object_id_array,
                                              fbe_u32_t num_luns,
                                              fbe_raid_group_number_t *raid_group_id,
                                              fbe_lun_number_t *lun_number,
                                              fbe_u32_t element_size,
                                              fbe_lba_t virtual_drive_capacity,
                                              fbe_lba_t raid_group_capacity,
                                              fbe_lba_t lun_capacity,
                                              fbe_u32_t width,
                                              fbe_chunk_size_t chunk_size,
                                              fbe_test_raid_group_disk_set_t *disk_set_p);

/* Lun configuration. */
typedef struct fbe_test_logical_unit_configuration_s
{
    fbe_lba_t                                   imported_capacity;
    fbe_lba_t                                   capacity;
    fbe_lun_number_t                            lun_number;
    fbe_raid_group_type_t                       raid_type;
    fbe_raid_group_number_t                     raid_group_id;
    fbe_u32_t                                   total_error_count;
    fbe_u64_t                                   job_number;
}
fbe_test_logical_unit_configuration_t;

/*!*******************************************************************
 * @def FBE_TEST_REMOVED_DRIVE_HISTORY_SIZE
 *********************************************************************
 * @brief The number of drives to store as a removal history for 
 *        a given test config object.
 *********************************************************************/
#define FBE_TEST_REMOVED_DRIVE_HISTORY_SIZE 3


#define FBE_TEST_HOOK_WAIT_MSEC 120 * 1000
#define FBE_TEST_HOOK_POLLING_MSEC 200

/*!*******************************************************************
 * @def FBE_TEST_RG_CONFIG_RANDOM_TAG
 *********************************************************************
 * @brief If this tag is entered for width, raid type, b_bandwidth
 *        then the function fbe_test_sep_util_randomize_rg_configuration_array()
 *        will know to select random values for these fields.
 *
 *********************************************************************/
#define FBE_TEST_RG_CONFIG_RANDOM_TAG FBE_U32_MAX - 1

/*!*******************************************************************
 * @def FBE_TEST_RG_CONFIG_RANDOM_TYPE_PARITY
 *********************************************************************
 * @brief If this tag is entered for raid type
 *        then the function fbe_test_sep_util_randomize_rg_configuration_array()
 *        will know to select a random parity raid group type.
 *
 *********************************************************************/
#define FBE_TEST_RG_CONFIG_RANDOM_TYPE_PARITY FBE_U32_MAX - 2

/*!*******************************************************************
 * @def FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT
 *********************************************************************
 * @brief If this tag is entered for raid type
 *        then the function fbe_test_sep_util_randomize_rg_configuration_array()
 *        will know to select a random redundant raid group type.
 *
 *********************************************************************/
#define FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT FBE_U32_MAX - 3

/*!*******************************************************************
 * @def FBE_TEST_RG_CONFIG_RANDOM_TYPE_SINGLE_REDUNDANT
 *********************************************************************
 * @brief If this tag is entered for raid type
 *        then the function fbe_test_sep_util_randomize_rg_configuration_array()
 *        will know to select a random single redundant raid group type.
 *
 *********************************************************************/
#define FBE_TEST_RG_CONFIG_RANDOM_TYPE_SINGLE_REDUNDANT FBE_U32_MAX - 4

/*!*******************************************************************
 * @def FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE
 *********************************************************************
 * @brief If this tag is entered for raid type
 *        then the function fbe_test_sep_util_randomize_rg_configuration_array()
 *        will know to select mixed 520 and 4160 block size.
 *
 *********************************************************************/
#define FBE_TEST_RG_CONFIG_MIXED_BLOCK_SIZE FBE_U32_MAX - 5

/*!*******************************************************************
 * @struct fbe_test_rg_configuration_t
 *********************************************************************
 * @brief Defines a raid group configuration that can be used by
 *        the test.  This can be accepted by a variety of functions
 *        for creating/destroying raid groups, and running tests.
 * 
 *********************************************************************/
typedef struct fbe_test_rg_configuration_s
{
    /*! Following fields must be populated to create raid group.
     */
    fbe_u32_t                                   width;
    fbe_lba_t                                   capacity;
    fbe_raid_group_type_t                       raid_type; /*! Type of raid group to create. */
    fbe_class_id_t                              class_id; /*! Class of this raid type to test. */
    fbe_block_size_t                            block_size;
    fbe_raid_group_number_t                     raid_group_id;
    fbe_bool_t                                  b_bandwidth;

    /*! Following fields are populated as needed (initialized to zero).
     */
    fbe_test_raid_group_disk_set_t              rg_disk_set[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_api_terminator_device_handle_t          drive_handle[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_api_terminator_device_handle_t          peer_drive_handle[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_test_logical_unit_configuration_t       logical_unit_configuration_list[128];
    fbe_u32_t                                   number_of_luns_per_rg;
    fbe_u64_t                                   job_number;
    fbe_u32_t                                   num_removed; /*! Number of drives removed so far. */
    fbe_u32_t                                   num_needing_spare; /*!< Total drives to spare. */
    /*! Array of drive positions in rg we removed and those that require a spare
     */
    fbe_u32_t                                   drives_removed_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t                                   drives_needing_spare_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t                                   drives_removed_history[FBE_TEST_REMOVED_DRIVE_HISTORY_SIZE];
    fbe_u32_t                                   specific_drives_to_remove[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /*! This is used to store unused drive location info.We can increase the
     size of the array if required.
     */
    fbe_test_raid_group_disk_set_t              unused_disk_set[FBE_RAID_MAX_DISK_ARRAY_WIDTH];                

    /*! This is used to store extra drive location info. 
      */
    fbe_test_raid_group_disk_set_t              extra_disk_set[FBE_RAID_MAX_DISK_ARRAY_WIDTH];                
    fbe_u32_t                                   num_of_extra_drives;

    /*! This is used to save the `source' of a copy operation.
     */
    fbe_test_raid_group_disk_set_t              source_disk;                

    /*! This is FBE_TRUE when we will create this raid group for the current pass.
     */
    fbe_u32_t                                   b_create_this_pass;
    /*! We set this to FBE_FALSE if there are not enough drives to create this config.
     */
    fbe_u32_t                                   b_valid_for_physical_config;
    /*! We set this to FBE_TRUE when we have already tested with this config.
     */
    fbe_u32_t                                   b_already_tested;
    fbe_bool_t                                  b_use_fixed_disks; /*!< Tells test to use drives provided. */
    fbe_u32_t                                   magic; /*!< Tells us we have been initted. */
    fbe_u32_t                                   drive_type;
    fbe_chunk_size_t                            extra_chunk; /*!< extra chunk that is not consumed by the LUNs */
    fbe_u32_t                                   test_case_bitmask;
}
fbe_test_rg_configuration_t;

typedef void(*fbe_test_rg_config_test)(fbe_test_rg_configuration_t *rg_config_p, void * context_p);

/*!*******************************************************************
 * @struct fbe_test_create_raid_group_params_t
 *********************************************************************
 * @brief struct to pass in when creating a rg.
 *
 *********************************************************************/
typedef struct fbe_test_create_raid_group_params_s
{
    void * context_p;
    fbe_test_rg_config_test test_fn;
    fbe_u32_t luns_per_rg;
    fbe_u32_t chunks_per_lun;
    fbe_bool_t destroy_rg_config;
    fbe_bool_t b_encrypted;
    fbe_bool_t b_skip_create;
}
fbe_test_create_raid_group_params_t;

/*!*******************************************************************
 * @struct fbe_test_logical_error_table_info_t
 *********************************************************************
 * @brief Defines a set of test parameters for a logical error test.
 *
 *********************************************************************/
typedef struct fbe_test_logical_error_table_info_s
{
    fbe_u32_t number_of_errors; /*< Number of coincident errors to inject. */

    /*! FBE_TRUE if this table injects randomly so either correctable or uncorrectable 
     *  errors are possible.
     */
    fbe_bool_t b_random_errors; 
    fbe_u32_t table_number;
    fbe_bool_t b_raid6_only; /*!< This table only applies to Raid 6. */
    fbe_u32_t record_count;
}
fbe_test_logical_error_table_info_t;

/*!*******************************************************************
 * @def FBE_TEST_RG_CONFIGURATION_MAGIC
 *********************************************************************
 * @brief Tells us if this fbe_test_rg_configuration_t has been
 *        initialized yet.
 *
 *********************************************************************/
#define FBE_TEST_RG_CONFIGURATION_MAGIC 0xFBE12345

typedef struct fbe_test_hs_configuration_s
{
    fbe_test_raid_group_disk_set_t              disk_set;
    fbe_block_size_t                            block_size;
    fbe_object_id_t                             hs_pvd_object_id;
    fbe_api_terminator_device_handle_t          drive_handle;
}
fbe_test_hs_configuration_t;

#define FBE_TEST_MAX_RAID_GROUP_COUNT 20
#define FBE_TEST_MAX_TABLES 30

/*!*******************************************************************
 * @def FBE_TEST_INIT_INVALID_EXTRA_CHUNK
 *********************************************************************
 * @brief
 *   Tells us about the initial or invalid value for extra chunk that
 *   not used by the LUNs.
 *
 *********************************************************************/
#define FBE_TEST_INIT_INVALID_EXTRA_CHUNK  0

typedef struct fbe_test_logical_error_table_test_configuration_s
{
    fbe_test_rg_configuration_t raid_group_config[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_u32_t error_table_num[FBE_TEST_MAX_TABLES];
    fbe_u32_t num_drives_to_power_down[FBE_TEST_MAX_TABLES];
}
fbe_test_logical_error_table_test_configuration_t;

/*!*******************************************************************
 * @struct fbe_test_config_for_io_test_t
 *********************************************************************
 * @brief Definition of a configuration for an I/O test.
 *
 *********************************************************************/
typedef struct fbe_test_config_for_io_test_s
{
    fbe_test_rg_configuration_t *rg_config_p; /*< Raid group configuration. */
    fbe_u32_t rg_count;  /*!< Number of raid groups. */
    fbe_u32_t *element_size_p; /*!< Array of element sizes. */
    fbe_u32_t num_element_sizes; /*!< Number of element sizes to test. */
    fbe_u32_t luns_per_rg;  /*!< Number of luns to configure per rg. */
}
fbe_test_config_for_io_test_t;
/*!*******************************************************************
 * @enum fbe_test_block_size_t
 *********************************************************************
 * @brief Enum of block sizes we will test with.
 *
 *********************************************************************/
typedef enum fbe_test_block_size_e
{
    FBE_TEST_BLOCK_SIZE_INVALID = 0,
    FBE_TEST_BLOCK_SIZE_520,
    FBE_TEST_BLOCK_SIZE_512,
    FBE_TEST_BLOCK_SIZE_4096,
    FBE_TEST_BLOCK_SIZE_4160,
    FBE_TEST_BLOCK_SIZE_LAST,
}
fbe_test_block_size_t;

/*!*************************************************
 * @enum fbe_test_protocol_error_type_t
 ***************************************************
 * @brief This defines classes of errors which we use
 *        to map to a physical error type depending on
 *        the protocol being used (scsi/sata).
 *
 ***************************************************/
typedef enum fbe_test_protocol_error_type_e
{
    FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID = 0,
    FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
    FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE,
    FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
    FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
    FBE_TEST_PROTOCOL_ERROR_TYPE_SOFT_MEDIA_ERROR,
    FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_BAD_KEY_HANDLE,
    FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_KEY_WRAP_ERROR,
    FBE_TEST_PROTOCOL_ERROR_TYPE_ENCRYPT_NOT_ENABLED,
    FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
    FBE_TEST_PROTOCOL_ERROR_TYPE_LAST,
}
fbe_test_protocol_error_type_t;

/*!*******************************************************************
 * @struct fbe_test_drive_count_t
 *********************************************************************
 * @brief Array of drive counts for various block sizes.
 *
 *********************************************************************/
typedef struct fbe_test_drive_count_s
{
    /*! Number of drives for this given block size.
     */
    fbe_u32_t block_size_count[FBE_TEST_BLOCK_SIZE_LAST];
}
fbe_test_drive_count_t;

/*!*******************************************************************
 * @def FBE_TEST_MAX_DRIVE_LOCATIONS
 *********************************************************************
 * @brief This is the max number of drive locations we will keep track of.
 *
 *********************************************************************/
#define FBE_TEST_MAX_DRIVE_LOCATIONS 200

/*!*******************************************************************
 * @struct fbe_test_discovered_drive_locations_t
 *********************************************************************
 * @brief Array describing the locations of drives in the
 *        physical configuration.  
 * 
 *
 *********************************************************************/
typedef struct fbe_test_discovered_drive_locations_s
{
    /*! Tells us how many drives are being used in the below disk_set 
     *  per block size. 
     */
    fbe_u32_t drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    /*! Filled in with the actual locations of the disks of various 
     * block sizes and disk types in our physical config. 
     */
    fbe_test_raid_group_disk_set_t disk_set[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST][FBE_TEST_MAX_DRIVE_LOCATIONS];
    /*! Drive capacity for every drive 
     */
     //@todo: enable for herry_monster
    //fbe_block_count_t drive_capacity[FBE_TEST_BLOCK_SIZE_LAST][FBE_TEST_MAX_DRIVE_LOCATIONS];
}
fbe_test_discovered_drive_locations_t;

/*!*******************************************************************
 * @struct fbe_test_drive_removed_info_t
 *********************************************************************
 * @brief Structure to track info on drives we have removed for
 *        a given raid group.
 *
 *********************************************************************/
typedef struct fbe_test_drive_removed_info_s
{
    fbe_u32_t num_removed; /*! Number of drives removed so far. */
    /*! Array of drive positions in rg we removed. 
     */
    fbe_u32_t drives_removed_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t num_needing_spare; /*!< Total drives to spare. */
}
fbe_test_drive_removed_info_t;
/*!*******************************************************************
 * @enum fbe_test_drive_removal_mode_e
 *********************************************************************
 * @brief Modes of drive removal.
 *
 *********************************************************************/
typedef enum fbe_test_drive_removal_mode_e
{
    FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL,
    FBE_DRIVE_REMOVAL_MODE_RANDOM,
    FBE_DRIVE_REMOVAL_MODE_SPECIFIC,
} fbe_test_drive_removal_mode_t;

/*!*******************************************************************
 * @def FBE_TEST_EVENT_LOG_TEST_MAX_ERROR_RECORDS
 *********************************************************************
 * @brief Max number of error records for each test case.
 *
 *********************************************************************/
#define FBE_TEST_EVENT_LOG_TEST_MAX_ERROR_RECORDS 16

/*!*******************************************************************
 * @def FBE_TEST_EVENT_LOG_TEST_MAX_EVENT_LOG_MESSAGE_PER_ERROR
 *********************************************************************
 * @brief Maximum number of event log messages expected for any type
 *        of injected error for any given raid group.
 *
 *********************************************************************/
#define FBE_TEST_EVENT_LOG_TEST_MAX_EVENT_LOG_MESSAGE_PER_ERROR 25



/*!*******************************************************************
 * @def FBE_TEST_EVENT_LOG_TEST_DESCRIPTION_MAX_CHARS
 *********************************************************************
 * @brief Maximum no of characters a test group description can have.
 *
 *********************************************************************/
#define FBE_TEST_EVENT_LOG_TEST_DESCRIPTION_MAX_CHARS 128



/*!*******************************************************************
 * @def FBE_TEST_EVENT_LOG_TEST_MAX_TEST_CASE_PER_GROUP
 *********************************************************************
 * @brief Maximum number of test cases a test-group can have
 *
 *********************************************************************/
#define FBE_TEST_EVENT_LOG_TEST_MAX_TEST_CASE_PER_GROUP 64




/*!*******************************************************************
 * @def FBE_TEST_EVENT_LOG_TEST_MAX_TEST_GROUPS
 *********************************************************************
 * @brief Max number of test groups we can have.
 *
 *********************************************************************/
#define FBE_TEST_EVENT_LOG_TEST_MAX_TEST_GROUPS  10


/*!*******************************************************************
 * @def FBE_TEST_EVENT_LOG_MAX_TEST_RECORD
 *********************************************************************
 * @brief Max number of test records a test cases can have
 *
 *********************************************************************/
#define FBE_TEST_EVENT_LOG_MAX_TEST_RECORD  5

/*!*******************************************************************
 * @def FBE_TEST_EVENT_LOG_TEST_MAX_ERROR_RECORDS
 *********************************************************************
 * @brief Max number of I/O specification a test case will use.
 *
 *********************************************************************/
#define FBE_TEST_EVENT_LOG_TEST_MAX_IO_SPEC_RECORDS  10

#define FBE_TEST_SEP_UTIL_INVALID_POSITION ((fbe_u32_t) -1)


/*!***************************************************************
 * @fn fbe_test_event_log_test_validate_rdgen_result_t
 * 
 * @brief This function validates rdgen operation statistics
 *
 ******************************************************************/
typedef fbe_status_t (* fbe_test_event_log_test_validate_rdgen_result_t)(fbe_status_t rdgen_request_status,
                                                                         fbe_rdgen_operation_status_t rdgen_op_status,
                                                                         fbe_rdgen_statistics_t *rdgen_stats_p);


/*!*******************************************************************
 * @struct fbe_test_event_log_test_log_detail_per_fru_t
 *********************************************************************
 * @brief Defines a set of result parameter expected in single event 
 *        log message reported for a given injected errror.
 *
 *********************************************************************/
typedef struct fbe_test_event_log_test_log_detail_per_fru_s
{
    fbe_u32_t fru_pos;       /*!< the fru position (against reported error) in event log */
    fbe_u32_t error_code;    /*!< the expected error code in event log */
    fbe_u32_t error_info;    /*!< the expected error type in event log */
    fbe_lba_t error_lba;     /*!< the expected lba in event log message */
    fbe_u32_t error_blocks;  /*!< the expected lba in event log message */
}
fbe_test_event_log_test_log_detail_per_fru_t;



/*!*******************************************************************
 * @struct fbe_test_event_log_test_result_t
 *********************************************************************
 * @brief Defines expected event-log message (which can be more than
 *        one) for a given injected error.
 *
 *********************************************************************/
typedef struct fbe_test_event_log_test_result_s
{
    /*!< number of expected event log message 
     */
    fbe_u32_t num_msgs; 

    /*!< expected details of event log records 
     */
    fbe_test_event_log_test_log_detail_per_fru_t event_validation_record[FBE_TEST_EVENT_LOG_TEST_MAX_EVENT_LOG_MESSAGE_PER_ERROR]; 
                        
}
fbe_test_event_log_test_result_t;


/*!********************************************************************* 
 * @struct Describes parameters of injected logical-error 
 *
 **********************************************************************/
typedef struct fbe_test_event_log_test_error_record_s
{
    /*!< Bitmap for each position to inject error 
     */
    fbe_u16_t pos_bitmap;         

    /*!< Physical address to begin errors    
     */
    fbe_lba_t lba;                

    /*!< Blocks to extend the errors for     
     */
    fbe_u32_t blocks;

    /*!< Type of errors 
     */
    fbe_api_logical_error_injection_type_t err_type; 

    /*!< Number of times error not to be injected. If it is 0 
     * then error is error injected always. 
     */
    fbe_u32_t err_limit;           
}
fbe_test_event_log_test_error_spec_t;



/*!*******************************************************************
 * @struct fbe_test_event_log_test_injected_error_info_t
 *********************************************************************
 * @brief Defines set of parameters used to inject error 
 *
 *********************************************************************/
typedef struct fbe_test_event_log_test_injected_error_record_s
{
    /*!< Number of injected errors 
     */
    fbe_u32_t injected_error_count; 

    /*!< Details of injected error records 
     */
    fbe_test_event_log_test_error_spec_t error_list[FBE_TEST_EVENT_LOG_TEST_MAX_ERROR_RECORDS];  
}
fbe_test_event_log_test_injected_error_info_t;


/*!*******************************************************************
 * @struct fbe_test_event_log_test_io_spec_t
 *********************************************************************
 * @brief Defines a set of parameters used in rdgen I/O specification
 *        to produce the error.
 *
 *********************************************************************/
typedef struct fbe_test_event_log_test_io_spec_s
{
    /*!< Opcode of I/O 
     */
    fbe_u32_t opcode;         

    /*!< Start lba of I/O 
     */
    fbe_lba_t start_lba;      

    /*!< Number of blocks of I/O 
     */
    fbe_block_count_t blocks; 

    /*!< Flag to determine whether or not to align errors
     * writes and recovery verify expands range, but degraded
     * parity read does not
     */
    fbe_bool_t do_not_align_errors;
}
fbe_test_event_log_test_io_spec_t;



/*!*******************************************************************
 * @struct fbe_test_event_log_test_record_s
 *********************************************************************
 * @brief Defines set of related parameter which is required to
 *        provoke occurence of event log message and validate those
 *        with expected one.
 * 
 *  This strucutre specifies following three different set of 
 *  parameters used in validation of event log messages:
 *      1. I/O specification
 *      2. Error specification
 *      3. Expected event log message 
 *
 *  Also, it specifies a pointer to function which will be used to 
 *  validate rdgen statistics after performing rdgen operation.
 *
 *********************************************************************/
typedef struct fbe_test_event_log_test_record_s
{
    /*!< Pointer to function used to validate rdgen statisitcis
     */
    fbe_test_event_log_test_validate_rdgen_result_t validate_rdgen_result_fn_p;


    /*!< specifices I/O list used to cause invalid pattern
     */
    fbe_test_event_log_test_io_spec_t io_spec_list[FBE_TEST_EVENT_LOG_TEST_MAX_IO_SPEC_RECORDS];

    /*!< Details of injected error records 
     */
    fbe_test_event_log_test_injected_error_info_t injected_errors;

    /*!< Expected event log message for injected error(s)
     */ 
    fbe_test_event_log_test_result_t event_log_result;
}
fbe_test_event_log_test_record_t;


/*!*******************************************************************
 * @struct fbe_test_event_log_test_case_t
 *********************************************************************
 * @brief Defines a set of parameters used to define the test case.
 *
 *********************************************************************/
typedef struct fbe_test_event_log_test_case_s
{
    /*!< Description of test  
     */
    fbe_u8_t description[FBE_TEST_EVENT_LOG_TEST_DESCRIPTION_MAX_CHARS];

    /*!< Width of the raid group 
     */
    fbe_u32_t width;

    /*!< Count of test records
     */
    fbe_u32_t record_count;

    /*! Array of test records. In some case we may need more than one.
     *  For example, if we want to create an invalid sectors on strip
     *  and later on we want to validate occurence of event log 
     *  messages in association of other errors (like CRC), we will 
     *  need two test record.
     */
    fbe_test_event_log_test_record_t test_records[FBE_TEST_EVENT_LOG_MAX_TEST_RECORD];

}
fbe_test_event_log_test_case_t;


/*!***************************************************************
 * @fn fbe_test_event_log_test_run_single_test_case_t
 * 
 * @brief The function used to validate rdgen result
 *
 ******************************************************************/
typedef fbe_status_t (* fbe_test_event_log_test_run_single_test_case_t)(fbe_test_rg_configuration_t * raid_group_p,
                                                                        fbe_test_event_log_test_case_t * test_case_p);





/*!*******************************************************************
 * @struct fbe_test_event_log_test_group_t
 *********************************************************************
 * @brief Defines parametrs related to test group. A test group is
 *        group a bunch of similar test cases. 
 *
 *********************************************************************/
typedef struct fbe_test_event_log_test_group_s
{
    /*!< Description of test group 
     */
    fbe_u8_t description[FBE_TEST_EVENT_LOG_TEST_DESCRIPTION_MAX_CHARS];

    /*!< Pointer to function used to run test cases of group.
     */
    fbe_test_event_log_test_run_single_test_case_t execute_single_test_case_fn_p;

    /*!< test cases belonging to the group
     */
    fbe_test_event_log_test_case_t test_case_list[FBE_TEST_EVENT_LOG_TEST_MAX_TEST_CASE_PER_GROUP];
}
fbe_test_event_log_test_group_t;



/*!*******************************************************************
 * @struct fbe_test_event_log_testtest_group_table_t
 *********************************************************************
 * @brief This contains a table of test groups
 *
 *********************************************************************/
typedef struct fbe_test_event_log_test_group_table_s
{
    fbe_test_event_log_test_group_t *table[FBE_TEST_EVENT_LOG_TEST_MAX_TEST_GROUPS];
}
fbe_test_event_log_test_group_table_t;


/*!*******************************************************************
 * @struct fbe_test_event_log_test_configuration_t
 *********************************************************************
 * @brief Defines configuration attributes of raid group
 *
 *********************************************************************/
typedef struct fbe_test_event_log_test_configuration_s
{
     /*!< raid group configuration 
      */
    fbe_test_rg_configuration_t raid_group_config[FBE_TEST_MAX_RAID_GROUP_COUNT];

    /*!< Information of drives to be removed
     */
    fbe_test_drive_removed_info_t drives_to_remove; 

    /*!< test group containing test cases.
     */
    fbe_test_event_log_test_group_table_t test_group_table;
}
fbe_test_event_log_test_configuration_t;

#define FBE_TEST_EXTRA_CAPACITY_FOR_METADATA 0x100000 /*Extra blocks for objects metadata */

/*!*******************************************************************
 * @def FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE 20

/*!*******************************************************************
 * @def FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT 20

typedef fbe_test_rg_configuration_t fbe_test_rg_configuration_array_t[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT];

typedef enum fbe_test_sep_rg_config_type_e{
    FBE_TEST_SEP_RG_CONFIG_TYPE_ONE_R5,
    FBE_TEST_SEP_RG_CONFIG_TYPE_ONE_OF_EACH
}fbe_test_sep_rg_config_type_t;

typedef struct fbe_test_sep_util_hot_spare_dummy_info_s{
    fbe_u32_t           magic_num;
    fbe_object_id_t     object_id;
    fbe_u32_t           port;
    fbe_u32_t           encl;
    fbe_u32_t           slot;
}fbe_test_sep_util_hot_spare_dummy_info_t;

extern fbe_bool_t is_esp_loaded;

/************************************************************
 *  FUNCTION PROTOTYPES
 ************************************************************/

fbe_u32_t fbe_test_sep_util_get_io_seconds(void);

fbe_status_t
fbe_test_sep_util_wait_for_database_service(fbe_u32_t timeout_ms);
fbe_status_t fbe_test_sep_util_wait_for_database_service_ready(fbe_u32_t timeout_ms);

fbe_status_t fbe_test_sep_util_wait_for_ica_stamp_generation(fbe_u32_t timeout_ms);


fbe_status_t fbe_test_sep_util_create_raid0(fbe_database_transaction_id_t transaction_id,
                                            fbe_object_id_t *object_id,
                                            fbe_raid_group_number_t *raid_group_id,
                                            fbe_lba_t capacity,
                                            fbe_u32_t width,
                                            fbe_u32_t element_size,
                                            fbe_chunk_size_t raid_group_chunk_size,
                                            fbe_raid_group_debug_flags_t debug_flags);

fbe_status_t fbe_test_sep_util_create_raid1(fbe_database_transaction_id_t transaction_id,
                                            fbe_object_id_t *object_id,
                                            fbe_raid_group_number_t *raid_group_id,
                                            fbe_lba_t capacity,
                                            fbe_u32_t width,
                                            fbe_u32_t element_size,
                                            fbe_chunk_size_t raid_group_chunk_size,
                                            fbe_raid_group_debug_flags_t debug_flags);

fbe_status_t fbe_test_sep_util_create_raid5(fbe_database_transaction_id_t transaction_id,
                                            fbe_object_id_t *object_id,
                                            fbe_raid_group_number_t *raid_group_id,
                                            fbe_lba_t capacity,
                                            fbe_u32_t width,
                                            fbe_u32_t element_size,
                                            fbe_u32_t elements_per_parity,
                                            fbe_chunk_size_t raid_group_chunk_size,
                                            fbe_raid_group_debug_flags_t debug_flags);

fbe_status_t fbe_test_sep_util_create_raid3(fbe_database_transaction_id_t transaction_id,
                                            fbe_object_id_t *object_id,
                                            fbe_raid_group_number_t *raid_group_id,
                                            fbe_lba_t capacity,
                                            fbe_u32_t width,
                                            fbe_u32_t element_size,
                                            fbe_u32_t elements_per_parity,
                                            fbe_chunk_size_t raid_group_chunk_size,
                                            fbe_raid_group_debug_flags_t debug_flags);

fbe_status_t fbe_test_sep_util_create_raid6(fbe_database_transaction_id_t transaction_id,
                                            fbe_object_id_t *object_id,
                                            fbe_raid_group_number_t *raid_group_id,
                                            fbe_lba_t capacity,
                                            fbe_u32_t width,
                                            fbe_u32_t element_size,
                                            fbe_u32_t elements_per_parity,
                                            fbe_chunk_size_t raid_group_chunk_size,
                                            fbe_raid_group_debug_flags_t debug_flags);

fbe_status_t fbe_test_sep_util_create_lun(fbe_database_transaction_id_t transaction_id,
                                          fbe_object_id_t *object_id,
                                          fbe_lun_number_t *lun_number,
                                          fbe_lba_t capacity);

fbe_status_t fbe_test_sep_util_get_virtual_drive_spare_info(fbe_object_id_t vd_object_id, 
                                                            fbe_spare_drive_info_t* spare_drive_info_p);

fbe_status_t fbe_test_sep_util_get_virtual_drive_object_id_by_position(fbe_object_id_t rg_object_id,
                                                                       fbe_u32_t position,
                                                                       fbe_object_id_t * vd_object_id_p);
void fbe_test_sep_util_get_ds_object_list(fbe_object_id_t object_id,
                                          fbe_api_base_config_downstream_object_list_t *ds_list_p);
fbe_status_t fbe_test_sep_util_quiesce_all_raid_groups(fbe_class_id_t class_id);
fbe_status_t fbe_test_sep_util_unquiesce_all_raid_groups(fbe_class_id_t class_id);
void fbe_test_sep_util_get_raid_group_disk_info(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_sep_util_raid_group_refresh_disk_info(fbe_test_rg_configuration_t * rg_config_p, 
                                                    fbe_u32_t raid_group_count);
void fbe_test_sep_util_raid_group_refresh_extra_disk_info(fbe_test_rg_configuration_t * rg_config_p, 
                                                    fbe_u32_t raid_group_count);
fbe_status_t fbe_test_sep_util_lun_initiate_verify(fbe_object_id_t in_lun_object_id, 
                                                   fbe_lun_verify_type_t in_verify_type,
                                                   fbe_bool_t in_b_verify_entire_lun,
                                                   fbe_lba_t in_start_lba,
                                                   fbe_block_count_t in_blocks);
fbe_status_t fbe_test_util_initiate_verify_on_specific_area(fbe_object_id_t in_raid_object_id, 
                                                            fbe_raid_verify_type_t in_verify_type,
                                                            fbe_lba_t  verify_lba,
                                                            fbe_lba_t  block_count);
fbe_status_t fbe_test_verify_start_on_all_rg_luns(fbe_test_rg_configuration_t *rg_config_p, 
                                                  fbe_lun_verify_type_t in_verify_type);
fbe_status_t fbe_test_sep_util_raid_group_initiate_verify(fbe_object_id_t in_raid_object_id, 
                                                         fbe_lun_verify_type_t in_verify_type);

fbe_status_t fbe_test_sep_util_lun_get_verify_report(fbe_object_id_t in_lun_object_id, 
                                    fbe_lun_verify_report_t* out_verify_report_p);
fbe_status_t fbe_test_sep_util_lun_get_verify_status(fbe_object_id_t in_lun_object_id, 
                                    fbe_lun_get_verify_status_t* out_verify_status_p);
fbe_status_t fbe_test_sep_util_lun_clear_verify_report(fbe_object_id_t in_lun_object_id);
fbe_status_t fbe_test_sep_util_lun_config_clear_total_error_count(fbe_test_logical_unit_configuration_t *logical_unit_configuration_p);
fbe_status_t fbe_test_sep_util_lun_config_increment_total_error_count(fbe_test_logical_unit_configuration_t *logical_unit_configuration_p,
                                                                      fbe_u32_t pass_count,
                                                                      fbe_u32_t new_errors_to_add);

void fbe_test_sep_util_wait_for_lun_verify_completion(fbe_object_id_t            object_id,
                                                      fbe_lun_verify_report_t*   verify_report_p,
                                                      fbe_u32_t                  pass_count);

void fbe_test_sep_util_wait_for_rg_metada_verify_completion(fbe_object_id_t            rg_object_id,
                                                            fbe_u32_t                  pass_count);

fbe_status_t fbe_test_sep_util_lookup_bvd(fbe_object_id_t* bvd_object_id_p);
                                              
fbe_status_t fbe_test_sep_util_get_raid_type_string(fbe_raid_group_type_t raid_group_type,
                                                    const fbe_char_t **string_p);
fbe_status_t fbe_test_sep_util_get_block_size_string(fbe_block_size_t block_size_enum,
                                                     const fbe_char_t **string_p);
void fbe_test_sep_util_wait_for_all_objects_ready(fbe_test_rg_configuration_t *rg_config_p);

void fbe_test_sep_util_get_raid_group_object_id(fbe_test_rg_configuration_t * rg_config_p,
                                                fbe_object_id_t *object_id_p);

void fbe_test_sep_util_get_lun_object_id(fbe_object_id_t first_object_id,
                                         fbe_u32_t width,
                                         fbe_object_id_t *object_id_p);
fbe_status_t fbe_test_sep_util_get_raid_class_info(fbe_raid_group_type_t raid_type,
                                                   fbe_class_id_t class_id,
                                                   fbe_u32_t width,
                                                   fbe_lba_t capacity,
                                                   fbe_u16_t *data_disks_p,
                                                   fbe_lba_t *capacity_p);

fbe_status_t fbe_test_sep_util_get_next_disk_in_set(fbe_u32_t disk_set_index,
                                                    fbe_test_raid_group_disk_set_t *base_disk_set_p,
                                                    fbe_test_raid_group_disk_set_t **disk_set_p);
fbe_status_t fbe_test_sep_util_setup_rg_config(fbe_test_rg_configuration_t * rg_config_p,
                                               fbe_test_discovered_drive_locations_t *drive_locations_p);
void fbe_test_sep_util_discover_all_drive_information(fbe_test_discovered_drive_locations_t *drive_locations_p);

void fbe_test_sep_util_discover_all_power_save_capable_drive_information(fbe_test_discovered_drive_locations_t *drive_locations_p);


fbe_status_t fbe_test_get_520_drive_location(fbe_test_discovered_drive_locations_t *drive_locations_p, 
                                             fbe_test_raid_group_disk_set_t * disk_set);

fbe_status_t fbe_test_get_4160_drive_location(fbe_test_discovered_drive_locations_t *drive_locations_p, 
                                             fbe_test_raid_group_disk_set_t * disk_set);

void fbe_test_sep_util_get_all_unused_pvd_location(fbe_test_discovered_drive_locations_t *drive_locations_p, fbe_test_discovered_drive_locations_t *unused_pvd_locations_p);
fbe_status_t fbe_test_sep_util_get_any_unused_pvd_object_id(fbe_object_id_t *unused_pvd_object_id);
fbe_bool_t fbe_test_rg_config_is_enabled(fbe_test_rg_configuration_t * rg_config_p);
fbe_status_t fbe_test_sep_util_report_created_rgs(fbe_test_rg_configuration_t * rg_config_p);
void fbe_test_check_launch_cli(void);
void fbe_test_run_test_on_rg_config(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun);
void fbe_test_run_test_on_rg_config_with_time_save(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun,
                                    fbe_bool_t destroy_rg_config);
void fbe_test_run_test_on_rg_config_encrypted(fbe_test_rg_configuration_t *rg_config_p,
                                              void * context_p,
                                              fbe_test_rg_config_test test_fn,
                                              fbe_u32_t luns_per_rg,
                                              fbe_u32_t chunks_per_lun);
void fbe_test_run_test_on_rg_config_encrypted_with_time_save(fbe_test_rg_configuration_t *rg_config_p,
                                                             void * context_p,
                                                             fbe_test_rg_config_test test_fn,
                                                             fbe_u32_t luns_per_rg,
                                                             fbe_u32_t chunks_per_lun,
                                                             fbe_bool_t destroy_rg_config);
void fbe_test_set_case_bitmask(fbe_u32_t bitmask);
void fbe_test_run_test_on_rg_config_params(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_test_create_raid_group_params_t *params_p);
void fbe_test_create_raid_group_params_init(fbe_test_create_raid_group_params_t *params_p);
void fbe_test_create_raid_group_params_init_for_time_save(fbe_test_create_raid_group_params_t *params_p,
                                                          void * context_p,
                                                          fbe_test_rg_config_test test_fn,
                                                          fbe_u32_t luns_per_rg,
                                                          fbe_u32_t chunks_per_lun,
                                                          fbe_bool_t b_no_destroy);
void fbe_test_run_test_on_rg_config_with_extra_disk(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun);
void fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun,
                                    fbe_bool_t destroy_rg_config);


void fbe_test_run_test_on_rg_config_no_create(fbe_test_rg_configuration_t *rg_config_p,
                                              void * context_p,
                                              fbe_test_rg_config_test test_fn);
fbe_status_t fbe_test_sep_util_get_pdo_location_by_object_id(fbe_object_id_t pdo_object_id,
                                                             fbe_port_number_t *port_p,
                                                             fbe_enclosure_number_t *enc_p,
                                                             fbe_enclosure_slot_number_t *slot_p);
fbe_status_t fbe_test_sep_util_get_upstream_object_count(fbe_object_id_t object_id, 
                                                         fbe_u32_t *number_of_upstream_edges);
fbe_status_t fbe_test_sep_util_get_block_size_enum(fbe_block_size_t block_size,
                                                   fbe_test_block_size_t *block_size_enum_p);
fbe_status_t fbe_test_sep_util_fill_raid_group_disk_set(fbe_test_rg_configuration_t * raid_configuration_p, fbe_u32_t raid_group_count,
                                                        fbe_test_raid_group_disk_set_t * disk_set_520_p, fbe_test_raid_group_disk_set_t * disk_set_4160_p);
fbe_status_t fbe_test_sep_util_discover_raid_group_disk_set(fbe_test_rg_configuration_t * raid_configuration_list_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_test_discovered_drive_locations_t *drive_locations_p);
fbe_status_t fbe_test_sep_util_discover_raid_group_disk_set_with_capacity(fbe_test_rg_configuration_t * raid_configuration_list_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_test_discovered_drive_locations_t *drive_locations_p);

void fbe_test_get_vd_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_object_id_t *vd_object_ids_p,
                                fbe_u32_t rg_position);

void fbe_test_get_pvd_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                 fbe_object_id_t *pvd_object_ids_p,
                                 fbe_u32_t rg_position);

void fbe_test_get_rg_ds_object_id(fbe_test_rg_configuration_t *rg_config_p,
                                  fbe_object_id_t *rg_object_id_p,
                                  fbe_u32_t mirror_index);

void fbe_test_get_rg_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_object_id_t *rg_object_ids_p);

void fbe_test_get_top_rg_object_ids(fbe_test_rg_configuration_t *rg_config_p,
                                     fbe_object_id_t *rg_object_ids_p);
/*
* @todo: herry_monster: enable for herry_monster
fbe_status_t fbe_test_sep_util_fill_rg_disks( fbe_test_rg_configuration_t *raid_confg_list_p,
                                                                               fbe_u32_t rg_index,
                                                                               fbe_u32_t * disk_set_index,
                                                                               fbe_block_count_t rg_drive_capacity, 
                                                                               fbe_test_raid_group_disk_set_t* in_disk_set_p,
                                                                               fbe_block_count_t *in_drive_cap);

fbe_status_t fbe_test_sep_util_fill_raid_group_disk_set_with_cap(fbe_test_rg_configuration_t * raid_configuration_list_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_test_drive_locations_t *drive_locations_p);
*/

fbe_status_t fbe_test_sep_util_fill_logical_unit_configuration(fbe_test_logical_unit_configuration_t * logical_unit_configuration_p,
                                                               fbe_raid_group_number_t raid_group_id, 
                                                               fbe_raid_group_type_t raid_type,
                                                               fbe_lun_number_t lun_number,
                                                  fbe_lba_t lun_capacity,
                                                  fbe_lba_t imported_capacity);
void fbe_test_sep_util_fill_lun_configurations_rounded(fbe_test_rg_configuration_t *rg_config_p,
                                                       fbe_u32_t raid_group_count, 
                                                       fbe_u32_t chunks_per_lun,
                                                       fbe_u32_t luns_per_rg);
fbe_u32_t fbe_test_sep_util_increase_lun_chunks_for_config(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t in_chunks_per_lun,
                                                           fbe_u32_t data_disks);
fbe_status_t fbe_test_sep_util_generate_encryption_key(fbe_object_id_t object_id,
                                                       fbe_database_control_setup_encryption_key_t *key_info);
fbe_status_t fbe_test_sep_util_generate_invalid_encryption_key(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_object_id_t rg_object_id,
                                                               fbe_database_control_setup_encryption_key_t *key_info);
fbe_status_t fbe_test_sep_util_invalidate_key(fbe_object_id_t object_id,
                                              fbe_database_control_setup_encryption_key_t *key_info,
                                              fbe_u32_t key_index,
                                              fbe_encryption_key_mask_t mask);
fbe_status_t fbe_test_sep_util_fill_raid_group_lun_configuration(fbe_test_rg_configuration_t * raid_group_configuration_p,
                                                                 fbe_u32_t raid_group_count,
                                                                 fbe_lun_number_t lun_number,
                                                                 fbe_u32_t logical_unit_count,
                                                                 fbe_lba_t lun_capacity);
fbe_status_t fbe_test_sep_util_generate_next_key(fbe_object_id_t object_id,
                                                 fbe_database_control_setup_encryption_key_t *key_info,
                                                 fbe_u32_t key_index,
                                                 fbe_u32_t generation,
                                                 fbe_encryption_key_mask_t mask);

fbe_status_t fbe_test_sep_util_create_raid_group_configuration(fbe_test_rg_configuration_t * raid_configuration_p, fbe_u32_t raid_group_count);
fbe_status_t fbe_test_sep_util_create_configuration(fbe_test_rg_configuration_t * raid_group_configuration_p,
                                                    fbe_u32_t raid_group_count,
                                                    fbe_bool_t encryption_on);
fbe_status_t fbe_test_sep_util_set_rg_keys(fbe_test_rg_configuration_t * raid_group_configuration_p,
                                           fbe_u32_t current_index,
                                           fbe_u32_t current_generation,
                                           fbe_u32_t next_index,
                                           fbe_u32_t next_generation,
                                           fbe_bool_t b_start);

fbe_status_t fbe_test_sep_util_wait_rg_encryption_state(fbe_object_id_t rg_object_id,
                                                        fbe_base_config_encryption_state_t expected_state,
                                                        fbe_u32_t wait_ms);
fbe_status_t fbe_test_sep_util_destroy_raid_group_configuration(fbe_test_rg_configuration_t * raid_configuration_p, fbe_u32_t raid_group_count);

fbe_status_t fbe_test_sep_util_create_logical_unit_configuration(fbe_test_logical_unit_configuration_t * logical_unit_configuration_list_p, fbe_u32_t luns_per_raid_group);
fbe_status_t fbe_test_sep_util_destroy_logical_unit_configuration(fbe_test_logical_unit_configuration_t * logical_unit_configuration_list_p, fbe_u32_t luns_per_raid_group);

fbe_status_t fbe_test_sep_util_create_raid_group(fbe_test_rg_configuration_t * raid_group_configuration_p);
fbe_status_t fbe_test_sep_util_destroy_logical_unit(fbe_lun_number_t   lun_number);

fbe_status_t fbe_test_sep_util_create_logical_unit(fbe_test_logical_unit_configuration_t * lun_configuration_p);
fbe_status_t fbe_test_sep_util_destroy_raid_group(fbe_raid_group_number_t raid_group_id);
void fbe_test_sep_util_set_no_initial_verify(fbe_bool_t b_value);
fbe_bool_t fbe_test_sep_util_get_no_initial_verify(void);

fbe_status_t fbe_test_sep_util_destroy_all_user_raid_groups(void);
fbe_status_t fbe_test_sep_util_destroy_all_user_luns(void);
void fbe_test_sep_util_set_service_trace_flags(fbe_trace_type_t trace_type, 
                                              fbe_service_id_t service_id, 
                                              fbe_package_id_t package_id,
                                              fbe_trace_level_t level);
void fbe_test_sep_util_set_rg_trace_level(fbe_test_rg_configuration_t *rg_config_p, fbe_trace_level_t level);
void fbe_test_sep_util_set_rg_debug_flags(fbe_test_rg_configuration_t *rg_config_p, fbe_raid_group_debug_flags_t debug_flags);
void fbe_test_sep_util_set_rg_trace_level_both_sps(fbe_test_rg_configuration_t *rg_config_p, fbe_trace_level_t level);
void fbe_test_sep_util_set_rg_debug_flags_both_sps(fbe_test_rg_configuration_t *rg_config_p, fbe_raid_group_debug_flags_t debug_flags);
void fbe_test_sep_util_set_rg_library_debug_flags(fbe_test_rg_configuration_t *rg_config_p, 
                                                  fbe_raid_library_debug_flags_t debug_flags);
void fbe_test_sep_util_set_rg_library_debug_flags_both_sps(fbe_test_rg_configuration_t *rg_config_p, 
                                                           fbe_raid_library_debug_flags_t debug_flags);
fbe_lba_t fbe_test_sep_util_get_blocks_per_lun(fbe_lba_t rg_capacity,
                                               fbe_u32_t luns_per_rg,
                                               fbe_u32_t data_drives,
                                               fbe_chunk_size_t chunk_size);
void fbe_test_sep_util_get_all_vd_object_ids(fbe_object_id_t rg_object_id,
                                             fbe_object_id_t * vd_object_ids_p);
void fbe_test_sep_wait_for_rg_objects_ready(fbe_test_rg_configuration_t * const rg_config_p);
void fbe_test_sep_wait_for_rg_objects_ready_both_sps(fbe_test_rg_configuration_t * const rg_config_p);
fbe_status_t fbe_test_sep_util_send_job_service_command (fbe_object_id_t object_id, fbe_job_service_control_t jon_control);

fbe_status_t fbe_test_sep_util_provision_drive_clear_verify_report( fbe_object_id_t in_provision_drive_object_id );
fbe_status_t fbe_test_sep_util_provision_drive_disable_verify( fbe_object_id_t provision_drive_object_id );
fbe_status_t fbe_test_sep_util_provision_drive_enable_verify( fbe_object_id_t provision_drive_object_id );
fbe_status_t fbe_test_sep_util_provision_drive_get_verify_report( fbe_object_id_t in_provision_drive_object_id, fbe_provision_drive_verify_report_t* out_verify_report_p );
fbe_status_t fbe_test_sep_util_provision_drive_get_verify_status( fbe_object_id_t provision_drive_object_id, fbe_provision_drive_get_verify_status_t* verify_status_p );
fbe_status_t fbe_test_sep_util_provision_drive_set_verify_checkpoint( fbe_object_id_t in_provision_drive_object_id, fbe_lba_t in_checkpoint );
fbe_status_t fbe_test_sep_util_provision_drive_pool_id(fbe_object_id_t in_pvd_object_id, fbe_u32_t pool_id);
fbe_status_t fbe_test_sep_util_update_multi_provision_drive_pool_id(fbe_object_id_t *in_pvd_object_id_p, 
                                                       fbe_u32_t *pool_id_p, fbe_u32_t pvd_count);
fbe_status_t fbe_test_sep_util_provision_drive_wait_disk_zeroing(fbe_object_id_t object_id, fbe_lba_t   expect_zero_checkpoint, fbe_u32_t   max_wait_msecs);
fbe_status_t fbe_test_sep_util_wait_for_pvd_np_flags(fbe_object_id_t object_id, fbe_provision_drive_np_flags_t flag, fbe_u32_t timeout_ms);
fbe_status_t fbe_test_sep_util_provision_drive_disable_zeroing( fbe_object_id_t provision_drive_object_id );
fbe_status_t fbe_test_sep_util_provision_drive_enable_zeroing( fbe_object_id_t provision_drive_object_id );
fbe_status_t fbe_test_sep_util_rg_config_disable_zeroing( fbe_test_rg_configuration_t *rg_config_p);
fbe_status_t fbe_test_sep_util_rg_config_enable_zeroing( fbe_test_rg_configuration_t *rg_config_p);
fbe_status_t fbe_test_sep_util_wait_for_scrub_complete(fbe_u32_t timeout_ms);
void fbe_test_sep_util_wait_for_pvd_creation(fbe_test_raid_group_disk_set_t * disk_set,fbe_u32_t disk_count,fbe_u32_t wait_time_ms);
void fbe_test_sep_util_zero_object_capacity(fbe_object_id_t rg_object_id, fbe_lba_t start_offset);
void fbe_test_sep_util_randmly_disable_system_zeroing(void);
void fbe_test_enable_rg_background_ops_system_drives(void);
void fbe_test_sep_util_enable_system_drive_zeroing(void);
void fbe_test_sep_util_set_default_trace_level(fbe_trace_level_t level);
void fbe_test_sep_util_set_trace_level(fbe_trace_level_t level,
                                       fbe_package_id_t package_id);
void fbe_test_sep_util_reduce_sector_trace_level(void);
void fbe_test_sep_util_restore_sector_trace_level(void);
fbe_status_t fbe_test_sep_util_set_sector_trace_debug_flags(fbe_sector_trace_error_flags_t new_dbg_flags);
fbe_status_t fbe_test_sep_util_set_sector_trace_stop_on_error(fbe_bool_t stop_on_err);
fbe_status_t fbe_test_sep_util_set_sector_trace_flags(fbe_sector_trace_error_flags_t new_error_flags);
fbe_status_t fbe_test_sep_util_set_sector_trace_level(fbe_sector_trace_error_level_t new_error_level);
void fbe_test_sep_util_set_dualsp_test_mode(fbe_bool_t b_set_dualsp_mode);
fbe_bool_t fbe_test_sep_util_get_encryption_test_mode(void);
void fbe_test_sep_util_set_encryption_test_mode(fbe_bool_t b_set_encrypted_mode);
fbe_bool_t fbe_test_sep_util_get_dualsp_test_mode(void);
void fbe_test_sep_util_set_error_injection_test_mode(fbe_bool_t b_set_err_inj_mode);
fbe_bool_t fbe_test_sep_util_get_error_injection_test_mode(void);
void fbe_test_set_object_id_for_r10(fbe_object_id_t *rg_object_id);
fbe_status_t fbe_test_pp_util_pull_drive_hw(fbe_u32_t port_number, 
                                            fbe_u32_t enclosure_number, 
                                            fbe_u32_t slot_number);
fbe_status_t fbe_test_pp_util_reinsert_drive_hw(fbe_u32_t port_number,
                                                fbe_u32_t enclosure_number,
                                                fbe_u32_t slot_number);
void fbe_test_sep_util_validate_no_raid_errors(void);
void fbe_test_sep_util_validate_no_raid_errors_both_sps(void);

void fbe_test_sep_util_set_remove_on_both_sps_mode(fbe_bool_t b_set_remove_both_sps_mode);
fbe_bool_t fbe_test_sep_util_should_remove_on_both_sps(void);

/*!*******************************************************************
 * @def FBE_TEST_SEP_UTIL_RB_WAIT_SEC
 *********************************************************************
 * @brief Time to wait for a drive to rebuild.
 *
 *********************************************************************/
#define FBE_TEST_SEP_UTIL_RB_WAIT_SEC 360
fbe_status_t fbe_test_sep_util_wait_rg_pos_rebuild(fbe_object_id_t raid_group_object_id,
                                                   fbe_u32_t       position_to_wait_for,
                                                   fbe_u32_t       wait_seconds);
fbe_status_t fbe_test_sep_util_wait_for_raid_group_to_rebuild(fbe_object_id_t raid_group_object_id);
fbe_status_t fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(fbe_object_id_t raid_group_object_id);
fbe_status_t fbe_test_sep_util_get_expected_verify_error_counts(fbe_object_id_t raid_group_object_id,
                                                                fbe_lba_t  start_lba,
                                                                fbe_u32_t  errors_injected_on_first_position,
                                                                fbe_u32_t  errors_injected_on_second_position,
                                                                fbe_u32_t *expected_num_of_correctable_errors_p,
                                                                fbe_u32_t *expected_num_of_uncorrectable_errors_p);
fbe_status_t fbe_test_util_wait_for_encryption_state(fbe_object_id_t object_id, 
                                                     fbe_base_config_encryption_state_t expected_state,
                                                     fbe_u32_t total_timeout_in_ms);
fbe_status_t fbe_test_util_wait_for_rg_object_id_by_number(fbe_raid_group_number_t rg_number, 
                                                           fbe_object_id_t *object_id, 
                                                           fbe_u32_t total_timeout);

fbe_bool_t fbe_test_sep_util_get_cmd_option_int(char *option_name_p, fbe_u32_t *value_p);
fbe_bool_t fbe_test_sep_util_get_cmd_option(char *option_name_p);
fbe_u32_t fbe_sep_test_util_get_raid_testing_extended_level(void);
fbe_u32_t fbe_sep_test_util_get_extended_testing_level(void);
fbe_bool_t fbe_sep_test_util_raid_type_enabled(fbe_raid_group_type_t raid_type);
void fbe_test_sep_util_fail_drive(void);
fbe_u32_t fbe_test_sep_util_get_drive_remove_delay(fbe_u32_t in_drive_remove_delay);
fbe_u32_t fbe_test_sep_util_get_drive_insert_delay(fbe_u32_t in_drive_insert_delay);
fbe_u32_t fbe_test_sep_util_get_raid_library_debug_flags(fbe_u32_t in_debug_flags);
void fbe_test_sep_util_set_raid_library_default_debug_flags(fbe_raid_library_debug_flags_t flags);
void fbe_test_sep_util_set_default_io_flags(void);
void fbe_test_sep_util_set_default_terminator_and_rba_flags(void);
void fbe_test_sep_util_set_virtual_drive_class_debug_flags(fbe_virtual_drive_debug_flags_t flags);
void fbe_test_sep_util_set_pvd_class_debug_flags(fbe_provision_drive_debug_flags_t flags);
void fbe_test_sep_util_set_terminator_drive_debug_flags(fbe_terminator_drive_debug_flags_t in_terminator_drive_debug_flags);
fbe_u32_t fbe_test_get_rg_count(fbe_test_rg_configuration_t *rg_config_p);
fbe_u32_t fbe_test_get_rg_array_length(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_rg_config_mark_invalid(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_set_specific_drives_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t *drives_to_remove_p);
void fbe_test_sep_util_enable_trace_limits(fbe_bool_t sep_only);
void fbe_test_sep_util_disable_trace_limits(void);
void fbe_test_sep_util_enable_pkg_trace_limits(fbe_package_id_t package_id);
void fbe_test_sep_util_disable_pkg_trace_limits(fbe_package_id_t package_id);
void fbe_test_sep_util_rg_config_array_load_physical_config(fbe_test_rg_configuration_array_t *array_p);
void fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(fbe_test_rg_configuration_array_t *array_p);
fbe_u32_t fbe_test_sep_util_rg_config_array_count(fbe_test_rg_configuration_array_t *array_p);
void fbe_test_sep_util_get_rg_num_enclosures_and_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_u32_t num_raid_groups,
                                                        fbe_u32_t *num_520_drives_p,
                                                        fbe_u32_t *num_4160_drives_p);
void fbe_test_sep_util_get_rg_num_extra_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_u32_t num_raid_groups,
                                                        fbe_u32_t *num_520_drives_p,
                                                        fbe_u32_t *num_4160_drives_p);
void fbe_test_sep_util_populate_rg_num_extra_drives(fbe_test_rg_configuration_t *rg_config_p);

void fbe_test_sep_util_get_rg_type_num_extra_drives(fbe_raid_group_type_t    raid_type,
                                                                                fbe_u32_t      rg_width,
                                                                                    fbe_u32_t *num_extra_drives_p);
void fbe_test_sep_util_rg_fill_disk_sets(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_u32_t num_raid_groups,
                                         fbe_u32_t num_520_drives,
                                         fbe_u32_t num_4160_drives);

void fbe_test_sep_util_banner(const char *func, const char *message);

//void fbe_test_verify_object_counts(fbe_u32_t expected_num_pvds,
//                                   fbe_u32_t expected_num_vds,
//                                   fbe_u32_t expected_num_rgs,
//                                   fbe_u32_t expected_num_luns,
//                                   fbe_u32_t expected_num_edges);

void fbe_test_sep_util_get_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id,
                                                            fbe_virtual_drive_configuration_mode_t * configuration_mode_p);

fbe_status_t fbe_test_sep_util_disable_recovery_queue(fbe_object_id_t object_id);
fbe_status_t fbe_test_sep_util_enable_recovery_queue(fbe_object_id_t object_id);
fbe_status_t fbe_test_sep_util_disable_creation_queue(fbe_object_id_t object_id);
fbe_status_t fbe_test_sep_util_enable_creation_queue(fbe_object_id_t object_id);

fbe_status_t fbe_test_util_build_job_service_control_element(fbe_job_queue_element_t *   job_queue_element,
                                                             fbe_job_service_control_t   control_code,
                                                             fbe_object_id_t object_id,
                                                             fbe_u8_t * command_data_p,
                                                             fbe_u32_t  command_size);
/* common destroy for sep test suite */
void fbe_test_sep_util_destroy_packages(const fbe_test_packages_t *unload_packages);
void fbe_test_sep_util_destroy_neit_sep_physical(void);
void fbe_test_sep_util_destroy_esp_neit_sep_physical(void);
void fbe_test_sep_util_destroy_neit_sep_physical_no_esp(void);
void fbe_test_sep_util_destroy_neit_sep_physical_both_sps(void);
void fbe_test_sep_util_destroy_esp_neit_sep_physical_both_sps(void);
void fbe_test_sep_util_destroy_neit_sep(void);
void fbe_test_sep_util_destroy_esp_neit_sep(void);
void fbe_test_sep_util_destroy_neit_sep_both_sps(void);
void fbe_test_sep_util_destroy_sep_physical(void);
void fbe_test_sep_util_destroy_sep_physical_both_sps(void);
void fbe_test_sep_util_destroy_sep(void);
void fbe_test_sep_util_sep_neit_physical_verify_no_trace_error(void);
void fbe_test_sep_util_sep_neit_physical_reset_error_counters(void);
void fbe_test_sep_util_destroy_kms(void);
void fbe_test_sep_util_destroy_kms_both_sps(void);
void fbe_test_sep_util_destroy_neit_sep_kms(void);
void fbe_test_sep_util_destroy_neit_sep_kms_both_sps(void);
fbe_status_t fbe_test_sep_util_enable_kms_encryption(void);
fbe_status_t fbe_test_sep_util_get_enable_kms_status(fbe_status_t *status);
fbe_status_t fbe_test_sep_util_clear_kms_hook(fbe_kms_hook_state_t state);
fbe_status_t fbe_test_sep_util_get_kms_hook(fbe_kms_hook_t *hook_p);
fbe_status_t fbe_test_util_wait_for_kms_hook(fbe_kms_hook_state_t state, fbe_u32_t timeout_ms);
fbe_status_t fbe_test_fill_system_raid_group_lun_config(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_u32_t num_luns_to_test,
                                                        fbe_u32_t physical_chunks_per_lun_to_test);
fbe_status_t fbe_test_populate_system_rg_config(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_u32_t num_luns_to_test,
                                                fbe_u32_t chunks_per_lun_to_test);
fbe_status_t fbe_test_sep_util_kms_start_rekey(void);
fbe_status_t fbe_test_sep_util_kms_get_rg_keys(fbe_kms_control_get_keys_t *get_keys);
fbe_status_t fbe_test_sep_util_kms_start_backup(fbe_u8_t *fname);
fbe_status_t fbe_test_sep_util_kms_complete_backup(fbe_u8_t *fname);
fbe_status_t fbe_test_sep_util_kms_restore(fbe_u8_t *fname);
fbe_status_t fbe_test_sep_util_kms_set_rg_keys(fbe_kms_control_set_keys_t *set_keys);
fbe_status_t fbe_test_sep_util_kms_push_rg_keys(fbe_kms_control_set_keys_t *set_keys);
void fbe_test_sep_util_quiesce_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_u32_t raid_group_count);
void fbe_test_sep_util_unquiesce_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_u32_t raid_group_count);
void fbe_test_sep_util_wait_for_rg_base_config_flags(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_u32_t raid_group_count,
                                                     fbe_u32_t wait_seconds,
                                                     fbe_base_config_clustered_flags_t base_config_clustered_flags,
                                                     fbe_bool_t b_cleared);
fbe_status_t fbe_test_sep_reset_raid_memory_statistics(fbe_test_rg_configuration_t *rg_config_p);

fbe_status_t fbe_test_sep_encryption_add_hook_and_enable_kms(fbe_test_rg_configuration_t * const rg_config_p, fbe_bool_t b_dualsp);
fbe_status_t fbe_test_sep_start_encryption_or_rekey(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_sep_reset_encryption_enabled(void);
void fbe_test_rg_wait_for_zeroing(fbe_test_rg_configuration_t *rg_config_p);

fbe_bool_t fbe_test_is_test_case_enabled(fbe_u32_t test_case_index,
                                         fbe_char_t *test_cases_options_p);
void fbe_test_sep_util_configure_pvd_as_hot_spare(fbe_object_id_t  pvd_object_id, 
                                                  fbe_u8_t *opaque_data, 
                                                  fbe_u32_t opaque_data_size,
                                                  fbe_bool_t b_allow_is_use_drives);

fbe_status_t fbe_test_sep_util_configure_hot_spares(fbe_test_hs_configuration_t * hot_spare_configuration_p,
                                                    fbe_u32_t hot_spare_count);

fbe_status_t fbe_test_sep_util_reconfigure_hot_spares(fbe_test_hs_configuration_t * hot_spare_configuration_p,
                                                      fbe_u32_t hot_spare_count);

void fbe_test_sep_util_configure_pvd_as_automatic_spare(fbe_object_id_t  pvd_object_id, 
                                                        fbe_u8_t *opaque_data, 
                                                        fbe_u32_t opaque_data_size,
                                                        fbe_bool_t b_allow_is_use_drives);

fbe_status_t fbe_test_sep_util_configure_automatic_spares(fbe_test_hs_configuration_t *spare_configuration_p,
                                                          fbe_u32_t spare_count);

fbe_status_t fbe_test_sep_util_remove_hotspares_from_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p, 
                                                                   fbe_u32_t raid_group_count);

fbe_status_t fbe_test_sep_util_add_hotspares_to_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p, 
                                                              fbe_u32_t raid_group_count);

fbe_status_t fbe_test_sep_util_swap_in_proactive_spare(fbe_object_id_t vd_object_id,
                                                       fbe_bool_t b_wait_for_job,
                                                       fbe_job_service_error_type_t * job_error_type_p);

fbe_status_t fbe_test_sep_util_user_copy(fbe_object_id_t vd_object_id,
                                         fbe_object_id_t pvd_object_id,
                                         fbe_bool_t b_wait_for_job,
                                         fbe_job_service_error_type_t * job_error_type_p);

fbe_status_t fbe_test_sep_util_update_permanent_spare_trigger_timer(fbe_u64_t permanent_spare_trigger_timer);
fbe_status_t fbe_test_sep_util_configure_drive_as_hot_spare(fbe_u32_t port, fbe_u32_t encl, fbe_u32_t slot);
fbe_status_t fbe_test_sep_util_wait_for_pvd_state(fbe_u32_t port, fbe_u32_t encl, fbe_u32_t slot,
                                                  fbe_lifecycle_state_t state,
                                                  fbe_u32_t max_wait_msecs);
fbe_status_t fbe_test_sep_util_check_permanent_spare_trigger_timer(fbe_u64_t permanent_spare_trigger_timer);

/* Function to add the verify results*/
void fbe_test_sep_util_sum_verify_results_data(fbe_lun_verify_counts_t* in_verify_results_p,
                                             fbe_u32_t* out_count_p);

void fbe_test_sep_util_remove_drives(fbe_u32_t position_to_remove,
                                    fbe_test_rg_configuration_t*);
void fbe_test_sep_util_insert_drives(fbe_u32_t position_to_insert,
                                    fbe_test_rg_configuration_t*);

void fbe_test_sep_util_wait_for_pdo_ready(fbe_u32_t position_to_insert,
                                             fbe_test_rg_configuration_t*);

/* Function to set provision drive debug flags */
void fbe_test_sep_util_provision_drive_set_debug_flag(
                                    fbe_object_id_t in_provision_drive_object_id,
                                    fbe_provision_drive_debug_flags_t in_flag);
void fbe_test_sep_util_provision_drive_set_class_debug_flag(fbe_provision_drive_debug_flags_t in_flag);


void fbe_test_set_use_fbe_sim_psl(fbe_bool_t b_use_sim_psl);
fbe_bool_t fbe_test_should_use_sim_psl(void);

void fbe_test_set_ext_pool_slice_blocks(fbe_block_count_t slice_blocks);
fbe_block_count_t fbe_test_get_ext_pool_slice_blocks(void);
void fbe_test_set_ext_pool_slice_blocks_sim(void);

/* Function to get the active SP identifier */
void fbe_test_sep_get_active_target_id(fbe_sim_transport_connection_target_t* out_active_sp_p);

static __inline fbe_sim_transport_connection_target_t 
fbe_test_sep_get_peer_target_id(fbe_sim_transport_connection_target_t current_sp_id)
{
    if (current_sp_id == FBE_SIM_SP_A)
    {
        return FBE_SIM_SP_B;
    }
    else
    {
        return FBE_SIM_SP_A;
    }
}
/* Functions to get and clear the raid group write log flush error counters */
fbe_status_t fbe_test_sep_util_raid_group_get_flush_error_counters(fbe_object_id_t in_raid_group_object_id, 
                                                                   fbe_raid_flush_error_counts_t* out_flush_error_counters_p);

fbe_status_t fbe_test_sep_util_raid_group_clear_flush_error_counters(fbe_object_id_t in_raid_group_object_id);

fbe_status_t fbe_test_sep_number_of_ios_outstanding(fbe_object_id_t object_id,
                                               UINT_32* ios_outstanding_p);
fbe_status_t fbe_test_sep_util_get_raid_group_io_info(fbe_object_id_t rg_object_id,
                                                      fbe_api_raid_group_get_io_info_t *raid_group_io_info_p);
fbe_status_t fbe_test_sep_utils_wait_for_base_config_clustered_flags(fbe_object_id_t rg_object_id,
                                                fbe_u32_t wait_seconds,
                                                fbe_base_config_clustered_flags_t base_config_flags,
                                                fbe_bool_t b_cleared);
fbe_status_t fbe_test_sep_utils_wait_pvd_for_base_config_clustered_flags(fbe_object_id_t pvd_object_id,
                                                fbe_u32_t wait_seconds,
                                                fbe_base_config_clustered_flags_t base_config_flags,
                                                fbe_bool_t b_cleared);

fbe_status_t fbe_test_sep_utils_wait_for_all_base_config_flags(fbe_class_id_t class_id,    
                                                     fbe_u32_t wait_seconds,
                                                     fbe_base_config_clustered_flags_t flags,
                                                     fbe_bool_t b_cleared);


/* power saving utils */
void fbe_test_sep_util_enable_system_power_saving(void);

void fbe_test_sep_util_init_rg_configuration(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_sep_util_init_rg_configuration_array(fbe_test_rg_configuration_t *rg_config_p);
fbe_u32_t fbe_sep_test_util_get_default_block_size(void);
void fbe_test_sep_util_randomize_rg_configuration_array(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_rg_setup_random_block_sizes(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_rg_setup_block_sizes(fbe_test_rg_configuration_t *rg_config_p, 
                                   fbe_block_size_t block_size);
fbe_u32_t fbe_test_sep_util_get_next_position_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_test_drive_removal_mode_t removal_mode);
void fbe_test_sep_util_add_removed_position(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t position_to_remove);
fbe_u32_t fbe_test_sep_get_drive_remove_history(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_u32_t history_index);
void fbe_test_sep_util_add_needing_spare_position(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_u32_t position_needing_spare);
fbe_u32_t fbe_test_sep_util_get_next_position_to_insert(fbe_test_rg_configuration_t *rg_config_p);
fbe_u32_t fbe_test_sep_util_get_random_position_to_insert(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_sep_util_removed_position_inserted(fbe_test_rg_configuration_t *rg_config_p,
                                                 fbe_u32_t inserted_position);
void fbe_test_sep_util_delete_needing_spare_position(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_u32_t spared_position);
fbe_u32_t fbe_test_sep_util_get_num_needing_spare(fbe_test_rg_configuration_t *rg_config_p);
fbe_u32_t fbe_test_sep_util_get_needing_spare_position_from_index(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t needing_spare_index);
fbe_u32_t fbe_test_sep_util_get_num_removed_drives(fbe_test_rg_configuration_t *rg_config_p);
fbe_status_t fbe_test_sep_util_wait_for_all_lun_objects_ready(fbe_test_rg_configuration_t *rg_config_p,
                                                              fbe_transport_connection_target_t target_sp_of_luns);

void fbe_test_sep_util_print_dps_statistics(void);
void fbe_test_sep_util_print_metadata_statistics(fbe_object_id_t object_id);
fbe_u32_t fbe_test_sep_get_data_drives_for_raid_group_config(fbe_test_rg_configuration_t *rg_config);
fbe_block_count_t fbe_test_sep_util_get_maximum_drive_capacity(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t num_raid_groups);     

void fbe_test_sep_util_set_unused_drive_info(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_u32_t num_raid_groups,
                                             fbe_u32_t unused_drive_count[][FBE_DRIVE_TYPE_LAST],
                                             fbe_test_discovered_drive_locations_t *drive_locations_p);  
void fbe_test_sep_util_update_extra_chunk_size(fbe_test_rg_configuration_t *rg_config_p, fbe_chunk_size_t in_extra_chunk);

fbe_status_t fbe_test_sep_util_unconfigure_all_spares_in_the_system(void);
fbe_status_t fbe_test_sep_util_verify_pdo_objects_state_in_the_system(void);
fbe_status_t fbe_test_sep_util_bring_drive_up(fbe_object_id_t object_id, fbe_u32_t wait_time);

void fbe_test_sep_util_clear_drive_errors(void);
void fbe_test_sep_util_disable_all_error_injection(void);
fbe_status_t fbe_hw_clear_error_records(void);
fbe_status_t fbe_test_sep_util_get_user_lun_count(fbe_u32_t * lun_count);
fbe_status_t fbe_test_sep_util_get_all_user_rg_count(fbe_u32_t * rg_count);
fbe_status_t fbe_test_sep_util_count_all_raid_group_of_class(fbe_class_id_t class_id, fbe_u32_t * rg_count);
void fbe_test_sep_util_get_drive_capacity(fbe_test_raid_group_disk_set_t * disk_set_p, 
                                                    fbe_lba_t *drive_capacity_p);
void fbe_test_sep_util_get_drive_offset(fbe_test_raid_group_disk_set_t *disk_set_p, 
                                        fbe_lba_t *drive_offset_p);

/* LUN edge connection to BVD (SEP)*/
fbe_status_t fbe_test_sep_util_wait_for_LUN_edge_on_sp(fbe_object_id_t lun_object_id, 
                                                       fbe_transport_connection_target_t target_sp_of_object,
                                                       fbe_u32_t timeout_ms);
fbe_status_t fbe_test_sep_util_wait_for_LUN_edge_on_all_sps(fbe_bool_t b_wait_for_both_sps,
                                                            fbe_object_id_t lun_object_id, 
                                                            fbe_u32_t timeout_ms);
fbe_status_t fbe_test_sep_util_raid_group_generate_error_trace(fbe_object_id_t rg_object_id);
fbe_status_t fbe_test_sep_util_get_minimum_lun_capacity_for_config(fbe_test_rg_configuration_t *rg_config_p,
                                                                   fbe_lba_t *min_lun_capacity_p);

fbe_status_t fbe_test_util_create_registry_file(void);

fbe_status_t fbe_test_sep_util_wait_for_LUN_create_event(fbe_object_id_t lun_object_id, 
                                                         fbe_lun_number_t lun_num);
void fbe_test_wait_for_objects_of_class_state(fbe_class_id_t class_id, 
                                              fbe_package_id_t package_id,
                                              fbe_lifecycle_state_t expected_lifecycle_state,
                                              fbe_u32_t timeout_ms);

void fbe_test_sep_util_set_lifecycle_debug_trace_flag(fbe_u32_t flags);
fbe_status_t fbe_test_sep_util_set_lifecycle_trace_control_flag(fbe_u32_t flags);
fbe_status_t fbe_test_sep_util_enable_object_lifecycle_trace(fbe_object_id_t object_id);
fbe_status_t fbe_test_sep_util_disable_object_lifecycle_trace(fbe_object_id_t object_id);
fbe_status_t fbe_test_sep_util_update_position_info(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_u32_t position_to_update,
                                                    fbe_test_raid_group_disk_set_t *original_disk_p,
                                                    fbe_test_raid_group_disk_set_t *new_disk_p);
fbe_bool_t fbe_test_rg_is_degraded(fbe_test_rg_configuration_t * rg_config_p);
fbe_status_t fbe_test_sep_util_wait_for_degraded_bitmask(fbe_object_id_t rg_object_id,
                                                         fbe_raid_position_bitmask_t expected_degraded_bitmask,
                                                         fbe_u32_t max_wait_msecs);
void fbe_test_rg_get_degraded_bitmask(fbe_object_id_t rg_object_id,
                                      fbe_raid_position_bitmask_t *degraded_bitmask_p);
void fbe_test_sep_util_validate_no_rg_verifies(fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_u32_t raid_group_count);
fbe_status_t fbe_test_sep_util_wait_for_lun_zeroing_complete(fbe_u32_t lun_number, fbe_u32_t timeout_ms, fbe_package_id_t package_id);
fbe_status_t fbe_test_sep_util_clear_disk_eol(fbe_u32_t bus,
                                              fbe_u32_t enclosure,
                                              fbe_u32_t slot);
fbe_status_t fbe_test_sep_util_set_drive_fault(fbe_u32_t bus,
                                               fbe_u32_t enclosure,
                                               fbe_u32_t slot);
fbe_status_t fbe_test_sep_util_clear_drive_fault(fbe_u32_t bus,
                                               fbe_u32_t enclosure,
                                               fbe_u32_t slot);
fbe_status_t fbe_test_sep_util_validate_drive_fault_state(fbe_u32_t bus,
                                                                 fbe_u32_t enclosure,
                                                                 fbe_u32_t slot,
                                                                 fbe_bool_t b_expected_state_is_set);

void fbe_test_sniff_enable_all_pvds(void);
void fbe_test_sniff_disable_all_pvds(void);
void fbe_test_sep_util_disable_system_drive_zeroing(void);
void fbe_test_disable_background_ops_system_drives(void);
void fbe_test_disable_rg_background_ops_system_drives(void);

fbe_status_t fbe_test_copy_wait_for_completion(fbe_object_id_t vd_object_id,
                                               fbe_u32_t dest_edge_index);
void fbe_test_verify_get_verify_start_substate(fbe_lun_verify_type_t verify_type,
                                               fbe_u32_t *substate_p);

fbe_status_t fbe_test_verify_wait_for_verify_completion(fbe_object_id_t rg_object_id,
                                                        fbe_u32_t wait_ms,
                                                        fbe_lun_verify_type_t verify_type);
fbe_status_t fbe_test_zero_wait_for_disk_zeroing_complete(fbe_object_id_t object_id, fbe_u32_t timeout_ms);
fbe_status_t fbe_test_zero_wait_for_zero_on_demand_state(fbe_object_id_t object_id, 
                                                         fbe_u32_t timeout_ms, 
                                                         fbe_bool_t zod_state);
void fbe_test_copy_determine_dest_index(fbe_object_id_t *vd_object_ids_p,
                                        fbe_u32_t vd_count,
                                        fbe_u32_t *dest_index_p);
void fbe_test_rg_populate_extra_drives(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_u32_t extra_drives);
fbe_status_t fbe_test_zero_wait_for_rg_zeroing_complete(fbe_test_rg_configuration_t * rg_config_p);
fbe_u32_t fbe_test_sep_util_get_element_size(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_sep_util_set_scsi_error_from_error_type(fbe_protocol_error_injection_scsi_error_t *protocol_error_p,
                                                      fbe_test_protocol_error_type_t error_type);
fbe_u8_t fbe_test_sep_util_get_scsi_command(fbe_payload_block_operation_opcode_t opcode);
void fbe_test_sep_util_set_active_target_for_rg(fbe_u32_t raid_group_number);
void fbe_test_sep_util_set_active_target_for_bes(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
void fbe_test_sep_util_set_active_target_for_pvd(fbe_object_id_t pvd_object_id);
void fbe_test_sep_util_wait_for_initial_verify(fbe_test_rg_configuration_t *rg_config_p);
fbe_status_t fbe_test_sep_util_wait_for_raid_group_verify(fbe_object_id_t rg_object_id,
                                                          fbe_u32_t wait_seconds);
fbe_status_t fbe_test_sep_util_get_active_passive_sp(fbe_object_id_t object_id,
                                                     fbe_sim_transport_connection_target_t *active_sp,
                                                     fbe_sim_transport_connection_target_t *passive_sp);
fbe_status_t fbe_test_utils_get_event_log_error_count(fbe_u32_t msg_id,
                                                         fbe_u32_t *count_p);
fbe_bool_t fbe_test_rg_is_broken(fbe_test_rg_configuration_t * rg_config_p);
fbe_status_t fbe_test_sep_wait_for_rg_objects_fail(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_u32_t raid_group_count,
                                                   fbe_bool_t b_is_shutdown);
fbe_status_t fbe_test_sep_validate_raid_groups_are_not_degraded(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count);
fbe_bool_t fbe_test_sep_is_spare_drive_healthy(fbe_test_raid_group_disk_set_t *disk_to_check_p);

fbe_status_t fbe_test_sep_util_set_update_peer_checkpoint_interval(fbe_u32_t update_peer_period);

fbe_status_t fbe_test_sep_util_wait_for_verify_start(fbe_object_id_t lun_object_id,                 
                                                     fbe_lun_verify_type_t verify_type,           
                                                     fbe_api_lun_get_status_t *lun_verify_status_p);

void fbe_test_sep_duplicate_config(fbe_test_rg_configuration_t * from_rg_config_p, 
                                    fbe_test_rg_configuration_t * to_rg_config_p);
void fbe_test_reboot_sp(fbe_test_rg_configuration_t *rg_config_p,
                        fbe_sim_transport_connection_target_t   target_sp);
void fbe_test_reboot_both_sp(fbe_test_rg_configuration_t *rg_config_p);

void fbe_test_boot_first_sp(fbe_test_rg_configuration_t *rg_config_p,
                            fbe_sim_transport_connection_target_t target_sp);
void fbe_test_boot_sp(fbe_test_rg_configuration_t *rg_config_p,
                      fbe_sim_transport_connection_target_t target_sp);
void fbe_test_boot_sp_for_config_array(fbe_test_rg_configuration_array_t *array_p,
                                       fbe_sim_transport_connection_target_t target_sp,
                                       fbe_sep_package_load_params_t *sep_params_p,
                                       fbe_neit_package_load_params_t *neit_params_p,
                                       fbe_bool_t b_first_sp);
void fbe_test_boot_sps(fbe_test_rg_configuration_t *rg_config_p,
                       fbe_sim_transport_connection_target_t target_sp);
void fbe_test_boot_both_sps(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_create_physical_config_for_rg(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t num_raid_groups);

void fbe_test_create_physical_config_with_capacity(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_u32_t num_raid_groups,
                                                   fbe_block_count_t extra_blocks);
void fbe_test_boot_sp_with_extra_drives_load_physical_config(fbe_test_rg_configuration_array_t *array_p,
                                                             fbe_sim_transport_connection_target_t target_sp);
void fbe_test_wait_for_all_pvds_ready(void);
void fbe_test_wait_for_rg_pvds_ready(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_provision_drive_get_paged_metadata_summary(fbe_object_id_t pvd_object_id, fbe_bool_t b_force_unit_access);

void fbe_test_sep_util_check_stripe_lock(fbe_object_id_t object_id);
fbe_status_t fbe_test_sep_util_disable_spare_library_confirmation(void);
fbe_status_t fbe_test_sep_util_enable_spare_library_confirmation(void);
fbe_status_t fbe_test_sep_util_set_spare_library_confirmation_timeout(fbe_u32_t new_spare_library_confirmation_timeout_secs);
fbe_status_t fbe_test_sep_util_wait_for_swap_request_to_complete(fbe_object_id_t vd_object_id);
fbe_status_t fbe_test_sep_util_glitch_drive(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t position_to_glitch);
fbe_u32_t fbe_test_sep_utils_get_bitcount(fbe_u32_t bitmask);


void fbe_test_sep_util_validate_encryption_mode_setup(fbe_test_rg_configuration_t *rg_config_ptr,
                                                      fbe_base_config_encryption_mode_t   expected_encryption_mode);
void fbe_test_sep_util_validate_key_setup(fbe_test_rg_configuration_t *rg_config_ptr);
void fbe_test_sep_util_validate_and_destroy_kek_setup(fbe_test_rg_configuration_t *rg_config_ptr);
void fbe_test_sep_util_start_rg_encryption(fbe_test_rg_configuration_t * const rg_config_p);

void fbe_test_sep_util_complete_rg_encryption(fbe_test_rg_configuration_t * const rg_config_p);
void fbe_test_sep_util_setup_kek(fbe_test_rg_configuration_t *rg_config_ptr);
fbe_status_t fbe_test_sep_util_wait_rg_encryption_state(fbe_object_id_t rg_object_id,
                                                             fbe_base_config_encryption_state_t expected_state,
                                                             fbe_u32_t wait_ms);

fbe_status_t fbe_test_sep_util_wait_rg_encryption_state_passed(fbe_object_id_t rg_object_id,
                                                        fbe_base_config_encryption_state_t expected_state,
                                                        fbe_u32_t wait_ms);
fbe_status_t fbe_test_sep_util_wait_all_rg_encryption_state(fbe_test_rg_configuration_t * const rg_config_p,
                                                            fbe_base_config_encryption_state_t expected_state,
                                                            fbe_u32_t wait_ms,
                                                            fbe_bool_t b_check_top_level);
void fbe_test_rg_validate_lbas_encrypted(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_lba_t lba,
                                          fbe_block_count_t blocks,
                                         fbe_block_count_t blocks_encrypted);
void fbe_test_sep_util_wait_object_encryption_mode(fbe_object_id_t     rg_object_id,
                                                   fbe_base_config_encryption_mode_t   expected_encryption_mode,
                                                   fbe_u32_t max_timeout_ms);
void fbe_test_sep_util_wait_object_encryption_state(fbe_object_id_t     rg_object_id,
                                                    fbe_base_config_encryption_state_t   expected_encryption_state,
                                                    fbe_u32_t max_timeout_ms);

void fbe_test_sep_util_wait_for_scrubbing_complete(fbe_object_id_t object_id,
                                                   fbe_u32_t max_timeout_ms);
void fbe_test_encryption_wait_rg_rekey(fbe_test_rg_configuration_t * const rg_config_p,  fbe_bool_t is_kms_enabled);

fbe_status_t fbe_test_sep_util_wait_all_rg_encryption_complete(fbe_test_rg_configuration_t * const rg_config_p,
                                                               fbe_u32_t wait_ms);

fbe_status_t fbe_test_sep_util_wait_rg_encryption_complete(fbe_object_id_t rg_object_id,
                                                           fbe_u32_t wait_ms);
fbe_status_t fbe_test_sep_util_wait_top_rg_encryption_complete(fbe_object_id_t rg_object_id,
                                                               fbe_u32_t wait_ms);
void fbe_test_encryption_init(void);
fbe_status_t fbe_test_sep_wait_all_levels_rg_encryption_state(fbe_test_rg_configuration_t * const rg_config_p,
                                                            fbe_base_config_encryption_state_t expected_state,
                                                            fbe_u32_t wait_ms);


void fbe_test_encryption_init(void);
fbe_status_t fbe_test_sep_wait_all_levels_rg_encryption_state_passed(fbe_test_rg_configuration_t * const rg_config_p,
                                                            fbe_base_config_encryption_state_t expected_state,
                                                            fbe_u32_t wait_ms);
void fbe_test_encryption_set_rg_keys(fbe_test_rg_configuration_t *current_rg_config_p);
void fbe_test_wait_for_critical_error(void);
void fbe_test_set_rg_vault_encrypt_wait_time(fbe_u32_t wait_time_ms);
void fbe_test_reset_rg_vault_encrypt_wait_time(void);

void fbe_test_set_timeout_ms(fbe_u32_t time_ms);
fbe_u32_t fbe_test_get_timeout_ms(void);

fbe_status_t fbe_test_sep_util_verify_rg_config_stripe_lock_state(fbe_test_rg_configuration_t *rg_config_p);
void fbe_test_set_vault_wait_time(void);
void fbe_test_pull_drive(fbe_test_rg_configuration_t *rg_config_p,
                         fbe_u32_t position_to_remove);
void fbe_test_reinsert_drive(fbe_test_rg_configuration_t *rg_config_p,
                             fbe_u32_t position_to_insert);

fbe_status_t fbe_test_sep_drive_pull_drive(fbe_test_raid_group_disk_set_t *drive_to_remove_p,
                                           fbe_api_terminator_device_handle_t *out_drive_info_p);
fbe_status_t fbe_test_sep_drive_reinsert_drive(fbe_test_raid_group_disk_set_t *drive_to_insert_p,
                                               fbe_api_terminator_device_handle_t drive_handle_to_insert);
void fbe_test_set_commit_level(fbe_u32_t commit_level);

// set the number of chunks per rebuild
fbe_status_t fbe_test_sep_util_set_chunks_per_rebuild(fbe_u32_t num_chunks_per_rebuild);
// set a random number of chunks per rebuild
void fbe_test_sep_util_set_random_chunks_per_rebuild(void);
// get the number of chunks per rebuild
fbe_u32_t fbe_test_sep_util_get_chunks_per_rebuild(void);

#endif /* SEP_UTILS_H */
