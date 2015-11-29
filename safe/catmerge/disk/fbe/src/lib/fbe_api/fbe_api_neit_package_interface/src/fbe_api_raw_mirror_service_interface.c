/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_raw_mirror_service_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains raw mirror service test context related functions
 *  for sending I/O to the raw mirror service.
 *
 * @version
 *   10/2011:  Created. Susan Rundbaken
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_raw_mirror_service_interface.h"


/*************************
 *   DEFINITIONS
 *************************/

/*!*******************************************************************
 * @def FBE_API_RAW_MIRROR_WAIT_MS
 *********************************************************************
 * @brief milliseconds the raw mirror service should wait for an I/O request.
 *
 *********************************************************************/
#define FBE_API_RAW_MIRROR_SVC_WAIT_MS 120000


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_api_raw_mirror_service_send_control_operation(fbe_api_raw_mirror_service_context_t *context_p,
                                                                      fbe_package_id_t package_id,
                                                                      fbe_payload_control_operation_opcode_t control_code);
static fbe_status_t fbe_api_raw_mirror_service_test_io_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_raw_mirror_service_io_specification_init(fbe_raw_mirror_service_io_specification_t *io_spec_p,
                                                                     fbe_raw_mirror_service_operation_t operation,
                                                                     fbe_data_pattern_t pattern,
                                                                     fbe_u32_t sequence_id,
                                                                     fbe_u32_t block_size,
                                                                     fbe_u32_t block_count,
                                                                     fbe_lba_t start_lba,
                                                                     fbe_u64_t sequence_num,
                                                                     fbe_bool_t dmc_expected_b,
                                                                     fbe_u32_t max_passes,
                                                                     fbe_u32_t num_threads);
static fbe_status_t fbe_api_raw_mirror_service_io_specification_set_pattern(fbe_raw_mirror_service_io_specification_t *io_spec_p,
                                                                            fbe_data_pattern_t pattern);



/*!**************************************************************
 * fbe_api_raw_mirror_service_test_context_init()
 ****************************************************************
 * @brief
 *  Initialize the test context with all the needed parameters.
 *
 * @param context_p - context structure to initialize.
 * @param raw_mirror_operation - type of raw mirror test to run.
 * @param pattern - data pattern to generate.
 * @param sequence_id - sequence identifier in data sector.
 * @param block_size - Size of a block.
 * @param block_count - Number of blocks.
 * @param start_lba - starting lba
 * @param sequence_num - sequence number
 * @param dmc_expected_b - boolean indicating if DMC should be detected or not
 * @param max_passes - maximum number of times to issue the I/O request
 * @param num_threads - number of threads to use for request
 * 
 * @note
 *  This structure must be destroyed via a call to
 *  fbe_api_raw_mirror_test_context_destroy()
 * 
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
fbe_status_t fbe_api_raw_mirror_service_test_context_init(fbe_api_raw_mirror_service_context_t *context_p,
                                                          fbe_raw_mirror_service_operation_t raw_mirror_operation,
                                                          fbe_data_pattern_t pattern,
                                                          fbe_u32_t sequence_id,
                                                          fbe_u32_t block_size,
                                                          fbe_u32_t block_count,
                                                          fbe_lba_t start_lba,
                                                          fbe_u64_t sequence_num,
                                                          fbe_bool_t dmc_expected_b,
                                                          fbe_u32_t max_passes,
                                                          fbe_u32_t num_threads)
{
    fbe_status_t status;
    fbe_status_t return_status = FBE_STATUS_OK;

    /* Set context fields
     */
    context_p->status = FBE_STATUS_INVALID;
    context_p->packet.magic_number = FBE_MAGIC_NUMBER_INVALID;
    context_p->packet_io.magic_number = FBE_MAGIC_NUMBER_INVALID;
    fbe_semaphore_init(&context_p->semaphore, 0, 1);
    fbe_semaphore_init(&context_p->semaphore_io, 0, 1);

    /* Initialize the context fields for determining if a raw mirror PVD edge is enabled or not.
     */
    context_p->start_io.pvd_edge_index = FBE_RAW_MIRROR_WIDTH + 1;
    context_p->start_io.pvd_edge_enabled_b = FBE_FALSE;
    context_p->start_io.timeout_ms = 0;

    fbe_zero_memory(&context_p->start_io, sizeof(context_p->start_io));

    /* Set the I/O specification
     */
    status = fbe_api_raw_mirror_service_io_specification_init(&context_p->start_io.specification,
                                                              raw_mirror_operation,
                                                              pattern,
                                                              sequence_id,
                                                              block_size,
                                                              block_count,
                                                              start_lba,
                                                              sequence_num,
                                                              dmc_expected_b,
                                                              max_passes,
                                                              num_threads);
    if (status != FBE_STATUS_OK)
    {
        return_status = status;
    }

    /* Note that we always return the last error status. 
     * By continuing on error, it allows the caller to test error cases 
     * where invalid parameters are actually sent to the raw mirror service. 
     */
    return return_status;
}
/******************************************
 * end fbe_api_raw_mirror_service_test_context_init()
 ******************************************/


/*!**************************************************************
 * fbe_api_raw_mirror_service_run_test()
 ****************************************************************
 * @brief
 *  Start the test for a given context structure.
 *
 * @param context_p - context structure to test.
 * @param package_id - package where raw mirror service lives.
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_raw_mirror_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  10/2011 - Created.  Susan Rundbaken
 *
 ****************************************************************/
fbe_status_t fbe_api_raw_mirror_service_run_test(fbe_api_raw_mirror_service_context_t *context_p,
                                                 fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_time_t wait_ms = 0;


    /* Start the test by sending a start operation to the raw mirror service.
     */
    status = fbe_api_raw_mirror_service_send_control_operation(context_p, 
                                                               package_id, 
                                                               FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_START_IO);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Wait for the test to complete only if context max_passes is non-zero.
     * 0 max passes means to run forever.  A call to fbe_api_raw_mirror_stop_test()
     * will stop the test.
     */
    if (context_p->start_io.specification.max_passes)
    {
        wait_ms = FBE_API_RAW_MIRROR_SVC_WAIT_MS;
    
        status = fbe_semaphore_wait_ms(&context_p->semaphore, (fbe_u32_t)wait_ms);
    
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
 * end fbe_api_raw_mirror_service_run_test()
 **************************************/


/*!**************************************************************
 * fbe_api_raw_mirror_service_stop_test()
 ****************************************************************
 * @brief
 *  Stop the test for a given context structure.
 *
 * @param context_p - context structure to test.
 * @param package_id - package where raw mirror service lives.
 *
 * @note
 *  We assume the structure was previously started by a call to
 *  fbe_api_raw_mirror_start_test().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  11/2011 - Created.  Susan Rundbaken
 *
 ****************************************************************/
fbe_status_t fbe_api_raw_mirror_service_stop_test(fbe_api_raw_mirror_service_context_t *context_p,
                                                  fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_time_t wait_ms = 0;


    /* Send a stop operation to the raw mirror service to stop the test.
     */
    status = fbe_api_raw_mirror_service_send_control_operation(context_p, 
                                                               package_id, 
                                                               FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_STOP_IO);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Wait for the command to be processed.
     */
    wait_ms = 360000;

    status = fbe_semaphore_wait_ms(&context_p->semaphore, (fbe_u32_t)wait_ms);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s semaphore wait failed with status of 0x%x\n",
                      __FUNCTION__, status);
        return status;
    }

    /* Let I/O be stopped.
     */
    fbe_api_sleep(5000);

    // we have to wait the infinite IO finished
    wait_ms = FBE_API_RAW_MIRROR_SVC_WAIT_MS;

    status = fbe_semaphore_wait_ms(&context_p->semaphore_io, (fbe_u32_t)wait_ms);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s semaphore wait failed with status of 0x%x\n",
                      __FUNCTION__, status);
        return status;
    }

    return status;
}
/**************************************
 * end fbe_api_raw_mirror_service_stop_test()
 **************************************/

/*!**************************************************************
 * fbe_api_raw_mirror_service_wait_pvd_edge_enabled()
 ****************************************************************
 * @brief
 *  Wait the given amount of time max for the raw mirror PVD edge at the
 *  given index to be enabled. This is used after a removed drive has
 *  been reinserted.
 *
 * @param context_p - context structure to test.
 * @param package_id - package where raw mirror service lives.
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  11/2011 - Created.  Susan Rundbaken
 *
 ****************************************************************/
fbe_status_t fbe_api_raw_mirror_service_wait_pvd_edge_enabled(fbe_api_raw_mirror_service_context_t *context_p,
                                                              fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_time_t wait_ms = 0;
    fbe_time_t current_time = 0;
    fbe_u32_t polling_interval = 100; /* msec */


    /* Wait the given amount of time max for edge to be in the enabled state.
     */
    while (current_time < context_p->start_io.timeout_ms)
    {
        /* Determine if the raw mirror PVD edge is enabled.
         */
        status = fbe_api_raw_mirror_service_send_control_operation(context_p, 
                                                                   package_id, 
                                                                   FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_PVD_EDGE_ENABLED);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }

        /* Wait for the command to be processed.
         */
        wait_ms = 360000;

        status = fbe_semaphore_wait_ms(&context_p->semaphore, (fbe_u32_t)wait_ms);

        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s semaphore wait failed with status of 0x%x\n",
                          __FUNCTION__, status);
            return status;
        }

        /* Check results.
         */
        if (context_p->start_io.pvd_edge_enabled_b)
        {
            return status;
        }

        current_time = current_time + polling_interval;
        fbe_api_sleep(polling_interval);
    }

    /* We timed out
     */
    status = FBE_STATUS_TIMEOUT;

    return status;
}
/**************************************
 * end fbe_api_raw_mirror_service_wait_pvd_edge_enabled()
 **************************************/

/*!**************************************************************
 * fbe_api_raw_mirror_service_test_context_destroy()
 ****************************************************************
 * @brief
 *  Destroy the context structure and clean up all resources.
 *
 * @param context_p - pointer to the structure to destroy.
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/

fbe_status_t fbe_api_raw_mirror_service_test_context_destroy(fbe_api_raw_mirror_service_context_t *context_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Only destroy the packet if we have initialized it.
     */
    if (context_p->packet.magic_number != FBE_MAGIC_NUMBER_INVALID)
    {
        status = fbe_transport_destroy_packet(&context_p->packet);
    }
    if (context_p->packet_io.magic_number != FBE_MAGIC_NUMBER_INVALID)
    {
        status = fbe_transport_destroy_packet(&context_p->packet_io);
    }
    
    fbe_semaphore_destroy(&context_p->semaphore);
    fbe_semaphore_destroy(&context_p->semaphore_io);
    return status;
}
/******************************************
 * end fbe_api_raw_mirror_service_test_context_destroy()
 ******************************************/


/*!**************************************************************
 * fbe_api_raw_mirror_service_send_control_operation()
 ****************************************************************
 * @brief
 *  Send the specified control code to the raw mirror NEIT service
 *  for the given test context.
 *
 * @param context_p - structure to send for.      
 * @param package_id - package where raw mirror service lives.  
 * @param control_code - raw mirror usurper command to send.
 *
 * @note 
 *  We assume the structure was previously initted via a
 *  call to fbe_api_raw_mirror_test_context_init().
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
static fbe_status_t fbe_api_raw_mirror_service_send_control_operation(fbe_api_raw_mirror_service_context_t *context_p,
                                                                      fbe_package_id_t package_id,
                                                                      fbe_payload_control_operation_opcode_t control_code)
{
    fbe_status_t status;
    fbe_packet_t * packet_p = &context_p->packet;

    if (control_code == FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_START_IO && (context_p->start_io.specification.max_passes == 0))
    {
        packet_p = &context_p->packet_io;
    }
    fbe_transport_initialize_sep_packet(packet_p);

    status = fbe_api_common_send_control_packet_to_service_asynch(packet_p,
                                                                  control_code,
                                                                  &context_p->start_io,
                                                                  sizeof(fbe_raw_mirror_service_control_start_io_t),
                                                                  FBE_SERVICE_ID_RAW_MIRROR,
                                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                                  fbe_api_raw_mirror_service_test_io_completion, 
                                                                  context_p,
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
 * end fbe_api_raw_mirror_service_send_control_operation()
 **************************************/


/*!**************************************************************
 * fbe_api_raw_mirror_service_test_io_completion()
 ****************************************************************
 * @brief
 *  The completion function for communicating with the raw mirror service.
 *
 * @param packet_p
 * @param context
 *
 * @return FBE_STATUS_OK
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/

static fbe_status_t 
fbe_api_raw_mirror_service_test_io_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_api_raw_mirror_service_context_t *context_p = (fbe_api_raw_mirror_service_context_t *)context;
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
    if (control_p->opcode == FBE_RAW_MIRROR_SERVICE_CONTROL_CODE_START_IO && (context_p->start_io.specification.max_passes == 0))
    {
        fbe_semaphore_release(&context_p->semaphore_io, 0, 1, FALSE);
    }
    else
    {
        fbe_semaphore_release(&context_p->semaphore, 0, 1, FALSE);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_api_raw_mirror_service_test_io_completion()
 ******************************************/


/*!**************************************************************
 * fbe_api_raw_mirror_service_io_specification_init()
 ****************************************************************
 * @brief
 *  Initialize the io specification.
 *
 * @param io_spec_p - io spec to init.
 * @param operation - Specific operation to perform
 *                    (read, write, write/read/check, etc).
 * @param pattern - data pattern.
 * @param sequence_id - sequence identifier in sector data.
 * @param block_size - block size of request.
 * @param block_count - number of blocks.
 * @param start_lba - start lba for request.
 * @param sequence_num - sequence number.
 * @param dmc_expected_b - boolean indicating if DMC should be detected or not
 * @param max_passes - max number of times to issue I/O request
 * @param num_threads - number of threads to use for request
 *
 * @return fbe_status_t - FBE_STATUS_OK on success.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/

static fbe_status_t fbe_api_raw_mirror_service_io_specification_init(fbe_raw_mirror_service_io_specification_t *io_spec_p,
                                                                     fbe_raw_mirror_service_operation_t operation,
                                                                     fbe_data_pattern_t pattern,
                                                                     fbe_u32_t sequence_id,
                                                                     fbe_u32_t block_size,
                                                                     fbe_u32_t block_count,
                                                                     fbe_lba_t start_lba,
                                                                     fbe_u64_t sequence_num,
                                                                     fbe_bool_t dmc_expected_b,
                                                                     fbe_u32_t max_passes,
                                                                     fbe_u32_t num_threads)
{   
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s io_spec_p is NULL.\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Range check the operation.
     */
    if ((operation == FBE_RAW_MIRROR_SERVICE_OPERATION_INVALID) ||
        (operation >= FBE_RAW_MIRROR_SERVICE_OPERATION_LAST))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }


    /* First zero out the memory for the io spec.
     */
    fbe_zero_memory(io_spec_p, sizeof(fbe_raw_mirror_service_io_specification_t));

    /* Setup the input values.
     */
    io_spec_p->operation = operation;
    io_spec_p->block_count = block_count;
    io_spec_p->block_size = block_size;
    io_spec_p->start_lba = start_lba;
    io_spec_p->sequence_num = sequence_num;
    io_spec_p->sequence_id = sequence_id;
    io_spec_p->dmc_expected_b = dmc_expected_b;
    io_spec_p->max_passes = max_passes;
    io_spec_p->num_threads = num_threads;

    status = fbe_api_raw_mirror_service_io_specification_set_pattern(io_spec_p, pattern);

    return status;
}
/******************************************
 * end fbe_api_raw_mirror_service_io_specification_init()
 ******************************************/


/*!**************************************************************
 *  fbe_api_raw_mirror_service_io_specification_set_pattern()
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
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/

static fbe_status_t fbe_api_raw_mirror_service_io_specification_set_pattern(fbe_raw_mirror_service_io_specification_t *io_spec_p,
                                                                            fbe_data_pattern_t pattern)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (io_spec_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        io_spec_p->pattern = pattern;
    }
    if (pattern >= FBE_DATA_PATTERN_LAST)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************
 * end fbe_api_raw_mirror_service_io_specification_set_pattern()
 ******************************************/


/*************************
 * end file fbe_api_raw_mirror_interface.c
 *************************/
