#ifndef FBE_API_RDGEN_INTERFACE_H
#define FBE_API_RDGEN_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_rdgen_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_api interface for the rdgen service.
 *  This service is named after the raid I/O generator.
 *  This service allows for generating I/O to any block edge.
 *
 * @ingroup fbe_api_neit_package_interface_class_files
 *
 * @version
 *  1/29/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_types.h"
#include "fbe_service_manager.h"
#include "fbe_topology_interface.h"
#include "fbe_rdgen.h"

enum {
    FBE_API_RDGEN_DEFAULT_MEM_SIZE_MB = 72, /*!< Default memory allocation */
    FBE_API_RDGEN_WAIT_MS = 500000,
 }; 

//----------------------------------------------------------------
// Define the top level group for the FBE NEIT Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_api_neit_package_class FBE NEIT Package APIs 
 *
 *  @brief 
 *    This is the set of definitions for FBE NEIT Package APIs.
 * 
 *  @ingroup fbe_api_neit_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API RDGEN
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_rdgen_interface_usurper_interface FBE API for Rdgen Service Interface 
 *  @brief
 *    This is the set of definitions that comprise the
 *    FBE API rdgen service interface structures.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!*******************************************************************
 * @enum fbe_api_rdgen_peer_options_t
 *********************************************************************
 * @brief
 *  Options flags that can be used to tailor the way we direct I/O to the peer.
 *********************************************************************/
typedef enum fbe_api_rdgen_peer_options_e
{
    /*! When the option is set to invalid we only issue locally.
     */
    FBE_API_RDGEN_PEER_OPTIONS_INVALID = 0,

        /* These are flags for now, we may later decide to make these into
         *  their own enum.  
         */
    FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER    = 0x001, /*!< Send using peer. */

    /*! We will always have the same number of threads 
     *   being issued locally and being issued by the peer.
     */
    FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE      = 0x002, 
    FBE_API_RDGEN_PEER_OPTIONS_RANDOM            = 0x004, /*!< Randomly send locally or to peer. */
    FBE_API_RDGEN_PEER_OPTIONS_LAST
}
fbe_api_rdgen_peer_options_t;

/*!********************************************************************* 
 * @struct fbe_api_rdgen_context_t 
 *  
 * @brief 
 *   Contains the context we use for making it easier to
 *   test with rdgen.
 *
 * @ingroup fbe_api_rdgen_interface
 * @ingroup fbe_api_rdgen_context
 **********************************************************************/
typedef struct fbe_api_rdgen_context_s
{
    fbe_bool_t b_initialized;
    fbe_object_id_t object_id; /*!< Object id to test. */
    
    /*! Semaphore to use to wait for rdgen control packet to finish on this object. 
     */
    fbe_semaphore_t semaphore; 

    fbe_status_t status; /*!< Status of the rdgen control operation. */
    fbe_packet_t packet; /*!< Packet to use for sending the control operation. */
    
    /*! The control operation to send to rdgen. 
     */
    fbe_rdgen_control_start_io_t start_io; 

    /*! Alternate completion and context in case we want to override completion.  
     * This is not needed by default. 
     */
    fbe_packet_completion_function_t completion_function;
    fbe_packet_completion_context_t completion_context;
}
fbe_api_rdgen_context_t;

/*!********************************************************************* 
 * @struct fbe_api_rdgen_get_stats_t 
 *  
 * @brief 
 *   Information returned with rdgen statistics.
 *
 * @ingroup fbe_api_rdgen_interface
 * @ingroup fbe_api_rdgen_get_stats
 **********************************************************************/
typedef struct fbe_api_rdgen_get_stats_s
{
    fbe_u32_t objects; /*!< Object info */
    fbe_u32_t threads; /*!< Threads that it runs on */
    fbe_u32_t requests; /*!< Total number of requests. */

    /*! Total number of I/Os completed for the operation. 
     *  This includes individual I/Os needed to complete a pass.
     *  So for example a write/read/check would have two I/os minimum,
     *  but will only account for one pass.
     */
    fbe_u32_t io_count;

    /*! Total number of passes completed for the operation.
     */
    fbe_u32_t pass_count;

    /*! Total number of errors encountered for the operation.
     */
    fbe_u32_t error_count;

    /*! Total number of media errors encountered for the operation.
     */
    fbe_u32_t media_error_count;

    /*! Total number of aborted errors encountered for the operation.
     */
    fbe_u32_t aborted_error_count;

    /*! Total number of io failure errors encountered for the operation.
     */
    fbe_u32_t io_failure_error_count;
    fbe_u32_t congested_err_count;
    fbe_u32_t still_congested_err_count;
    fbe_u32_t invalid_request_err_count;

    /*! Total number of invalidated blocks encoutnered during failed reads for the operation.
     */
    fbe_u32_t inv_blocks_count;

    /*! Total number of blocks encountered with bad crc's during reads for the operation.
     */
    fbe_u32_t bad_crc_blocks_count;

    /*! Overall historical statistics.
     */
    fbe_rdgen_io_statistics_t historical_stats;
    fbe_rdgen_memory_type_t memory_type; /*!< Kind of memory we allocated. */
    fbe_u32_t memory_size_mb;      /*!< Memory size in MB. */
    fbe_time_t io_timeout_msecs; /*!< milliseconds before we timeout I/Os. */
}
fbe_api_rdgen_get_stats_t;

typedef fbe_u64_t fbe_api_rdgen_handle_t;

/*!*******************************************************************
 * @struct fbe_api_rdgen_thread_info_t
 *********************************************************************
 * @brief
 *  Returns information about threads.
 *********************************************************************/
typedef struct fbe_api_rdgen_thread_info_s
{
    /*! This is the handle for this thread.
     */
    fbe_api_rdgen_handle_t thread_handle;

    /*! This is the handle to our object.
     */
    fbe_api_rdgen_handle_t object_handle;

    /*! This is the handle to the request that originated this thread.
     */
    fbe_api_rdgen_handle_t request_handle;

    /*! set to TRUE if we are aborted.
     */
    fbe_bool_t b_aborted;

    fbe_u32_t err_count;
    fbe_u32_t abort_err_count;
    fbe_u32_t media_err_count;
    fbe_u32_t io_failure_err_count;

    /*! This is used when seeding the pattern in a block.  
     * This seed typically does not go beyond 0xFF so we can fit it in 
     * the upper bits of an LBA. 
     */
    fbe_u32_t pass_count;

    /*! This is the number of total I/O passes we have performed.  
     * This does not account for the I/Os that occur due to a breakup. 
     * And if multiple I/Os are needed for a pass (write/read for example), 
     * then this only gets incremented once. 
     */
    fbe_u32_t io_count;

    fbe_u32_t partial_io_count;
    fbe_u32_t memory_size; /*! Number of bytes allocated.*/
    /*! Set to TRUE when this request has been stopped by the user and we are 
     * trying to terminate the thread. 
     */
    fbe_bool_t b_stop;

    fbe_u32_t elapsed_milliseconds; /*! Number of milliseconds since thread started. */

    fbe_time_t last_response_time;/*!< Last response took this long. */
    fbe_time_t max_response_time; /*!< Max of all responses. */

    fbe_time_t cumulative_response_time; /*!< In milliseconds */
    fbe_time_t cumulative_response_time_us; /*!< In microseconds. */
    fbe_u64_t total_responses; /*!< Number of responses that have gone into cumulative. */

    fbe_raid_verify_error_counts_t verify_errors;
}
fbe_api_rdgen_thread_info_t;

/*!*******************************************************************
 * @struct fbe_api_rdgen_get_thread_info_t
 *********************************************************************
 * @brief Returns information about threads.
 *********************************************************************/
typedef struct fbe_api_rdgen_get_thread_info_s
{
    fbe_u32_t num_threads; /*!< Output number of threads that follow. */

    fbe_api_rdgen_thread_info_t thread_info_array[1]; /*!< Must be last. */
}
fbe_api_rdgen_get_thread_info_t;

/*!*******************************************************************
 * @struct fbe_api_rdgen_request_info_t
 *********************************************************************
 * @brief
 *  Returns information about requests.
 *********************************************************************/
typedef struct fbe_api_rdgen_request_info_s
{
    /*! This is the handle for this request.
     */
    fbe_api_rdgen_handle_t request_handle;

    /*! This is the count of active ts on this object.
     */
    fbe_u32_t active_ts_count;

    /*! This is the specification of the I/O.
     */
    fbe_rdgen_io_specification_t specification;

    /*! Original filter for this request.
     */
    fbe_rdgen_filter_t filter;

    /*! Set to TRUE when this request has been stopped by the user.
     */
    fbe_bool_t b_stop;

    /*! This is the number of passes completed for this request. 
     */
    fbe_u32_t pass_count;

    /*! This is the number of total I/Os completed for this request. 
     */
    fbe_u32_t io_count;

    /*! Number of errors encountered for this request.
     */
    fbe_u32_t err_count;

    /*! Number of aborted errors encountered for this request.
     */
    fbe_u32_t aborted_err_count;

    /*! Number of media errors encountered for this request.
     */
    fbe_u32_t media_err_count;

    /*! Number of io_failure errors encountered for this request.
     */
    fbe_u32_t io_failure_err_count;

    /*! Time since the request was started.
     */
    fbe_u32_t elapsed_seconds;

    fbe_raid_verify_error_counts_t verify_errors;
}
fbe_api_rdgen_request_info_t;

/*!*******************************************************************
 * @struct fbe_api_rdgen_get_request_info_t
 *********************************************************************
 * @brief Returns information about requests.
 *********************************************************************/
typedef struct fbe_api_rdgen_get_request_info_s
{
    fbe_u32_t num_requests; /*!< Output number of requests that follow. */

    fbe_api_rdgen_request_info_t request_info_array[1]; /*!< Must be last. */
}
fbe_api_rdgen_get_request_info_t;

/*!*******************************************************************
 * @struct fbe_api_rdgen_object_info_t
 *********************************************************************
 * @brief
 *  Returns information about objects.
 *********************************************************************/
typedef struct fbe_api_rdgen_object_info_s
{
    /*! This is the handle for this object.
     */
    fbe_api_rdgen_handle_t object_handle;

    /*! This is the object we are testing.
     */
    fbe_object_id_t object_id;

    /*! This is package this object is in.
     */
    fbe_object_id_t package_id;

    /*! This is the count of active ts on this object.
     */
    fbe_u32_t active_ts_count;

    /*! This is the count of active requests on this object.
     */
    fbe_u32_t active_request_count;

    /*! This is the number of passes completed on this object 
     */
    fbe_u32_t num_passes;

    /*! This is the number of total I/Os completed on this object 
     */
    fbe_u32_t num_ios;

    /*! This is the object capacity.
     */
    fbe_lba_t capacity;

    /*! This is the object's stripe size.
     */
    fbe_block_count_t stripe_size;

    /*! Block size of the object.
     */
    fbe_block_size_t block_size;

    /*! The name of the object we are running to.
     */
    fbe_char_t driver_object_name[FBE_RDGEN_DEVICE_NAME_CHARS];
}
fbe_api_rdgen_object_info_t;

/*!*******************************************************************
 * @struct fbe_api_rdgen_get_object_info_t
 *********************************************************************
 * @brief Returns information about objects.
 *********************************************************************/
typedef struct fbe_api_rdgen_get_object_info_s
{
    fbe_u32_t num_objects; /*!< Output number of objects that follow. */

    fbe_api_rdgen_object_info_t object_info_array[1]; /*!< Must be last. */
}
fbe_api_rdgen_get_object_info_t;
/*!************************************************
 * @struct fbe_api_rdgen_test_io_case_t 
 *  
 *   Structure for specifying I/O cases to test.
 *  
 *************************************************/
typedef struct fbe_api_rdgen_test_io_case_s
{
    /* Specifies the block sizes to test with.
     */
    fbe_block_size_t exported_block_size;
    fbe_block_size_t imported_block_size;

    /* The below control the lba ranges and block counts that we test. 
     * Start/end lba control the outer loop, and start/end blocks controls the 
     * inner loop. 
     * The increment block also is used to increment the blocks for the inner 
     * loop. 
     */
    fbe_lba_t start_lba;
    fbe_lba_t end_lba;
    fbe_block_count_t start_blocks;
    fbe_block_count_t end_blocks;
    fbe_block_count_t increment_blocks;
    /* Specifies the optimal block sizes to test with.
     */
    fbe_block_size_t exported_optimal_block_size;
    fbe_block_size_t imported_optimal_block_size;
}
fbe_api_rdgen_test_io_case_t;

/*!*******************************************************************
 * @struct fbe_api_rdgen_send_one_io_params_t
 *********************************************************************
 * @brief Structure that specifies all parameters to the
 *        fbe_api_rdgen_send_on_io_params() function.
 *
 *********************************************************************/
typedef struct fbe_api_rdgen_send_one_io_params_s
{
    /*! object id to run against.If this is FBE_INVALID_OBJECT_ID,  
     *  we use the class id.  
     */
    fbe_object_id_t object_id;
    /*! class id to run against.  
     * We start one operation for every member of this class. 
     * If this is FBE_INVALID_CLASS_ID, we use the object id. 
     */
    fbe_class_id_t class_id;
    fbe_package_id_t package_id; /*!< Package to run I/O against.*/
    fbe_rdgen_operation_t rdgen_operation; /*! type of rdgen test to start. */

    /*!The data content to put into the buffers 
     * that will be written (if this is a media modifying operation)  
     * or the data content we will check for (if this is a read operation). 
     */ 
    fbe_rdgen_pattern_t pattern;

    fbe_lba_t lba; /*!< logical block address of operation */
    fbe_u64_t blocks;  /*!< number of blocks in operation */
    fbe_rdgen_options_t options;  /*!< special options. */
    /*! If this is 0, then this is unused. 
     *   Otherwise this is the milliseconds before this I/O expires.
     *     The time to expiration starts when rdgen issues the I/O.
     */
    fbe_time_t msecs_to_expiration;

    /*! If this is 0, then this is unused.  
     *   Otherwise this is the number of milliseconds
     *   that pass before the I/O should be aborted by rdgen.
     */
    fbe_time_t msecs_to_abort;
    /*! This block spec is optional.  We only use this if this is not invalid.
     */
    fbe_rdgen_block_specification_t block_spec;

    /*! This lba spec is optional.  We only use this if this is not invalid.
     */
    fbe_rdgen_lba_specification_t lba_spec;

    /*! This inc lba is optional.  We only use this if this is not 0.
     */
    fbe_block_count_t inc_lba;
    /*! This end lba is optional.  We only use this if this is not 0.
     */
    fbe_lba_t end_lba;

    fbe_bool_t b_async; /*!< TRUE if we should not wait for completion. FALSE to wait. */
}
fbe_api_rdgen_send_one_io_params_t;
/*! @} */ /* end of group fbe_api_rdgen_interface_usurper_interface */

/************************************************** 
 * FBE RDGEN CONTEXT API FUNCTIONS 
 *  
 * The below are functions used to test with the rdgen c 
 * ontext structure. 
 ***************************************************/
//----------------------------------------------------------------
// Define the group for the FBE API RDGEN Interface.  
// This is where all the function prototypes for the RDGEN API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_rdgen_interface FBE API RDGEN Interface
 *  @brief 
 *    This is the set of FBE API RDGEN Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_rdgen_interface.h.
 *
 *  @ingroup fbe_api_neit_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t fbe_api_rdgen_set_trace_level(fbe_u32_t trace_level);
fbe_status_t fbe_api_rdgen_get_object_info(fbe_api_rdgen_get_object_info_t *info_p,
                                           fbe_u32_t num_objects);
fbe_status_t fbe_api_rdgen_get_request_info(fbe_api_rdgen_get_request_info_t *info_p,
                                            fbe_u32_t num_requests);
fbe_status_t fbe_api_rdgen_get_thread_info(fbe_api_rdgen_get_thread_info_t *info_p,
                                           fbe_u32_t num_threads);
fbe_status_t fbe_api_rdgen_run_tests(fbe_api_rdgen_context_t *context_p,
                                      fbe_package_id_t package_id,
                                      fbe_u32_t num_tests);

fbe_status_t fbe_api_rdgen_wait_for_ios(fbe_api_rdgen_context_t *context_p,
                                        fbe_package_id_t package_id,
                                        fbe_u32_t num_tests);

fbe_status_t fbe_api_rdgen_start_tests(fbe_api_rdgen_context_t *context_p,
                                        fbe_package_id_t package_id,
                                        fbe_u32_t num_tests);
fbe_status_t fbe_api_rdgen_stop_tests(fbe_api_rdgen_context_t *context_p,
                                       fbe_u32_t num_tests);
fbe_status_t fbe_api_cli_rdgen_start_tests(fbe_api_rdgen_context_t *context_p, 
                                           fbe_package_id_t package_id,
                                           fbe_u32_t num_tests);
fbe_status_t fbe_api_rdgen_send_stop_packet(fbe_rdgen_filter_t *filter_p);
fbe_status_t fbe_api_rdgen_wait_for_stop_operation(fbe_api_rdgen_context_t *context_p);
fbe_status_t fbe_api_rdgen_test_context_init(fbe_api_rdgen_context_t *context_p,
                                         fbe_object_id_t object_id,
                                         fbe_class_id_t class_id,
                                         fbe_package_id_t package_id,
                                         fbe_rdgen_operation_t rdgen_operation,
                                         fbe_rdgen_pattern_t pattern,
                                         fbe_u32_t pass_count,
                                         fbe_u32_t num_ios,
                                         fbe_time_t time,
                                         fbe_u32_t threads,
                                         fbe_rdgen_lba_specification_t lba_spec,
                                         fbe_lba_t start_lba,
                                         fbe_lba_t min_lba,
                                         fbe_lba_t max_lba,
                                         fbe_rdgen_block_specification_t block_spec,
                                         fbe_u64_t min_blocks,
                                         fbe_u64_t max_blocks);
fbe_status_t fbe_api_rdgen_test_context_destroy(fbe_api_rdgen_context_t *context_p);
fbe_status_t fbe_api_rdgen_get_status(fbe_api_rdgen_context_t *context_p,
                                       fbe_u32_t num_tests);
fbe_status_t fbe_api_rdgen_test_context_run_all_tests(fbe_api_rdgen_context_t *context_p,
                                                      fbe_package_id_t package_id,
                                                      fbe_u32_t num_tests);
fbe_status_t fbe_api_rdgen_send_one_io(fbe_api_rdgen_context_t *context_p,
                                       fbe_object_id_t object_id,
                                       fbe_class_id_t class_id,
                                       fbe_package_id_t package_id,
                                       fbe_rdgen_operation_t rdgen_operation,
                                       fbe_rdgen_pattern_t pattern,
                                       fbe_lba_t lba,
                                       fbe_u64_t blocks,
                                       fbe_rdgen_options_t options,
                                       fbe_time_t msecs_to_expiration,
                                       fbe_time_t msecs_to_abort,
                                       fbe_api_rdgen_peer_options_t peer_options);

fbe_status_t fbe_api_rdgen_send_one_io_asynch(fbe_api_rdgen_context_t *context_p,
                                       fbe_object_id_t object_id,
                                       fbe_class_id_t class_id,
                                       fbe_package_id_t package_id,
                                       fbe_rdgen_operation_t rdgen_operation,
                                       fbe_rdgen_pattern_t pattern,
                                       fbe_lba_t lba,
                                       fbe_u64_t blocks,
                                       fbe_rdgen_options_t options);
void fbe_api_rdgen_get_interface_string(fbe_rdgen_interface_type_t io_interface,
                                        const fbe_char_t **interface_string_p);
fbe_status_t fbe_api_rdgen_get_stats(fbe_api_rdgen_get_stats_t *info_p,
                                     fbe_rdgen_filter_t *filter_p);
fbe_status_t fbe_api_rdgen_get_complete_stats(fbe_rdgen_control_get_stats_t *info_p,
                                              fbe_rdgen_filter_t *filter_p);
fbe_status_t fbe_api_rdgen_reset_stats(void);
void fbe_api_rdgen_get_operation_string(fbe_rdgen_operation_t operation,
                                        const fbe_char_t **operation_string_p);
fbe_status_t fbe_api_rdgen_set_trace_level(fbe_u32_t trace_level);
void fbe_api_rdgen_display_context_status(fbe_api_rdgen_context_t *context_p,
                                          fbe_trace_func_t trace_func,
                                          fbe_trace_context_t trace_context);
fbe_status_t fbe_api_rdgen_set_context(fbe_api_rdgen_context_t *context_p,
                                       fbe_packet_completion_function_t completion_function,
                                       fbe_packet_completion_context_t completion_context);
fbe_status_t fbe_api_rdgen_test_io_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
/*!*******************************************************************
 * @def FBE_API_RDGEN_TEST_IO_INVALID_FIELD
 *********************************************************************
 * @brief Used to mark the end of an array of structures.
 *
 *********************************************************************/
#define FBE_API_RDGEN_TEST_IO_INVALID_FIELD FBE_U32_MAX

fbe_status_t fbe_api_rdgen_test_io_write_read_check( fbe_object_id_t object_id,
                                                     fbe_lba_t lba,
                                                     fbe_block_count_t blocks,
                                                     fbe_block_size_t exported_block_size,
                                                     fbe_block_size_t exported_opt_block_size,
                                                     fbe_block_size_t imported_block_size,
                                                     fbe_payload_block_operation_status_t *const write_status_p,
                                                     fbe_payload_block_operation_qualifier_t * const write_qualifier_p,
                                                     fbe_payload_block_operation_status_t *const read_status_p,
                                                     fbe_payload_block_operation_qualifier_t * const read_qualifier_p );

fbe_status_t fbe_api_rdgen_test_io_run_write_same_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                                       fbe_object_id_t object_id,
                                                       fbe_u32_t max_case_index);

fbe_status_t fbe_api_rdgen_test_io_run_write_read_check_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                                             fbe_object_id_t object_id,
                                                             fbe_u32_t max_case_index);
fbe_status_t fbe_api_rdgen_test_io_run_write_verify_read_check_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                                                    fbe_object_id_t object_id,
                                                                    fbe_u32_t max_case_index);
fbe_status_t fbe_api_rdgen_test_io_run_write_only_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                                       fbe_object_id_t object_id,
                                                       fbe_u32_t max_case_index);
fbe_status_t fbe_api_rdgen_test_io_run_verify_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                                   fbe_object_id_t object_id,
                                                   fbe_u32_t max_case_index);
fbe_status_t fbe_api_rdgen_test_io_run_read_only_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                                      fbe_object_id_t object_id,
                                                      fbe_u32_t max_case_index);
fbe_status_t fbe_api_rdgen_wait_for_all_threads_to_stop(fbe_u32_t timeout_ms);
fbe_status_t fbe_api_rdgen_io_specification_set_affinity(fbe_rdgen_io_specification_t *io_spec_p,
                                                         fbe_rdgen_affinity_t affinity,
                                                         fbe_u32_t core_credit_table);
fbe_status_t fbe_api_rdgen_io_specification_set_max_passes(fbe_rdgen_io_specification_t *io_spec_p,
                                                           fbe_u32_t max_passes);
fbe_status_t fbe_api_rdgen_context_check_io_status(fbe_api_rdgen_context_t *context_p,
                                           fbe_u32_t num_tests,
                                           fbe_status_t status,
                                           fbe_u32_t err_count,
                                           fbe_bool_t b_non_zero_io_count,
                                           fbe_rdgen_operation_status_t rdgen_status,
                                           fbe_payload_block_operation_status_t bl_status,
                                           fbe_payload_block_operation_qualifier_t bl_qual);
fbe_status_t fbe_api_rdgen_context_validate_aborted_status(fbe_api_rdgen_context_t *context_p,
                                                           fbe_u32_t num_tests);
fbe_status_t fbe_api_rdgen_context_reinit(fbe_api_rdgen_context_t *context_p,
                                          fbe_u32_t num_tests);

fbe_status_t fbe_api_rdgen_io_params_init(fbe_api_rdgen_send_one_io_params_t *params_p);
fbe_status_t fbe_api_rdgen_send_one_io_params(fbe_api_rdgen_context_t *context_p,
                                              fbe_api_rdgen_send_one_io_params_t *params_p);
fbe_status_t fbe_api_rdgen_io_specification_set_alignment_size(fbe_rdgen_io_specification_t* io_spec_p,
                                                               fbe_u32_t align_blocks);
fbe_status_t fbe_api_rdgen_io_specification_set_interface(fbe_rdgen_io_specification_t* io_spec_p,
                                                          fbe_rdgen_interface_type_t io_interface);
fbe_status_t fbe_api_rdgen_set_sequence_count_seed(fbe_rdgen_io_specification_t *io_spec_p,
                                                   fbe_u32_t sequence_count_seed);
fbe_status_t fbe_api_rdgen_set_random_seed(fbe_rdgen_io_specification_t *io_spec_p,
                                           fbe_u32_t random_seed);
fbe_status_t fbe_api_rdgen_allocate_peer_memory(void);
fbe_status_t fbe_api_rdgen_set_default_timeout(fbe_time_t timeout_ms);
fbe_status_t fbe_api_rdgen_io_specification_set_originating_sp(fbe_rdgen_io_specification_t *io_spec_p,
                                                               fbe_rdgen_sp_id_t originating_sp);
void fbe_api_rdgen_get_status_string(fbe_rdgen_operation_status_t rdgen_status, const fbe_char_t **string_p);
void fbe_api_rdgen_get_block_spec_string(fbe_rdgen_block_specification_t block_spec, const fbe_char_t **string_p);
void fbe_api_rdgen_get_lba_spec_string(fbe_rdgen_lba_specification_t lba_spec, const fbe_char_t **string_p);

fbe_status_t fbe_api_rdgen_io_specification_set_reject_flags(fbe_rdgen_io_specification_t *spec_p,
                                                             fbe_u8_t client_reject_allowed_flags,
                                                             fbe_u8_t arrival_reject_allowed_flags);
fbe_status_t fbe_api_rdgen_set_priority(fbe_rdgen_io_specification_t *io_spec_p,
                                        fbe_packet_priority_t priority);
/************************************************** 
 * FBE RDGEN API FUNCTIONS 
 *  
 * The below are helpers that are used to initialize 
 * the contents of the structures sent to rdgen. 
 ***************************************************/

fbe_status_t fbe_api_rdgen_filter_init(fbe_rdgen_filter_t *filter_p,
                                       fbe_rdgen_filter_type_t filter_type,
                                       fbe_object_id_t object_id,
                                       fbe_class_id_t class_id,
                                       fbe_package_id_t package_id,
                                       fbe_u32_t edge_index);
fbe_status_t fbe_api_rdgen_filter_init_for_driver(fbe_rdgen_filter_t *filter_p,
                                                  fbe_rdgen_filter_type_t filter_type,
                                                  fbe_char_t *object_name_prefix_p,
                                                  fbe_u32_t object_number);
fbe_status_t fbe_api_rdgen_filter_init_for_playback(fbe_rdgen_filter_t *filter_p,
                                                    fbe_rdgen_filter_type_t filter_type,
                                                    fbe_char_t *file_name_p);
fbe_status_t fbe_api_rdgen_filter_init_for_block_device(fbe_rdgen_filter_t *filter_p,
                                                        fbe_rdgen_filter_type_t filter_type,
                                                        fbe_char_t *object_name);
fbe_status_t fbe_api_rdgen_io_specification_init(fbe_rdgen_io_specification_t *io_spec_p,
                                                 fbe_rdgen_operation_t operation,
                                                 fbe_u32_t threads);
fbe_status_t fbe_api_rdgen_io_specification_set_lbas(fbe_rdgen_io_specification_t *io_spec_p,
                                                     fbe_rdgen_lba_specification_t lba_spec,
                                                     fbe_lba_t start_lba,
                                                     fbe_lba_t min_lba,
                                                     fbe_lba_t max_lba);
fbe_status_t fbe_api_rdgen_io_specification_set_blocks(fbe_rdgen_io_specification_t *io_spec_p,
                                                       fbe_rdgen_block_specification_t block_spec,
                                                       fbe_block_count_t min_blocks,
                                                       fbe_block_count_t max_blocks);
fbe_status_t fbe_api_rdgen_io_specification_set_num_ios(fbe_rdgen_io_specification_t *io_spec_p,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t number_of_ios,
                                                        fbe_time_t msec_to_run);
fbe_status_t fbe_api_rdgen_io_specification_set_io_complete_flags(fbe_rdgen_io_specification_t *io_spec_p,
                                                                  fbe_rdgen_io_complete_flags_t flags);
fbe_status_t fbe_api_rdgen_io_specification_set_pattern(fbe_rdgen_io_specification_t *io_spec_p,
                                                        fbe_rdgen_pattern_t pattern);
fbe_status_t fbe_api_rdgen_io_specification_set_msecs_to_expiration(fbe_rdgen_io_specification_t *io_spec_p,
                                                                    fbe_time_t msecs_to_expiration);
fbe_status_t fbe_api_rdgen_io_specification_set_msecs_to_abort(fbe_rdgen_io_specification_t *io_spec_p,
                                                               fbe_time_t msecs_to_abort);
fbe_status_t fbe_api_rdgen_set_random_aborts(fbe_rdgen_io_specification_t *io_spec_p,
                                             fbe_time_t msecs_to_abort);
fbe_status_t fbe_api_rdgen_clear_random_aborts(fbe_rdgen_io_specification_t *io_spec_p);
fbe_status_t fbe_api_rdgen_io_specification_set_inc_lba_blocks(fbe_rdgen_io_specification_t *io_spec_p,
                                                               fbe_block_count_t inc_blocks,
                                                               fbe_block_count_t inc_lba);
fbe_status_t fbe_api_rdgen_io_specification_set_options(fbe_rdgen_io_specification_t *io_spec_p,
                                                        fbe_rdgen_options_t options);
fbe_status_t fbe_api_rdgen_io_specification_set_extra_options(fbe_rdgen_io_specification_t *io_spec_p,
                                                              fbe_rdgen_extra_options_t options);
fbe_status_t fbe_api_rdgen_io_specification_set_dp_flags(fbe_rdgen_io_specification_t *io_spec_p,
                                                         fbe_rdgen_data_pattern_flags_t flags);
                                                         
fbe_status_t fbe_api_rdgen_io_specification_set_peer_options(fbe_rdgen_io_specification_t *io_spec_p,
                                                             fbe_api_rdgen_peer_options_t options);

fbe_status_t fbe_api_rdgen_run_cmi_perf_test(fbe_rdgen_control_cmi_perf_t *cmi_perf_p);

fbe_status_t fbe_api_rdgen_init_dps_memory(fbe_u32_t mem_size_in_mb,
                                           fbe_rdgen_memory_type_t memory_type);
fbe_status_t fbe_api_rdgen_release_dps_memory(void);

fbe_status_t fbe_api_rdgen_configure_system_threads(fbe_bool_t enable);
fbe_status_t fbe_api_rdgen_set_svc_options(fbe_rdgen_svc_options_t *options_p);

fbe_status_t fbe_api_rdgen_set_msec_delay(fbe_rdgen_io_specification_t *io_spec_p,
                                          fbe_u32_t msec_delay);
/*! @} */ /* end of group fbe_api_rdgen_interface */

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


#endif /* FBE_API_RDGEN_INTERFACE_H */

/*************************
 * end file fbe_api_rdgen_interface.h
 *************************/
