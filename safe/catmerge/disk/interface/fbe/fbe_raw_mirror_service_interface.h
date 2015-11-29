#ifndef FBE_RAW_MIRROR_SERVICE_INTERFACE_H
#define FBE_RAW_MIRROR_SERVICE_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raw_mirror_service_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interface for the raw mirror service.
 *  This service allows I/O to be generated to a raw mirror.
 * 
 *  In production code, a raw mirror is used to access the system LUNs non-paged metadata
 *  and their configuration tables.  There is no object interface for this data.  It is
 *  accessed early in boot before any objects have been created.  
 * 
 *  The raw mirror service is used to test fbe_raid_raw_mirror_library
 *  and associated RAID library code.
 * 
 *  The code is this file is used by the raw mirror service and fbe_api to
 *  facilitate communication between test code and the test service.
 * 
 * @version
 *   10/2011:  Created. Susan Rundbaken
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_xor_api.h" 
#include "fbe_raid_library.h"


/*************************
 *   DEFINITIONS
 *************************/

/*!*******************************************************************
 * @enum fbe_raw_mirror_service_control_code_e
 *********************************************************************
 * @brief Control codes for the raw mirror service.
 *
 *********************************************************************/
typedef enum fbe_raw_mirror_service_control_code_e {
    FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_RAW_MIRROR),

    FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_START_IO,
    FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_STOP_IO,
    FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_PVD_EDGE_ENABLED,

    FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_LAST
} fbe_raw_mirror_service_control_code_t;


/*!*******************************************************************
 * @enum fbe_raw_mirror_service_operation_t
 *********************************************************************
 * @brief
 *  Describes the operation the raw mirror service will perform.
 *********************************************************************/
typedef enum fbe_raw_mirror_service_operation_e
{
    FBE_RAW_MIRROR_SERVICE_OPERATION_INVALID = 0,

    FBE_RAW_MIRROR_SERVICE_OPERATION_READ_ONLY, /*!< Only issue reads. */
    FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY, /*!< Only issue write commands. */
    FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_VERIFY, /*!< Write-verify only */
    FBE_RAW_MIRROR_SERVICE_OPERATION_READ_CHECK, /*!< read, and compare to a specific pattern. */

    FBE_RAW_MIRROR_SERVICE_OPERATION_LAST /*!<  Must be last. */
}
fbe_raw_mirror_service_operation_t;


/*!*******************************************************************
 * @struct fbe_raw_mirror_service_io_specification_t
 *********************************************************************
 * @brief
 *  Structure that describes an I/O specification to the raw mirror.
 *********************************************************************/
typedef struct fbe_raw_mirror_service_io_specification_s
{
    /*! Type of operation we need.  
     * e.g. write/read/check, write same/read/check, read only, write only, etc. 
     */
    fbe_raw_mirror_service_operation_t operation;

    /*!< Lba to use for the request. 
     */
    fbe_lba_t start_lba; 

    /*! Block size for the request. 
     */
    fbe_u32_t block_size;

    /*! Block count for the request. 
     */
    fbe_u32_t block_count;

    /*! Specifies the pattern to be used in cases where we are writing.
     */
    fbe_data_pattern_t pattern;

    /*! Sequence identifier used as part of data field. 
     */
    fbe_u32_t sequence_id;

    /*! Sequence number to add to data field for writes; used as part 
     *  of error handling on reads.
     */
    fbe_u64_t sequence_num;

    /*! Boolean indicating if DMC should be detected or not for this test. 
     */
    fbe_bool_t dmc_expected_b;

    /*! Max number of I/Os to issue before stopping. 
     */
    fbe_u32_t max_passes;

    /*! Number of threads to use for this request. 
     */
    fbe_u32_t num_threads;
}
fbe_raw_mirror_service_io_specification_t;


/*!*******************************************************************
 * @struct fbe_raw_mirror_service_control_start_io_t
 *********************************************************************
 * @brief
 *  Structure used to kick off I/O to the raw mirror.
 *********************************************************************/
typedef struct fbe_raw_mirror_service_control_start_io_s
{
    /*! This is the specification of the I/O being run to the raw mirror.
     */
    fbe_raw_mirror_service_io_specification_t specification;

    /*! Verify error info.
    */
    fbe_raid_verify_raw_mirror_errors_t verify_errors;

    /*! Total number of I/Os for this test. 
     */
    fbe_u32_t io_count;

    /*! Total number of errors. 
     */
    fbe_u32_t err_count;

    /*! Total number of media errors with data lost. 
     */
    fbe_u32_t dead_err_count;

    /*! Status and qualifier; used for single-I/O tests.
     */
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    /*! Edge index of a raw mirror PVD. Used to determine when PVD 
     *  edge is enabled after being reinserted as part of a drive
     *  removal/reinsert test.
     */
    fbe_u32_t pvd_edge_index;

    /*! Max time to wait for PVD edge to be enabled. 
     */
    fbe_u32_t timeout_ms;

    /*! Boolean used to determine if PVD edge enabled or not. 
     */
    fbe_bool_t pvd_edge_enabled_b;
}
fbe_raw_mirror_service_control_start_io_t;


#endif /* FBE_RAW_MIRROR_SERVICE_INTERFACE_H */

/*************************
 * end file fbe_raw_mirror_service_interface.h
 *************************/

