/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file job_service_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main functions to test the Job Service
 * 
 * @version
 *   11/04/2009:  Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_job_service_test_private.h"

/* set this value to 1 if user wants to exercixe job service
 * ability to cancel packet when a client's function
 * times out
 */
#define TEST_FOR_TIMEOUT 0

/* enable this value if user wants to exercise job service
 * ability to process next entry from queue when a client's
 * function return an error
 */
#define TEST_FOR_FUNCTION_ERROR 0

fbe_status_t fbe_job_service_test_set_cancel_function(
        fbe_packet_t*                   in_packet_p, 
        fbe_packet_cancel_function_t    in_packet_cancel_function_p,
        fbe_packet_cancel_context_t     in_packet_cancel_context_p);

static fbe_status_t fbe_job_service_test_cancel_function(
        fbe_packet_cancel_context_t     in_cancel_context_p);

/* ------------------------------------- */
/* Job service test interface functions  */
/* ------------------------------------- */

/* Job Service test validation function.
*/
static fbe_status_t fbe_job_service_test_validation_function(
        fbe_packet_t           *packet_p);

/* Job Service test selection function.
*/
static fbe_status_t fbe_job_service_test_selection_function(
        fbe_packet_t           *packet_p);

/* Job Service test update in memory function function.
*/
static fbe_status_t fbe_job_service_test_update_in_memory_function(
        fbe_packet_t          *packet_p);

/* Job Service test persist function function.
*/
static fbe_status_t fbe_job_service_test_persist_function(
        fbe_packet_t           *packet_p);

/* Job Service test rollback function function.
*/
static fbe_status_t fbe_job_service_test_rollback_function(
        fbe_packet_t           *packet_p);

/* Job Service test commit function function.
*/
static fbe_status_t fbe_job_service_test_commit_function(
        fbe_packet_t           *packet_p);

/* Job service test registration.
*/
fbe_job_service_operation_t fbe_test_job_service_operation = 
{
    FBE_JOB_TYPE_ADD_ELEMENT_TO_QUEUE_TEST,
    {
        /* validation function */
        fbe_job_service_test_validation_function, /* SAFEBUG *//*RCA-NPTF*/

        /* selection function */
        fbe_job_service_test_selection_function, /* SAFEBUG *//*RCA-NPTF*/

        /* update in memory function */
        //fbe_job_service_test_update_in_memory_function,
        NULL,

        /* persist function */
        //fbe_job_service_test_persist_function,
        NULL,

        /* rollback function */
        fbe_job_service_test_rollback_function, /* SAFEBUG *//*RCA-NPTF*/

        /* commit function */
        NULL
    }
};

/* ================================================================ 
 * following functions are defined to interact with the Job service
 * so that we can test the job service queing, dequeing processing
 * ================================================================ */


/*!**************************************************************
 * fbe_job_service_test_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate Job Service test validation 
 * request 
 *
 * @param context packet_p - The packet used to construct the request
 * @param context context - client's context
 *
 * @return fbe_status_t - status of call to job service
 *
 * @author
 *  10/29/2009 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_job_service_test_validation_function(
        fbe_packet_t           *packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_job_service_test_command_data_t   *fbe_job_service_test_element_p = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    fbe_job_service_test_element_p = (fbe_job_service_test_command_data_t *) job_queue_element_p->command_data;

    if (fbe_job_service_test_element_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status (packet_p, status, 0);
        fbe_transport_complete_packet (packet_p);

        return status;
    }

#if TEST_FOR_TIMEOUT
    /* do not call completion function, let the timer expire..test for semaphore wait
    */
    fbe_transport_exchange_state(packet_p, FBE_PACKET_STATE_QUEUED);
    fbe_job_service_test_set_cancel_function(packet_p,
            fbe_job_service_test_cancel_function, 
            (fbe_packet_cancel_context_t)packet_p);
#else
    fbe_transport_set_status (packet_p, status, 0);
    fbe_transport_complete_packet (packet_p);
#endif
    return status;
}
/************************************************
 * end fbe_job_service_test_validation_function()
 ***********************************************/

/*!**************************************************************
 * fbe_job_service_test_selection_function()
 ****************************************************************
 * @brief
 * This function is used to validate Job Service test selection 
 * request 
 *
 * @param context packet_p - The packet used to construct the request
 * @param context context - client's context
 *
 * @return fbe_status_t - status of call to job service
 *
 * @author
 *  10/29/2009 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_job_service_test_selection_function(
        fbe_packet_t           *packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_job_service_test_command_data_t   *fbe_job_service_test_element_p = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    fbe_job_service_test_element_p = (fbe_job_service_test_command_data_t *) job_queue_element_p->command_data;

#if TEST_FOR_FUNCTION_ERROR
    fbe_transport_set_status (packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    fbe_transport_complete_packet (packet_p);
#else
    fbe_transport_set_status (packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet (packet_p);
#endif

    return status;
}
/**********************************************
 * end fbe_job_service_test_selection_function()
 **********************************************/

/*!**************************************************************
 * Kilgore_trout_update_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to validate Job Service test update_in
 * memory request
 *
 * @param context packet_p - The packet used to construct the request
 * @param context context - client's context
 *
 * @return fbe_status_t - status of call to job service
 *
 * @author
 *  10/29/2009 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_job_service_test_update_in_memory_function(
        fbe_packet_t           *packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_job_service_test_command_data_t   *fbe_job_service_test_element_p = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    fbe_job_service_test_element_p = (fbe_job_service_test_command_data_t *) job_queue_element_p->command_data;

    if (fbe_job_service_test_element_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status (packet_p, status, 0);
        fbe_transport_complete_packet (packet_p);

        return status;
    }

    fbe_transport_set_status (packet_p, status, 0);
    fbe_transport_complete_packet (packet_p);

    return status;
}
/******************************************************
 * end fbe_job_service_test_update_in_memory_function()
 *****************************************************/

/*!**************************************************************
 * fbe_job_service_test_persist_function()
 ****************************************************************
 * @brief
 * This function is used to validate Job Service test persist
 * memory request
 *
 * @param context packet_p - The packet used to construct the request
 * @param context context - client's context
 *
 * @return fbe_status_t - status of call to job service
 *
 * @author
 *  10/29/2009 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_job_service_test_persist_function(
        fbe_packet_t           *packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_job_service_test_command_data_t   *fbe_job_service_test_element_p = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    fbe_job_service_test_element_p = (fbe_job_service_test_command_data_t *) job_queue_element_p->command_data;

    fbe_transport_set_status (packet_p, status, 0);
    fbe_transport_complete_packet (packet_p);

    return status;
}
/********************************************
 * end fbe_job_service_test_persist_function()
 ********************************************/

/*!**************************************************************
 * fbe_job_service_test_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to validate Job Service test rollback
 * memory request
 * 
 * @param context packet_p - The packet used to construct the request
 * @param context context - client's context
 *
 * @return fbe_status_t - status of call to job service
 *
 * @author
 *  10/29/2009 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_job_service_test_rollback_function(
        fbe_packet_t           *packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_job_service_test_command_data_t   *fbe_job_service_test_element_p = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    fbe_job_service_test_element_p = (fbe_job_service_test_command_data_t *) job_queue_element_p->command_data;

    fbe_transport_set_status (packet_p, status, 0);
    fbe_transport_complete_packet (packet_p);

    return status;
}
/**********************************************
 * end fbe_job_service_test_rollback_function()
 *********************************************/

/*!**************************************************************
 * fbe_job_service_test_commit_function()
 ****************************************************************
 * @brief
 * This function is used to validate Job Service test commit
 * memory request
 * 
 * @param context packet_p - The packet used to construct the request
 * @param context context - client's context
 *
 * @return fbe_status_t - status of call to job service
 *
 * @author
 *  10/29/2009 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_job_service_test_commit_function (
        fbe_packet_t           *packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_job_service_test_command_data_t   *fbe_job_service_test_element_p = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    fbe_job_service_test_element_p = (fbe_job_service_test_command_data_t *) job_queue_element_p->command_data;

    /*! Get the packet's status */
    status = fbe_transport_get_status_code(packet_p);

    fbe_transport_set_status (packet_p, status, 0);
    fbe_transport_complete_packet (packet_p);

    return status;
}
/********************************************
 * end fbe_job_service_test_commit_function()
 ********************************************/

/*!**************************************************************
 * fbe_job_service_test_set_cancel_function()
 ****************************************************************
 * @brief
 *  This function sets a cancel function for the 
 *  job service test
 * 
 * @param packet_p - packet
 * @param in_packet_cancel_function_p - cancel function
 * @param context -  Packet completion context.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 11/04/2009 - Created. Jesus Medina
 * 
 ****************************************************************/
fbe_status_t fbe_job_service_test_set_cancel_function(
        fbe_packet_t*                   in_packet_p, 
        fbe_packet_cancel_function_t    in_packet_cancel_function_p,
        fbe_packet_cancel_context_t     in_packet_cancel_context_p)
{
    /*  Set the packet cancellation and context. 
    */
    in_packet_p->packet_cancel_function = in_packet_cancel_function_p;
    in_packet_p->packet_cancel_context  = in_packet_cancel_context_p;
    return FBE_STATUS_OK;

} 
/************************************************
 * end fbe_job_service_test_set_cancel_function()
 ************************************************/

/*!**************************************************************
 * fbe_job_service_test_cancel_function()
 ****************************************************************
 * @brief
 *  This is a dummy function used to test the job service's
 *  cancellation code 
 * 
 * @param context -  Packet completion context.
 *
 * @return fbe_status_t4
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 11/09/2009 - Created. Jesus Medina
 * 
 ****************************************************************/
static fbe_status_t fbe_job_service_test_cancel_function(
        fbe_packet_cancel_context_t in_cancel_context_p)

{
    fbe_packet_t *in_packet_p = (fbe_packet_t*)in_cancel_context_p;

    fbe_transport_set_status (in_packet_p, FBE_STATUS_CANCELED, 0);
    fbe_transport_complete_packet (in_packet_p);

    return FBE_STATUS_OK;
} 
/********************************************
 * end fbe_job_service_test_cancel_function()
 ********************************************/


/*******************************
 * end fbe_job_service_test_file
 ******************************/





