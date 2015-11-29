/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_rdgen_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains rdgen test context related functions.
 *
 * @ingroup fbe_api_neit_package_interface_class_files
 * @ingroup fbe_api_rdgen_interface
 *
 * @version
 *   8/19/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_api_rdgen_test_context_init()
 ****************************************************************
 * @brief
 *  Initialize the test context with all the needed parameters.
 *
 * @param context_p - context structure to initialize.
 * @param object_id - object id to run against.
 *                   If this is FBE_INVALID_OBJECT_ID,
 *                   we use the class id.
 * @param class_id - class id to run against.
 *                   If this is FBE_INVALID_CLASS_ID,
 *                   we use the object id.
 * @param package_id - Package to run I/O against.
 * @param rdgen_operation - type of rdgen test to start.
 * @param pattern - data pattern to generate
 * @param max_passes - maximum passes
 * @param num_ios - Number of ios to run.
 * @param msecs_to_run - if num ios is zero we use time.
 * @param threads - number of threads to kick off in parallel.
 * @param lba_spec - lba spec
 * @param start_lba - starting lba
 * @param min_lba - minimum lba
 * @param max_lba - maximun lba
 * @param block_spec - block spec
 * @param min_blocks - minimum blocks
 * @param max_blocks - maximum blocks
 * 
 * @note
 *  This structure must be destroyed via a call to
 *  fbe_api_rdgen_test_context_destroy()
 * 
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  9/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_test_context_init(fbe_api_rdgen_context_t *context_p,
                                             fbe_object_id_t object_id,
                                             fbe_class_id_t class_id,
                                             fbe_package_id_t package_id,
                                             fbe_rdgen_operation_t rdgen_operation,
                                             fbe_rdgen_pattern_t pattern,
                                             fbe_u32_t max_passes,
                                             fbe_u32_t num_ios,
                                             fbe_time_t msecs_to_run,
                                             fbe_u32_t threads,
                                             fbe_rdgen_lba_specification_t lba_spec,
                                             fbe_lba_t start_lba,
                                             fbe_lba_t min_lba,
                                             fbe_lba_t max_lba,
                                             fbe_rdgen_block_specification_t block_spec,
                                             fbe_u64_t min_blocks,
                                             fbe_u64_t max_blocks)
{
    fbe_status_t status;
    fbe_status_t return_status = FBE_STATUS_OK;

    context_p->object_id = object_id;
    context_p->status = FBE_STATUS_INVALID;
    context_p->packet.magic_number = FBE_MAGIC_NUMBER_INVALID;
    fbe_semaphore_init(&context_p->semaphore, 0, 1);

    fbe_api_rdgen_set_context(context_p, fbe_api_rdgen_test_io_completion, context_p);

    fbe_zero_memory(&context_p->start_io, sizeof(context_p->start_io));

    if ((object_id == FBE_OBJECT_ID_INVALID) &&
        (class_id != FBE_OBJECT_ID_INVALID))
    {
        /* Init a class filter.
         */
        status = fbe_api_rdgen_filter_init(&context_p->start_io.filter,
                                       FBE_RDGEN_FILTER_TYPE_CLASS,
                                       FBE_OBJECT_ID_INVALID,
                                       class_id,
                                       package_id,
                                       0 /* edge index */);
    }
    else
    {
        /* Init an object filter.
         */
        status = fbe_api_rdgen_filter_init(&context_p->start_io.filter,
                                       FBE_RDGEN_FILTER_TYPE_OBJECT,
                                       object_id,
                                       FBE_CLASS_ID_INVALID,
                                       package_id,
                                       0 /* edge index */);
    }

    status = fbe_api_rdgen_io_specification_init(&context_p->start_io.specification,
                                             rdgen_operation,
                                             threads /* Threads */);
    if (status != FBE_STATUS_OK)
    {
        return_status = status;
    }

    status = fbe_api_rdgen_io_specification_set_blocks(&context_p->start_io.specification,
                                                   block_spec,
                                                   min_blocks,
                                                   max_blocks);
    if (status != FBE_STATUS_OK)
    {
        return_status = status;
    }

    status = fbe_api_rdgen_io_specification_set_lbas(&context_p->start_io.specification,
                                                 lba_spec,
                                                 start_lba, min_lba, max_lba );
    if (status != FBE_STATUS_OK)
    {
        return_status = status;
    }

    status = fbe_api_rdgen_io_specification_set_num_ios(&context_p->start_io.specification, 
                                                    max_passes, num_ios, msecs_to_run);
    if (status != FBE_STATUS_OK)
    {
        return_status = status;
    }

    status = fbe_api_rdgen_io_specification_set_pattern(&context_p->start_io.specification,
                                                    pattern);
    if (status != FBE_STATUS_OK)
    {
        return_status = status;
    }

    /* Note that we always return the last error status. 
     * By continuing on error, it allows the caller to test error cases 
     * where invalid parameters are actually sent to rdgen. 
     */
    context_p->b_initialized = FBE_TRUE; 
    return return_status;
}
/******************************************
 * end fbe_api_rdgen_test_context_init()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_test_context_destroy()
 ****************************************************************
 * @brief
 *  Destroy the context structure and clean up all resources.
 *
 * @param context_p - pointer to the structure to destroy.
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  9/11/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_test_context_destroy(fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Only destroy the packet if we have initialized it.
     */
    if (context_p->packet.magic_number != FBE_MAGIC_NUMBER_INVALID)
    {
        status = fbe_transport_destroy_packet(&context_p->packet);
    }
    if (context_p->b_initialized = FBE_FALSE)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "destroying uninitialized context: %p\n", context_p);
    }

    context_p->b_initialized = FBE_FALSE; 
    fbe_semaphore_destroy(&context_p->semaphore);
    return status;
}
/******************************************
 * end fbe_api_rdgen_test_context_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_test_io_completion()
 ****************************************************************
 * @brief
 *  The completion function for sending tests to rdgen.
 *
 * @param packet_p
 * @param context
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  9/11/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t 
fbe_api_rdgen_test_io_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_api_rdgen_context_t *context_p = (fbe_api_rdgen_context_t *)context;
    fbe_status_t packet_status;

    packet_status = fbe_transport_get_status_code(packet_p);

    /* Get the payload and control payload.
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Save status inside context.
     */
    if ((control_p->status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
        (packet_status != FBE_STATUS_OK))
    {
        context_p->status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        context_p->status = FBE_STATUS_OK;
    }
    /* Wake up the caller.
     */
    fbe_semaphore_release(&context_p->semaphore, 0, 1, FALSE);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_api_rdgen_test_io_completion()
 ******************************************/
fbe_status_t fbe_api_rdgen_set_context(fbe_api_rdgen_context_t *context_p,
                                       fbe_packet_completion_function_t completion_function,
                                       fbe_packet_completion_context_t completion_context)
{
    context_p->completion_context = completion_context;
    context_p->completion_function = completion_function;
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_api_cli_rdgen_send_start_operation()
 ****************************************************************
 * @brief
 *  Kick off testing for a given context structure. This is purely
 *  used when rdgen is started from fbecli. Nobody else should be
 *  using this interface.
 *
 * @param context_p - structure to send for.      
 * @param package_id - package where rdgen service is.  
 * @param control_code - rdgen usurper command to send.
 *
 * @note 
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  21/11/2012 - Created. Arun S
 *
 ****************************************************************/
fbe_status_t fbe_api_cli_rdgen_send_start_operation(fbe_api_rdgen_context_t *context_p, 
                                                    fbe_package_id_t package_id, 
                                                    fbe_payload_control_operation_opcode_t control_code)
{
    fbe_status_t status;
    fbe_packet_t * packet_p = &context_p->packet;
	fbe_api_control_operation_status_info_t status_info;

    if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_transport_initialize_packet(packet_p);
    }
    else {
        fbe_transport_initialize_sep_packet(packet_p);
    }

    status = fbe_api_common_send_control_packet_to_service(control_code,
                                                                  &context_p->start_io,
                                                                  sizeof(fbe_rdgen_control_start_io_t),
                                                                  FBE_SERVICE_ID_RDGEN,
                                                                  FBE_PACKET_FLAG_RDGEN,
																  &status_info,
                                                                  package_id);

    /* If the status is either OK or pending we consider this successful.
     */
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, control_code: 0x%x status:%d\n", 
                       __FUNCTION__, control_code, status); 
         return status;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_cli_rdgen_send_start_operation()
 **************************************/

/*!**************************************************************
 * fbe_api_rdgen_send_start_operation()
 ****************************************************************
 * @brief
 *  Kick off testing for a given context structure.
 *
 * @param context_p - structure to send for.      
 * @param package_id - package where rdgen service is.  
 * @param control_code - rdgen usurper command to send.
 *
 * @note 
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  9/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_send_start_operation(fbe_api_rdgen_context_t *context_p,
                                                fbe_package_id_t package_id,
                                                fbe_payload_control_operation_opcode_t control_code)
{
    fbe_status_t status;
    fbe_packet_t * packet_p = &context_p->packet;

    if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_transport_initialize_packet(packet_p);
    }
    else {
        fbe_transport_initialize_sep_packet(packet_p);
    }

    status = fbe_api_common_send_control_packet_to_service_asynch(packet_p,
                                                                  control_code,
                                                                  &context_p->start_io,
                                                                  sizeof(fbe_rdgen_control_start_io_t),
                                                                  FBE_SERVICE_ID_RDGEN,
                                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                                  context_p->completion_function, 
                                                                  context_p->completion_context,
                                                                  package_id);

    /* If the status is either OK or pending we consider this successful.
     */
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, control_code: 0x%x status:%d\n", 
                       __FUNCTION__, control_code, status); 
         return status;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_send_start_operation()
 **************************************/

/*!**************************************************************
 * fbe_api_rdgen_send_stop_packet()
 ****************************************************************
 * @brief
 *  Kick off testing for a given context structure.
 *
 * @param filter_p - structure to send for.        
 * 
 * @return FBE_STATUS_OK
 *
 * @author
 *  4/14/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_send_stop_packet(fbe_rdgen_filter_t *filter_p)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_rdgen_control_stop_io_t stop_info;
    fbe_status_t status;

    stop_info.filter = *filter_p;

    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_STOP_IO,
                                                  &stop_info,
                                                  sizeof(fbe_rdgen_control_stop_io_t),
                                                  FBE_SERVICE_ID_RDGEN,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_NEIT);
    
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                   "%s stop of rdgen failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_send_stop_packet()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_wait_for_stop_operation()
 ****************************************************************
 * @brief
 *  Kick off testing for a given context structure.
 *
 * @param context_p - structure to send for.        
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  9/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_wait_for_stop_operation(fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t status;

    if (context_p->b_initialized != FBE_TRUE)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s context not initialized\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Wait for the operation we stopped to complete.
     */
    status = fbe_semaphore_wait_ms(&context_p->semaphore, FBE_API_RDGEN_WAIT_MS);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, 
                      "%s semaphore wait failed with status of 0x%x\n",
                      __FUNCTION__, status);
        return status;
    }
    if (context_p->start_io.filter.filter_type == FBE_RDGEN_FILTER_TYPE_OBJECT)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                      "rdgen test for object id: 0x%x stopped successfully. \n",
                      context_p->object_id);
    }
    else
    {
        fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                      "rdgen test for class id: 0x%x stopped successfully. \n",
                      context_p->start_io.filter.class_id);
    }
    fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                  "passes: 0x%x io_cnt: 0x%x err_cnt: 0x%x abrt_cnt: 0x%x media_e_cnt: 0x%x io_fail_cnt: 0x%x \n",
                  context_p->start_io.statistics.pass_count,
                  context_p->start_io.statistics.io_count,
                  context_p->start_io.statistics.error_count,
                  context_p->start_io.statistics.aborted_error_count,
                  context_p->start_io.statistics.media_error_count,
                  context_p->start_io.statistics.io_failure_error_count);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_wait_for_stop_operation()
 **************************************/

/*!**************************************************************
 * fbe_api_rdgen_wait_for_stop_operation_and_destroy()
 ****************************************************************
 * @brief
 *  Kick off testing for a given context structure.
 *
 * @param context_p - structure to send for.        
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  10/23/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_wait_for_stop_operation_and_destroy(fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t status;

    status = fbe_api_rdgen_wait_for_stop_operation(context_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    fbe_api_rdgen_test_context_destroy(context_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_wait_for_stop_operation_and_destroy()
 **************************************/

/*!**************************************************************
 * fbe_api_rdgen_stop_tests()
 ****************************************************************
 * @brief
 *  Kick off testing for a given context structure array.
 *
 * @param context_p - array of structures to test.
 * @param num_tests - number of tests in structure. 
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_stop_tests(fbe_api_rdgen_context_t *context_p,
                                       fbe_u32_t num_tests)
{
    fbe_status_t status;
    fbe_status_t return_status = FBE_STATUS_OK;
    fbe_u32_t test_index;
    /* Stop all tests.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        status = fbe_api_rdgen_send_stop_packet(&context_p[test_index].start_io.filter);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                          "%s stop of rdgen failed status: 0x%x\n", __FUNCTION__, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Wait for them to complete.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        status = fbe_api_rdgen_wait_for_stop_operation_and_destroy(&context_p[test_index]);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }
    return return_status;
}
/**************************************
 * end fbe_api_rdgen_stop_tests()
 **************************************/

/*!**************************************************************
 * fbe_api_cli_rdgen_start_tests()
 ****************************************************************
 * @brief
 *  Kick off testing for a given context structure array. This is 
 *  purely for the users who starts rdgen from fbecli. Nobody else
 *  should be using this. 
 *
 * @param context_p - array of structures to test.
 * @param package_id - package where rdgen service is.  
 * @param num_tests - number of tests in structure. 
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_cli_rdgen_start_tests(fbe_api_rdgen_context_t *context_p, 
                                           fbe_package_id_t package_id, 
                                           fbe_u32_t num_tests)
{
    fbe_status_t status;
    fbe_u32_t test_index;

    /* Start tests. */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        status = fbe_api_cli_rdgen_send_start_operation(&context_p[test_index], 
                                                        package_id, 
                                                        FBE_RDGEN_CONTROL_CODE_START_IO);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }
    return status;
}
/**************************************
 * end fbe_api_cli_rdgen_start_tests()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_start_tests()
 ****************************************************************
 * @brief
 *  Kick off testing for a given context structure array.
 *
 * @param context_p - array of structures to test.
 * @param package_id - package where rdgen service is.  
 * @param num_tests - number of tests in structure. 
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_start_tests(fbe_api_rdgen_context_t *context_p,
                                        fbe_package_id_t package_id,
                                        fbe_u32_t num_tests)
{
    fbe_status_t status;
    fbe_u32_t test_index;
    fbe_payload_control_operation_opcode_t control_code;

    /* Start tests.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        if (context_p[test_index].start_io.specification.operation == FBE_RDGEN_OPERATION_UNLOCK_RESTART) {
            control_code = FBE_RDGEN_CONTROL_CODE_UNLOCK;
        } else {
            control_code = FBE_RDGEN_CONTROL_CODE_START_IO;
        }
        status = fbe_api_rdgen_send_start_operation(&context_p[test_index],
                                                     package_id,
                                                     control_code);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }
    return status;
}
/**************************************
 * end fbe_api_rdgen_start_tests()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_display_context_status()
 ****************************************************************
 * @brief
 *  Display the completion status for an rdgen context.
 *  This includes displaying some performance numbers.
 *
 * @param context_p - Context to display for.               
 *
 * @return None. 
 *
 * @author
 *  4/28/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_api_rdgen_display_context_status(fbe_api_rdgen_context_t *context_p,
                                          fbe_trace_func_t trace_func,
                                          fbe_trace_context_t trace_context)
{
    fbe_u64_t ios_per_second = 0;
    fbe_u64_t min_ios_per_second = 0;
    fbe_u64_t max_ios_per_second = 0;
    fbe_u64_t seconds_per_io = 0;
    fbe_u64_t min_seconds_per_io = 0;
    fbe_u64_t max_seconds_per_io = 0;
    if (context_p->start_io.statistics.elapsed_msec)
    {
        ios_per_second = (context_p->start_io.statistics.io_count * FBE_TIME_MILLISECONDS_PER_SECOND) /
                         context_p->start_io.statistics.elapsed_msec;
    }
    if (context_p->start_io.statistics.min_rate_msec)
    {
        min_ios_per_second = (context_p->start_io.statistics.min_rate_ios * FBE_TIME_MILLISECONDS_PER_SECOND) /
                             context_p->start_io.statistics.min_rate_msec;
    }
    if (context_p->start_io.statistics.max_rate_msec)
    {
        max_ios_per_second = (context_p->start_io.statistics.max_rate_ios * FBE_TIME_MILLISECONDS_PER_SECOND) /
                             context_p->start_io.statistics.max_rate_msec;
    }
    if (context_p->start_io.statistics.io_count)
    {
        seconds_per_io = context_p->start_io.statistics.elapsed_msec /
                         (context_p->start_io.statistics.io_count * FBE_TIME_MILLISECONDS_PER_SECOND);
    }
    if (context_p->start_io.statistics.min_rate_ios)
    {
        min_seconds_per_io = context_p->start_io.statistics.min_rate_msec /
                             (context_p->start_io.statistics.min_rate_ios * FBE_TIME_MILLISECONDS_PER_SECOND);
    }
    if (context_p->start_io.statistics.max_rate_ios)
    {
        max_seconds_per_io = context_p->start_io.statistics.max_rate_msec /
                             (context_p->start_io.statistics.max_rate_ios * FBE_TIME_MILLISECONDS_PER_SECOND);
    }

    if (context_p->object_id != FBE_OBJECT_ID_INVALID)
    {
        trace_func(trace_context,
                   "rdgen complete obj: 0x%x passes: 0x%x ios: 0x%x msec: %llu "
                   "errs: 0x%x io/s: %llu min io/s: %llu max io/s: %llu\n",
                   context_p->object_id,
                   context_p->start_io.statistics.pass_count,
		   context_p->start_io.statistics.io_count,
                   (unsigned long long)context_p->start_io.statistics.elapsed_msec,
		   context_p->start_io.statistics.error_count,
                   (unsigned long long)ios_per_second,
		   (unsigned long long)min_ios_per_second,
		   (unsigned long long)max_ios_per_second);
        /* If I/O is very slow, we will display both io/s and sec/io
         */
        if ((ios_per_second == 0) || (min_ios_per_second == 0) || (max_ios_per_second == 0))
        {
            trace_func(trace_context, "rdgen seconds/io: %llu min seconds/io: %llu max seconds/io: %llu\n",
                       (unsigned long long)seconds_per_io,
		       (unsigned long long)min_seconds_per_io,
		       (unsigned long long)max_seconds_per_io);
        }
    }
    else
    {
        trace_func(trace_context,
                   "rdgen complete class: 0x%x passes: 0x%x ios: 0x%x msec: 0x%llu errs: 0x%x\n",
                   context_p->start_io.filter.class_id,
                   context_p->start_io.statistics.pass_count,
                   context_p->start_io.statistics.io_count,
                   (unsigned long long)context_p->start_io.statistics.elapsed_msec,
                   context_p->start_io.statistics.error_count);

        trace_func(trace_context,
                   "rdgen  io/s: %llu min io/s: %llu min obj: 0x%x max io/s: %llu max obj: 0x%x\n",
                   (unsigned long long)ios_per_second,
                   (unsigned long long)min_ios_per_second,
                   context_p->start_io.statistics.min_rate_object_id,
                   (unsigned long long)max_ios_per_second,
                   context_p->start_io.statistics.max_rate_object_id);

        /* If I/O is very slow, we will display both io/s and sec/io
         */
        if ((ios_per_second == 0) || (min_ios_per_second == 0) || (max_ios_per_second == 0))
        {
            trace_func(trace_context, 
                       "rdgen seconds/io: %llu min seconds/io: %llu min obj: 0x%x max seconds/io: %llu  max obj: 0x%x\n",
                       (unsigned long long)seconds_per_io, 
                       (unsigned long long)min_seconds_per_io,
		       context_p->start_io.statistics.min_rate_object_id,
                       (unsigned long long)max_seconds_per_io,
		       context_p->start_io.statistics.max_rate_object_id);
        }
    }
    return;
}
/******************************************
 * end fbe_api_rdgen_display_context_status()
 ******************************************/
/*!**************************************************************
 * fbe_api_rdgen_run_tests()
 ****************************************************************
 * @brief
 *  Kick off testing for a given context structure array.
 *
 * @param context_p - array of structures to test.
 * @param package_id - package where rdgen service is.
 * @param num_tests - number of tests in structure.
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  9/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_run_tests(fbe_api_rdgen_context_t *context_p,
                                      fbe_package_id_t package_id,
                                      fbe_u32_t num_tests)
{
    fbe_status_t status;
    fbe_u32_t test_index;

    status = fbe_api_rdgen_start_tests(context_p, package_id, num_tests);

    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Wait for all tests to finish.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        
        fbe_time_t wait_ms = 0;
        if (context_p[test_index].start_io.specification.max_number_of_ios != 0)
        {
            /* We will conservatively guess that it takes one second per I/O as an upper limit.
             */ 
            wait_ms = (fbe_time_t)FBE_TIME_MILLISECONDS_PER_SECOND * context_p[test_index].start_io.specification.max_number_of_ios;
        }
        else if (context_p[test_index].start_io.specification.max_passes != 0)
        {
            /* We will conservatively guess that it takes one second per I/O as an upper limit.
             */ 
            wait_ms = (fbe_time_t)FBE_TIME_MILLISECONDS_PER_SECOND * context_p[test_index].start_io.specification.max_passes;
        }
        else if (context_p[test_index].start_io.specification.msec_to_run)
        {
            /* Just use the milliseconds.
             */
            wait_ms = context_p[test_index].start_io.specification.msec_to_run;
        }

        if (wait_ms < FBE_API_RDGEN_WAIT_MS)
        {
            /* Small number of I/Os or something other than passes, num_ios or msecs specified.  Just wait for a
             * constant time. 
             */
            wait_ms = FBE_API_RDGEN_WAIT_MS;
        }
        status = fbe_semaphore_wait_ms(&context_p[test_index].semaphore, (fbe_u32_t)wait_ms);

        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s semaphore wait failed with status of 0x%x\n",
                          __FUNCTION__, status);
            return status;
        }
    }
    return status;
}
/**************************************
 * end fbe_api_rdgen_run_tests()
 **************************************/



/*!**************************************************************
 * fbe_api_rdgen_wait_for_ios()
 ****************************************************************
 * @brief
 *  Wait for the ios to complete
 *
 * @param context_p - array of structures to test.
 * @param package_id - package where rdgen service is.
 * @param num_tests - number of tests in structure.
 *
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  07/31/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_wait_for_ios(fbe_api_rdgen_context_t *context_p,
                                        fbe_package_id_t package_id,
                                        fbe_u32_t num_tests)
{    
    fbe_status_t status;
    fbe_u32_t test_index;   

    /* Wait for all tests to finish.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        status = fbe_semaphore_wait_ms(&context_p[test_index].semaphore, FBE_API_RDGEN_WAIT_MS);

        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, 
                          "%s semaphore wait failed with status of 0x%x\n",
                          __FUNCTION__, status);
            return status;
        }
        if (context_p[test_index].object_id != FBE_OBJECT_ID_INVALID)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                       "rdgen test for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x \n",
                       context_p[test_index].object_id,
                       context_p[test_index].start_io.statistics.pass_count,
                       context_p[test_index].start_io.statistics.io_count,
                       context_p[test_index].start_io.statistics.error_count);
        }
        else
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                       "rdgen test for class id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x \n",
                       context_p[test_index].start_io.filter.class_id,
                       context_p[test_index].start_io.statistics.pass_count,
                       context_p[test_index].start_io.statistics.io_count,
                       context_p[test_index].start_io.statistics.error_count);
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_wait_for_ios()
 **************************************/


/*!**************************************************************
 * fbe_api_rdgen_get_status()
 ****************************************************************
 * @brief
 *  Determines the status for a set of context structures.
 *
 * @param context_p - array of structures to determine status for.
 * @param num_tests - number of tests in structure.
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  9/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_get_status(fbe_api_rdgen_context_t *context_p,
                                       fbe_u32_t num_tests)
{
    fbe_u32_t test_index;

    /* Iterate over all tests, looking for an error.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        if (context_p[test_index].status != FBE_STATUS_OK)
        {
            /* We received an error, just return the error status.
             */
            return context_p[test_index].status;
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_get_status()
 **************************************/

/*!**************************************************************
 * fbe_api_rdgen_test_context_run_all_tests()
 ****************************************************************
 * @brief
 *  Run the entire test from start to finish.
 *
 * @param context_p - The context structure.
 * @param package_id - package where rdgen service is.
 * @param num_tests - The number of tests to run               
 *
 * @return fbe_status_t
 *
 * @author
 *  9/25/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_test_context_run_all_tests(fbe_api_rdgen_context_t *context_p,
                                                      fbe_package_id_t package_id,
                                                      fbe_u32_t num_tests)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t test_index;
    fbe_api_rdgen_context_t * const orig_context_p = context_p;

    /* Run all the tests. */
    status = fbe_api_rdgen_run_tests(context_p, package_id, num_tests);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s line %d status %d \n", __FUNCTION__, __LINE__, status);
    }

    /* Get the status from the context structure.
     * Make sure no errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(context_p, num_tests);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                  "%s line %d status %d \n", __FUNCTION__, __LINE__, status);
    }

    /* Destroy all the contexts (even if we got an error).*/
    context_p = orig_context_p;
    for ( test_index = 0; test_index < num_tests; test_index++)
    {
        fbe_status_t destroy_status;
        /* Destroy the test objects.*/
        destroy_status = fbe_api_rdgen_test_context_destroy(context_p);

        if (destroy_status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                          "%s line %d status %d\n", __FUNCTION__, __LINE__, destroy_status);
            status = destroy_status;
        }
        context_p++;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_test_context_run_all_tests()
 ******************************************/
/*!**************************************************************
 * fbe_api_rdgen_test_context_run_all_tests_check_status()
 ****************************************************************
 * @brief
 *  Run the entire test from start to finish.
 *
 * @param context_p - The context structure.
 * @param package_id - package where rdgen service is.
 * @param num_tests - The number of tests to run               
 *
 * @return fbe_status_t
 *
 * @author
 *  5/19/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_test_context_run_all_tests_check_status(fbe_api_rdgen_context_t *context_p,
                                                                   fbe_package_id_t package_id,
                                                                   fbe_u32_t num_tests)
{
    fbe_status_t status;
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, package_id, num_tests);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in starting test status:%d\n",
                      __FUNCTION__, status);
        return status; 
    }

    if (context_p->start_io.status.status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in running test status:%d\n",
                      __FUNCTION__, context_p->start_io.status.status);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    if ((context_p->start_io.status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) ||
        (context_p->start_io.status.block_qualifier != FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE))
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s block payload error in running test status: 0x%x qualifier: 0x%x\n",
                      __FUNCTION__, context_p->start_io.status.block_status, context_p->start_io.status.block_qualifier);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    return status;
}
/******************************************
 * end fbe_api_rdgen_test_context_run_all_tests_check_status()
 ******************************************/
/*!**************************************************************
 *  fbe_api_rdgen_send_one_io()
 ****************************************************************
 * @brief
 *  Run just a single I/O through rdgen.
 *
 * @param context_p - context structure to initialize.
 * @param object_id - object id to run against.
 *                   If this is FBE_INVALID_OBJECT_ID,
 *                   we use the class id.
 * @param class_id - class id to run against.
 *                   We start one operation for every member of this class.
 *                   If this is FBE_INVALID_CLASS_ID,
 *                   we use the object id.
 * @param package_id - Package to run I/O against.
 * @param rdgen_operation - type of rdgen test to start.
 * @param pattern - The data content to put into the buffers
 *                  that will be written (if this is a media modifying operation)
 *                  or the data content we will check for 
 *                  (if this is a read operation).
 * @param lba - lba to issue op to.
 * @param blocks - Blocks to issue for op.
 * @param options - Special options for this I/O.
 * @param msecs_to_expiration - If this is 0, then this is unused.
 *                              Otherwise this is the milliseconds before this I/O expires.
 *                              The time to expiration starts
 *                              when rdgen issues the I/O.
 * @param msecs_to_abort - If this is 0, then this is unused.
 *                         Otherwise this is the number of milliseconds
 *                         that pass before the I/O should be aborted
 *                         by rdgen.
 * @param peer_options - Peer options for this I/O
 *
 * @return fbe_status_t
 *
 * @author
 *  2/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

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
                                       fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t                status;
    fbe_lba_t                   end_lba = lba + blocks - 1;

    /* If the lba is invalid we are trying to write to the last lba in the LUN. 
     * Adjust the end lba appropriately. 
     */
    if (lba == FBE_LBA_INVALID)
    {
        end_lba = FBE_LBA_INVALID;
    }

    /* We set this up as a fixed lba constant block size pattern for one I/O.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id,
                                             class_id, 
                                             package_id,
                                             rdgen_operation,
                                             pattern,
                                             0x1,    /* We do one I/O. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_FIXED,
                                             lba,    /* Start lba*/
                                             lba,    /* Min lba */
                                             end_lba,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             blocks,    /* Min blocks */
                                             blocks    /* Max blocks */ );
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s init context status %d \n", __FUNCTION__, status);
        return status;
    }
    /* If we have options then set them.
     */
    if (options != FBE_RDGEN_OPTIONS_INVALID)
    {
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                            options);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                          "%s unable to set options status %d \n", __FUNCTION__, status);
            return status;
        }
    }
    fbe_api_rdgen_io_specification_set_msecs_to_expiration(&context_p->start_io.specification,
                                                           msecs_to_expiration);
    fbe_api_rdgen_io_specification_set_msecs_to_abort(&context_p->start_io.specification,
                                                      msecs_to_abort);

    /* If we have peer options set them.
     */
    if (peer_options != FBE_API_RDGEN_PEER_OPTIONS_INVALID)
    {
        fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                        peer_options);
    }

    /* Write a background pattern to the LUNs.
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    return status;
}
/******************************************
 * end fbe_api_rdgen_send_one_io()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_send_one_io_asynch()
 ****************************************************************
 * @brief
 *  Run just a single I/O through rdgen asynchronously.
 *
 * @param context_p - context structure to initialize.
 * @param object_id - object id to run against.
 *                   If this is FBE_INVALID_OBJECT_ID,
 *                   we use the class id.
 * @param class_id - class id to run against.
 *                   If this is FBE_INVALID_CLASS_ID,
 *                   we use the object id.
 * @param package_id - Package to run I/O against.
 * @param rdgen_operation - type of rdgen test to start.
 * @param pattern - The data content to put into the buffers
 *                  that will be written (if this is a media modifying operation)
 *                  or the data content we will check for 
 *                  (if this is a read operation).
 * @param lba - lba to issue op to.
 * @param blocks - Blocks to issue for op.
 * @param options - Special options for this I/O.
 *
 * @return fbe_status_t
 *
 * @author
 *  03/12/2010 - Created. Ashwin Tamilarasan
 *  08/09/2012 - Added options field. Dave Agans
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_send_one_io_asynch(fbe_api_rdgen_context_t *context_p,
                                       fbe_object_id_t object_id,
                                       fbe_class_id_t class_id,
                                       fbe_package_id_t package_id,
                                       fbe_rdgen_operation_t rdgen_operation,
                                       fbe_rdgen_pattern_t pattern,
                                       fbe_lba_t lba,
                                       fbe_u64_t blocks,
                                       fbe_rdgen_options_t options)
{
    fbe_status_t status;

    /* We set this up as a fixed lba constant block size pattern for one I/O.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id,
                                             class_id, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             rdgen_operation,
                                             pattern,
                                             0x1,    /* We do one I/O. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_FIXED,
                                             lba,    /* Start lba*/
                                             lba,    /* Min lba */
                                             lba + blocks - 1,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             blocks,    /* Min blocks */
                                             blocks    /* Max blocks */ );
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s line %d status %d", __FUNCTION__, __LINE__, status);
        return status;
    }

    /* If we have options then set them.
     */
    if (options != FBE_RDGEN_OPTIONS_INVALID)
    {
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                            options);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                          "%s unable to set options status %d \n", __FUNCTION__, status);
            return status;
        }
    }

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    return status;
}
/**************************************
 * end fbe_api_rdgen_send_one_io_asynch()
 **************************************/

/*!**************************************************************
 * fbe_api_rdgen_get_operation_string()
 ****************************************************************
 * @brief
 *  convert an operation enum into a string that does not exceed
 *  5 characters in length.
 *
 * @param operation - op to convert to string
 * @param operation_string_p - string representing this operation
 * 
 * @return none
 *
 * @author
 *  4/15/2010 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_api_rdgen_get_operation_string(fbe_rdgen_operation_t operation,
                                        const fbe_char_t **operation_string_p)
{
    /* Note the string should not exceed 5 characters in length.
     */
    switch (operation) 
    {
        case FBE_RDGEN_OPERATION_VERIFY:
            *operation_string_p = "vr";
            break;
        case FBE_RDGEN_OPERATION_READ_ONLY:
            *operation_string_p = "read";
            break;
        case FBE_RDGEN_OPERATION_READ_CHECK:
            *operation_string_p = "rc";
            break;
        case FBE_RDGEN_OPERATION_WRITE_ONLY:
            *operation_string_p = "write";
            break;
        case FBE_RDGEN_OPERATION_WRITE_READ_CHECK:
            *operation_string_p = "wrc";
            break;
        case FBE_RDGEN_OPERATION_VERIFY_WRITE:
            *operation_string_p = "vw";
            break;
        case FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK:
            *operation_string_p = "vwrc";
            break;
        case FBE_RDGEN_OPERATION_ZERO_ONLY:
            *operation_string_p = "zero";
            break;
        case FBE_RDGEN_OPERATION_ZERO_READ_CHECK:
            *operation_string_p = "zrc";
            break;
        case FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK:
            *operation_string_p = "ccrc";
            break;
        case FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY:
            *operation_string_p = "codta";
            break;
        case FBE_RDGEN_OPERATION_READ_ONLY_VERIFY:
            *operation_string_p = "rover";
            break;
        case FBE_RDGEN_OPERATION_WRITE_SAME:
            *operation_string_p = "wrsme";
            break;
        case FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK:
            *operation_string_p = "wsrc";
            break;
        case FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK:
            *operation_string_p = "cdrc";
            break;
        case FBE_RDGEN_OPERATION_UNMAP:
            *operation_string_p = "unmap";
            break;
        default:
            *operation_string_p = "Unk";
            break;
    }
    return;
}
/**************************************
 * end fbe_api_rdgen_get_operation_string()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_get_interface_string()
 ****************************************************************
 * @brief
 *  convert a interface enum into a string.
 *
 * @param io_interface - interface to convert
 * @param interface_string_p - string representing this interface
 * 
 * @return none
 *
 * @author
 *  3/19/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_api_rdgen_get_interface_string(fbe_rdgen_interface_type_t io_interface,
                                        const fbe_char_t **interface_string_p)
{
    /* Note the string should not exceed 5 characters in length.
     */
    switch (io_interface) 
    {
        case FBE_RDGEN_INTERFACE_TYPE_IRP_DCA:
            *interface_string_p = "IRP DCA";
            break;
        case FBE_RDGEN_INTERFACE_TYPE_IRP_MJ:
            *interface_string_p = "IRP MJ";
            break;
        case FBE_RDGEN_INTERFACE_TYPE_IRP_SGL:
            *interface_string_p = "IRP SGL";
            break;
        case FBE_RDGEN_INTERFACE_TYPE_PACKET:
            *interface_string_p = "Packet";
            break;
        default:
            *interface_string_p = "UNKNOWN interface";
            break;
    }
    return;
}
/**************************************
 * end fbe_api_rdgen_get_interface_string()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_get_stats()
 ****************************************************************
 * @brief
 *  Get status from rdgen.
 *
 * @param info_p - structure to send for.        
 * @param filter_p - rdgen usurper command to send.
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  9/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_get_stats(fbe_api_rdgen_get_stats_t *info_p,
                                     fbe_rdgen_filter_t *filter_p)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_rdgen_control_get_stats_t stats;
    fbe_status_t status;
    
    stats.filter = *filter_p;

    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_GET_STATS,
                                                  &stats,
                                                  sizeof(fbe_rdgen_control_get_stats_t),
                                                  FBE_SERVICE_ID_RDGEN,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_NEIT);

    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                   "%s get status failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info_p->aborted_error_count = stats.in_process_io_stats.aborted_error_count;
    info_p->error_count = stats.in_process_io_stats.error_count;
    info_p->io_count = stats.in_process_io_stats.io_count;
    info_p->io_failure_error_count = stats.in_process_io_stats.io_failure_error_count;
    info_p->congested_err_count = stats.in_process_io_stats.congested_error_count;
    info_p->still_congested_err_count = stats.in_process_io_stats.still_congested_error_count;
    info_p->inv_blocks_count = stats.in_process_io_stats.inv_blocks_count;
    info_p->bad_crc_blocks_count = stats.in_process_io_stats.bad_crc_blocks_count;
    info_p->media_error_count = stats.in_process_io_stats.media_error_count;
    info_p->objects = stats.objects;
    info_p->pass_count = stats.in_process_io_stats.pass_count;
    info_p->threads = stats.threads;
    info_p->requests = stats.requests;
    info_p->historical_stats = stats.historical_stats;
    info_p->memory_size_mb = stats.memory_size_mb;
    info_p->memory_type = stats.memory_type;
    info_p->io_timeout_msecs = stats.io_timeout_msecs;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_get_stats()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_get_complete_stats()
 ****************************************************************
 * @brief
 *  Get status from rdgen.
 *
 * @param info_p - structure to send for.        
 * @param filter_p - rdgen usurper command to send.
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  7/22/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_get_complete_stats(fbe_rdgen_control_get_stats_t *stats_p,
                                              fbe_rdgen_filter_t *filter_p)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;
    
    stats_p->filter = *filter_p;

    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_GET_STATS,
                                                           stats_p,
                                                           sizeof(fbe_rdgen_control_get_stats_t),
                                                           FBE_SERVICE_ID_RDGEN,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);

    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                   "%s get status failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_get_complete_stats()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_reset_stats()
 ****************************************************************
 * @brief
 *  Reset overall rdgen stats.
 *    
 * @param filter_p - rdgen usurper command to send.
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  5/7/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_reset_stats(void)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_RESET_STATS,
                                                           NULL, /* no structure*/
                                                           0, /* no structure */
                                                           FBE_SERVICE_ID_RDGEN,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);

    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                   "%s get status failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_reset_stats()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_get_thread_info()
 ****************************************************************
 * @brief
 *  Get information on all rdgen threads.
 *
 * @param info_p - structure to send for.        
 * @param num_threads - The number of threads allocated
 *                      in the info_p.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/28/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_get_thread_info(fbe_api_rdgen_get_thread_info_t *info_p,
                                           fbe_u32_t num_threads)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_rdgen_control_get_thread_info_t *thread_info_p = NULL;
    fbe_u32_t thread_index = 0;
    fbe_u32_t total_bytes;
    fbe_status_t status;

    if (num_threads == 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s num threads to fetch is zero\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The size to allocate is the number of fbe_rdgen_control_thread_info_t plus the size 
     * of the fbe_rdgen_control_get_thread_info_t less the fbe_rdgen_control_thread_info_t 
     * structure. 
     */
    total_bytes = sizeof(fbe_rdgen_control_get_thread_info_t) - sizeof(fbe_rdgen_control_thread_info_t) +
        (num_threads * sizeof(fbe_rdgen_control_thread_info_t));
    /* Allocate the memory for thread info.
     */
    thread_info_p = fbe_api_allocate_memory(total_bytes);

    if (thread_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s unable to allocate memory\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    
    thread_info_p->num_threads = num_threads;

    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_GET_THREAD_INFO,
                                                  thread_info_p,
                                                  total_bytes,
                                                  FBE_SERVICE_ID_RDGEN,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_NEIT);

    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(thread_info_p);
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_free_memory(thread_info_p);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s get failed packet_status: 0x%x qual: 0x%x "
                      "control payload status: 0x%x control qualifier: 0x%x\n",
                      __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                      status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Copy over the count.
     */
    info_p->num_threads = thread_info_p->num_threads;

    /* Copy the info to the user.
     * We copy up to the max of what the user passed in or 
     * the max of what was returned. 
     */
    while ((thread_index < info_p->num_threads) &&
           (thread_index < num_threads))
    {
        info_p->thread_info_array[thread_index].thread_handle = 
            thread_info_p->thread_info_array[thread_index].thread_handle;
        info_p->thread_info_array[thread_index].abort_err_count = 
            thread_info_p->thread_info_array[thread_index].abort_err_count;
        info_p->thread_info_array[thread_index].b_aborted = 
            thread_info_p->thread_info_array[thread_index].b_aborted;
        info_p->thread_info_array[thread_index].err_count = 
            thread_info_p->thread_info_array[thread_index].err_count;
        info_p->thread_info_array[thread_index].io_count = 
            thread_info_p->thread_info_array[thread_index].io_count;
        info_p->thread_info_array[thread_index].io_failure_err_count = 
            thread_info_p->thread_info_array[thread_index].io_failure_err_count;
        info_p->thread_info_array[thread_index].media_err_count = 
            thread_info_p->thread_info_array[thread_index].media_err_count;
        info_p->thread_info_array[thread_index].memory_size = 
            thread_info_p->thread_info_array[thread_index].memory_size;
        info_p->thread_info_array[thread_index].object_handle = 
            thread_info_p->thread_info_array[thread_index].object_handle;
        info_p->thread_info_array[thread_index].partial_io_count = 
            thread_info_p->thread_info_array[thread_index].partial_io_count;
        info_p->thread_info_array[thread_index].pass_count = 
            thread_info_p->thread_info_array[thread_index].pass_count;
        info_p->thread_info_array[thread_index].request_handle = 
            thread_info_p->thread_info_array[thread_index].request_handle;
        info_p->thread_info_array[thread_index].verify_errors = 
            thread_info_p->thread_info_array[thread_index].verify_errors;
        info_p->thread_info_array[thread_index].elapsed_milliseconds = 
            thread_info_p->thread_info_array[thread_index].elapsed_milliseconds;
        info_p->thread_info_array[thread_index].cumulative_response_time = 
            thread_info_p->thread_info_array[thread_index].cumulative_response_time;
        info_p->thread_info_array[thread_index].cumulative_response_time_us = 
            thread_info_p->thread_info_array[thread_index].cumulative_response_time_us;
        info_p->thread_info_array[thread_index].total_responses = 
            thread_info_p->thread_info_array[thread_index].total_responses;
        info_p->thread_info_array[thread_index].max_response_time = 
            thread_info_p->thread_info_array[thread_index].max_response_time;
        info_p->thread_info_array[thread_index].last_response_time = 
            thread_info_p->thread_info_array[thread_index].last_response_time;
        thread_index++;
    }
    fbe_api_free_memory(thread_info_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_get_thread_info()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_get_request_info()
 ****************************************************************
 * @brief
 *  Get information on all rdgen requests.
 *
 * @param info_p - structure to send for.        
 * @param num_requests - The number of requests allocated
 *                      in the info_p.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/28/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_get_request_info(fbe_api_rdgen_get_request_info_t *info_p,
                                           fbe_u32_t num_requests)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_rdgen_control_get_request_info_t *request_info_p = NULL;
    fbe_u32_t request_index = 0;
    fbe_u32_t total_bytes;
    fbe_status_t status;

    if (num_requests == 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s num requests to fetch is zero\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* The size to allocate is the number of fbe_rdgen_control_request_info_t plus the size 
     * of the fbe_rdgen_control_get_request_info_t less the fbe_rdgen_control_request_info_t 
     * structure. 
     */
    total_bytes = sizeof(fbe_rdgen_control_get_request_info_t) - sizeof(fbe_rdgen_control_request_info_t) +
        (num_requests * sizeof(fbe_rdgen_control_request_info_t));

    /* Allocate the memory for request info.
     */
    request_info_p = fbe_api_allocate_memory(total_bytes);

    if (request_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s unable to allocate memory\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    request_info_p->num_requests = num_requests;

    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_GET_REQUEST_INFO,
                                                  request_info_p,
                                                  total_bytes,
                                                  FBE_SERVICE_ID_RDGEN,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(request_info_p);
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_free_memory(request_info_p);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                   "%s get failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Copy over the count.
     */
    info_p->num_requests = request_info_p->num_requests;

    /* Copy the info to the user.
     * We copy up to the max of what the user passed in or 
     * the max of what was returned. 
     */
    while ((request_index < info_p->num_requests) &&
           (request_index < num_requests))
    {
        info_p->request_info_array[request_index].request_handle = 
            request_info_p->request_info_array[request_index].request_handle;
        info_p->request_info_array[request_index].aborted_err_count = 
            request_info_p->request_info_array[request_index].aborted_err_count;
        info_p->request_info_array[request_index].active_ts_count = 
            request_info_p->request_info_array[request_index].active_ts_count;
        info_p->request_info_array[request_index].b_stop = 
            request_info_p->request_info_array[request_index].b_stop;
        info_p->request_info_array[request_index].elapsed_seconds = 
            request_info_p->request_info_array[request_index].elapsed_seconds;
        info_p->request_info_array[request_index].err_count = 
            request_info_p->request_info_array[request_index].err_count;
        info_p->request_info_array[request_index].filter = 
            request_info_p->request_info_array[request_index].filter;
        info_p->request_info_array[request_index].io_count = 
            request_info_p->request_info_array[request_index].io_count;
        info_p->request_info_array[request_index].io_failure_err_count = 
            request_info_p->request_info_array[request_index].io_failure_err_count;
        info_p->request_info_array[request_index].media_err_count = 
            request_info_p->request_info_array[request_index].media_err_count;
        info_p->request_info_array[request_index].pass_count = 
            request_info_p->request_info_array[request_index].pass_count;
        info_p->request_info_array[request_index].specification = 
            request_info_p->request_info_array[request_index].specification;
        info_p->request_info_array[request_index].verify_errors = 
            request_info_p->request_info_array[request_index].verify_errors;
        request_index++;
    }
    fbe_api_free_memory(request_info_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_get_request_info()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_get_object_info()
 ****************************************************************
 * @brief
 *  Get information on all rdgen objects.
 *
 * @param info_p - structure to send for.        
 * @param num_objects - The number of objects allocated
 *                      in the info_p.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/28/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_get_object_info(fbe_api_rdgen_get_object_info_t *info_p,
                                           fbe_u32_t num_objects)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_rdgen_control_get_object_info_t *object_info_p = NULL;
    fbe_u32_t object_index = 0;
    fbe_u32_t total_bytes;
    fbe_status_t status;

    if (num_objects == 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s num objects to fetch is zero\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* The size to allocate is the number of fbe_rdgen_control_object_info_t plus the size 
     * of the fbe_rdgen_control_get_object_info_t less the fbe_rdgen_control_object_info_t 
     * structure. 
     */
    total_bytes = sizeof(fbe_rdgen_control_get_object_info_t) - sizeof(fbe_rdgen_control_object_info_t) +
        (num_objects * sizeof(fbe_rdgen_control_object_info_t));

    /* Allocate the memory for object info.
     */
    object_info_p = fbe_api_allocate_memory(total_bytes);

    if (object_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s unable to allocate memory\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    
    object_info_p->num_objects = num_objects;

    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_GET_OBJECT_INFO,
                                                  object_info_p,
                                                  total_bytes,
                                                  FBE_SERVICE_ID_RDGEN,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_NEIT);

    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(object_info_p);
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_free_memory(object_info_p);
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                   "%s get failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Copy over the count.
     */
    info_p->num_objects = object_info_p->num_objects;

    /* Copy the info to the user.
     * We copy up to the max of what the user passed in or 
     * the max of what was returned. 
     */
    while ((object_index < info_p->num_objects) &&
           (object_index < num_objects))
    {
        info_p->object_info_array[object_index].object_handle = 
            object_info_p->object_info_array[object_index].object_handle;
        info_p->object_info_array[object_index].active_request_count = 
            object_info_p->object_info_array[object_index].active_request_count;
        info_p->object_info_array[object_index].active_ts_count = 
            object_info_p->object_info_array[object_index].active_ts_count;
        info_p->object_info_array[object_index].block_size = 
            object_info_p->object_info_array[object_index].block_size;
        info_p->object_info_array[object_index].capacity = 
            object_info_p->object_info_array[object_index].capacity;
        info_p->object_info_array[object_index].num_ios = 
            object_info_p->object_info_array[object_index].num_ios;
        info_p->object_info_array[object_index].num_passes = 
            object_info_p->object_info_array[object_index].num_passes;
        info_p->object_info_array[object_index].object_id = 
            object_info_p->object_info_array[object_index].object_id;
        info_p->object_info_array[object_index].package_id = 
            object_info_p->object_info_array[object_index].package_id;
        info_p->object_info_array[object_index].stripe_size = 
            object_info_p->object_info_array[object_index].stripe_size;
        csx_p_strncpy(&info_p->object_info_array[object_index].driver_object_name[0],
                      object_info_p->object_info_array[object_index].driver_object_name, 
                      FBE_RDGEN_DEVICE_NAME_CHARS);
        object_index++;
    }
    fbe_api_free_memory(object_info_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_get_object_info()
 **************************************/
/*!***************************************************************
 * fbe_api_rdgen_set_trace_level()
 ****************************************************************
 * @brief
 *  This function sets the trace level for rdgen service
 *
 * @param trace_level - the trace level to use
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_set_trace_level(fbe_u32_t trace_level)
{
    fbe_status_t                               status = FBE_STATUS_OK;
    fbe_base_service_control_set_trace_level_t set_trace_level;
    fbe_api_control_operation_status_info_t status_info;
    
    set_trace_level.trace_level = trace_level;
    
    status = fbe_api_common_send_control_packet_to_service(FBE_BASE_SERVICE_CONTROL_CODE_SET_TRACE_LEVEL,
                                                   &set_trace_level, sizeof(set_trace_level),
                                                  FBE_SERVICE_ID_RDGEN,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                   "%s set trace level failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_rdgen_set_trace_level()
 **************************************/

/*!**************************************************************
 *  fbe_api_rdgen_filter_init()
 ****************************************************************
 * @brief
 *  Initialize the io specification filter. 
 *
 * @param filter_p - filter to init.
 * @param filter_type - Filter type to use.
 * @param object_id - object to send to.
 * @param class_id - class to send to.
 * @param package_id - package to run I/O against
 * @param edge_index - Edge index to target.
 *
 * @return fbe_status_t - FBE_STATUS_OK on success.
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_filter_init(fbe_rdgen_filter_t *filter_p,
                                       fbe_rdgen_filter_type_t filter_type,
                                       fbe_object_id_t object_id,
                                       fbe_class_id_t class_id,
                                       fbe_package_id_t package_id,
                                       fbe_u32_t edge_index)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (filter_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Range check the type.
     */
    if ((filter_type == FBE_RDGEN_FILTER_TYPE_INVALID) ||
        (filter_type >= FBE_RDGEN_FILTER_TYPE_LAST))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    filter_p->filter_type = filter_type;
    filter_p->package_id = package_id;

    if ((filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_OBJECT) ||
        (filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_RAID_GROUP))
    {
        /* Sanity check the fields for an object filter.
         */
        if (object_id == FBE_OBJECT_ID_INVALID)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        filter_p->object_id = object_id;
        filter_p->class_id = FBE_CLASS_ID_INVALID;
        filter_p->edge_index = edge_index;
    }
    else if (filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_CLASS)
    {
        /* Sanity check the fields for an object filter.
         */
        if (class_id == FBE_CLASS_ID_INVALID)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        filter_p->class_id = class_id;
        filter_p->object_id = FBE_OBJECT_ID_INVALID;
        filter_p->edge_index = edge_index;
    }
    else
    {
        /* Unknown filter type.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_filter_init()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_filter_init_for_driver()
 ****************************************************************
 * @brief
 *  Initialize the io specification filter when we will
 *  be running to another driver.
 *
 * @param filter_p - filter to init.
 * @param filter_type - Filter type to use.
 * @param object_name_prefix_p - Name driver uses for prefix.
 * @param object_number - number to use for object.
 *
 * @return fbe_status_t - FBE_STATUS_OK on success.
 *
 * @author
 *  3/12/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_filter_init_for_driver(fbe_rdgen_filter_t *filter_p,
                                                  fbe_rdgen_filter_type_t filter_type,
                                                  fbe_char_t *object_name_prefix_p,
                                                  fbe_u32_t object_number)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (filter_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s filter_p is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Range check the type.
     */
    if ((filter_type == FBE_RDGEN_FILTER_TYPE_INVALID) ||
        (filter_type >= FBE_RDGEN_FILTER_TYPE_LAST))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s filter type is out of range\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    filter_p->filter_type = filter_type;
    filter_p->package_id = FBE_PACKAGE_ID_INVALID;

    if (filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE)
    {
        filter_p->object_id = object_number;
        filter_p->class_id = FBE_CLASS_ID_INVALID;
        filter_p->edge_index = 0;

        if (object_number != FBE_RDGEN_INVALID_OBJECT_NUMBER)
        {
            /* User specified the name, number separately.
             */
            csx_p_snprintf(&filter_p->device_name[0], FBE_RDGEN_DEVICE_NAME_CHARS, 
                           "\\Device\\%s%d", object_name_prefix_p, object_number);
        }
        else
        {
            /* User specified the device name completely.  No object id used.
             */
            csx_p_snprintf(filter_p->device_name, FBE_RDGEN_DEVICE_NAME_CHARS, 
                           "\\Device\\%s", object_name_prefix_p);
        }
    }
#if 0
    else if (filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_CLASS)
    {
        /* Sanity check the fields for an object filter.
         */
        if (class_id == FBE_CLASS_ID_INVALID)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        filter_p->class_id = class_id;
        filter_p->object_id = FBE_OBJECT_ID_INVALID;
        filter_p->edge_index = edge_index;
    }
#endif
    else
    {
        /* Unknown filter type.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_filter_init_for_driver()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_filter_init_for_playback()
 ****************************************************************
 * @brief
 *  Initialize the io specification filter when we will
 *  be running to another driver.
 *
 * @param filter_p - filter to init.
 * @param filter_type - Filter type to use.
 * @param file_name_p - Name driver uses for prefix.
 *
 * @return fbe_status_t - FBE_STATUS_OK on success.
 *
 * @author
 *  7/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_filter_init_for_playback(fbe_rdgen_filter_t *filter_p,
                                                    fbe_rdgen_filter_type_t filter_type,
                                                    fbe_char_t *file_name_p)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (filter_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s filter_p is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Range check the type.
     */
    if ((filter_type == FBE_RDGEN_FILTER_TYPE_INVALID) ||
        (filter_type >= FBE_RDGEN_FILTER_TYPE_LAST))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s filter type is out of range\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    filter_p->filter_type = filter_type;
    filter_p->package_id = FBE_PACKAGE_ID_INVALID;

    if (filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_PLAYBACK_SEQUENCE)
    {
        filter_p->object_id = FBE_OBJECT_ID_INVALID;
        filter_p->class_id = FBE_CLASS_ID_INVALID;
        filter_p->edge_index = 0;

        /* User specified the device name completely.  No object id used.
         */
        csx_p_snprintf(filter_p->device_name, FBE_RDGEN_DEVICE_NAME_CHARS, 
                       "%s", file_name_p);
    }
    else
    {
        /* Unknown filter type.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_filter_init_for_playback()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_filter_init_for_block_device()
 ****************************************************************
 * @brief
 *  Initialize the io specification filter when we will
 *  be running to another driver.
 *
 * @param filter_p - filter to init.
 * @param filter_type - Filter type to use.
 * @param object_name - Name driver uses for prefix.
 *
 * @return fbe_status_t - FBE_STATUS_OK on success.
 *
 * @author
 *  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_filter_init_for_block_device(fbe_rdgen_filter_t *filter_p,
                                                        fbe_rdgen_filter_type_t filter_type,
                                                        fbe_char_t *object_name)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (filter_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s filter_p is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Range check the type.
     */
    if ((filter_type == FBE_RDGEN_FILTER_TYPE_INVALID) ||
        (filter_type >= FBE_RDGEN_FILTER_TYPE_LAST))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s filter type is out of range\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    filter_p->filter_type = filter_type;
    filter_p->package_id = FBE_PACKAGE_ID_INVALID;

    if (filter_p->filter_type == FBE_RDGEN_FILTER_TYPE_BLOCK_DEVICE)
    {
        filter_p->object_id = FBE_RDGEN_INVALID_OBJECT_NUMBER;
        filter_p->class_id = FBE_CLASS_ID_INVALID;
        filter_p->edge_index = 0;

        /* User specified the device name completely.  No object id used.
         */
        csx_p_snprintf(filter_p->device_name, FBE_RDGEN_DEVICE_NAME_CHARS, 
                       "/dev/%s", object_name);
    }
    else
    {
        /* Unknown filter type.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_filter_init_for_block_device()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_io_specification_init()
 ****************************************************************
 * @brief
 *  Initialize the io specification.
 *  The user only specifies the operation to perform and the
 *  number of threads to launch.
 * 
 *  We will setup the remainder of the parameters with
 *  default values which can be overridden with other accessors.
 * 
 *  If the user wants to set any of the other parameters such as
 *  min blocks, max blocks, alignment, lba range, etc, then they need
 *  to call these other api functions to initialize those values.
 * 
 *  Note that it is important that this function be called first
 *  before calling any other method with this io spec, so that this
 *  structure can be initialized.
 *
 * @param io_spec_p - io spec to init.
 * @param operation - Specific operation to perform
 *                    (read, write, write/read/check, etc).
 * @param threads - Number of threads to start.
 *
 * @return fbe_status_t - FBE_STATUS_OK on success.
 *
 * @author
 *  6/14/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_init(fbe_rdgen_io_specification_t *io_spec_p,
                                                 fbe_rdgen_operation_t operation,
                                                 fbe_u32_t threads)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Range check the operation.
     */
    if ((operation == FBE_RDGEN_OPERATION_INVALID) ||
        (operation >= FBE_RDGEN_OPERATION_LAST))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check the number of threads.  We do have an upper limit on how many 
     * threads a given operation can kick off. 
     */
    if ((threads > FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION) ||
        (threads == 0))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* First zero out the memory for the io spec.
     */
    fbe_zero_memory(io_spec_p, sizeof(fbe_rdgen_io_specification_t));

    /* Setup the input values.
     */
    io_spec_p->threads = threads;
    io_spec_p->operation = operation;
    io_spec_p->io_interface = FBE_RDGEN_INTERFACE_TYPE_PACKET;

    /* Setup defaults for the remainder of the values that need to be set.
     */
    io_spec_p->block_spec = FBE_RDGEN_BLOCK_SPEC_RANDOM;
    io_spec_p->lba_spec = FBE_RDGEN_LBA_SPEC_RANDOM;

    /* Setup the default blocks to between 1 and FBE_RDGEN_DEFAULT_BLOCKS. 
     * The default lbas go between 0 and  FBE_LBA_INVALID (meaning capacity).
     */
    io_spec_p->min_blocks = 1;
    io_spec_p->max_blocks = FBE_RDGEN_DEFAULT_BLOCKS;
    io_spec_p->min_lba = 0;
    io_spec_p->max_lba = FBE_LBA_INVALID;
    io_spec_p->pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    io_spec_p->inc_blocks = 0;
    io_spec_p->inc_lba = 0;

    /* If this is set to 0, then it is not used.
     */
    io_spec_p->msecs_to_expiration = 0;

    /* If this is set to 0, then it is not used.
     */
    io_spec_p->msecs_to_abort = 0;

    /* By default we do not retry I/O failures.
     */
    io_spec_p->b_retry_ios = FBE_FALSE;

    /* The user never sets this.  Only requests that come from the peer 
     * will ever set this to FBE_TRUE. 
     */
    io_spec_p->b_from_peer = FBE_FALSE;

    /* By default we do not affine requests to any specific core.
     */
    io_spec_p->affinity = FBE_RDGEN_AFFINITY_NONE;
    io_spec_p->core = FBE_U32_MAX;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_init()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_io_specification_set_lbas()
 ****************************************************************
 * @brief
 *  Set the lba values to use for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param lba_spec - How to choose lba.
 * @param start_lba - First lba to issue.
 * @param min_lba - For remainder of requests minimum lba.
 * @param max_lba - For remainder of requests maximum lba.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  6/14/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_lbas(fbe_rdgen_io_specification_t *io_spec_p,
                                                     fbe_rdgen_lba_specification_t lba_spec,
                                                     fbe_lba_t start_lba,
                                                     fbe_lba_t min_lba,
                                                     fbe_lba_t max_lba)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s io_spec_p is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((lba_spec == FBE_RDGEN_LBA_SPEC_INVALID) ||
        (lba_spec >= FBE_RDGEN_LBA_SPEC_LAST))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s lba spec invalid: %d\n", __FUNCTION__, lba_spec);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    if (min_lba > max_lba)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s min_lba: 0x%llx > max_lba: 0x%llx\n", 
                      __FUNCTION__, min_lba, max_lba);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* No range checks for start, min, max, since theoretically these could 
     * contain any values including 0.
     */
    io_spec_p->lba_spec = lba_spec;
    io_spec_p->start_lba = start_lba;
    io_spec_p->min_lba = min_lba;
    io_spec_p->max_lba = max_lba;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_lbas()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_io_specification_set_blocks()
 ****************************************************************
 * @brief
 *  Set the blocks values to use for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param block_spec - How to choose blocks.
 * @param min_blocks - minimum blocks.
 * @param max_blocks - maximum blocks.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  6/14/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_blocks(fbe_rdgen_io_specification_t *io_spec_p,
                                                       fbe_rdgen_block_specification_t block_spec,
                                                       fbe_block_count_t min_blocks,
                                                       fbe_block_count_t max_blocks)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((block_spec == FBE_RDGEN_BLOCK_SPEC_INVALID) ||
        (block_spec >= FBE_RDGEN_BLOCK_SPEC_LAST))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s block spec invalid: %d\n", __FUNCTION__, block_spec);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    if (min_blocks > max_blocks)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s min_blocks: 0x%llx > max_blocks: 0x%llx\n", 
                      __FUNCTION__, min_blocks, max_blocks);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* No range checks for min, max, since theoretically these could contain any values
     * including 0. 
     */
    io_spec_p->block_spec = block_spec;
    io_spec_p->min_blocks = min_blocks;
    io_spec_p->max_blocks = max_blocks;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_blocks()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_io_specification_set_num_ios()
 ****************************************************************
 * @brief
 *  Set the number of I/Os to issue before stopping.
 *
 * @param io_spec_p - io spec to init.
 * @param max_passes - Number of I/Os to issue before stopping.
 *                        If this is 0 then not used.
 * @param number_of_ios - Passes to complete before completing.
 *                        If this is 0 then not used.
 * @param msec_to_run - Time to run before stopping (millisec).
 *               If number of I/Os or passes is set, this is ignored.
 *               If time is set to 0 it means run forever.
 * 
 * @return fbe_status_t FBE_STATUS_OK if success.
 *
 * @author
 *  6/14/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_num_ios(fbe_rdgen_io_specification_t *io_spec_p,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t number_of_ios,
                                                        fbe_time_t msec_to_run)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    io_spec_p->max_passes = max_passes;
    io_spec_p->max_number_of_ios = number_of_ios;
    io_spec_p->msec_to_run = msec_to_run;
    io_spec_p->io_complete_flags = FBE_RDGEN_IO_COMPLETE_FLAGS_NONE;

    if (number_of_ios != 0)
    {
        /* When number of I/Os is set, time to run is not used.
         */
        io_spec_p->msec_to_run = 0;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_num_ios()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_io_specification_set_io_complete_flags()
 ****************************************************************
 * @brief
 *  Set flags to indicate when we should stop the I/O.
 *
 * @param io_spec_p - io spec to init.
 * @param flags - io complete flags to set.
 * 
 * @return fbe_status_t FBE_STATUS_OK if success.
 *
 * @author
 *  7/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_io_complete_flags(fbe_rdgen_io_specification_t *io_spec_p,
                                                                  fbe_rdgen_io_complete_flags_t flags)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    io_spec_p->io_complete_flags = flags;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_io_complete_flags()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_io_specification_set_pattern()
 ****************************************************************
 * @brief
 *  Set the data pattern value to use for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param pattern - data pattern.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  10/26/2009 - Created. Moe Valois
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_pattern(fbe_rdgen_io_specification_t *io_spec_p,
                                                        fbe_rdgen_pattern_t pattern)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (pattern >= FBE_RDGEN_PATTERN_LAST)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    io_spec_p->pattern = pattern;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_pattern()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_io_specification_set_msecs_to_expiration()
 ****************************************************************
 * @brief
 *  Set the expiration time value to use for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param msecs_to_expiration - Number of milliseconds before this
 *                              packet will expire.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  4/13/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_msecs_to_expiration(fbe_rdgen_io_specification_t *io_spec_p,
                                                                    fbe_time_t msecs_to_expiration)
{   
    fbe_status_t status = FBE_STATUS_OK;
    io_spec_p->msecs_to_expiration = msecs_to_expiration;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_msecs_to_expiration()
 ******************************************/
/*!**************************************************************
 * fbe_api_rdgen_io_specification_set_msecs_to_abort()
 ****************************************************************
 * @brief
 *  Set the abort time value to use for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param msecs_to_abort - Number of milliseconds before this
 *                              packet will get cancelled.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  4/22/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_msecs_to_abort(fbe_rdgen_io_specification_t *io_spec_p,
                                                               fbe_time_t msecs_to_abort)
{   
    fbe_status_t status = FBE_STATUS_OK;
    io_spec_p->msecs_to_abort = msecs_to_abort;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_msecs_to_abort()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_set_random_aborts()
 ****************************************************************
 * @brief
 *  Set random aborts for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param msecs_to_abort - Number of milliseconds before this
 *                              packet will get cancelled.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  2/08/2010 - Created. Mahesh Agarkar
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_set_random_aborts(fbe_rdgen_io_specification_t *io_spec_p,
                                             fbe_time_t msecs_to_abort)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_api_rdgen_io_specification_set_options(io_spec_p,
                                                        FBE_RDGEN_OPTIONS_RANDOM_ABORT);

    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    status = fbe_api_rdgen_io_specification_set_msecs_to_abort(io_spec_p, msecs_to_abort);

    return status;
}
/******************************************
 * end fbe_api_rdgen_set_random_aborts()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_clear_random_aborts()
 ****************************************************************
 * @brief
 *  Set random aborts for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param msecs_to_abort - Number of milliseconds before this
 *                              packet will get cancelled.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  2/08/2010 - Created. Mahesh Agarkar
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_clear_random_aborts(fbe_rdgen_io_specification_t *io_spec_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_options_t updated_options = io_spec_p->options;

    updated_options &= ~FBE_RDGEN_OPTIONS_RANDOM_ABORT;
    io_spec_p->options = 0;
    status = fbe_api_rdgen_io_specification_set_options(io_spec_p,
                                                        updated_options);

    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* A zero msecs to abort means not to abort.
     */
    status = fbe_api_rdgen_io_specification_set_msecs_to_abort(io_spec_p, 0);

    return status;
}
/******************************************
 * end fbe_api_rdgen_clear_random_aborts()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_io_specification_set_inc_lba_blocks()
 ****************************************************************
 * @brief
 *  Setup the increment for lba and for blocks.
 *
 * @param io_spec_p - io spec to init.
 * @param inc_blocks - Amount to increment block count by.
 * @param inc_lba - Amount to incrment lba by.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  5/3/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_inc_lba_blocks(fbe_rdgen_io_specification_t *io_spec_p,
                                                               fbe_block_count_t inc_blocks,
                                                               fbe_block_count_t inc_lba)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    io_spec_p->inc_blocks = inc_blocks;
    io_spec_p->inc_lba = inc_lba;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_inc_lba_blocks()
 ******************************************/
/*!**************************************************************
 *  fbe_api_rdgen_io_specification_set_options()
 ****************************************************************
 * @brief
 *  Set the option value to use for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param options - the options to set.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  1/29/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_options(fbe_rdgen_io_specification_t *io_spec_p,
                                                        fbe_rdgen_options_t options)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (options >= FBE_RDGEN_OPTIONS_NEXT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    io_spec_p->options = options;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_options()
 ******************************************/
/*!**************************************************************
 *  fbe_api_rdgen_io_specification_set_extra_options()
 ****************************************************************
 * @brief
 *  Set the extra option value to use for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param options - the options to set.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  4/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_extra_options(fbe_rdgen_io_specification_t *io_spec_p,
                                                              fbe_rdgen_extra_options_t options)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (options >= FBE_RDGEN_OPTIONS_NEXT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_rdgen_specification_extra_options_set(io_spec_p, options);
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_extra_options()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_io_specification_set_dp_flags()
 ****************************************************************
 * @brief
 *  Set the option value to use for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param flags - the flags to set.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  12/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_dp_flags(fbe_rdgen_io_specification_t *io_spec_p,
                                                         fbe_rdgen_data_pattern_flags_t flags)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (flags > FBE_RDGEN_DATA_PATTERN_FLAGS_MAX_VALUE)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    io_spec_p->data_pattern_flags = flags;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_dp_flags()
 ******************************************/
/*!**************************************************************
 *  fbe_api_rdgen_io_specification_set_peer_options()
 ****************************************************************
 * @brief
 *  Set the option value to use for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param options - the options to set.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  1/29/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_peer_options(fbe_rdgen_io_specification_t *io_spec_p,
                                                             fbe_api_rdgen_peer_options_t options)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (options >= FBE_API_RDGEN_PEER_OPTIONS_LAST)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     From Robert Foley's email dated 3/11/2013:
      "This function can be changed to take the fbe_api_rdgen_peer_options_t.
       io_spec_p->peer_options must remain with type fbe_rdgen_peer_options_t"
    */
    io_spec_p->peer_options = options;
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_peer_options()
 ******************************************/

static fbe_api_rdgen_context_t fbe_api_rdgen_test_io_context;
struct fbe_api_rdgen_test_io_params_s;
typedef fbe_status_t (*fbe_api_rdgen_test_io_test_fn_t)(fbe_api_rdgen_test_io_case_t *const cases_p,
                                                        struct fbe_api_rdgen_test_io_params_s *params_p);
/*!*******************************************************************
 * @struct fbe_api_rdgen_test_io_params_t
 *********************************************************************
 * @brief This defines the test rdgen should execute.
 *
 *********************************************************************/
typedef struct fbe_api_rdgen_test_io_params_s
{
    fbe_object_id_t object_id;
    fbe_package_id_t package_id;
    fbe_rdgen_operation_t rdgen_operation;
    fbe_rdgen_pattern_t background_pattern;
    fbe_rdgen_pattern_t pattern;
    fbe_api_rdgen_test_io_test_fn_t setup;
    fbe_api_rdgen_test_io_test_fn_t test;
    fbe_lba_t current_lba;
    fbe_block_count_t current_blocks;
}
fbe_api_rdgen_test_io_params_t;


/*!***************************************************************
 * fbe_api_rdgen_test_io_fill_area()
 ****************************************************************
 * @brief
 *  This function fills an area with a known pattern.
 *
 * @param io_case_p - io case to run for.
 * @param params_p - The parameters to use in test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  05/20/08 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_test_io_fill_area(fbe_api_rdgen_test_io_case_t *io_case_p,
                                             fbe_api_rdgen_test_io_params_t *params_p)
{
    fbe_status_t status;
    fbe_lba_t lba = io_case_p->start_lba;
    fbe_block_count_t max_lba;
    fbe_api_rdgen_context_t *context_p = &fbe_api_rdgen_test_io_context;
    fbe_block_count_t max_blocks;
    /* Start out with writing the total number of blocks we will be reading.
     *  
     */
    max_lba = (fbe_block_count_t)(io_case_p->end_lba - lba + io_case_p->end_blocks);

    /* Then round the max_lba up to the optimal block size boundary if
     * needed. 
     */
    if (max_lba % io_case_p->exported_optimal_block_size)
    {
        max_lba += (io_case_p->exported_optimal_block_size - 
                            (max_lba % io_case_p->exported_optimal_block_size));
    }
    /* Try for a large block count 0x800 is a reasonable count.  Make sure it is 
     * no larger than the max lba. 
     */
    max_blocks = FBE_MIN(0x800, max_lba);
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             params_p->object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             params_p->package_id,
                                             params_p->rdgen_operation,
                                             params_p->background_pattern,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             max_lba,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             max_blocks,    /* Min blocks */
                                             max_blocks    /* Max blocks */ );
    
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in initting context status:%d\n",
                      __FUNCTION__, status);
        return status; 
    }
    status = fbe_api_rdgen_test_context_run_all_tests_check_status(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    return status;
}
/* end fbe_api_rdgen_test_io_fill_area() */

/*!***************************************************************
 * fbe_api_rdgen_test_io_send_io
 ****************************************************************
 * @brief
 *  This function fills an area with a known pattern.
 *
 * @param io_case_p - io case to run for.
 * @param params_p - The parameters to use in test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  05/20/08 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_test_io_send_io(fbe_api_rdgen_test_io_case_t *io_case_p,
                                           fbe_api_rdgen_test_io_params_t *params_p)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *context_p = &fbe_api_rdgen_test_io_context;

    status = fbe_api_rdgen_test_context_init(context_p, 
                                             params_p->object_id, 
                                             FBE_CLASS_ID_INVALID,
                                             params_p->package_id,
                                             params_p->rdgen_operation,
                                             params_p->pattern,
                                             1,    /* One pass */
                                             0,    /* io count not used */
                                             0,    /* time not used*/
                                             1,
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             io_case_p->start_lba,    /* Start lba*/
                                             io_case_p->start_lba,    /* Min lba */
                                             io_case_p->end_lba, /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_INCREASING,
                                             io_case_p->start_blocks,    /* Min blocks */
                                             io_case_p->end_blocks    /* Max blocks */ );
    
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in initting context status:%d\n",
                      __FUNCTION__, status);
        return status; 
    }

    /* Set up the test to that we will iterate over both lba and blocks.
     */
    status = fbe_api_rdgen_io_specification_set_inc_lba_blocks(&context_p->start_io.specification,
                                                               io_case_p->increment_blocks,
                                                               0);
    
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in setting inc lba blocks :%d\n",
                      __FUNCTION__, status);
        return status; 
    }

    /* Run the test.
     */
    status = fbe_api_rdgen_test_context_run_all_tests_check_status(context_p, FBE_PACKAGE_ID_NEIT, 1);
    return status;
}
/**************************************
 * end fbe_api_rdgen_test_io_send_io()
 **************************************/
/*!***************************************************************
 * fbe_api_rdgen_test_io_loop_over_cases()
 ****************************************************************
 * @brief
 *  This function performs an io test.
 * 
 * @param io_test_object_p - Test object to run cases for.
 * @param cases_p - These are the test cases we are executing.
 * @param max_case - This is the maximum case index we will execute.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  5/3/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_test_io_loop_over_cases(fbe_api_rdgen_test_io_case_t *const cases_p,
                                                   fbe_api_rdgen_test_io_params_t *params_p,
                                                   fbe_u32_t max_case)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;

    /* Loop over all of our test cases.
     * Stop when either we hit the max case or 
     * when we hit the end of this test array (FBE_U32_MAX). 
     */
    while (cases_p[index].exported_block_size != FBE_U32_MAX &&
           index < max_case &&
           status == FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, "fbe test io case slba: 0x%llx elba: 0x%llx "
                      "sbl: 0x%llx ebl: 0x%llx exp: 0x%x imp_bl: 0x%x Status: %d", 
                      (unsigned long long)cases_p[index].start_lba, (unsigned long long)cases_p[index].end_lba, 
                      (unsigned long long)cases_p[index].start_blocks, (unsigned long long)cases_p[index].end_blocks,
                      cases_p[index].exported_block_size,
                      cases_p[index].imported_block_size,
                      status);
        status = params_p->setup(&cases_p[index], params_p);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        status = params_p->test(&cases_p[index], params_p);
        
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        index++;
    } /* End while cases_p[index].exported_block_size != 0 */

    return status;
}
/* end fbe_api_rdgen_test_io_loop_over_cases() */
/*!***************************************************************
 *  fbe_api_rdgen_test_io_run_write_read_check_test()
 ****************************************************************
 * @brief
 *  This function performs a write/read/check test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - maximum case number to run.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_rdgen_test_io_run_write_read_check_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                      fbe_object_id_t object_id,
                                      fbe_u32_t max_case_index)
{
    fbe_api_rdgen_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    test_params.setup = fbe_api_rdgen_test_io_fill_area;
    test_params.test = fbe_api_rdgen_test_io_send_io;
    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_api_rdgen_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_api_rdgen_test_io_run_write_read_check_test() */

/*!***************************************************************
 * fbe_api_rdgen_test_io_run_write_verify_read_check_test()
 ****************************************************************
 * @brief
 *  This function performs a write verify/read/check test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - maximum case number to run.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  12/09/08 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_rdgen_test_io_run_write_verify_read_check_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                             fbe_object_id_t object_id,
                                             fbe_u32_t max_case_index)
{
    fbe_api_rdgen_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    test_params.setup = fbe_api_rdgen_test_io_fill_area;
    test_params.test = fbe_api_rdgen_test_io_send_io;
    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_api_rdgen_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_api_rdgen_test_io_run_write_verify_read_check_test() */

/*!***************************************************************
 * fbe_api_rdgen_test_io_run_write_same_test()
 ****************************************************************
 * @brief
 *  This function performs a write same io test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - The highest case index to test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_rdgen_test_io_run_write_same_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                fbe_object_id_t object_id,
                                fbe_u32_t max_case_index)
{
    fbe_api_rdgen_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.setup = fbe_api_rdgen_test_io_fill_area;
    test_params.test = fbe_api_rdgen_test_io_send_io;

    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_api_rdgen_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_api_rdgen_test_io_run_write_same_test() */

/*!***************************************************************
 * @fn fbe_api_rdgen_test_io_run_write_only_test(
 *                         fbe_api_rdgen_test_io_case_t *const cases_p,
 *                         fbe_object_id_t object_id,
 *                         fbe_u32_t max_case_index)
 ****************************************************************
 * @brief
 *  This function performs a write only test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - The highest case index to test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_rdgen_test_io_run_write_only_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                                fbe_object_id_t object_id,
                                fbe_u32_t max_case_index)
{
    fbe_api_rdgen_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    test_params.setup = fbe_api_rdgen_test_io_fill_area;
    test_params.test = fbe_api_rdgen_test_io_send_io;

    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_api_rdgen_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_api_rdgen_test_io_run_write_only_test() */

/*!***************************************************************
 * @fn fbe_api_rdgen_test_io_run_read_only_test(
 *                         fbe_api_rdgen_test_io_case_t *const cases_p,
 *                         fbe_object_id_t object_id,
 *                         fbe_u32_t max_case_index)
 ****************************************************************
 * @brief
 *  This function performs a read only test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - The highest case index to test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_rdgen_test_io_run_read_only_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                               fbe_object_id_t object_id,
                               fbe_u32_t max_case_index)
{
    fbe_api_rdgen_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_READ_ONLY;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    test_params.setup = fbe_api_rdgen_test_io_fill_area;
    test_params.test = fbe_api_rdgen_test_io_send_io;

    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_api_rdgen_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_api_rdgen_test_io_run_read_only_test() */

/*!***************************************************************
 * fbe_api_rdgen_test_io_run_verify_test()
 ****************************************************************
 * @brief
 *  This function performs a verify test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - The highest case index to test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_rdgen_test_io_run_verify_test(fbe_api_rdgen_test_io_case_t *const cases_p,
                            fbe_object_id_t object_id,
                            fbe_u32_t max_case_index)
{

    fbe_api_rdgen_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_VERIFY;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    test_params.setup = fbe_api_rdgen_test_io_fill_area;
    test_params.test = fbe_api_rdgen_test_io_send_io;

    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_api_rdgen_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_api_rdgen_test_io_run_verify_test() */

/*!**************************************************************
 * fbe_api_rdgen_wait_for_all_threads_to_stop()
 ****************************************************************
 * @brief
 *  Send a stop and wait for all theads to stop.
 *
 * @param timeout_ms - Timeout in msecs to wait for. 
 *
 * @return fbe_status_t   
 *
 * @author
 *  5/10/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_wait_for_all_threads_to_stop(fbe_u32_t timeout_ms)
{
    fbe_status_t status;
    fbe_rdgen_filter_t filter;
    fbe_u32_t time = 0;
    fbe_api_rdgen_get_stats_t info;

    #define FBE_API_RDGEN_STOP_WAIT_MS 1000

    /* Set the filter type to invalid so we stop all threads.
     */
    filter.filter_type = FBE_RDGEN_FILTER_TYPE_INVALID;
    status = fbe_api_rdgen_send_stop_packet(&filter);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /* Continue to poll until all threads have stopped.
     */
    while (time < timeout_ms)
    {
        status = fbe_api_rdgen_get_stats(&info, &filter);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        if (info.threads == 0)
        {
            /* No more threads, break out.
             */
            break;
        }
        fbe_api_sleep(FBE_API_RDGEN_STOP_WAIT_MS);
        time += FBE_API_RDGEN_STOP_WAIT_MS;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_wait_for_all_threads_to_stop()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_io_specification_set_affinity()
 ****************************************************************
 * @brief
 *  Set the affinity for the threads that are generated
 *  for this io spec.
 * 
 *  We can specify a fixed affinity for all threads to a given core,
 *  or we can spread the affinity of all threads across cores,
 *  or we can allow the affinity to change over the life of the threads.
 *
 * @param io_spec_p - io spec to init.
 * @param affinity - How to choose affinity.
 * @param core - Core to affine this to.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  7/5/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_affinity(fbe_rdgen_io_specification_t *io_spec_p,
                                                         fbe_rdgen_affinity_t affinity,
                                                         fbe_u32_t core)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else  //Coverity fix - added to assure no possible NULL pointer (io_spec_p) is dereferenced
    {
        /* No range checks for core since we do not know how to validate this.
         * The rdgen service will validate the core. 
         */
        io_spec_p->affinity = affinity;
        io_spec_p->core = core;
    }
    if ((affinity == FBE_RDGEN_AFFINITY_INVALID) ||
        (affinity >= FBE_RDGEN_AFFINITY_LAST))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_affinity()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_run_cmi_perf_test()
 ****************************************************************
 * @brief
 *  run the cmi performance test. this is a blocking API
 *
 * @param cmi_perf_p - data strucutre with information for test
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  9/6/2011 - Created. Shay Harel
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_run_cmi_perf_test(fbe_rdgen_control_cmi_perf_t *cmi_perf_p)
{
	fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t         status_info;
    
	status = fbe_api_common_send_control_packet_to_service (FBE_RDGEN_CONTROL_CODE_TEST_CMI_PERFORMANCE,
															cmi_perf_p,
															sizeof(fbe_rdgen_control_cmi_perf_t),
															FBE_SERVICE_ID_RDGEN,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_NEIT);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
						status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

		if (status != FBE_STATUS_OK) {
			return status;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
	}

    return status;

}
/******************************************
 * end fbe_api_rdgen_run_cmi_perf_test()
 ******************************************/
/*!**************************************************************
 * fbe_api_rdgen_io_specification_set_max_passes()
 ****************************************************************
 * @brief
 *  Set the max number of passes for this io spec.
 *
 * @param io_spec_p - io spec to init.
 * @param max_passes - new max passes
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  9/7/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_max_passes(fbe_rdgen_io_specification_t *io_spec_p,
                                                           fbe_u32_t max_passes)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    { 
        /* No range checks for max_passes since all values are valid.
        */
        io_spec_p->max_passes = max_passes;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_max_passes()
 ******************************************/

/*!**************************************************************
 * fbe_api_rdgen_context_reinit()
 ****************************************************************
 * @brief
 *  Re-initialize a previously used test context.
 * 
 * @param context_p - array of structures to determine status for.
 * @param num_tests - number of tests in structure.
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  9/7/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_context_reinit(fbe_api_rdgen_context_t *context_p,
                                          fbe_u32_t num_tests)
{
    fbe_u32_t test_index;

    /* Iterate over all tests, looking for an error.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        //fbe_semaphore_destroy(&context_p[test_index].semaphore); /* SAFEBUG - need destroy - CGCG bad change kill this */
        fbe_semaphore_init(&context_p[test_index].semaphore, 0, 1);
        context_p[test_index].b_initialized = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_context_reinit()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_context_check_io_status()
 ****************************************************************
 * @brief
 *  Determines if I/O was successful for all the contexts.
 *
 * @param context_p - Array of contexts.
 * @param num_tests - Number of entries in array.
 * @param status - Expected packet status.
 * @param err_count - Expected number of errors.
 * @param b_non_zero_io_count - FBE_TRUE to check for a non-zero io count
 *                             FBE_FALSE to not check the io count.
 * @param rdgen_status - expected rdgen operation status.
 * @param bl_status - Expected block status
 * @param bl_qual - Expected block qualifier.
 * 
 * @return fbe_status_t
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  9/7/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_context_check_io_status(fbe_api_rdgen_context_t *context_p,
                                                   fbe_u32_t num_tests,
                                                   fbe_status_t status,
                                                   fbe_u32_t err_count,
                                                   fbe_bool_t b_non_zero_io_count,
                                                   fbe_rdgen_operation_status_t rdgen_status,
                                                   fbe_payload_block_operation_status_t bl_status,
                                                   fbe_payload_block_operation_qualifier_t bl_qual)
{
    fbe_u32_t test_index;

    /* Iterate over all tests, looking for an error.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        /* The status will always be GENERIC_FAILURE when any error is encountered. 
         * Thus we skip the validation of context.status on GENERIC_FAILURE and validate 
         * the other status values instead. 
         */
        if (((context_p[test_index].status != status) && (context_p[test_index].status != FBE_STATUS_GENERIC_FAILURE)) ||
            (context_p[test_index].start_io.status.status != status) ||
            (context_p[test_index].start_io.status.block_status != bl_status) ||
            (context_p[test_index].start_io.status.block_qualifier != bl_qual) ||
            (context_p[test_index].start_io.status.rdgen_status != rdgen_status))
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s error index: %d cntxt: %p st: 0x%x pk_st: 0x%x bl_st: 0x%x bl_q: 0x%x\n",
                          __FUNCTION__, test_index, &context_p[test_index],
                          context_p[test_index].status, context_p[test_index].start_io.status.status,
                          context_p[test_index].start_io.status.block_status,
                          context_p[test_index].start_io.status.block_qualifier);
            /* Status unexpected, return error.
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if ( (b_non_zero_io_count && (context_p[test_index].start_io.statistics.io_count == 0)) ||
             (context_p[test_index].start_io.statistics.error_count != err_count))
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s error1 index: %d cntxt: %p st: 0x%x pk_st: 0x%x bl_st: 0x%x bl_q: 0x%x\n",
                          __FUNCTION__, test_index, &context_p[test_index],
                          context_p[test_index].status, context_p[test_index].start_io.status.status,
                          context_p[test_index].start_io.status.block_status,
                          context_p[test_index].start_io.status.block_qualifier);
            /* Counts unexpected, return error.
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_context_check_io_status()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_context_validate_aborted_status()
 ****************************************************************
 * @brief
 *  Confirms that we got aborted errors and that the only errors
 *  are aborted errors.
 *
 * @param context_p - Array of contexts.
 * @param num_tests - Number of entries in array.
 * 
 * @return fbe_status_t
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  9/7/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_api_rdgen_context_validate_aborted_status(fbe_api_rdgen_context_t *context_p,
                                              fbe_u32_t num_tests)
{
    fbe_u32_t test_index;

    /* Iterate over all tests, looking for an error.
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        if (context_p[test_index].start_io.statistics.error_count != 
            context_p[test_index].start_io.statistics.aborted_error_count)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s error index: %d cntxt: %p error_count %d != aborted_error_count %d\n",
                          __FUNCTION__, test_index, &context_p[test_index],
                          context_p[test_index].start_io.statistics.error_count,
                          context_p[test_index].start_io.statistics.aborted_error_count);
            /* Status unexpected, return error.
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_rdgen_context_validate_aborted_status()
 **************************************/
/*!**************************************************************
 *  fbe_api_rdgen_send_one_io_params()
 ****************************************************************
 * @brief
 *  Run just a single I/O through rdgen.
 *
 * @param context_p - context structure to initialize.
 * @param params_p - structure with passed in parameters.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/23/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_send_one_io_params(fbe_api_rdgen_context_t *context_p,
                                              fbe_api_rdgen_send_one_io_params_t *params_p)
{
    fbe_status_t status;
    fbe_lba_t end_lba = params_p->lba + params_p->blocks - 1;
    fbe_rdgen_block_specification_t block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
    fbe_rdgen_lba_specification_t lba_spec = FBE_RDGEN_LBA_SPEC_FIXED;

    /* If the params_p->lba is invalid we are trying to write to the last params_p->lba in the LUN. 
     * Adjust the end params_p->lba appropriately. 
     */
    if (params_p->lba == FBE_LBA_INVALID)
    {
        end_lba = FBE_LBA_INVALID;
    }
    if (params_p->end_lba != 0)
    {
        end_lba = params_p->end_lba;
    }
    /* Block_spec is an optional input.
     */
    if (params_p->block_spec != FBE_RDGEN_BLOCK_SPEC_INVALID)
    {
        block_spec = params_p->block_spec;
    }
    if (params_p->lba_spec != FBE_RDGEN_LBA_SPEC_INVALID)
    {
        lba_spec = params_p->lba_spec;
    }
    /* We set this up as a fixed lba constant block size pattern for one I/O.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             params_p->object_id,
                                             params_p->class_id, 
                                             params_p->package_id,
                                             params_p->rdgen_operation,
                                             params_p->pattern,
                                             0x1,    /* We do one I/O. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             lba_spec,
                                             params_p->lba,    /* Start lba*/
                                             params_p->lba,    /* Min lba */
                                             end_lba,    /* max lba */
                                             block_spec,
                                             params_p->blocks,    /* Min blocks */
                                             params_p->blocks    /* Max blocks */ );
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s init context status %d \n", __FUNCTION__, status);

        return status;
    }
    status = fbe_api_rdgen_io_specification_set_inc_lba_blocks(&context_p->start_io.specification,
                                                               0, /* inc blocks not used */
                                                               params_p->inc_lba);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s init lba blocks status %d \n", __FUNCTION__, status);

        return status;
    }
    /* If we have options then set them.
     */
    if (params_p->options != FBE_RDGEN_OPTIONS_INVALID)
    {
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                            params_p->options);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                          "%s unable to set options status %d \n", __FUNCTION__, status);

            fbe_api_rdgen_test_context_destroy(context_p);
            return status;
        }
    }
    fbe_api_rdgen_io_specification_set_msecs_to_expiration(&context_p->start_io.specification,
                                                           params_p->msecs_to_expiration);
    fbe_api_rdgen_io_specification_set_msecs_to_abort(&context_p->start_io.specification,
                                                      params_p->msecs_to_abort);
    if (params_p->b_async)
    {
        /* Run our I/O test.
         */
        status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    }
    else
    {
        /* Write a background pattern to the LUNs.
         */
        status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_send_one_io_params()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_io_params_init()
 ****************************************************************
 * @brief
 *  Init the io parameters
 *
 * @param context_p - context structure to initialize.
 * @param params_p - structure with passed in parameters.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/26/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_params_init(fbe_api_rdgen_send_one_io_params_t *params_p)
{
    fbe_zero_memory(params_p, sizeof(fbe_api_rdgen_send_one_io_params_t));
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_api_rdgen_io_params_init()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_io_specification_set_alignment_size()
 ****************************************************************
 * @brief
 *  Set the size we will align all lbas/blocks to.
 *
 * @param io_spec_p - io spec to init.
 * @param align_blocks - blocks to align all lba/blocks to.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  2/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_alignment_size(fbe_rdgen_io_specification_t* io_spec_p,
                                                               fbe_u32_t align_blocks)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        io_spec_p->alignment_blocks = align_blocks;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_alignment_size()
 ******************************************/
/*!**************************************************************
 *  fbe_api_rdgen_io_specification_set_interface()
 ****************************************************************
 * @brief
 *  Set the interface to use to deliver I/O.
 *
 * @param io_spec_p - io spec to init.
 * @param interface - interface to deliver I/O with.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  3/19/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_io_specification_set_interface(fbe_rdgen_io_specification_t* io_spec_p,
                                                          fbe_rdgen_interface_type_t io_interface)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        io_spec_p->io_interface = io_interface;
    }
    return status;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_interface()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_set_sequence_count_seed()
 ****************************************************************
 * @brief
 *  Set the seed to use when checking the patterm
 *
 * @param io_spec_p - io spec to init.
 * @param sequence_count_seed - the sequence_count_seed to set.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  5/21/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_set_sequence_count_seed(fbe_rdgen_io_specification_t *io_spec_p,
                                                   fbe_u32_t sequence_count_seed)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    io_spec_p->sequence_count_seed = sequence_count_seed;
    return status;
}
/******************************************
 * end fbe_api_rdgen_set_sequence_count_seed()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_set_random_seed()
 ****************************************************************
 * @brief
 *  Set the seed for generating all the random numbers for the thread.
 *  This is used as a base for this seed, not the entire seed.
 *
 * @param io_spec_p - io spec to init.
 * @param random_seed - the random_seed to set.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  2/5/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_set_random_seed(fbe_rdgen_io_specification_t *io_spec_p,
                                           fbe_u32_t random_seed)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    io_spec_p->random_seed_base = random_seed;
    return status;
}
/******************************************
 * end fbe_api_rdgen_set_random_seed()
 ******************************************/

/*!**************************************************************
 *  fbe_api_rdgen_set_priority()
 ****************************************************************
 * @brief
 *  Set the priority for this request.  
 *
 * @param io_spec_p - io spec to init.
 * @param priority - the priority to set.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  3/20/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_set_priority(fbe_rdgen_io_specification_t *io_spec_p,
                                        fbe_packet_priority_t priority)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    io_spec_p->options |= FBE_RDGEN_OPTIONS_USE_PRIORITY;
    io_spec_p->priority = priority;
    return status;
}
/******************************************
 * end fbe_api_rdgen_set_priority()
 ******************************************/

/*!***************************************************************
 * fbe_api_rdgen_init_dps_memory()
 ****************************************************************
 * @brief
 *  This function tells rdgen to allocate memory.
 * 
 *  This is required before we start I/O.
 *
 * @param mem_size_in_mb - Amount of memory to allocate to rdgen
 * @param memory_type - Type of memory to get cache or CMM.
 * 
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_init_dps_memory(fbe_u32_t mem_size_in_mb,
                                           fbe_rdgen_memory_type_t memory_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_control_alloc_mem_info_t alloc_mem_info;
    fbe_api_control_operation_status_info_t status_info;
    
    alloc_mem_info.mem_size_in_mb = mem_size_in_mb;
    alloc_mem_info.memory_type = memory_type;
    
    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_INIT_DPS_MEMORY,
                                                           &alloc_mem_info, sizeof(alloc_mem_info),
                                                           FBE_SERVICE_ID_RDGEN,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                   "%s set trace level failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_rdgen_init_dps_memory()
 **************************************/
/*!***************************************************************
 * fbe_api_rdgen_release_dps_memory()
 ****************************************************************
 * @brief
 *  This function release all dps memory from neit.
 * 
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_release_dps_memory(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    
    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_RELEASE_DPS_MEMORY,
                                                           NULL, 0, /* No input */
                                                           FBE_SERVICE_ID_RDGEN,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                   "%s set trace level failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_rdgen_release_dps_memory()
 **************************************/

/*!***************************************************************
 * fbe_api_rdgen_allocate_peer_memory()
 ****************************************************************
 * @brief
 *  Rdgen needs to allocate peer memory before starting to initiate
 *  requests to the peer.
 *  This api allows rdgen to allocate and initialize the peer memory
 *  for use over CMI.
 *
 * @param trace_level - the trace level to use
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_allocate_peer_memory(void)
{
    fbe_status_t                               status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    
    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_ALLOC_PEER_MEMORY,
                                                  NULL, 0, /* No input buffer. */
                                                  FBE_SERVICE_ID_RDGEN,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                   "%s alloc peer mem failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_rdgen_allocate_peer_memory()
 **************************************/

/*!***************************************************************
 * fbe_api_rdgen_set_default_timeout()
 ****************************************************************
 * @brief
 *  Allows setting the default milliseconds before rdgen will
 *  abort an I/O.
 *
 * @param timeout_ms - Timeout in milliseconds.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_set_default_timeout(fbe_time_t timeout_ms)
{
    fbe_status_t                               status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_rdgen_control_set_timeout_info_t timeout_info;

    timeout_info.default_io_timeout = timeout_ms;
    
    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_SET_TIMEOUT,
                                                           &timeout_info, sizeof(fbe_rdgen_control_set_timeout_info_t),
                                                           FBE_SERVICE_ID_RDGEN,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                   "%s alloc peer mem failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_rdgen_set_default_timeout()
 **************************************/

/*!**************************************************************
 *  fbe_api_rdgen_io_specification_set_originating_sp()
 ****************************************************************
 * @brief
 *  Set the originating SP in the I/O specification.
 *
 * @param io_spec_p - io spec to init.
 * @param originating_sp - The SP id tht wrote the data
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  11/29/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_io_specification_set_originating_sp(fbe_rdgen_io_specification_t *io_spec_p,
                                                               fbe_rdgen_sp_id_t originating_sp)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((originating_sp != FBE_DATA_PATTERN_SP_ID_A) &&
        (originating_sp != FBE_DATA_PATTERN_SP_ID_B)    )
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    io_spec_p->originating_sp_id = originating_sp;
    return status;
}
/*********************************************************
 * end fbe_api_rdgen_io_specification_set_originating_sp()
 *********************************************************/

/*!**************************************************************
 * fbe_api_rdgen_get_status_string()
 ****************************************************************
 * @brief
 *  Return the opcode we should use from a char that was entered
 *  on the command line.
 *
 * @param op_ch - char entered on command line
 * @param op_p - operation that is being returned.
 *
 * @return FBE_STATUS_OK on success.
 *         FBE_STATUS_GENERIC_FAILURE if input char not valid.
 *
 * @author
 *  10/24/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_api_rdgen_get_status_string(fbe_rdgen_operation_status_t rdgen_status, const fbe_char_t **string_p)
{
    switch (rdgen_status)
    {
        case FBE_RDGEN_OPERATION_STATUS_INVALID:
            *string_p = "INVALID";
            break;
        case FBE_RDGEN_OPERATION_STATUS_BAD_CRC_MISMATCH:
            *string_p = "BAD_CRC_MISMATCH";
            break;
        case FBE_RDGEN_OPERATION_STATUS_INVALID_BLOCK_MISMATCH:
            *string_p = "INVALID_BLOCK_MISMATCH";
            break;
        case FBE_RDGEN_OPERATION_STATUS_INVALID_OPERATION:
            *string_p = "INVALID_OPERATION";
            break;
        case FBE_RDGEN_OPERATION_STATUS_IO_ERROR:
            *string_p = "IO_ERROR";
            break;
        case FBE_RDGEN_OPERATION_STATUS_MEM_UNINITIALIZED:
            *string_p = "MEMORY_UNINITIALIZED";
            break;
        case FBE_RDGEN_OPERATION_STATUS_MISCOMPARE:
            *string_p = "MISCOMPARE";
            break;
        case FBE_RDGEN_OPERATION_STATUS_NO_OBJECTS_OF_CLASS:
            *string_p = "NO_OBJECTS_OF_CLASS";
            break;
        case FBE_RDGEN_OPERATION_STATUS_OBJECT_DOES_NOT_EXIST:
            *string_p = "OBJECT_DOES_NOT_EXIST";
            break;
        case FBE_RDGEN_OPERATION_STATUS_PEER_DIED:
            *string_p = "PEER_DIED";
            break;
        case FBE_RDGEN_OPERATION_STATUS_PEER_ERROR:
            *string_p = "PEER_ERROR";
            break;
        case FBE_RDGEN_OPERATION_STATUS_PEER_OUT_OF_RESOURCES:
            *string_p = "PEER_OUT_OF_RESOURCES";
            break;
        case FBE_RDGEN_OPERATION_STATUS_SUCCESS:
            *string_p = "STATUS_SUCCESS";
            break;
        case FBE_RDGEN_OPERATION_STATUS_UNEXPECTED_ERROR:
            *string_p = "UNEXPECTED_ERROR";
            break;
        case FBE_RDGEN_OPERATION_STATUS_UNRESOLVED_CONFLICT:
            *string_p = "UNRESOLVED_CONFLICT";
            break;
        default:
            *string_p = "unknown";
    };
}
/**************************************
 * end fbe_api_rdgen_get_status_string()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_get_lba_spec_string()
 ****************************************************************
 * @brief
 *  Return the opcode we should use from a char that was entered
 *  on the command line.
 *
 * @param lba_spec - spec to map
 * @param string_p - mapped string.
 *
 * @return FBE_STATUS_OK on success.
 *         FBE_STATUS_GENERIC_FAILURE if input char not valid.
 *
 * @author
 *  10/24/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_api_rdgen_get_lba_spec_string(fbe_rdgen_lba_specification_t lba_spec, const fbe_char_t **string_p)
{
    switch (lba_spec)
    {
        case FBE_RDGEN_LBA_SPEC_INVALID:
            *string_p = "INVALID";
            break;
        case FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING:
            *string_p = "CATERPILLAR_DECREASING";
            break;
        case FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING:
            *string_p = "CATERPILLAR_INCREASING";
            break;
        case FBE_RDGEN_LBA_SPEC_FIXED:
            *string_p = "FIXED";
            break;
        case FBE_RDGEN_LBA_SPEC_RANDOM:
            *string_p = "RANDOM";
            break;
        case FBE_RDGEN_LBA_SPEC_SEQUENTIAL_DECREASING:
            *string_p = "SEQUENTIAL_DECREASING";
            break;
        case FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING:
            *string_p = "SEQUENTIAL_INCREASING";
            break;
        case FBE_RDGEN_LBA_SPEC_LAST:
            *string_p = "LAST";
            break;
        default:
            *string_p = "unknown";
    };
}
/**************************************
 * end fbe_api_rdgen_get_lba_spec_string()
 **************************************/
/*!**************************************************************
 * fbe_api_rdgen_get_block_spec_string()
 ****************************************************************
 * @brief
 *  Return the opcode we should use from a char that was entered
 *  on the command line.
 *
 * @param block_spec - spec to map
 * @param string_p - mapped string.
 *
 * @return FBE_STATUS_OK on success.
 *         FBE_STATUS_GENERIC_FAILURE if input char not valid.
 *
 * @author
 *  10/24/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_api_rdgen_get_block_spec_string(fbe_rdgen_block_specification_t block_spec, const fbe_char_t **string_p)
{
    switch (block_spec)
    {
        case FBE_RDGEN_BLOCK_SPEC_INVALID:
            *string_p = "INVALID";
            break;
        case FBE_RDGEN_BLOCK_SPEC_CONSTANT:
            *string_p = "CONSTANT";
            break;
        case FBE_RDGEN_BLOCK_SPEC_DECREASING:
            *string_p = "DECREASING";
            break;
        case FBE_RDGEN_BLOCK_SPEC_INCREASING:
            *string_p = "INCREASING";
            break;
        case FBE_RDGEN_BLOCK_SPEC_RANDOM:
            *string_p = "RANDOM";
            break;
        case FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE:
            *string_p = "STRIPE_SIZE";
            break;
        case FBE_RDGEN_BLOCK_SPEC_LAST:
            *string_p = "LAST";
            break;
        default:
            *string_p = "unknown";
    };
}
/**************************************
 * end fbe_api_rdgen_get_lba_spec_string()
 **************************************/

/*!**************************************************************
 * fbe_api_rdgen_io_specification_set_reject_flags()
 ****************************************************************
 * @brief
 *  Setup the memory io master flags.
 *
 * @param spec_p
 * @param client_reject_allowed_flags
 * @param arrival_reject_allowed_flags
 * 
 * @return fbe_status_t
 *
 * @author
 *  2/19/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_io_specification_set_reject_flags(fbe_rdgen_io_specification_t *spec_p,
                                                             fbe_u8_t client_reject_allowed_flags,
                                                             fbe_u8_t arrival_reject_allowed_flags)
{   
    spec_p->client_reject_allowed_flags = client_reject_allowed_flags;
    spec_p->arrival_reject_allowed_flags = arrival_reject_allowed_flags;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_api_rdgen_io_specification_set_reject_flags()
 ******************************************/

/*!***************************************************************
 * fbe_api_rdgen_configure_system_threads()
 ****************************************************************
 * @brief
 *  This function tells rdgen to use the system threads to generate IO.
 * 
 * @param enable - if true enable, if false disable
  * 
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_configure_system_threads(fbe_bool_t enable)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_api_control_operation_status_info_t status_info;
    fbe_rdgen_control_code_t operation = FBE_RDGEN_CONTROL_CODE_DISABLE_SYSTEM_THREADS;

    if (FBE_IS_TRUE(enable))
    {
        operation = FBE_RDGEN_CONTROL_CODE_ENABLE_SYSTEM_THREADS;
    }
  
    status = fbe_api_common_send_control_packet_to_service(operation,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_RDGEN,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                   "%s set trace level failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_rdgen_configure_system_threads()
 **************************************/

/*!**************************************************************
 *  fbe_api_rdgen_set_msec_delay()
 ****************************************************************
 * @brief
 *  Set the delay to use after each I/O
 *
 * @param io_spec_p - io spec to init.
 * @param msec_delay - the delay to set.
 *
 * @return fbe_status_t FBE_STATUS_OK if success.   
 *
 * @author
 *  4/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_rdgen_set_msec_delay(fbe_rdgen_io_specification_t *io_spec_p,
                                          fbe_u32_t msec_delay)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    io_spec_p->msecs_to_delay = msec_delay;
    return status;
}
/******************************************
 * end fbe_api_rdgen_set_msec_delay()
 ******************************************/
/*!***************************************************************
 * fbe_api_rdgen_set_svc_options()
 ****************************************************************
 * @brief
 *  Set various rdgen options.
 *
 * @param options_p - Options to set.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t fbe_api_rdgen_set_svc_options(fbe_rdgen_svc_options_t *options_p)
{
    fbe_status_t                               status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    
    status = fbe_api_common_send_control_packet_to_service(FBE_RDGEN_CONTROL_CODE_SET_SVC_OPTIONS,
                                                           options_p, sizeof(fbe_rdgen_control_set_svc_options_t),
                                                           FBE_SERVICE_ID_RDGEN,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_NEIT);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    
    if ((status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                   "%s alloc peer mem failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_api_rdgen_set_svc_options()
 **************************************/
/*************************
 * end file fbe_api_rdgen_interface.c
 *************************/
